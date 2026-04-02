package main

import (
	"fmt"
	"os"

	"github.com/usexfg/xfg-eth-swap/app"
)

func main() {
	cfg := app.DefaultConfig()

	// Parse CLI flags
	for i := 1; i < len(os.Args); i++ {
		switch os.Args[i] {
		case "--efier", "-e":
			if i+1 < len(os.Args) {
				i++
				cfg.EfierRPC = os.Args[i]
			}
		case "--eth-rpc":
			if i+1 < len(os.Args) {
				i++
				cfg.EthRPC = os.Args[i]
			}
		case "--help", "-h":
			fmt.Println("xfg-eth-swap — Fuego/ETH atomic swap client")
			fmt.Println()
			fmt.Println("Usage: xfg-eth-swap [options]")
			fmt.Println()
			fmt.Println("Options:")
			fmt.Println("  --efier, -e <url>    EFier node RPC (default: http://127.0.0.1:18180)")
			fmt.Println("  --eth-rpc <url>      Ethereum RPC endpoint")
			fmt.Println("  --help, -h           Show this help")
			os.Exit(0)
		}
	}

	if err := app.Run(cfg); err != nil {
		fmt.Fprintf(os.Stderr, "error: %v\n", err)
		os.Exit(1)
	}
}
