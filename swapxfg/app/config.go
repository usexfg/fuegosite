package app

type Config struct {
	DaemonRPC  string // fuegod RPC endpoint
	WalletRPC  string // fire_wallet RPC endpoint (empty = no wallet)
	Testnet    bool
	StartPair  uint8 // initial pair to display
	NoSplash   bool
	Compact    bool
	BridgePort int    // 0 = random; bridge serves MetaMask/Phantom pages
	NoBridge   bool   // disable bridge server entirely
	BchRPC     string // Electron Cash RPC endpoint (empty = no BCH)
	NoBch      bool   // disable BCH connection
	// Solana network configuration
	SolNetwork string // "mainnet", "testnet", "devnet" (default: "mainnet")
	// Ethereum network configuration
	EthNetwork string // "mainnet", "goerli", "sepolia" (default: "mainnet")
}

func DefaultConfig() Config {
	return Config{
		DaemonRPC:  "http://127.0.0.1:18180",
		WalletRPC:  "",
		StartPair:  PairSOL,
		BchRPC:     "http://127.0.0.1:7773",
		SolNetwork: "mainnet",
		EthNetwork: "mainnet",
	}
}
