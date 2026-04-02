# swapxfg M3: MetaMask + Phantom Bridges

> **For agentic workers:** Use superpowers:executing-plans to implement task-by-task.

**Goal:** Add ETH/HEAT/LUSD signing via MetaMask and SOL signing via Phantom to swapxfg. Each bridge is a local HTTP server that serves a tiny browser page; the TUI communicates with it over WebSocket.

**Spec:** `docs/superpowers/specs/2026-03-22-swaphub-design.md` §1.5 (bridge section)

**Prerequisite:** M2 (wallet RPC) — the XFG side of the swap must be fundable before the counterparty side matters.

**Not in scope:** BCH (M4), CD market (M7).

---

## Architecture

```
swapxfg TUI
  └── BridgeServer (Go, localhost:random)
        ├── GET /bridge/eth  → serves eth-bridge.html (inline, no files)
        ├── GET /bridge/sol  → serves sol-bridge.html (inline, no files)
        └── WS  /bridge/ws   → bidirectional JSON messages

Browser tab (opened by swapxfg via xdg-open / open)
  └── bridge.html
        ├── ethers.js (CDN)   or  @solana/web3.js (CDN)
        ├── MetaMask / Phantom extension
        └── WebSocket → /bridge/ws
```

Message protocol (JSON, both directions):

```json
// TUI → Browser: request
{"id": "req-1", "action": "eth_getBalance", "params": {"address": "0x..."}}
{"id": "req-2", "action": "eth_sendTx",     "params": {"to": "0x...", "value": "0x...", "data": "0x..."}}
{"id": "req-3", "action": "sol_getBalance", "params": {"pubkey": "..."}}
{"id": "req-4", "action": "sol_sendTx",     "params": {"txBase64": "..."}}

// Browser → TUI: response
{"id": "req-1", "result": "1234567890", "error": null}
{"id": "req-2", "result": "0xabc...",   "error": null}  // tx hash
{"id": "req-2", "result": null,          "error": "user rejected"}
```

---

## New files

```
swapxfg/app/
├── bridge.go         BridgeServer: HTTP + WebSocket, message dispatch, pending request map
├── bridge_eth.go     eth-bridge.html template (inline string), ETH/HEAT/LUSD action handlers
├── bridge_sol.go     sol-bridge.html template (inline string), SOL action handlers
└── bridge_test.go    Unit tests for message serialization and pending request tracking
```

---

## Task 1: `bridge.go` — core bridge server

- [ ] **Step 1: Define message types**

```go
// swapxfg/app/bridge.go
package app

import (
  "encoding/json"
  "net"
  "net/http"
  "sync"
  "github.com/gorilla/websocket"
)

type BridgeRequest struct {
  ID     string          `json:"id"`
  Action string          `json:"action"`
  Params json.RawMessage `json:"params"`
}

type BridgeResponse struct {
  ID     string `json:"id"`
  Result string `json:"result"`
  Error  string `json:"error,omitempty"`
}

type pendingReq struct {
  ch chan BridgeResponse
}

type BridgeServer struct {
  ethHTML  string
  solHTML  string
  port     int
  upgrader websocket.Upgrader
  mu       sync.Mutex
  pending  map[string]pendingReq
  conn     *websocket.Conn  // one connection at a time
  srv      *http.Server
}
```

- [ ] **Step 2: Implement Start() — bind to random port, serve HTTP routes**

```go
func NewBridgeServer() (*BridgeServer, error) {
  l, err := net.Listen("tcp", "127.0.0.1:0")
  // ...
  b.port = l.Addr().(*net.TCPAddr).Port
  // Serve /bridge/eth, /bridge/sol, /bridge/ws
}
```

- [ ] **Step 3: Implement Send(req BridgeRequest) (BridgeResponse, error)**

Non-blocking send + wait for response via channel with 30s timeout.

- [ ] **Step 4: Implement Stop()**

Close WebSocket, shut down HTTP server.

---

## Task 2: `bridge_eth.go` — ETH bridge HTML + helpers

- [ ] **Step 1: Write ethBridgeHTML constant**

Inline HTML string containing:
- `<script src="https://cdn.ethers.io/lib/ethers-5.7.2.umd.min.js">` 
- WebSocket client that connects to `ws://127.0.0.1:<port>/bridge/ws`
- Dispatches actions: `eth_getBalance`, `eth_sendTx`, `erc20_getBalance`, `erc20_approve`, `erc20_transfer`
- Calls `window.ethereum` (MetaMask) for signing

- [ ] **Step 2: Go helper methods on BridgeServer**

```go
func (b *BridgeServer) EthGetBalance(address string) (string, error)
func (b *BridgeServer) EthSendTransaction(to, value, data string) (string, error)
func (b *BridgeServer) Erc20Balance(token, address string) (string, error)
func (b *BridgeServer) OpenEthBridge() error  // xdg-open / open the URL
```

---

## Task 3: `bridge_sol.go` — Solana bridge HTML + helpers

- [ ] **Step 1: Write solBridgeHTML constant**

Inline HTML with:
- `<script src="https://cdn.jsdelivr.net/npm/@solana/web3.js@latest/lib/index.iife.min.js">`
- WebSocket client
- Actions: `sol_getBalance`, `sol_sendTx` (passes base64 tx for Phantom to sign + send)

- [ ] **Step 2: Go helper methods**

```go
func (b *BridgeServer) SolGetBalance(pubkey string) (uint64, error)
func (b *BridgeServer) SolSendTransaction(txBase64 string) (string, error)  // returns sig
func (b *BridgeServer) OpenSolBridge() error
```

---

## Task 4: Config + CLI integration

- [ ] **Step 1: Update config.go**

```go
type Config struct {
  // ... existing ...
  BridgePort int   // 0 = random (default)
  NoBridge   bool  // --no-bridge: disable bridge server
}
```

- [ ] **Step 2: Update main.go** — add `--bridge-port`, `--no-bridge` flags

- [ ] **Step 3: Update app.go Run()**

If `!cfg.NoBridge`, start `BridgeServer` before launching TUI. Pass bridge to `tuiModel`.

---

## Task 5: TUI integration

- [ ] **Step 1: Add bridge + ETH/SOL balance fields to tuiModel**

```go
type tuiModel struct {
  // ... existing ...
  bridge    *BridgeServer
  ethAddr   string
  ethBal    string
  solAddr   string
  solBal    string
}
```

- [ ] **Step 2: Wire `connect metamask` and `connect phantom` commands**

```
connect metamask   → bridge.OpenEthBridge(), set ethAddr
connect phantom    → bridge.OpenSolBridge(), set solAddr
```

- [ ] **Step 3: Display ETH/SOL balance in ticker bar alongside XFG balance**

When both wallet + bridge connected:
```
BAL 4200.000 XFG  0.45 ETH  :18180
```

- [ ] **Step 4: Wire ETH HTLC send command (for swap completion)**

```
eth lock <amount_eth> <htlc_contract> <hashlock> <timeout>
```
Calls `bridge.EthSendTransaction(htlcContract, value, htlcCalldata)`.

---

## Task 6: Verify

- [ ] `go build ./...` — zero errors
- [ ] `./swapxfg --bridge-port 9090` → `http://127.0.0.1:9090/bridge/eth` serves HTML
- [ ] `./swapxfg --no-bridge` → bridge server not started, no port opened
- [ ] `connect metamask` command opens browser page in tests (mock xdg-open)
