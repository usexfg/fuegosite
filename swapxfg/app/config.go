package app

type Config struct {
	DaemonRPC string // fuegod RPC endpoint
	WalletRPC string // fire_wallet RPC endpoint (empty = no wallet)
	Testnet   bool
	StartPair uint8 // initial pair to display
	NoSplash  bool
	Compact   bool
}

func DefaultConfig() Config {
	return Config{
		DaemonRPC: "http://127.0.0.1:18180",
		WalletRPC: "",
		StartPair: PairSOL,
	}
}
