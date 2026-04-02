# swapxfg M2: Wallet RPC Integration

> **For agentic workers:** Use superpowers:executing-plans to implement task-by-task.

**Goal:** Connect swapxfg to the local wallet RPC so users can view their XFG balance, initiate swaps (fund escrow), and sign offers — all from within the TUI. Also wire the `confirm` command that M1 left incomplete.

**Spec:** `docs/superpowers/specs/2026-03-22-swaphub-design.md` §1.5 (wallet RPC section)

**Prerequisite:** New wallet RPC endpoints must be added to `src/Wallet/WalletRpcServer` (C++ side) before the Go client can call them. This plan covers both sides.

**Not in scope:** MetaMask/Phantom/BCH bridges (M3/M4), SwapDaemon state machine (M5).

---

## What currently exists

- `WalletRpcServer` has: `transfer`, `get_balance` (see `WalletRpcServerCommandsDefinitions.h`)
- `FuegoRpcClient` (C++ in SwapDaemon) calls `sendTransfer` via wallet JSON-RPC
- swapxfg `tui.go`: balance slot in status bar is rendered as empty string
- swapxfg `tui.go` `handleCommand`: `initiate` resolves alias + shows address but stops there — no `confirm` step

## What needs to be built

### C++ side — new wallet RPC endpoints

| Endpoint | Method | Purpose |
|----------|--------|---------|
| `get_address` | GET | Return the wallet's primary XFG address |
| `sign_offer` | POST | Sign a swap offer payload with the wallet's spend key |
| `create_htlc` | POST | Lock XFG in a new HTLC output (returns tx hash + HTLC index) |
| `claim_htlc` | POST | Claim an HTLC with preimage (returns tx hash) |
| `refund_htlc` | POST | Refund a timed-out HTLC (returns tx hash) |

### Go side — `swapxfg/app/wallet.go` (new file)

`WalletClient` struct with methods mirroring the C++ endpoints above, plus balance polling.

### Go side — updates to existing files

- `config.go`: add `WalletRPC string` field
- `main.go`: add `--wallet` / `-w` flag
- `tui.go`: poll balance, populate status bar, wire `confirm` command
- `rpc.go`: add `SwapOffer.IsSell` field (needed for sign_offer)

---

## Task 1: C++ — `get_address` endpoint

**Files:**
- Modify: `src/Wallet/WalletRpcServerCommandsDefinitions.h`
- Modify: `src/Wallet/WalletRpcServer.h`
- Modify: `src/Wallet/WalletRpcServer.cpp`

- [ ] **Step 1: Add COMMAND_RPC_GET_ADDRESS struct**

In `WalletRpcServerCommandsDefinitions.h`, after `COMMAND_RPC_GET_BALANCE`:

```cpp
struct COMMAND_RPC_GET_ADDRESS {
  typedef CryptoNote::EMPTY_STRUCT request;

  struct response {
    std::string address;
    std::string status;

    void serialize(ISerializer& s) {
      KV_MEMBER(address)
      KV_MEMBER(status)
    }
  };
};
```

- [ ] **Step 2: Register + implement in WalletRpcServer**

In `WalletRpcServer.h`, add declaration:
```cpp
bool on_get_address(const wallet_rpc::COMMAND_RPC_GET_ADDRESS::request& req,
                    wallet_rpc::COMMAND_RPC_GET_ADDRESS::response& res);
```

In `WalletRpcServer.cpp`, register the route (in the URL map) and implement:
```cpp
bool WalletRpcServer::on_get_address(const wallet_rpc::COMMAND_RPC_GET_ADDRESS::request& req,
                                     wallet_rpc::COMMAND_RPC_GET_ADDRESS::response& res) {
  res.address = m_wallet.getAddress();
  res.status = WALLET_RPC_STATUS_OK;
  return true;
}
```

---

## Task 2: C++ — `sign_offer` endpoint

**Files:** Same 3 as Task 1

Allows swapxfg to get a wallet-signed offer without exposing the private key.

- [ ] **Step 1: Add COMMAND_RPC_SIGN_OFFER**

```cpp
struct COMMAND_RPC_SIGN_OFFER {
  struct request {
    uint64_t xfgAmount;
    uint64_t rateNum;
    uint8_t  pair;
    uint32_t ttlBlocks;

    void serialize(ISerializer& s) {
      KV_MEMBER(xfgAmount)
      KV_MEMBER(rateNum)
      KV_MEMBER(pair)
      KV_MEMBER(ttlBlocks)
    }
  };

  struct response {
    std::string offerId;       // SHA-256(makerPubKey || pair || amount || rate || timestamp) hex
    std::string makerPubKey;   // hex
    std::string signature;     // hex
    uint64_t    timestamp;
    std::string status;

    void serialize(ISerializer& s) {
      KV_MEMBER(offerId)
      KV_MEMBER(makerPubKey)
      KV_MEMBER(signature)
      KV_MEMBER(timestamp)
      KV_MEMBER(status)
    }
  };
};
```

- [ ] **Step 2: Implement in WalletRpcServer.cpp**

Use the wallet's view/spend keys to compute offerId hash and sign it.

---

## Task 3: C++ — `create_htlc`, `claim_htlc`, `refund_htlc`

**Files:** Same 3 as Task 1, plus `src/WalletLegacy/WalletTransactionSender.cpp`

These call into existing HTLC transaction construction (already used by `SimpleWallet`).

- [ ] **Step 1: Add COMMAND_RPC_CREATE_HTLC**

```cpp
struct COMMAND_RPC_CREATE_HTLC {
  struct request {
    uint64_t    amount;         // atomic units
    std::string recipient;      // XFG address (counterparty)
    std::string hashLock;       // 64-char hex SHA-256 hash
    uint32_t    timeoutHeight;  // absolute block height for refund

    void serialize(ISerializer& s) {
      KV_MEMBER(amount)
      KV_MEMBER(recipient)
      KV_MEMBER(hashLock)
      KV_MEMBER(timeoutHeight)
    }
  };

  struct response {
    std::string txHash;
    uint32_t    htlcIndex;   // global HTLC output index
    std::string status;

    void serialize(ISerializer& s) {
      KV_MEMBER(txHash)
      KV_MEMBER(htlcIndex)
      KV_MEMBER(status)
    }
  };
};
```

- [ ] **Step 2: Add COMMAND_RPC_CLAIM_HTLC**

```cpp
struct COMMAND_RPC_CLAIM_HTLC {
  struct request {
    uint32_t    htlcIndex;
    std::string preimage;   // 64-char hex preimage

    void serialize(ISerializer& s) {
      KV_MEMBER(htlcIndex)
      KV_MEMBER(preimage)
    }
  };

  struct response {
    std::string txHash;
    std::string status;

    void serialize(ISerializer& s) {
      KV_MEMBER(txHash)
      KV_MEMBER(status)
    }
  };
};
```

- [ ] **Step 3: Add COMMAND_RPC_REFUND_HTLC** (same shape as claim, no preimage field)

- [ ] **Step 4: Implement all three in WalletRpcServer.cpp**

Delegate to existing `WalletTransactionSender` HTLC construction methods.

---

## Task 4: Go — `swapxfg/app/wallet.go` (new file)

- [ ] **Step 1: Create wallet.go**

```go
// swapxfg/app/wallet.go
package app

import "fmt"

// WalletClient talks to the local fire_wallet JSON-RPC.
type WalletClient struct {
  fc *FuegoClient // reuse same HTTP client, different endpoint
}

func NewWalletClient(endpoint string) *WalletClient {
  return &WalletClient{fc: NewFuegoClient(endpoint)}
}

type WalletBalance struct {
  Available uint64 `json:"available_balance"`
  Locked    uint64 `json:"locked_amount"`
}

type SignedOffer struct {
  OfferID     string `json:"offerId"`
  MakerPubKey string `json:"makerPubKey"`
  Signature   string `json:"signature"`
  Timestamp   uint64 `json:"timestamp"`
}

type HtlcResult struct {
  TxHash    string `json:"txHash"`
  HtlcIndex uint32 `json:"htlcIndex"`
}

func (w *WalletClient) GetBalance() (*WalletBalance, error) {
  var resp WalletBalance
  if err := w.fc.post("/get_balance", nil, &resp); err != nil {
    return nil, err
  }
  return &resp, nil
}

func (w *WalletClient) GetAddress() (string, error) {
  var resp struct {
    Address string `json:"address"`
  }
  if err := w.fc.post("/get_address", nil, &resp); err != nil {
    return "", err
  }
  return resp.Address, nil
}

func (w *WalletClient) SignOffer(xfgAmount, rateNum uint64, pair uint8, ttlBlocks uint32) (*SignedOffer, error) {
  req := map[string]interface{}{
    "xfgAmount": xfgAmount,
    "rateNum":   rateNum,
    "pair":      pair,
    "ttlBlocks": ttlBlocks,
  }
  var resp SignedOffer
  if err := w.fc.post("/sign_offer", req, &resp); err != nil {
    return nil, err
  }
  return &resp, nil
}

func (w *WalletClient) CreateHtlc(amount uint64, recipient, hashLock string, timeoutHeight uint32) (*HtlcResult, error) {
  req := map[string]interface{}{
    "amount":        amount,
    "recipient":     recipient,
    "hashLock":      hashLock,
    "timeoutHeight": timeoutHeight,
  }
  var resp HtlcResult
  if err := w.fc.post("/create_htlc", req, &resp); err != nil {
    return nil, err
  }
  return &resp, nil
}

func (w *WalletClient) ClaimHtlc(htlcIndex uint32, preimage string) (string, error) {
  req := map[string]interface{}{"htlcIndex": htlcIndex, "preimage": preimage}
  var resp struct{ TxHash string `json:"txHash"` }
  if err := w.fc.post("/claim_htlc", req, &resp); err != nil {
    return "", err
  }
  return resp.TxHash, nil
}

func (w *WalletClient) RefundHtlc(htlcIndex uint32) (string, error) {
  req := map[string]interface{}{"htlcIndex": htlcIndex}
  var resp struct{ TxHash string `json:"txHash"` }
  if err := w.fc.post("/refund_htlc", req, &resp); err != nil {
    return "", err
  }
  return resp.TxHash, nil
}

// FormatBalance converts atomic units to XFG display string.
func FormatBalance(atomic uint64) string {
  return fmt.Sprintf("%.7f", float64(atomic)/1e7)
}
```

---

## Task 5: Go — config + CLI flag

- [ ] **Step 1: Update config.go**

```go
type Config struct {
  DaemonRPC string
  WalletRPC string  // NEW — empty means no wallet connected
  Testnet   bool
  StartPair uint8
  NoSplash  bool
  Compact   bool
}

func DefaultConfig() Config {
  return Config{
    DaemonRPC: "http://127.0.0.1:18180",
    WalletRPC: "",   // opt-in
    StartPair: PairSOL,
  }
}
```

- [ ] **Step 2: Update main.go** — add `--wallet / -w` flag parsing (mirrors `--daemon` pattern)

---

## Task 6: Go — TUI balance display + confirm command

- [ ] **Step 1: Add wallet fields to tuiModel in tui.go**

```go
type tuiModel struct {
  // ... existing fields ...
  wallet      *WalletClient  // nil if --wallet not set
  balance     *WalletBalance
  walletAddr  string
}
```

- [ ] **Step 2: Poll balance alongside data refresh**

In `fetchData()`, if `m.wallet != nil`, fetch balance in parallel with offer/price/trade data.

- [ ] **Step 3: Populate status bar**

In `RenderInputBar`, replace empty balance slot:
```
BAL 4200.0000000 XFG  ■ :18180
```
When wallet not connected:
```
[no wallet]  ■ :18180
```

- [ ] **Step 4: Wire `confirm` command in handleCommand**

```
confirm <address> <amount_xfg> <pair>
```

Flow:
1. Parse address, amount, pair
2. Call `w.wallet.SignOffer(...)` → get signed offer
3. Call `w.client.post("/submitswap", signedOffer, ...)` → submit to daemon gossip
4. Show offerId in statusMsg: `"offer posted: abc123..."`

- [ ] **Step 5: Wire `htlc create/claim/refund` commands**

```
htlc create <amount> <recipient> <hashlock> <timeout_height>
htlc claim <htlc_index> <preimage>
htlc refund <htlc_index>
```

Each calls the corresponding `WalletClient` method and shows tx hash in statusMsg.

---

## Task 7: Verify

- [ ] Run `go build ./...` in `swapxfg/` — zero errors
- [ ] `./swapxfg --wallet http://127.0.0.1:18182` — balance appears in status bar
- [ ] `./swapxfg` (no --wallet) — "[no wallet]" shown, all read-only features still work
- [ ] `initiate` → `confirm` flow produces an offer visible in `/getswapoffers`
