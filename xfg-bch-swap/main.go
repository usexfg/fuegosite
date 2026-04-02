package main

import (
	"fmt"
	"os"

	"github.com/usexfg/xfg-bch-swap/app"
)

func main() {
	cfg := app.DefaultConfig()

	for i := 1; i < len(os.Args); i++ {
		switch os.Args[i] {
		case "--efier", "-e":
			if i+1 < len(os.Args) {
				i++
				cfg.EfierRPC = os.Args[i]
			}
		case "--bch-rpc":
			if i+1 < len(os.Args) {
				i++
				cfg.BchRPC = os.Args[i]
			}
		case "--help", "-h":
			fmt.Println("xfg-bch-swap — Fuego/Bitcoin Cash atomic swap client")
			fmt.Println()
			fmt.Println("Usage: xfg-bch-swap [options]")
			fmt.Println()
			fmt.Println("Options:")
			fmt.Println("  --efier, -e <url>    EFier node RPC (default: http://127.0.0.1:18180)")
			fmt.Println("  --bch-rpc <url>      Bitcoin Cash RPC endpoint")
			fmt.Println("  --help, -h           Show this help")
			os.Exit(0)
		}
	}

	if err := app.Run(cfg); err != nil {
		fmt.Fprintf(os.Stderr, "error: %v\n", err)
		os.Exit(1)
	}
}
