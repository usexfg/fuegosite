# DIGM on Fuego: Architecture Quick Guide

> **Status:** Draft / Brainstorm Output
> **Date:** 2026-04-02 (rev 2026-04-03: I2P, mobile-first full node, ParaDio-only rewards, per-album pools)
> **Context:** Fuego drops L2/L3/Elderfier path. Focuses on XFG + CD + atomic swaps + hidden amounts. DIGM becomes the use-case layer that drives swap volume and gives CDs real yield.
> **Note:** The .digm audio container / proof-of-origin format from DIGM-origins is NOT part of this design. However, several economic decisions (PARA rate, curator share, audio codec) are inherited from DIGM-origins docs and noted inline.

---

## 1. The Decision

**We chose:** XFG + CD + atomic swaps (no L2, no L3, no Elderfiers required)

**Why:**
- Elderfiers were only consensus-critical for L2 merkle root signing
- Fee pool is purely algorithmic (consensus rules, no EFier quorum needed)
- Atomic swaps work P2P (adaptor sigs, indistinguishable from ring spends)
- Engineering focus shifts to hidden amounts (BP+ / MLSAG) — the real privacy unlock
- Lower regulatory surface (no bridge, no ERC-20 tokens)
- DIGM becomes the swap volume driver: music purchases in XFG drive fees drive CD yield

**What Fuego becomes:** A privacy-focused store-of-value chain with native atomic swaps, on-chain yield (CD fee pool), and DIGM as its killer app.

---

## 2. The Eight Assets

### On-Chain (Fuego L1)

| Asset | Type | Purpose |
|-------|------|---------|
| **XFG** | Native coin | Real-value payments. Buy albums, swap cross-chain, mine. The money. |
| **CD** | Commitment Deposit | Locked XFG that earns yield from swap fee pool (0.3% per swap). Privacy flywheel — more locks = bigger ring pools = better privacy. |
| **DIGM** | Colored coin (on-chain) | Artist/publisher hosting rights. Fixed supply: 100,000. Required to publish albums on the platform. Anti-spam + quality gate. |

### In-App (DIGM platform layer, anchored on-chain)

| Asset | Type | Purpose |
|-------|------|---------|
| **PARA** | Earned token | Earned by listening (streaming proofs). Staked on albums as "like pools." Fuel for the entire DIGM economy. |
| **VOX** | Transmuted token | Created when PARA transmutes (your staked album hits #1). Platform currency for cosmetics, add-ons, customization. Status symbol. Burn for CURA. |
| **CURA** | Burned-to-mint token | Minted only by burning VOX. Grants public curator rights — playlist creation, editorial power, posting permissions. Scarce and earned. |
| **nfVOX** | Non-fungible award | One-per-#1-album. Permanent commemoration awarded to the artist whose album hits #1. Proof of a hit. |
| **TOP** | Singleton trophy | Rolling championship token. Held by the artist of the *current* #1 album. Transferred when dethroned. Only 1 exists. |

### Asset Flow Diagram

```
Mining/Swaps ──→ XFG ──→ Buy albums (artist gets 100%)
                  │
                  ├──→ Lock as CD ──→ Earn yield from fee pool
                  │                        ↑
                  │              Swap fees (0.3%) feed pool
                  │                        ↑
                  └──→ Atomic Swap (SOL/ETH/XMR ↔ XFG)
                              ↑
                    Music purchases drive swap demand


Listening ──→ PARA (streaming proofs)
               │
               ├──→ Stake on album pool (digm_id)
               │         │
               │     Album hits #1?
               │      /          \
               │    YES           NO
               │     │             └──→ PARA stays PARA
               │     │                  (withdraw anytime)
               │     ▼
               │   TRANSMUTATION:
               │   ├─ All PARA in pool → given to artist
               │   ├─ Each staker's PARA → VOX (time-weighted multiplier)
               │   ├─ Artist gets nfVOX (permanent #1 trophy)
               │   └─ Artist holds TOP (until dethroned)
               │
               └──→ VOX
                     │
                     ├──→ Buy cosmetics, add-ons, customization
                     ├──→ Burn → request next song on Paradio
                     └──→ Burn → mint CURA (curator rights)
```

---

## 3. Album Pool Mechanics

### How Staking Works

1. Artist publishes album → gets a `digm_id` (content hash of the album)
2. Listeners stake PARA into the album's pool
3. Earlier stakers earn higher time-weighted multiplier
4. Rankings determined by PARA pool size per epoch

### When an Album Hits #1 (Epoch Winner)

```
┌─────────────────────────────────────────────┐
│  EPOCH #1 DETERMINATION                     │
│  (largest PARA pool at epoch close)         │
│                                             │
│  1. ALL PARA drained from winning pool      │
│  2. Raw PARA → sent to artist               │
│  3. Artist awarded nfVOX (permanent)        │
│  4. Artist receives TOP (held until         │
│     dethroned by next epoch's winner)       │
│  5. Each staker's PARA transmutes to VOX:   │
│     vox_amount = para_staked *              │
│       multiplier(time_since_stake)          │
│     (earlier deposit = higher multiplier)   │
│                                             │
│  EPOCH WINNERS ARE COMMEMORATED PERMANENTLY │
│  (not to be forgotten)                      │
└─────────────────────────────────────────────┘
```

### Non-winning Pools

- PARA stays staked, continues accruing time weight
- Listener can withdraw anytime (but loses time position)
- Pools persist across epochs until album wins or listener withdraws

---

## 4. The Listener Platform (RP1/Oasis Vibes)

### Identity
- **Aliases** — public leaderboard names, wallet stays private
- **Profiles** — public stats, private identity
- **Listening history** — opt-in visibility

### Leaderboards
- **Listener rankings** — by VOX earned, curator accuracy, streaks
- **PARA LP leaders** — who's staking the most across pools
- **Album charts** — top by pool size, growth rate, #1 history
- **Curator rankings** — CURA holders ranked by successful picks
- **Epoch Hall of Fame** — permanent record of every #1 winner

### Interactions
- **Hashrate voting** — miners vote on next track for Paradio
- **VOX burn → request song** — spend VOX to influence radio queue
- **CURA powers** — public playlists, editorial posts, curation rights

### Philosophy
- **About the music, not the artist** — staking is per-album, not per-person
- **Anonymous artists work** — publish without revealing identity
- **Every release stands alone** — no carryover reputation from past albums
- **Gamified listener experience** — discovery, staking, competing, earning

### Two-Sided Platform Design

The DIGM app is intentionally split into two surfaces, both privacy-preserving:

| Side | Audience | Surfaces |
|------|----------|----------|
| **Artist / Album side** | Publishers, labels, anonymous creators | DIGM coin holding, album registration, single uploads (OGG/Opus), pricing in XFG, per-album staking-pool dashboard, ParaDio earnings dashboard, revenue dashboard, payout in XFG |
| **Listener side** | Everyone | ParaDio (gamified single streaming with rewards), album marketplace, staking pools, leaderboards (aliased), profile/customization (VOX), curator tools (CURA) |

Both sides ship in the same app — switching is just a UI toggle. A listener with a DIGM coin becomes a publisher; a publisher who explores ParaDio becomes a listener.

---

## 4a. ParaDio Reward Model (the only reward source)

**Decision:** ParaDio (single streaming) is the **only** activity that pays PARA rewards. No rewards for:
- Offline listening (cached playback)
- Album purchases (artist already gets paid in XFG)
- Album-mode plays (full album playback after purchase)

**Why:** Rewards are an incentive for *discovery*, and discovery only happens via ParaDio's curated/random/algorithmic single stream. Buying an album is already its own reward (you wanted it). Offline plays can't be verified by the network anyway.

### Per-Stream Reward Distribution

When a single is streamed via ParaDio and the listen is verified (anti-bot proof passes):

```
1 verified ParaDio listen
        │
        ├──────────────────────────────────────────────────────┐
        │                                                      │
   LISTENER                                              ARTIST
   share of PARA                                       share of PARA
        │                                                      │
   ┌────┴─────┐                                          ┌────┴─────┐
   │          │                                          │          │
 70% to     30% to                                     70% to     30% to
 listener   curator                                    artist     curator
            (if listened                                          (if listened
             via curator's                                         via curator's
             playlist)                                             playlist)
```

**Curator role in rewards:**
- Curators do **not** earn directly from a system pool
- A curator earns **30% of the listener's PARA share** + **30% of the artist's PARA share** when a stream is played through that curator's playlist
- If the listen is *not* via a curator playlist, the listener and artist each keep 100% of their share (curator slice = 0)
- This matches DIGM-origins `CURATOR_REVENUE_SHARE_MODEL.md` and `AUDIO_FORMAT_STRATEGY.md`

**Base PARA earning rate:** 1 PARA per minute of verified listening *(inherited from DIGM-origins AUDIO_FORMAT_STRATEGY.md line 98)*. Subject to epoch caps (TBD).

### PARA flow: earning is global, staking is per-album

Two things are cleanly separated:

1. **Earning** — listeners earn PARA from ParaDio streams (listener + artist split per §4a, curator skims when applicable). Earned PARA lands in the listener's / artist's balance. It is **not** earmarked to any album.
2. **Staking** — listeners take PARA (from *any* source — earned, bought, gifted) and stake it into an album's staking pool as a discovery / belief signal. This is the "like pool."

```
                  ┌──────────────────────┐
                  │   Your PARA balance  │
                  │ (earned from ANY     │
                  │  ParaDio stream)     │
                  └──────────┬───────────┘
                             │ stake freely
             ┌───────────────┼───────────────┐
             │               │               │
             ▼               ▼               ▼
     ┌────────────┐  ┌────────────┐  ┌────────────┐
     │  Album A   │  │  Album B   │  │  Album C   │
     │ stake pool │  │ stake pool │  │ stake pool │
     │ (time-     │  │ (time-     │  │ (time-     │
     │  weighted) │  │  weighted) │  │  weighted) │
     └────────────┘  └────────────┘  └────────────┘
```

- Each album pool is purely a **staking / discovery pool**. It has no "fee pool" sub-compartment. It's just PARA locked in with a timestamp.
- CD fee pool (Fuego L1 swap fees) is **completely separate** from DIGM and does not interact with PARA pools in either direction. DIGM is a use-case *built on top of* Fuego; it does not siphon from or feed the CD yield.

### Epoch

**Decision:** epoch = **~5 days = 900 blocks** (same cadence as the Fuego swap-fee epoch). Every 900 blocks the #1 album is determined, transmutation fires on the winning pool, and leaderboards close out.

### The "stays #1" problem (still open — see §11)

What happens if an album wins epoch N and is still the top pool at the end of epoch N+1? And what if it drops to #2 for one epoch then comes back to #1? Possible directions to pick from:

- **A. Drain-and-reset every win.** Each epoch, the winning pool fully drains (PARA→VOX, artist gets the raw PARA lump, nfVOX minted). If the same album wins the next epoch, it means listeners re-staked — so it's earned again. Downside: artist gets repeated nfVOX; TOP thrashing.
- **B. Drain-and-reset, but nfVOX only on first #1.** Same as A, but nfVOX is one-per-album-lifetime. Subsequent wins give artist the raw PARA + keep TOP but no extra nfVOX.
- **C. Partial drain.** Only stake older than X blocks drains; fresh stake rolls forward. Lets believers "leave some in" without losing position.
- **D. Dynasty mode.** First win drains; while the album holds #1 across consecutive epochs, no drain fires (artist earns a per-epoch "reign stipend" instead). First epoch the album is dethroned, a final drain fires on the position as of the dethrone block. Comeback from #2 → #1 restarts a new dynasty.

My gut says **D (dynasty)** is the most interesting and best matches "epoch winners are not to be forgotten." Tell me which you want and I'll spec it.

### What counts as a listen?

**Decision (rough):** Listener side runs a **per-second** listen proof. A listen is valid when all of these hold:
- Stream is actually being decoded (per-second audio-frame heartbeat to local node)
- Network volume is consistent with an Opus 96 kbps stream (chunk fetch cadence matches decode cadence)
- Listener's **not-a-bot score** is above threshold (scored locally on interaction signals + longitudinally per alias)
- The serving peer co-signs a per-minute listen receipt (listener↔seeder receipt pair, settled in the merkle anchor)

Exact formula, scoring weights, and on-chain vs in-merkle settlement are §11 open questions.

---

## 5. Where Things Live

### Fuego L1 (consensus-level)

- XFG transfers (ring signatures, privacy)
- CD locks + fee pool yield (commitment deposits)
- Atomic swaps (adaptor signatures: SOL, ETH, XMR)
- Swap fee collection (0.3% → fee pool)
- DIGM colored coin (hosting rights, fixed 100K supply)
- Periodic state anchoring (merkle root of DIGM-app balances)

### DIGM App Layer (off-chain, anchored)

- PARA earning (streaming proofs)
- PARA staking pools (per-album "like pools", tracked by digm_id — any PARA from any source can be staked)
- VOX transmutation (when album hits #1)
- CURA minting (VOX burn)
- nfVOX awards (per #1 album)
- TOP trophy (current #1 holder)
- Leaderboards, aliases, profiles
- Paradio radio + hashrate voting + VOX song requests

---

## 6. The Flywheels

DIGM and the CD fee pool are **two independent flywheels** that share XFG as a medium. They do not feed each other's pools — they just both benefit when XFG has more users and more swap volume.

### Flywheel 1: Fuego core (CD ↔ swaps ↔ privacy)

```
Users want to hold / pay privately
          │
    More swaps (SOL/ETH/XMR ↔ XFG)
          │
    0.3% swap fee → CD fee pool
          │
    CDs pay real yield → more XFG locked as CD
          │
    Ring pool grows → privacy improves
          │
    More users want XFG → more swaps ──→ (loop)
```

This flywheel is **self-contained on Fuego L1**. It does not know DIGM exists.

### Flywheel 2: DIGM (ParaDio ↔ staking ↔ discovery)

```
Listeners stream via ParaDio
          │
    Earn PARA (listener + artist + curator split)
          │
    Stake PARA in album "like pools" for discovery
          │
    Top pools win the epoch → PARA transmutes to VOX
          │
    VOX = status, cosmetics, CURA (curator rights)
          │
    Better curation → better discovery → more streams ──→ (loop)
```

### Where the two touch

- **Album purchases** are XFG → artist direct payment. That XFG may come from a user who swapped into XFG, which means DIGM *indirectly* generates swap volume that feeds the CD pool. But no PARA, VOX, CURA, or DIGM-app state ever touches the CD pool.
- **XFG is the only shared asset.** PARA is not a gas token, does not pay swap fees, and does not interact with CDs.

---

## 7. Audio Storage: Invisible P2P over I2P (Decided)

**Decision:** I2P/μTP P2P distribution where every DIGM client is a node. Seeding is automatic and invisible — the listener never thinks about it.

**Why I2P over Tor:**
- I2P is built for peer-to-peer (Tor is built for client→server)
- Garlic routing handles bidirectional flows better than onion circuits
- Built-in distributed hashtable (netDb) — no separate DHT layer
- Every I2P node is a router by default (matches "every client is a seeder")
- No exit nodes needed — all traffic stays inside the I2P network
- Tor's stance discourages high-bandwidth P2P; I2P welcomes it

### How It Works

The DIGM app is a music player that happens to be a P2P node. Like BitTorrent — you don't configure DHT or chunks, you just use the app.

```
┌─────────────────────────────────────────────────────────┐
│  WHAT THE LISTENER SEES     WHAT ACTUALLY HAPPENS       │
│  ─────────────────────────  ──────────────────────────  │
│  "Browse albums"          → P2P discovery over I2P      │
│  "Buy album" (pay XFG)   → Download + decrypt chunks,  │
│                             auto-seed                   │
│  "Play track"             → Stream from local cache     │
│                             or nearest peer             │
│  "Stake PARA on album"   → Pin chunks, start seeding   │
│  "Close app"              → Background daemon seeds     │
│                             (toggle off in settings)    │
└─────────────────────────────────────────────────────────┘
```

### Seeding Rules (All Automatic, No User Config)

| Condition | Behavior |
|-----------|----------|
| Bought an album | Seed it (you have the chunks anyway) |
| Staked PARA on album | Pin it (don't evict from cache) |
| Cache full | Evict oldest unstaked albums first |
| App closed | Background seeding on by default (user can toggle off) |
| New album, zero buyers | Only artist's node has it (bootstraps from there) |

### Availability Model

```
Album published → artist node is sole seeder
         ↓
First buyers → each buyer becomes a seeder
         ↓
PARA stakers → pinned chunks, reliable seeders
         ↓
Popular album → many seeders → high availability
         ↓
Organic CDN: popularity = availability
```

- **Popular albums** are highly available (many buyers + stakers seeding)
- **Niche albums** rely on artist node + small buyer base
- **PARA staking = availability commitment** — stakers keep the music alive on the network
- **No central servers, no pinning fees, no infrastructure cost**

### Technical Stack

- **Transport:** μTP over I2P streaming sockets (LEDBAT congestion control, zero port forwarding)
- **Privacy:** All transfers through I2P destinations (IP never exposed, garlic-routed)
- **Chunks:** 256 KB encrypted chunks (ChaCha20), SHA-256 content addressing — chunk size inherited from DIGM-origins (DIGM_COLD_CELESTIA_DETAIL.md)
- **Discovery:** I2P netDb (built-in DHT) + peer exchange — no separate DHT to bootstrap
- **Audio formats:**
  - **Singles (ParaDio streaming):** OGG/Opus 96 kbps VBR — small, gapless, royalty-free *(inherited from DIGM-origins AUDIO_FORMAT_STRATEGY.md)*
  - **Album downloads:** FLAC lossless *(inherited from DIGM-origins AUDIO_FORMAT_STRATEGY.md)*
- **Encryption:** Buyer receives decryption key on XFG payment, decrypts on-the-fly. ParaDio singles are unencrypted (free streaming, reward-bearing).

### Why Not Something Else

| Alternative | Why Not |
|-------------|---------|
| Celestia DA | Shelved with L2/L3 |
| IPFS + pinning | Costs money, IPs exposed, requires infrastructure |
| Arweave | ~$1600/TB, overkill for streaming audio |
| Central CDN | Against everything DIGM stands for |
| Elder Nodes (staked) | Killed with Elderfiers |

---

## 8. App Architecture: Mobile-First Full Node

**Decision:** DIGM is **mobile-first** and ships a **full Fuego node** (not SPV) inside the app. Every listener phone is a real Fuego validator AND an I2P seeder for the music it touches. Desktop is a secondary build of the same Rust core.

**Why a full node on mobile:**
- Listeners need to verify CD yields, fee pool state, and DIGM colored coin transfers without trusting a remote node
- A full node is the only way to seed P2P music chunks from a phone with strong privacy
- Fuego's 8-minute block target + ~180 blocks/day means the chain stays small enough to run on a phone (storage budget below)
- It eliminates the SPV trust assumption, which is the entire point of running on Fuego

### What the User Sees vs What's Running

```
┌─────────────────────────────────────┐
│          DIGM App (user-facing)     │
│  ┌───────────┐  ┌───────────────┐  │
│  │ Music     │  │ Wallet        │  │
│  │ Player    │  │ (XFG/PARA/VOX)│  │
│  └───────────┘  └───────────────┘  │
│  ┌───────────┐  ┌───────────────┐  │
│  │ Browse /  │  │ Staking /     │  │
│  │ Discovery │  │ Leaderboards  │  │
│  └───────────┘  └───────────────┘  │
├─────────────────────────────────────┤
│      Under the hood (invisible)     │
│  ┌───────────┐  ┌───────────────┐  │
│  │ Full      │  │ I2P Router    │  │
│  │ Fuego Node│  │ (i2pd or      │  │
│  │ (libfuego)│  │  i2p-rs)      │  │
│  └───────────┘  └───────────────┘  │
│  ┌───────────┐  ┌───────────────┐  │
│  │ Chunk     │  │ Audio Codec   │  │
│  │ Cache +   │  │ (Symphonia/   │  │
│  │ Seeder    │  │  Opus + FLAC) │  │
│  └───────────┘  └───────────────┘  │
└─────────────────────────────────────┘
```

### Mobile (PRIMARY TARGET)

| Component | Tech | Size | Notes |
|-----------|------|------|-------|
| App shell | Native (Swift on iOS, Kotlin/Compose on Android) | ~15 MB | Native UI for the best mobile UX, calls Rust core via FFI |
| **Full Fuego node** | Rust crate `libfuego` (cross-compiled to arm64) | ~12 MB | Real validator: blocks, mempool, ring sigs, CDs, fee pool, DIGM colored coin |
| I2P router | Rust (i2p-rs) or i2pd binding | ~6 MB | Pure-Rust I2P router preferred for cross-compile simplicity |
| μTP transport | libutp (Rust bindings) | ~1 MB | LEDBAT congestion control over I2P streaming sockets |
| Audio decode | Symphonia (Rust) — Opus + FLAC | ~2 MB | Cross-platform, no ffmpeg, matches OGG-Opus singles + FLAC albums |
| Chunk cache | SQLite | ~1 MB | Track metadata, chunk index, pin/seed state |
| **Total** | | **~37 MB binary** | Reasonable for a full music app + node |

**Storage budget on phone:**
- Fuego full chain @ ~180 blocks/day, growing slowly: ~2 GB at year 5, ~5 GB at year 10 (estimate, validate with current chain growth)
- I2P netDb + tunnel state: ~50 MB
- Music chunk cache (configurable): default 4 GB on mobile, user can raise/lower
- Total default footprint: ~6-10 GB. Acceptable on modern phones (128 GB+)

**Mobile reality check:**
- iOS Background App Refresh + audio playback entitlements keep the node + seeding alive during music playback
- When music stops + app backgrounded, OS will eventually suspend; on resume, the node syncs the gap
- "Charger + WiFi" mode: full seeding, full sync, full I2P router participation
- "Mobile data + battery" mode: throttle seeding, prioritize playback + reward-claim verification
- Pruning mode (optional): keep only headers + UTXO set + last 10k blocks for storage-constrained devices

### Desktop (Secondary Build of Same Core)

| Component | Tech | Size | Notes |
|-----------|------|------|-------|
| App shell | Tauri (Rust + system WebView) | ~10 MB | Same Rust core, web-frontend UI |
| UI | Web (HTML/CSS/JS) | ~5 MB | System WebView, no bundled Chromium |
| Full Fuego node | `libfuego` (same crate) | ~12 MB | Identical validator code |
| I2P router | i2p-rs / i2pd | ~6 MB | Same as mobile |
| μTP transport | libutp | ~1 MB | Same |
| Audio decode | Symphonia | ~2 MB | Same |
| Chunk cache | SQLite | ~1 MB | Same schema |
| **Total** | | **~37 MB** | Compare: Spotify ~300 MB |

Desktop runs the same Rust core; only the UI shell differs. Desktop gets larger default cache (32 GB) and unrestricted background seeding.

### Shared Rust Core (`libfuego` + companions)

```
libfuego_core (Rust workspace)
├── fuego-node       (full validator, mempool, RPC — derived from C++ src/CryptoNoteCore)
├── fuego-wallet     (key mgmt, ring sigs, CDs, atomic swap state machine)
├── digm-app         (PARA earning, staking pools, merkle anchoring, leaderboards)
├── i2p-net          (I2P router + μTP transport)
├── chunk-store      (SQLite-backed, content-addressed, pin/seed scheduler)
└── ffi-bridge       (UniFFI / cbindgen — Swift + Kotlin + Tauri all consume this)
```

One Rust core, three UI shells (iOS Swift, Android Kotlin, desktop Tauri). Same node behavior everywhere.

### Startup Sequence

```
1. Launch app                                  (~0.5s)
2. Open SQLite + key store                     (~0.1s)
3. Start I2P router (resume saved tunnels)     (~3-8s first time, ~1s warm)
4. Resume Fuego full-node sync                 (~0.5s — pick up from last block)
5. Resume seeding pinned chunks                (~0.2s)
6. UI ready — library + discovery from cache   (immediate)
7. Sync gap while user browses                 (background, parallelizable)
```

First launch on a fresh install: longer (~initial Fuego sync via I2P peers, time depends on chain size — show progress + let user browse cached metadata immediately).

---

## 9. What We Shelved (Can Revisit Later)

- L2/L3 HEAT/COLD token minting (Arbitrum, STARK proofs)
- Elderfier consensus (merkle root signing, quorum)
- C0DL3 gas token / L3 rollup
- Celestia DA for audio blob storage
- EVM composability (NFTs, DeFi on music)
- xfg-stark-cold-starks/ codebase (preserved, deprioritized)
- DIGM-origins `.digm` audio container format + hardware-Secure-Enclave proof-of-recording (the *format* is shelved; PARA economics, OGG-Opus, FLAC, curator share, chunk size, and other prior decisions are inherited)

### HEAT clarification

HEAT is **still a concept** — it just isn't an L2 ERC-20 anymore. With L2/STARKs/COLD shelved, HEAT survives as:

- **A unit/denomination label for very small amounts of XFG**, useful in app UI ("track price: 250,000 HEAT" reads better than tiny XFG decimals).
- **A possible rebranding for the smallest ParaDio reward unit** so listeners "earn HEAT" for streams (more visceral than "earn 0.0000xxx PARA").

HEAT no longer implies a burn → ERC-20 mint flow. There is no HEAT contract on Arbitrum. There is no HEAT bridge. Anywhere you see "HEAT" in DIGM UI, read it as a unit label.

---

## 10. Token Tracking: Hybrid Anchoring (Decided)

**Decision:** Hybrid off-chain with periodic on-chain merkle anchoring.

### How It Works

```
DIGM app tracks all balances in real-time (fast, flexible)
         │
    Every epoch boundary (N blocks)
         │
    Compute merkle root of ALL app-layer state:
    ├─ PARA balances + active staking positions
    ├─ VOX balances
    ├─ CURA balances
    ├─ nfVOX assignments (album digm_id → artist)
    ├─ TOP current holder
    └─ All active album staking pools
         │
    Publish root in Fuego tx_extra (new tag, 32 bytes + metadata)
         │
    Anyone can request merkle proof of their balance
    against the published root → verifiable, auditable
```

### On-Chain vs App-Layer per Asset

| Asset | Where It Lives | Why |
|-------|---------------|-----|
| **XFG** | Fuego L1 (native) | Real money, consensus-level |
| **CD** | Fuego L1 (commitment deposits) | Consensus-level yield + privacy |
| **DIGM** | Fuego L1 (colored coin) | Hard 100K cap, publishing rights, transfers rarely |
| **PARA** | DIGM app, anchored | High-frequency (earned per listen), merkle-anchored |
| **VOX** | DIGM app, anchored | Transmutation events, merkle-anchored |
| **CURA** | DIGM app, anchored | Burn-to-mint, merkle-anchored |
| **nfVOX** | DIGM app, anchored | One per #1 album, merkle-anchored |
| **TOP** | DIGM app, anchored | Singleton, merkle-anchored |

### Why This Works

- No EFiers needed — DIGM app publishes the root as a regular Fuego tx
- Reuses the CommitmentIndex merkle tree pattern already in the codebase
- Chain bloat = 1 small tx per epoch (32-byte root), not millions of token ops
- Disputes are resolved by merkle proof against the on-chain root
- DIGM colored coin is "real" on-chain because it's low-frequency and controls access

---

## 11. Open Questions

> Many prior topics were **answered** in DIGM-origins docs and have been folded into the body of this guide. The list below is only what is still genuinely undecided.

### Already decided (inherited from DIGM-origins, see body)
- **PARA earning rate:** 1 PARA per minute of verified listening *(AUDIO_FORMAT_STRATEGY.md)*
- **PARA total supply / emission:** 1 quintillion total, 1% initial annual emission, halving every 4 years over 30 years *(PARA_TOKEN_ECONOMICS.md)* — keep or revisit?
- **Curator revenue split:** 30% of artist's PARA + 30% of listener's PARA when listened via curator playlist *(AUDIO_FORMAT_STRATEGY.md, CURATOR_REVENUE_SHARE_MODEL.md)*
- **Audio formats:** OGG/Opus 96 kbps for ParaDio singles, FLAC for album downloads *(AUDIO_FORMAT_STRATEGY.md)*
- **Chunk size:** 256 KB content-addressed chunks *(DIGM_COLD_CELESTIA_DETAIL.md)*
- **Album metadata fields:** albumId, title, artistName, description, releaseDate, genre, priceXFG, coverImage, paradioPreviewTrackIds, payment code, artist key *(DEV_IMPLEMENTATION_PLAN.md)*
- **Album pricing:** artist sets USD target, XFG amount adjusts via oracle *(DEV_IMPLEMENTATION_PLAN.md)*
- **DIGM coin utility:** required to sign album release tx (0x0A type), 10 releases per coin, updates free *(DEV_IMPLEMENTATION_PLAN.md)*
- **CURA token:** 100M fixed supply, minted by burning PARA (DIGM-origins) — but our design says minted by burning **VOX**. Conflict — see below.
- **Offline plays:** supported but no rewards (matches our decision)

### Decisions made in this revision
- **CURA mint = burn VOX only.** DIGM-origins' "burn PARA → CURA" is overridden. VOX is the only path to CURA.
- **PARA reward split** = listener / artist (curator skims 30% of each share if listened via curator playlist). No Elderfier / LP slice — those DIGM-origins buckets are gone with the rest of the L2/EFier stack.
- **HEAT is still viable** as a concept / denomination / possible sybil-cost gate. Specific use (unit label vs min-balance check vs both) still TBD.
- **CD fee pool is completely separate from DIGM.** DIGM does not feed or consume CD yield. They share XFG as a medium only.
- **Epoch length = ~5 days = 900 blocks**, aligned with the Fuego swap-fee epoch.

### Still open: "stays #1" semantics
Pick one of A / B / C / D from §4a, or propose a variant:
- (A) drain-and-reset every win
- (B) drain-and-reset, nfVOX only on first win
- (C) partial drain (stake older than X drains, fresh stake rolls)
- (D) **dynasty mode** — no drain while the reign continues, single drain on dethrone *(my recommendation)*
- Related: if the album is dethroned and comes back to #1, does that start a new dynasty or resume the old one?
- Related: can listeners leave "persistent belief" PARA in a pool across drains without losing it? (e.g. a small reserved percentage that never transmutes)

### Epoch & Timing (still open)
- How long does an album's staking pool stay open? Indefinite until win or withdraw?
- Does TOP transfer instantly when dethroned, or at epoch boundary (every 900 blocks)?
- Grace period for staking after album publish? (prevent sniping at epoch close)

### VOX / CURA / nfVOX / TOP — net new tokens, no prior docs
- VOX time-multiplier curve on transmutation (linear? exponential? step function?)
- VOX → CURA burn ratio
- Is VOX P2P-tradable or only burnable for CURA + spendable for cosmetics?
- nfVOX: any on-chain anchor (it's a permanent trophy) or pure app state?
- TOP transfer mechanics — automatic on dethrone, or signed by previous holder?

### Streaming proof / anti-bot (direction decided, numbers open)
Decided: per-second local decode heartbeat + volume/bandwidth consistency check + not-a-bot score + seeder-co-signed per-minute receipts. Still open:
- Scoring weights for the not-a-bot score (interaction signals, longitudinal per-alias behavior, etc.)
- How often seeder↔listener receipts settle into the merkle anchor
- Does a seeder get rewarded for co-signing receipts? (if yes, seeders have an incentive to lie for the listener — need game theory)
- HEAT minimum-balance gate (DIGM-origins style, 1M HEAT min): keep as a cheap sybil cost, raise, lower, or drop?
- Should CD balance or XFG balance act as an additional sybil cost on listener aliases?

### Alias / leaderboard system (no prior decisions)
- Alias registration: unique? changeable? cost VOX?
- Leaderboard categories and ranking formulas
- Sybil resistance for aliases (CD-locked deposit per alias?)
- Public profile fields: which stats are public-by-default vs opt-in?

### Platform & UX
- What customizations / add-ons can VOX buy? (themes, badges, profile flair, radio priority?)
- CURA permissions: exact powers? (public playlists, featured picks, editorial posts, slashing?)
- Paradio queue: hashrate-vote weight vs VOX-burn weight? Any algorithmic baseline?
- Should hashrate voting be 1-miner-1-vote or weighted by hashrate?

### Integration with Fuego Core
- CD fee pool split now that Elderfiers are gone: (a) 90/10 CD/Treasury, (b) 85/15 CD/Treasury, (c) keep 80/10/10 and reassign the 10% EF slice to something else **unrelated to DIGM** (dev fund? burn?). *Note: DIGM is explicitly NOT in scope for the CD fee pool — this is a pure Fuego-core question.*
- DIGM colored coin tx_extra tag format (transfer rules, 100K mint, hosting-rights enforcement)
- Merkle anchor tx_extra tag (next available after 0xD5 — pick a value)
- Authentication for the merkle root publisher tx — dedicated wallet? threshold of DIGM-coin holders?
- Fraud-proof / dispute resolution if a published merkle root is wrong

### App Engineering
- iOS Background App Refresh strategy for keeping the full node + I2P router warm
- Pruning mode policy (when does a phone switch from full archival to pruned?)
- I2P router choice: pure-Rust `i2p-rs` (less battle-tested) vs FFI to `i2pd` (C++, mature). Likely start with i2pd FFI, migrate to i2p-rs as it matures.
- Cache eviction policy (LRU with pinning exceptions)
- Audio streaming buffer strategy (prefetch next N chunks)
- Initial Fuego sync time on mobile via I2P — acceptable UX or do we need a "fast-sync" snapshot mechanism?
- Storage budget validation: real growth curve of Fuego chain at current block rate
