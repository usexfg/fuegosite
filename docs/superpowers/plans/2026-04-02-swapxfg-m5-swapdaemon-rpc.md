# swapxfg M5: SwapDaemon → fuegod RPC Bridge

> **For agentic workers:** Use superpowers:executing-plans to implement task-by-task.

**Goal:** Move SwapDaemon state machine logic into `fuegod` as a component (alongside `SwapOfferRelay`), exposing swap initiation, acceptance, state queries, and timeout checking via fuegod's existing RPC server. This eliminates the separate `swapdaemon` binary — swapxfg talks to a single `fuegod` endpoint for everything.

**Prerequisite:** None (independent of M2/M3/M4 — pure C++ change).

**Not in scope:** Go TUI changes — those happen automatically because swapxfg already calls `/getswapstatus` and `/getactiveswaps` which will just start returning real data.

---

## What currently exists

- `src/SwapDaemon/SwapDaemon.{h,cpp}` — full adaptor sig state machine, loads from `SwapDatabase`
- `src/SwapDaemon/SwapDatabase.{h,cpp}` — SQLite-backed swap persistence
- `src/SwapDaemon/FuegoRpcClient.{h,cpp}` — calls fuegod HTTP RPC internally
- `src/SwapDaemon/SwapStateMachine.{h,cpp}` — state transitions
- `src/CryptoNoteCore/SwapOfferRelay.{h,cpp}` — already in fuegod, handles gossip
- `src/Rpc/RpcServer.{h,cpp}` — fuegod RPC, has swap offer endpoints but no swap execution endpoints
- `src/Daemon/Daemon.cpp` — fuegod main, instantiates `SwapOfferRelay`

## What's missing

- `SwapDaemon` not instantiated in `Daemon.cpp`
- No RPC endpoints for: `/initiate`, `/accept`, `/getswapstatus`, `/getactiveswaps`, `/processswap`, `/refundswap`
- `SwapDaemon` currently constructs its own `FuegoRpcClient` pointing to fuegod — when embedded in fuegod, it should call core directly (bypass HTTP)

---

## Task 1: Embed SwapDaemon in fuegod

**Files:**
- Modify: `src/Daemon/Daemon.cpp`
- Modify: `src/CryptoNoteCore/Core.h` (add SwapDaemon accessor if needed)

- [ ] **Step 1: Add SwapDaemon include + member to Daemon**

In `Daemon.cpp`, alongside the `SwapOfferRelay` instantiation:

```cpp
#include "SwapDaemon/SwapDaemon.h"

// In daemon init, after SwapOfferRelay construction:
std::string swapDataDir = Tools::getDefaultDataDirectory() + "/swaps";
boost::filesystem::create_directories(swapDataDir);
auto swapDaemon = std::make_unique<XfgSwap::SwapDaemon>(
  "127.0.0.1", rpcPort, swapDataDir, logManager);
swapDaemon->start();
```

- [ ] **Step 2: Pass swapDaemon pointer to RpcServer**

`RpcServer` already receives `core&` and `SwapOfferRelay*`. Add `XfgSwap::SwapDaemon*` to `RpcServer` constructor.

---

## Task 2: New RPC structs

**File:** `src/Rpc/CoreRpcServerCommandsDefinitions.h`

- [ ] **Step 1: Add COMMAND_RPC_INITIATE_SWAP**

```cpp
struct COMMAND_RPC_INITIATE_SWAP {
  struct request {
    std::string pair;          // "sol", "eth", "xmr", "bch"
    uint64_t    xfgAmount;
    uint64_t    ctrAmount;
    std::string ctrAddress;    // counterparty chain address
    std::string peerEndpoint;  // counterparty's p2p/rpc endpoint

    void serialize(ISerializer& s) {
      KV_MEMBER(pair) KV_MEMBER(xfgAmount) KV_MEMBER(ctrAmount)
      KV_MEMBER(ctrAddress) KV_MEMBER(peerEndpoint)
    }
  };

  struct response {
    std::string swapId;
    std::string status;

    void serialize(ISerializer& s) {
      KV_MEMBER(swapId) KV_MEMBER(status)
    }
  };
};
```

- [ ] **Step 2: Add COMMAND_RPC_ACCEPT_SWAP**

```cpp
struct COMMAND_RPC_ACCEPT_SWAP {
  struct request {
    std::string swapId;
    void serialize(ISerializer& s) { KV_MEMBER(swapId) }
  };
  struct response {
    std::string status;
    void serialize(ISerializer& s) { KV_MEMBER(status) }
  };
};
```

- [ ] **Step 3: Add COMMAND_RPC_GET_SWAP_STATUS**

```cpp
struct COMMAND_RPC_GET_SWAP_STATUS {
  struct request {
    std::string swapId;
    void serialize(ISerializer& s) { KV_MEMBER(swapId) }
  };

  struct response {
    std::string swapId;
    std::string pair;
    std::string role;        // "ALICE" / "BOB"
    std::string state;       // human-readable state name
    uint64_t    xfgAmount;
    uint64_t    ctrAmount;
    std::string escrowTxHash;
    std::string ctrLockTxId;
    std::string peerEndpoint;
    uint64_t    createdAt;
    uint64_t    updatedAt;
    std::string status;

    void serialize(ISerializer& s) {
      KV_MEMBER(swapId) KV_MEMBER(pair) KV_MEMBER(role)
      KV_MEMBER(state) KV_MEMBER(xfgAmount) KV_MEMBER(ctrAmount)
      KV_MEMBER(escrowTxHash) KV_MEMBER(ctrLockTxId) KV_MEMBER(peerEndpoint)
      KV_MEMBER(createdAt) KV_MEMBER(updatedAt) KV_MEMBER(status)
    }
  };
};
```

- [ ] **Step 4: Add COMMAND_RPC_GET_ACTIVE_SWAPS**

Response contains `std::vector<COMMAND_RPC_GET_SWAP_STATUS::response> swaps`.

- [ ] **Step 5: Add COMMAND_RPC_PROCESS_SWAP + COMMAND_RPC_REFUND_SWAP**

Thin wrappers over `SwapDaemon::processSwap(swapId)` and `SwapDaemon::refundSwap(swapId)`.

---

## Task 3: RpcServer handlers

**Files:**
- Modify: `src/Rpc/RpcServer.h`
- Modify: `src/Rpc/RpcServer.cpp`

- [ ] **Step 1: Declare handlers in RpcServer.h**

```cpp
bool on_initiate_swap(const COMMAND_RPC_INITIATE_SWAP::request& req,
                      COMMAND_RPC_INITIATE_SWAP::response& res);
bool on_accept_swap(...);
bool on_get_swap_status(...);
bool on_get_active_swaps(...);
bool on_process_swap(...);
bool on_refund_swap(...);
```

- [ ] **Step 2: Register routes in RpcServer.cpp**

In the URL map (where `/getswapoffers` is registered), add:
```cpp
{ "/initiate",        { jsonMethod<COMMAND_RPC_INITIATE_SWAP>(&RpcServer::on_initiate_swap), false } },
{ "/accept",          { jsonMethod<COMMAND_RPC_ACCEPT_SWAP>(&RpcServer::on_accept_swap), false } },
{ "/getswapstatus",   { jsonMethod<COMMAND_RPC_GET_SWAP_STATUS>(&RpcServer::on_get_swap_status), true } },
{ "/getactiveswaps",  { jsonMethod<COMMAND_RPC_GET_ACTIVE_SWAPS>(&RpcServer::on_get_active_swaps), true } },
{ "/processswap",     { jsonMethod<COMMAND_RPC_PROCESS_SWAP>(&RpcServer::on_process_swap), false } },
{ "/refundswap",      { jsonMethod<COMMAND_RPC_REFUND_SWAP>(&RpcServer::on_refund_swap), false } },
```

- [ ] **Step 3: Implement handlers**

Each handler delegates to `m_swapDaemon->initiate(...)` etc. Convert `SwapStateMachine` fields to response struct.

---

## Task 4: SwapDaemon internal RPC bypass

Currently `SwapDaemon` uses `FuegoRpcClient` which makes HTTP calls back to itself. Replace with direct `core&` calls for the embedded case.

- [ ] **Step 1: Add `setCore(core& c)` method to SwapDaemon**

When set, `sendRawTransaction` calls `core.broadcastTransaction()` directly instead of HTTP.

- [ ] **Step 2: Call `swapDaemon->setCore(ccore)` in Daemon.cpp** after construction.

---

## Task 5: Background tick

SwapDaemon needs periodic `processSwap` + `checkTimeouts` calls.

- [ ] **Step 1: Add a background thread to SwapDaemon** (or reuse the existing cleanup thread pattern from `SwapOfferRelay`)

Tick every 30 seconds:
1. `checkTimeouts()` — refund any expired swaps
2. For each active swap: `processSwap(swapId)` — advance state if chain conditions met

- [ ] **Step 2: Start/stop thread in `start()` / destructor**

---

## Task 6: Remove swapdaemon binary from build (optional cleanup)

- [ ] Update `src/CMakeLists.txt` to not build `SwapDaemon/main.cpp` as a separate executable (the library target can remain for tests)

---

## Task 7: Verify

- [ ] `fuegod` starts without crash, log shows "SwapDaemon started, recovered N swap(s)"
- [ ] `curl -s http://127.0.0.1:18180/getactiveswaps` returns `{"swaps":[],"status":"OK"}`
- [ ] `curl -s http://127.0.0.1:18180/initiate` with valid params returns a swapId
- [ ] swapxfg `getswapstatus <id>` shows correct state
