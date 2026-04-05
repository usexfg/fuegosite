# Fuego TUI

A minimal terminal UI for controlling `fuegod` and `walletd`, with full support for **Burn2Mint (XFG→HEAT)** flows.

## Prerequisites

- **Go 1.20+** installed: https://go.dev/dl/
- Build the C++ binaries (`fuegod`, `walletd`) in `build/src` or have them on your PATH
- *Optional*: `xfg-stark` CLI for STARK proof generation (for Burn2Mint)

## Build

```bash
cd tui
go mod tidy
go build -o fuego-tui
```

## Run

```bash
./fuego-tui
```

Navigate with **arrow keys** or **j/k**, select with **Enter**, quit with **q** or **Ctrl+C**.

---

## Features

### 🔥 Node & Wallet Controls
- **Start/Stop Node** - Launch `fuegod` daemon with RPC
- **Start Wallet RPC** - Launch `walletd` for wallet operations
- **Create Wallet** - Generate new XFG wallet
- **Get Balance** - Query wallet balance
- **Send Transaction** - Transfer XFG to another address

### 🔥➡️💎 Burn2Mint Menu
Complete **XFG → HEAT** minting flow:

#### Flow Steps:
1. **Burn XFG on Fuego**
   - Choose burn amount: **0.8 XFG** (minimum) or **800 XFG** (large)
   - Creates `burn_deposit` transaction on Fuego blockchain
   
2. **Wait for Confirmations**
   - Transaction must be confirmed on-chain (10+ blocks)
   
3. **Request Consensus Proof**
   - Queries for **proof of burn transaction**
   - Nodes verify and sign the burn
   - Returns `eldernode_proof` used as STARK inputs
   
4. **Generate STARK Proof**
   - Calls `xfg-stark generate-proof` CLI with:
     - `--tx-hash` (burn transaction hash)
     - `--amount` (burned amount in atomic units)
     - `--eldernode-proof` (consensus proof from step 3)
   - Produces **XFG-STARK proof** for L2 verification
   
5. **Submit to Arbitrum L2**
   - User calls `claimHEAT()` on Arbitrum with:
     - STARK proof (from step 4)
     - Eldernode proof (from step 3)
     - L1 gas fees (msg.value: ~0.001-0.01 ETH)
   
6. **Receive HEAT on Ethereum L1**
   - Arbitrum relays transaction to L1
   - HEAT tokens minted on Ethereum mainnet

**RPC Endpoints Used:**
- `create_burn_deposit` - Creates burn transaction
- `request_consensus` - Requests consensus proof

**External Tools:**
- `xfg-stark` CLI - Generates zk-STARK proof

---

## Architecture

### Burn2Mint Flow Diagram

```
┌─────────────────────────────────────────────────────────────┐
│  1. Burn XFG on Fuego (0.8 or 800 XFG)                      │
│     └─> create_burn_deposit RPC → tx_hash                   │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│  2. Wait for Confirmations (10+ blocks)                     │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│  3. Request Consensus Proof                                 │
│     └─> request_consensus RPC                               │
│         Input:  tx_hash, amount                             │
│         Output: eldernode_proof ◄── CRITICAL INPUT          │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│  4. Generate STARK Proof (xfg-stark CLI)                    │
│     └─> xfg-stark generate-proof \                          │
│           --tx-hash <hash> \                                │
│           --amount <atomic> \                               │
│           --eldernode-proof <proof>  ◄── FROM STEP 3        │
│         Output: STARK proof file                            │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│  5. Submit to Arbitrum L2 (Manual/Web UI)                   │
│     └─> claimHEAT() with:                                   │
│         • STARK proof                                       │
│         • eldernode_proof                                   │
│         • msg.value (L1 gas: 0.001-0.01 ETH + 20% buffer)  │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│  6. Arbitrum → Ethereum L1                                  │
│     └─> HEAT minted on Ethereum mainnet                     │
│         Leftover gas fees automatically refunded            │
└─────────────────────────────────────────────────────────────┘
```

---

## Usage Notes

### Burn2Mint Menu
- **IMPORTANT**: Consensus is required BEFORE STARK generation
- The `eldernode_proof` from consensus is a **required input** to `xfg-stark`
- Without consensus, STARK proof cannot be generated
- L1 gas fees: Always include **20% buffer** to avoid restart costs

### Binary Detection
- TUI looks for binaries in `../build/src/` (development)
- Falls back to `$PATH` (production)
- Supported binaries: `fuegod`, `walletd`, `xfg-stark`

---

## Development

### RPC Endpoints to Implement

The following RPC methods are called by the TUI but may need implementation:

**Burn2Mint Consensus:**
- `create_burn_deposit` - Creates burn transaction
- `get_burn2mint_requests` - Lists pending Burn2Mint consensus requests
- `provide_consensus_proof` - Provides consensus proof for burn tx
- `request_consensus` - **Critical**: Returns eldernode_proof for STARK

### Testing

```bash
# Start node and wallet
./fuego-tui
# Select: Start Node → Start Wallet RPC

# Test Burn2Mint (requires xfg-stark)
# Select: Burn2Mint Menu → Choose amount → Follow prompts
```

---

## Next Steps

- [ ] Implement missing RPC endpoints in `walletd`
- [ ] Build `xfg-stark` CLI for proof generation
- [ ] Web UI for L2 submission with MetaMask integration
