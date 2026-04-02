# CD/XFG Secondary Market — Design Spec

**Date:** 2026-04-02
**Status:** Draft
**Depends on:** `2026-03-22-transferable-cd-fee-pool-design.md` (TransactionInputCommitmentTransfer)

---

## Overview

This spec defines the discovery and execution layer for buying and selling CDs (Commitment Deposits) on-chain. CD transfers are already handled at the consensus layer — this spec covers the gossip, orderbook, and wallet UX that makes them accessible to users.

**No EFier dependency.** All relay runs over the existing P2P gossip network on any `fuegod` node. SwapHub connects via `--daemon` to any node.

---

## Architecture Summary

```
Seller                    Buyer
  │                         │
  │  1. post CD offer        │
  │──────────────────────►  │  (gossip via COMMAND_CD_OFFER, P2P_COMMANDS_POOL_BASE + 15)
  │                         │
  │         fuegod nodes relay offer to all peers
  │                         │
  │  2. buyer accepts       │
  │◄────────────────────────│  (RPC: /acceptcd → returns partial tx to buyer)
  │                         │
  │  3. co-sign & broadcast  │
  │──────────────────────►  │  (both sign via wallet, broadcast to mempool)
  │                         │
  │  4. confirm on-chain    │
  │◄───────────────────────►│  (TransactionInputCommitmentTransfer confirmed)
```

---

## 1. CdOfferMsg — Gossip Payload

Extends the existing swap offer pattern from `SwapOfferMsg`. The seller posts what CD they're selling and what XFG price they want.

```cpp
// src/CryptoNoteCore/SwapOfferRelay.h — added alongside SwapOfferMsg

struct CdOfferMsg {
  std::string  offerId;        // SHA-256(makerPubKey || cdKeyImage || askPrice || timestamp) hex
  bool         isSell;         // true = selling CD for XFG; false = buying CD (bid)
  uint64_t     cdAmount;       // CD principal in atomic units (must be valid CD tier)
  uint32_t     cdTerm;         // remaining/new term offered (seller-declared, >= 1)
  uint32_t     cdEpoch;        // epoch the CD was created (informs interest accrued)
  Crypto::KeyImage cdKeyImage; // key image of the CD being sold (prevents double-listing)
  uint64_t     askPrice;       // XFG price in atomic units (what seller wants for the CD)
  Crypto::PublicKey makerPubKey;
  Crypto::Signature signature; // signs offerId with makerPubKey
  uint64_t     timestamp;
  uint32_t     ttlBlocks;      // offer expires after this many blocks
  uint32_t     postedHeight;
};
```

**Why `cdKeyImage` in the offer:** Prevents the same CD from being listed multiple times. Nodes reject duplicate `cdKeyImage` offers. When a CD transfer transaction is confirmed on-chain, its key image is spent — `SwapOfferRelay` prunes all offers for that key image automatically.

**Why `cdEpoch`:** Buyers can estimate accrued and future interest before accepting. The epoch is public information derivable from the chain; including it in the offer is a convenience for the buyer's UI.

---

## 2. P2P Command

```cpp
// src/P2p/P2pProtocolDefinitions.h — after COMMAND_SWAP_CANCEL (ID 14)

struct COMMAND_CD_OFFER
{
  enum { ID = P2P_COMMANDS_POOL_BASE + 15 };

  struct request {
    std::string  offerId;
    bool         isSell;
    uint64_t     cdAmount;
    uint32_t     cdTerm;
    uint32_t     cdEpoch;
    Crypto::KeyImage cdKeyImage;
    uint64_t     askPrice;
    Crypto::PublicKey makerPubKey;
    Crypto::Signature signature;
    uint64_t     timestamp;
    uint32_t     ttlBlocks;
    uint32_t     postedHeight;

    void serialize(ISerializer& s) {
      KV_MEMBER(offerId)
      KV_MEMBER(isSell)
      KV_MEMBER(cdAmount)
      KV_MEMBER(cdTerm)
      KV_MEMBER(cdEpoch)
      KV_MEMBER(cdKeyImage)
      KV_MEMBER(askPrice)
      KV_MEMBER(makerPubKey)
      KV_MEMBER(signature)
      KV_MEMBER(timestamp)
      KV_MEMBER(ttlBlocks)
      KV_MEMBER(postedHeight)
    }
  };
  typedef EMPTY_STRUCT response;
};

struct COMMAND_CD_CANCEL
{
  enum { ID = P2P_COMMANDS_POOL_BASE + 16 };

  struct request {
    std::string offerId;
    Crypto::PublicKey makerPubKey;
    Crypto::Signature signature;  // signs "cdcancel:<offerId>"

    void serialize(ISerializer& s) {
      KV_MEMBER(offerId)
      KV_MEMBER(makerPubKey)
      KV_MEMBER(signature)
    }
  };
  typedef EMPTY_STRUCT response;
};
```

---

## 3. SwapOfferRelay Extensions

Add CD offer handling to `SwapOfferRelay` (same class, new methods):

```cpp
// New methods on SwapOfferRelay:

// Incoming gossip handlers
void handleCdOfferMessage(const CdOfferMsg& offer);
void handleCdCancelMessage(const std::string& offerId, const Crypto::PublicKey& pubkey,
                           const Crypto::Signature& sig);

// Query interface (used by RPC)
std::vector<CdOfferMsg> getCdOffers(uint64_t amount = 0) const;  // 0 = all amounts
std::vector<CdOfferMsg> getAllCdOffers() const;

// Submit from local RPC
bool submitCdOffer(const CdOfferMsg& offer);
bool cancelCdOffer(const std::string& offerId, const Crypto::PublicKey& pubkey,
                   const Crypto::Signature& sig);

// Auto-prune when CD key image appears in confirmed block
void pruneCdOffersForKeyImage(const Crypto::KeyImage& ki);
```

**Validation rules** (mirror `SwapOfferMsg` validation):
1. `cdAmount` must be a valid CD tier amount (AMOUNT_TIER_0..AMOUNT_TIER_N)
2. `cdTerm >= CD_TRANSFER_MIN_REMAINING_TERM` (= 1)
3. `askPrice > 0`
4. Signature verifies: `Crypto::checkSignature(Hash(offerId), makerPubKey, signature)`
5. No existing offer with same `cdKeyImage` (duplicate listing)
6. `timestamp` within ±10 minutes of node wall clock
7. `postedHeight` within 20 blocks of current chain tip

---

## 4. RPC Endpoints

```
GET  /getcdoffers          → list active CD sell offers (optional ?amount=N filter)
GET  /getcdprice           → implied CD price stats (discount/premium vs face value)
POST /submitcd             → submit a signed CD sell/buy offer
POST /cancelcd             → cancel own offer (signed cancellation)
POST /acceptcd             → begin accept flow: returns partial tx for buyer to co-sign
```

### `/acceptcd` Flow

This is the key endpoint that replaces the manual co-sign coordination.

**Request:**
```json
{
  "offerId": "abc123...",
  "buyerCommitKey": "<buyer's commitKey pubkey for the new CD>",
  "buyerKeyImage": "<buyer's key image for the XFG input they'll spend>"
}
```

**Response:**
```json
{
  "partialTx": "<base64-encoded partial tx hex>",
  "expiresAt": 184220,
  "instructions": "Sign with wallet command: sign_cd_accept <partialTx>"
}
```

**The partial tx contains:**
- `TransactionInputCommitmentTransfer` (seller's ring sig pending)
- `KeyInput` (buyer's XFG input, unsigned)
- `TransactionOutputCommitment` (new CD to buyer's `commitKey`, `term = cdTerm`)
- `KeyOutput` (XFG to seller)

Both parties must sign. The flow:
1. Seller posts offer, keeps offer alive
2. Buyer calls `/acceptcd` → gets partial tx
3. Buyer wallet signs the `KeyInput`
4. Buyer sends signed partial tx to seller out-of-band (or via HTLC-style timed acceptance — see below)
5. Seller wallet signs the `CommitmentTransfer`
6. Either party broadcasts

**v2 enhancement (timed acceptance):** Use the existing HTLC infrastructure to make step 4-5 atomic — buyer locks XFG in an HTLC keyed to the seller's signature on the CD transfer. Seller releases preimage by signing. This removes out-of-band coordination entirely.

---

## 5. SwapHub Integration

Add a **CD Market** tab to SwapHub (key `6`, or `c` for CD).

### Layout

```
┌─────────────────────────────────────────────────────────────────────────┐
│ ◆ SWAPHUB  ETH ▲0.46  SOL ▲1.82  BCH ▲21.3  CD/XFG ▓▓▓▓▓             │
│                                              BLK 184209                 │
├─────────────────────────────────────────┬───────────────────────────────┤
│   CD OFFERS (sorted by discount)        │   CD DETAILS                  │
│                                         │                               │
│  AMOUNT    TERM  EPOCH  ASK      DISC   │  Amount:  80.0000000 XFG     │
│  ─────── ─────── ─────  ──────── ─────  │  Term:    12 months          │
│  800 XFG   12mo  ep.9   775 XFG  -3.1%  │  Epoch:   9                  │
│  80 XFG     6mo  ep.8    76 XFG  -5.0%  │  Est. interest: ~2.1 XFG    │
│  800 XFG    3mo  ep.7   802 XFG  +0.2%  │  Ask:     76.0000000 XFG    │
│  8 XFG      1mo  ep.9     7 XFG -12.5%  │  Disc:    -5.0%              │
│  0.8 XFG    6mo  ep.8   0.77 XFG -3.7%  │  Seller:  xfg1a3f...        │
│                                         │                               │
│                                         │  [accept]  [b=bid counter]   │
│                                         │                               │
├─────────────────────────────────────────┴───────────────────────────────┤
│ > _                                     BAL 4200.00 XFG  ■ :18180     │
└─────────────────────────────────────────────────────────────────────────┘
```

### New Commands (CD tab only)

```
sell cd <amount> <ask_xfg>      Post a CD for sale at ask_xfg price
buy cd <amount> <bid_xfg>       Post a buy bid for a CD of given amount
accept <offer_id>               Accept listed CD offer (initiates co-sign flow)
cancel <offer_id>               Cancel own CD offer
```

### Discount/Premium Display

For each offer:
```
discount = (askPrice - cdAmount) / cdAmount * 100
```
Negative = selling at a discount (need liquidity). Positive = selling at a premium (interest accrual makes it worth more than face).

Display estimated interest alongside: `est_interest = cdAmount * sum(epochFeeRate[cdEpoch..now])` — fetched from `/getcdprice`.

---

## 6. Wallet Commands

Two new wallet commands in `SimpleWallet`:

```
sell_cd <key_image> <ask_price_xfg>
  Posts a CD sell offer to the gossip network.
  key_image: identifies which CD to sell.
  ask_price: what you want in liquid XFG.

buy_cd <offer_id> <my_commit_key>
  Accepts a CD offer. Constructs partial tx, prints it for out-of-band
  delivery to seller. Seller completes via complete_cd_swap.

complete_cd_swap <partial_tx_hex>
  Seller completes a co-sign. Verifies buyer's KeyInput, adds
  CommitmentTransfer ring sig, broadcasts.
```

---

## 7. Files Requiring Changes

| File | Change |
|------|--------|
| `src/CryptoNoteCore/SwapOfferRelay.h` | Add `CdOfferMsg` struct; add CD offer methods |
| `src/CryptoNoteCore/SwapOfferRelay.cpp` | Implement CD offer validation, storage, pruning |
| `src/P2p/P2pProtocolDefinitions.h` | Add `COMMAND_CD_OFFER` (ID 15), `COMMAND_CD_CANCEL` (ID 16) |
| `src/P2p/NetNode.cpp` | Register handlers for new command IDs |
| `src/Rpc/CoreRpcServerCommandsDefinitions.h` | Add structs for `/getcdoffers`, `/submitcd`, `/cancelcd`, `/acceptcd`, `/getcdprice` |
| `src/Rpc/RpcServer.h` | Declare new handlers |
| `src/Rpc/RpcServer.cpp` | Implement handlers |
| `src/SimpleWallet/SimpleWallet.cpp` | Add `sell_cd`, `buy_cd`, `complete_cd_swap` commands |
| `src/WalletLegacy/WalletTransactionSender.cpp` | Add partial tx construction for CD swap |
| `swapxfg/` (SwapHub Go) | Add CD market tab, new pair `cd`, offer/accept flows |

---

## 8. No EFier Dependency — Confirmed

| Component | Dependency |
|-----------|-----------|
| `SwapOfferRelay` | `core`, `NodeServer`, `IP2pEndpoint` only |
| `COMMAND_CD_OFFER` gossip | Any `fuegod` peer — no special node required |
| SwapHub `--daemon` | Any `fuegod` RPC endpoint |
| `/submitcd` / `/acceptcd` | Wallet RPC on same machine |

EFier code can be stripped independently without affecting any of this.

---

## 9. Risk Analysis

| Risk | Severity | Mitigation |
|------|----------|-----------|
| Stale CD offers (CD already transferred) | Medium | `pruneCdOffersForKeyImage()` called on every confirmed block; client shows "stale" warning if offer's key image appears in recent blocks |
| Seller ghosts after buyer co-signs | Medium | v2: HTLC-based atomic acceptance. v1: ttlBlocks enforces offer expiry, buyer's partial tx is worthless without seller's ring sig |
| Price manipulation via fake offers | Low | Offers require valid wallet key sig; no financial cost but also no on-chain consequence until both parties sign |
| CD interest double-counting in price discovery | Low | `/getcdprice` shows est. interest separately from ask; buyers can calibrate |
