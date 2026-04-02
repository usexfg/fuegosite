# swapxfg M4: BCH Connection (Electron Cash RPC)

> **For agentic workers:** Use superpowers:executing-plans to implement task-by-task.

**Goal:** Add Bitcoin Cash support to swapxfg via Electron Cash JSON-RPC. The C++ `HtlcScript.cpp` already exists, this plan covers the Go RPC client and TUI commands.

**BCH swap mechanics (confirmed from code):**
- **XFG side: adaptor sigs** — same Musig2 escrow flow as SOL/ETH. `SwapStateMachine` only has valid `ADAPTOR_*` transitions; HTLC states are dead code.
- **BCH side: P2SH HTLC** — `BchHtlcScript` builds a standard `OP_IF OP_HASH160 / OP_ELSE CLTV OP_ENDIF` script. The hashlock is derived from the adaptor secret `t`, so when Alice claims the BCH HTLC (revealing `preimage` on BCH chain), Bob recovers `t` and adapts his Musig2 partial sig to sweep the XFG escrow.

**Spec:** `docs/superpowers/specs/2026-03-22-swaphub-design.md` §1.5 (Electron Cash section)

**Prerequisite:** M2 (wallet RPC) — XFG side must be fundable.

**Not in scope:** ETH/SOL bridges (M3). BCH HTLC script construction is in C++ (`src/SwapDaemon/BitcoinCash/HtlcScript.cpp`) — this plan only covers the Go client and TUI.

---

## What exists

- `src/SwapDaemon/BitcoinCash/HtlcScript.cpp` — full P2SH HTLC construction: `createRedeemScript`, `computeP2shAddress`, `createClaimScriptSig`, `createRefundScriptSig`, `buildRawTransaction` (with BIP143 sighash + SIGHASH_FORKID)
- `src/SwapDaemon/AdaptorSwap.{h,cpp}` — XFG-side Musig2 escrow (shared with SOL/ETH)
- Electron Cash exposes a JSON-RPC API on `:7773` (default) — standard Bitcoin-compatible JSON-RPC

---

## New files

```
swapxfg/app/
├── bch_rpc.go        BchClient: Electron Cash RPC
└── bch_rpc_test.go   Tests for response parsing
```

---

## Task 1: `bch_rpc.go`

Electron Cash uses standard Bitcoin JSON-RPC (`method`/`params`/`id` envelope).

- [ ] **Step 1: Define BchClient**

```go
// swapxfg/app/bch_rpc.go
package app

import (
  "bytes"
  "encoding/json"
  "fmt"
  "io"
  "net/http"
  "time"
)

type BchClient struct {
  endpoint string
  client   *http.Client
  id       int
}

func NewBchClient(endpoint string) *BchClient {
  return &BchClient{
    endpoint: endpoint,
    client:   &http.Client{Timeout: 15 * time.Second},
  }
}

type bchRpcRequest struct {
  Jsonrpc string        `json:"jsonrpc"`
  Method  string        `json:"method"`
  Params  []interface{} `json:"params"`
  ID      int           `json:"id"`
}

type bchRpcResponse struct {
  Result json.RawMessage `json:"result"`
  Error  *struct {
    Code    int    `json:"code"`
    Message string `json:"message"`
  } `json:"error"`
  ID int `json:"id"`
}

func (c *BchClient) call(method string, params []interface{}, result interface{}) error {
  c.id++
  reqBody, _ := json.Marshal(bchRpcRequest{
    Jsonrpc: "1.0",
    Method:  method,
    Params:  params,
    ID:      c.id,
  })
  resp, err := c.client.Post(c.endpoint, "application/json", bytes.NewReader(reqBody))
  if err != nil {
    return fmt.Errorf("bch rpc %s: %w", method, err)
  }
  defer resp.Body.Close()
  raw, _ := io.ReadAll(resp.Body)
  var rpcResp bchRpcResponse
  if err := json.Unmarshal(raw, &rpcResp); err != nil {
    return fmt.Errorf("bch decode %s: %w", method, err)
  }
  if rpcResp.Error != nil {
    return fmt.Errorf("bch %s error %d: %s", method, rpcResp.Error.Code, rpcResp.Error.Message)
  }
  return json.Unmarshal(rpcResp.Result, result)
}
```

- [ ] **Step 2: Implement methods**

```go
// GetBalance returns confirmed + unconfirmed BCH balance in satoshis.
func (c *BchClient) GetBalance() (confirmed, unconfirmed int64, err error) {
  var result struct {
    Confirmed   int64 `json:"confirmed"`
    Unconfirmed int64 `json:"unconfirmed"`
  }
  err = c.call("getbalance", nil, &result)
  return result.Confirmed, result.Unconfirmed, err
}

// GetNewAddress returns a fresh BCH P2PKH address.
func (c *BchClient) GetNewAddress() (string, error) {
  var addr string
  return addr, c.call("getnewaddress", nil, &addr)
}

// BroadcastTx broadcasts a raw hex transaction.
func (c *BchClient) BroadcastTx(rawHex string) (string, error) {
  var txid string
  return txid, c.call("broadcast", []interface{}{rawHex}, &txid)
}

// GetRawTransaction fetches a raw transaction by txid.
func (c *BchClient) GetRawTransaction(txid string) (string, error) {
  var raw string
  return raw, c.call("gettransaction", []interface{}{txid, false}, &raw)
}

// IsConnected performs a lightweight ping (getblockcount).
func (c *BchClient) IsConnected() bool {
  var height int
  return c.call("getblockcount", nil, &height) == nil
}
```

---

## Task 2: Config + CLI

- [ ] **Step 1: Update config.go**

```go
type Config struct {
  // ... existing ...
  BchRPC string  // Electron Cash RPC endpoint (default: http://127.0.0.1:7773)
  NoBch  bool    // --no-bch: skip BCH connection
}

func DefaultConfig() Config {
  return Config{
    // ...
    BchRPC: "http://127.0.0.1:7773",
  }
}
```

- [ ] **Step 2: Update main.go** — add `--bch-rpc` flag

---

## Task 3: TUI integration

- [ ] **Step 1: Add bch fields to tuiModel**

```go
type tuiModel struct {
  // ... existing ...
  bch    *BchClient
  bchBal string   // formatted BCH balance
}
```

- [ ] **Step 2: Poll BCH balance alongside data refresh**

If `m.bch != nil && m.bch.IsConnected()`, fetch balance in background goroutine during refresh tick.

- [ ] **Step 3: Display BCH balance in ticker bar**

When BCH connected:
```
BAL 4200.000 XFG  0.05 BCH  ■ :18180
```

- [ ] **Step 4: Add `connect bch` command**

```
connect bch   → test BchClient.IsConnected(), show "BCH: connected" or error
```

- [ ] **Step 5: Add BCH HTLC commands**

```
bch lock <amount_bch_sat> <hashlock_hex> <timeout_blocks> <counterparty_bch_addr>
bch claim <htlc_txid> <htlc_vout> <preimage_hex>
bch refund <htlc_txid> <htlc_vout>
```

These commands call the SwapDaemon RPC (M5) to construct the P2SH script + tx hex, then call `bch.BroadcastTx(hex)`.

---

## Task 4: Tests

- [ ] `bch_rpc_test.go` — table-driven tests with mock HTTP server, cover: GetBalance parse, BroadcastTx, error response handling

---

## Task 5: Verify

- [ ] `go build ./...` — zero errors
- [ ] `./swapxfg --no-bch` — BCH column absent from ticker, no connection attempt
- [ ] `./swapxfg --bch-rpc http://127.0.0.1:7773` with Electron Cash running — balance shown
- [ ] `connect bch` command responds with connected status
