# swapxfg M7: CD Market Tab

> **For agentic workers:** Use superpowers:executing-plans to implement task-by-task.

**Goal:** Add a CD/XFG secondary market tab to swapxfg. Users can post CDs for sale, browse offers, view discount/premium pricing, and initiate co-signed CD transfers — all from within the TUI.

**Spec:** `docs/superpowers/specs/2026-04-02-cd-market-design.md`

**Prerequisites:**
- M2 (wallet RPC) — needed for `sell cd` and `accept` (require wallet signing)
- v11 consensus (transferable CDs) — the on-chain mechanics must be live
- `/getcdoffers`, `/submitcd`, `/cancelcd`, `/acceptcd`, `/getcdprice` RPC endpoints in fuegod (spec: `2026-04-02-cd-market-design.md` §4)

**Not in scope:** The fuegod C++ side (CdOfferMsg, P2P gossip, RPC handlers) — that is part of the transferable CD plan (`2026-03-22-transferable-cd-fee-pool.md`).

---

## New files

```
swapxfg/app/
├── cd_rpc.go          CdClient: /getcdoffers, /getcdprice, /submitcd, /cancelcd, /acceptcd
├── cd_market.go       CD market tab TUI model + view
├── cd_orderbook.go    CD offer list renderer (sorted by discount)
└── cd_rpc_test.go     Tests for response parsing
```

---

## Task 1: `cd_rpc.go` — RPC client

- [ ] **Step 1: Define types**

```go
// swapxfg/app/cd_rpc.go
package app

type CdOffer struct {
  OfferID      string `json:"offerId"`
  IsSell       bool   `json:"isSell"`
  CdAmount     uint64 `json:"cdAmount"`
  CdTerm       uint32 `json:"cdTerm"`
  CdEpoch      uint32 `json:"cdEpoch"`
  CdKeyImage   string `json:"cdKeyImage"`
  AskPrice     uint64 `json:"askPrice"`
  MakerPubKey  string `json:"makerPubKey"`
  Timestamp    uint64 `json:"timestamp"`
  TTLBlocks    uint32 `json:"ttlBlocks"`
  PostedHeight uint32 `json:"postedHeight"`
}

type CdPriceStats struct {
  Amount       uint64  `json:"cdAmount"`
  FaceValue    uint64  `json:"faceValue"`
  EstInterest  uint64  `json:"estInterest"`     // atomic units, from fee pool
  MedianAsk    uint64  `json:"medianAsk"`
  DiscountPct  float64 `json:"discountPct"`     // negative = below face value
  ActiveOffers int     `json:"activeOffers"`
}

type AcceptCdResponse struct {
  PartialTx  string `json:"partialTx"`
  ExpiresAt  uint32 `json:"expiresAt"`
  Status     string `json:"status"`
}
```

- [ ] **Step 2: Add methods to FuegoClient**

```go
func (c *FuegoClient) GetCdOffers(amount uint64) ([]CdOffer, error) {
  req := map[string]interface{}{"amount": amount}  // 0 = all
  var resp struct {
    Offers []CdOffer `json:"offers"`
    Status string    `json:"status"`
  }
  if err := c.post("/getcdoffers", req, &resp); err != nil {
    return nil, err
  }
  return resp.Offers, nil
}

func (c *FuegoClient) GetCdPrice(amount uint64) (*CdPriceStats, error) {
  req := map[string]interface{}{"amount": amount}
  var resp CdPriceStats
  return &resp, c.post("/getcdprice", req, &resp)
}

func (c *FuegoClient) AcceptCdOffer(offerID, buyerCommitKey string) (*AcceptCdResponse, error) {
  req := map[string]interface{}{
    "offerId":        offerID,
    "buyerCommitKey": buyerCommitKey,
  }
  var resp AcceptCdResponse
  return &resp, c.post("/acceptcd", req, &resp)
}

func (c *FuegoClient) SubmitCdOffer(offer CdOffer) error {
  var resp struct{ Status string `json:"status"` }
  return c.post("/submitcd", offer, &resp)
}

func (c *FuegoClient) CancelCdOffer(offerID, pubKey, sig string) error {
  req := map[string]interface{}{"offerId": offerID, "makerPubKey": pubKey, "signature": sig}
  var resp struct{ Status string `json:"status"` }
  return c.post("/cancelcd", req, &resp)
}
```

- [ ] **Step 3: Helper — compute discount**

```go
// CdDiscount returns discount percentage vs face value.
// Negative = below face (selling at discount), positive = premium.
func CdDiscount(cdAmount, askPrice uint64) float64 {
  if cdAmount == 0 {
    return 0
  }
  return (float64(askPrice) - float64(cdAmount)) / float64(cdAmount) * 100.0
}
```

---

## Task 2: `cd_orderbook.go` — offer list renderer

- [ ] **Step 1: Sort offers by discount (most discounted first)**

```go
func sortCdOffers(offers []CdOffer) []CdOffer {
  sorted := make([]CdOffer, len(offers))
  copy(sorted, offers)
  sort.Slice(sorted, func(i, j int) bool {
    di := CdDiscount(sorted[i].CdAmount, sorted[i].AskPrice)
    dj := CdDiscount(sorted[j].CdAmount, sorted[j].AskPrice)
    return di < dj  // most discounted (most negative) first
  })
  return sorted
}
```

- [ ] **Step 2: Render function**

```go
func RenderCdOrderbook(offers []CdOffer, selected int, width, height int) string
```

Output format (each row):
```
AMOUNT      TERM   EPOCH   ASK         DISC
────────── ────── ──────  ──────────  ──────
800.0 XFG    12mo   ep.9   775.0 XFG   -3.1%   ← selected row highlighted
 80.0 XFG     6mo   ep.8    76.0 XFG   -5.0%
800.0 XFG     3mo   ep.7   802.0 XFG   +0.2%
```

Color: discounts in yellow, premiums in green, header in muted.

- [ ] **Step 3: Render CD detail panel**

```go
func RenderCdDetail(offer *CdOffer, price *CdPriceStats, width int) string
```

Shows: Amount, Term, Epoch, Est. interest, Ask, Discount, Seller pubkey (first 12 chars), `[press enter to accept]`.

---

## Task 3: `cd_market.go` — CD market TUI model

The CD market is implemented as an embedded sub-model within `tuiModel`, activated when pair `cd` is selected.

- [ ] **Step 1: Define CdMarketModel**

```go
type CdMarketModel struct {
  offers   []CdOffer
  prices   map[uint64]*CdPriceStats  // keyed by CD amount tier
  selected int
  scroll   int
}
```

- [ ] **Step 2: Implement Init / Update / View**

- `Init`: triggers fetch of CD offers
- `Update`: handles up/down navigation, enter to accept, `s`/`b` for sell/buy
- `View`: renders two-panel layout (offer list left, detail right)

```
┌─────────────────────────────────────────────────────────────────────────┐
│ ◆ SWAPXFG  SOL ▲0.46  ETH ▲1.82  BCH ▲21.3  CD/XFG ▓▓▓▓▓              │
├─────────────────────────────────────────┬───────────────────────────────┤
│   CD OFFERS (sorted by discount)        │   SELECTED OFFER              │
│                                         │                               │
│  AMOUNT    TERM  EPOCH  ASK      DISC   │  Amount:  80.0000000 XFG     │
│  ─────── ────── ─────  ──────── ─────   │  Term:    6 months           │
│  800 XFG   12mo  ep.9  775 XFG  -3.1%   │  Epoch:   8                  │
│▶  80 XFG    6mo  ep.8   76 XFG  -5.0%   │  Est. interest: ~2.1 XFG    │
│  800 XFG    3mo  ep.7  802 XFG  +0.2%   │  Ask:     76.0000000 XFG    │
│    8 XFG    1mo  ep.9    7 XFG -12.5%   │  Disc:    -5.0%              │
│  0.8 XFG    6mo  ep.8  0.77 XFG -3.7%   │  Seller:  xfg1pub3a4f...    │
│                                         │                               │
│                                         │  [enter: accept]  [b: bid]   │
│                                         │                               │
├─────────────────────────────────────────┴───────────────────────────────┤
│ > _                                     BAL 4200.00 XFG  ■ :18180     │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## Task 4: Pairs integration

- [ ] **Step 1: Add `PairCD` constant to pairs.go**

```go
const PairCD uint8 = 99  // not a swap pair — special CD market mode
```

- [ ] **Step 2: Add to hotkeys**

```go
case 'c':
  return PairCD
```

- [ ] **Step 3: Update ticker bar**

When CD offers exist, show "CD/XFG" with offer count or median discount in ticker:
```
CD/XFG -3.1% avg
```

---

## Task 5: CD commands in tui.go

- [ ] **Step 1: Add to handleCommand**

```
sell cd <key_image> <ask_price_xfg>
  → calls wallet /sign_cd_offer (new wallet endpoint) → submits via /submitcd
  → shows "CD offer posted: <offerId>"

buy cd <amount_xfg> <bid_price_xfg>
  → posts a buy-side bid offer via /submitcd (isSell=false)

cancel <offer_id>
  → calls wallet to sign cancellation → /cancelcd

accept [offer_id]
  → if offer_id omitted, uses selected row in CD orderbook
  → prompts for buyerCommitKey (or fetches from wallet /get_cd_commit_key — new wallet endpoint)
  → calls /acceptcd → shows partial tx hex + instructions
```

- [ ] **Step 2: Add wallet endpoint `get_cd_commit_key`**

New wallet RPC method that returns the next unused `commitKey` for a new CD. Wallet generates this from the spend key + a counter. Add to `WalletRpcServerCommandsDefinitions.h` (C++) and `WalletClient` (Go).

---

## Task 6: AllPairData update

- [ ] **Step 1: Add CD offers + prices to AllPairData**

```go
type AllPairData struct {
  // ... existing swap pair fields ...
  CdOffers []CdOffer
  CdPrices map[uint64]*CdPriceStats
}
```

- [ ] **Step 2: Add CD fetch to FetchAll()**

Alongside existing pair fetches, add:
```go
wg.Add(2)
go func() {
  defer wg.Done()
  offers, err := client.GetCdOffers(0)
  // store in data.CdOffers
}()
go func() {
  defer wg.Done()
  // fetch prices for each active CD tier
}()
```

---

## Task 7: Verify

- [ ] `go build ./...` — zero errors
- [ ] Press `c` in swapxfg → CD market tab renders with empty offer list (when no offers exist)
- [ ] `sell cd` command with valid args → offer appears in `/getcdoffers` on the daemon
- [ ] Up/down navigate offer list, selected offer shows in detail panel
- [ ] `accept` on selected offer with wallet connected → partial tx hex printed to status bar
- [ ] `cancel <offer_id>` removes offer from list on next refresh
