# xfg-bch-swap

Trustless XFG/BCH atomic swap TUI client for Bitcoin Cash holders.

Connects to a Fuego EFier node, displays live swap offers, and walks
you through the HTLC flow to trade BCH for XFG without any
intermediary.

## Build

```bash
cd xfg-bch-swap
go build -o xfg-bch-swap .
```

## Run

```bash
./xfg-bch-swap                          # default EFier at 127.0.0.1:18180
./xfg-bch-swap --efier http://HOST:PORT # custom EFier node
./xfg-bch-swap --bch-rpc http://...     # custom BCH RPC endpoint
```

## TUI commands

| Command | Description |
|---|---|
| `accept <id>` | Accept a swap offer (generates preimage + hashlock) |
| `htlc [idx]` | Query HTLC outputs on chain |
| `swap` | Show active swap details |
| `r` | Refresh order book |
| `1` / `2` / `3` | Switch tabs: Book, Trades, Info |
| `q` | Quit |

## Swap flow

1. Browse XFG sell offers on the Book tab.
2. `accept <offer_id>` -- client generates a preimage and hashlock.
3. Lock your BCH in an HTLC using the displayed hashlock.
4. XFG maker sees your BCH lock, locks XFG on Fuego.
5. Claim XFG with your preimage (reveals it to maker).
6. Maker claims your BCH using the revealed preimage.

## Requirements

- Go 1.21+
- Running `fuegod` EFier node (mainnet RPC default port 18180)
