# HEAT vs LP Pool ZK Proofs — Why Separate

## The Core Problem

The existing fuego-prover circuit proves **one thing**:
> "These commitment hashes really exist in these blockchain blocks"

The LP pool needs to prove **something completely different**:
> "This swap/deposit/withdrawal was calculated correctly according to AMM rules"

---

## Diagram: Two Different Worlds

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        HEAT COMMITMENT PROOF                            │
│                         (fuego-circuit)                                 │
│                                                                         │
│  INPUT:                                                                 │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐                     │
│  │ Block 1001  │  │ Block 1002  │  │ Block 1003  │  ...                │
│  │ tx_extra[]  │  │ tx_extra[]  │  │ tx_extra[]  │                     │
│  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘                     │
│         │                │                │                              │
│         └────────────────┴────────────────┘                              │
│                          │                                               │
│              scan for tag 0x08                                           │
│              extract commitment hashes                                   │
│                          │                                               │
│              ┌───────────▼───────────┐                                   │
│              │   Build Merkle Tree   │                                   │
│              │   (just hash leaves)  │                                   │
│              └───────────┬───────────┘                                   │
│                          │                                               │
│  OUTPUT (public):                                                        │
│  ┌─────────────────────┬─────────────────────┬──────────────────┐       │
│  │ new_merkle_root     │ new_checkpoint_hash │ height_end       │       │
│  │ [32 bytes]          │ [32 bytes]          │ [4 bytes]        │       │
│  └─────────────────────┴─────────────────────┴──────────────────┘       │
│                                                                         │
│  WHAT IT PROVES: "I scanned real blocks and these commitments exist"    │
└─────────────────────────────────────────────────────────────────────────┘


┌─────────────────────────────────────────────────────────────────────────┐
│                        LP POOL PROOF (NEW - needed)                     │
│                       (fuego-lp-circuit)                                │
│                                                                         │
│  INPUT:                                                                 │
│  ┌───────────────────────────────────────────────────────────┐          │
│  │ Pool State BEFORE:                                        │          │
│  │   reserveA = 1,000,000 XFG                                │          │
│  │   reserveB = 500,000 BTC                                  │          │
│  │   totalLPShares = 707,106                                 │          │
│  └───────────────────────────────────────────────────────────┘          │
│                          │                                               │
│  ┌───────────────────────────────────────────────────────────┐          │
│  │ Operation: SWAP 100 XFG → BTC                             │          │
│  │   feeBps = 30 (0.3%)                                      │          │
│  └───────────────────────────────────────────────────────────┘          │
│                          │                                               │
│              ┌───────────▼───────────┐                                   │
│              │  1. Calculate fee:    │                                   │
│              │     fee = 100 * 0.003 = 0.3 XFG                          │
│              │  2. Apply AMM:        │                                   │
│              │     input = 99.7 XFG  │                                   │
│              │     output = k/(rA+input) - rB                           │
│              │  3. Update reserves   │                                   │
│              │  4. Rebuild Merkle trees                                   │
│              └───────────┬───────────┘                                   │
│                          │                                               │
│  OUTPUT (public):                                                        │
│  ┌──────────┬──────────┬───────────┬──────────┬─────────────────┐       │
│  │ lpRoot   │ feeRoot  │ reserveA  │ reserveB │ feeCollected    │       │
│  │ [32B]    │ [32B]    │ [8B]      │ [8B]     │ [8B]            │       │
│  └──────────┴──────────┴───────────┴──────────┴─────────────────┘       │
│                                                                         │
│  WHAT IT PROVES: "This swap math is correct, fees are fair"             │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## Why One Circuit Can't Do Both

```
┌──────────────────────────────────────────────────────────────────┐
│                        COMPARISON TABLE                          │
├─────────────────────┬────────────────────┬───────────────────────┤
│                     │ HEAT PROOF         │ LP POOL PROOF         │
├─────────────────────┼────────────────────┼───────────────────────┤
│ What's proven?      │ Commitments exist  │ Math is correct       │
│                     │ in the blockchain  │ (AMM, fees, shares)   │
├─────────────────────┼────────────────────┼───────────────────────┤
│ Input data          │ Block headers +    │ Pool state +          │
│                     │ tx_extra bytes     │ operation params      │
├─────────────────────┼────────────────────┼───────────────────────┤
│ Computation         │ Parse tag 0x08,    │ AMM formula,          │
│                     │ hash extraction    │ fee calc, share math  │
├─────────────────────┼────────────────────┼───────────────────────┤
│ Merkle tree         │ 1 tree:            │ 2 trees per pool:     │
│                     │ commitment hashes  │ LP shares + fee recs  │
├─────────────────────┼────────────────────┼───────────────────────┤
│ Public outputs      │ merkle_root,       │ lpRoot, feeRoot,      │
│                     │ checkpoint, height │ reserves, fees        │
├─────────────────────┼────────────────────┼───────────────────────┤
│ Can they merge?     │ NO — they prove    │                       │
│                     │ fundamentally      │                       │
│                     │ different things   │                       │
└─────────────────────┴────────────────────┴───────────────────────┘
```

---

## How They Connect (Fee Routing to fuego-prover)

```
┌─────────────────────────────────────────────────────────────────────┐
│                      THE FEE ROUTING FLOW                           │
│                                                                     │
│  ┌─────────────┐                                                    │
│  │  Banking    │  0.1% Elderfier fee                                │
│  │ Transaction │ ──────────────────────────────────────────┐        │
│  └─────────────┘                                           │        │
│         │                                                  │        │
│         ▼                                                  ▼        │
│  ┌─────────────────────────┐                    ┌──────────────────┐ │
│  │ LP Pool Circuit         │                    │ fuego-prover     │ │
│  │ (fuego-lp-circuit)      │                    │ (fuego-circuit)  │ │
│  │                         │                    │                  │ │
│  │ Proves: swap math OK    │                    │ Proves: HEAT     │ │
│  │ Outputs: feeCollected   │                    │ commitments OK   │ │
│  │           = 0.1% fee    │                    │                  │ │
│  └───────────┬─────────────┘                    └────────┬─────────┘ │
│              │                                           │           │
│              │  feeCollected becomes a                   │           │
│  ┌───────────▼─────────────┐                    ┌───────▼──────────┐ │
│  │  Fee Commitment         │                    │  Block           │ │
│  │  (new LP fee tree leaf) │                    │  Commitments     │ │
│  │                         │                    │  (tag 0x08)      │ │
│  └─────────────────────────┘                    └──────────────────┘ │
│              │                                           │           │
│              └───────────────────┬───────────────────────┘           │
│                                  │                                   │
│                                  ▼                                   │
│                    ┌─────────────────────────┐                       │
│                    │  Verifier / Bridge      │                       │
│                    │                         │                       │
│                    │  Checks both proofs     │                       │
│                    │  Confirms fee is valid  │                       │
│                    │  AND exists on-chain    │                       │
│                    └─────────────────────────┘                       │
└─────────────────────────────────────────────────────────────────────┘
```

---

## What CAN Be Shared (Reuse, Not Merge)

```
┌──────────────────────────────────────────────────────────────────┐
│                    SHARED BUILDING BLOCKS                        │
│              (used by BOTH circuits, but separate proofs)        │
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐  │
│  │  SP1 zkVM runtime                                          │  │
│  │  (the "engine" that runs both circuits)                    │  │
│  └────────────────────────────────────────────────────────────┘  │
│         │                    │                    │               │
│         ▼                    ▼                    ▼               │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────────┐      │
│  │ Merkle Tree │    │ Keccak256   │    │ Checkpoint Hash │      │
│  │ Functions   │    │ Hashing     │    │ Computation     │      │
│  │ (from       │    │ (from       │    │ (from           │      │
│  │  fuego-core)│    │  fuego-core)│    │  fuego-core)    │      │
│  └─────────────┘    └─────────────┘    └─────────────────┘      │
│                                                                  │
│  These are library functions — both circuits import them         │
│  from fuego-core/src/lib.rs. They don't merge the proofs.        │
└──────────────────────────────────────────────────────────────────┘
```

---

## Summary

```
HEAT proof  = "These commitments are in real blockchain blocks"
LP pool proof = "This swap/deposit/withdrawal math is correct"

They are as different as:
  - A receipt proving you bought something (HEAT proof)
  - A calculator proving the math on the receipt is right (LP proof)

Both are needed. Both use the same hash functions.
But they cannot be the same proof.
```
