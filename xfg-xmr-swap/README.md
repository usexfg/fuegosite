# xfg-xmr-swap

Trustless XFG/XMR atomic swap TUI client for Monero holders.

Connects to a Fuego EFier node, displays live swap offers, and walks
you through the HTLC flow to trade XMR for XFG without any
intermediary.

## Build

```bash
cd xfg-xmr-swap
go build -o xfg-xmr-swap .
```

## Run

```bash
./xfg-xmr-swap                          # default EFier at 127.0.0.1:18180
./xfg-xmr-swap --efier http://HOST:PORT # custom EFier node
./xfg-xmr-swap --xmr-rpc http://...     # custom Monero RPC endpoint
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
3. Lock your XMR using the displayed hashlock.
4. XFG maker sees your XMR lock, locks XFG on Fuego.
5. Claim XFG with your preimage (reveals it to maker).
6. Maker claims your XMR using the revealed preimage.

## Requirements

- Go 1.21+
- Running `fuegod` EFier node (mainnet RPC default port 18180)
