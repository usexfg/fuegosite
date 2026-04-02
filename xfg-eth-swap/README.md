# xfg-eth-swap

Trustless XFG/ETH atomic swap TUI client for Ethereum holders.

Connects to a Fuego EFier node, displays live swap offers, and walks
you through the HTLC flow to trade ETH for XFG without any
intermediary.

## Build

```bash
cd xfg-eth-swap
go build -o xfg-eth-swap .
```

## Run

```bash
./xfg-eth-swap                          # default EFier at 127.0.0.1:18180
./xfg-eth-swap --efier http://HOST:PORT # custom EFier node
./xfg-eth-swap --eth-rpc http://...     # custom Ethereum RPC endpoint
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
3. Lock your ETH in an HTLC smart contract using the displayed hashlock.
4. XFG maker sees your ETH lock, locks XFG on Fuego.
5. Claim XFG with your preimage (reveals it to maker).
6. Maker claims your ETH using the revealed preimage.

## Requirements

- Go 1.21+
- Running `fuegod` EFier node (mainnet RPC default port 18180)
