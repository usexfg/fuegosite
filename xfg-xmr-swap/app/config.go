package app

// Config holds runtime configuration for the swap client.
type Config struct {
	EfierRPC string // fuegod RPC endpoint (EFier node)
	XmrRPC   string // Monero RPC endpoint
}

func DefaultConfig() Config {
	return Config{
		EfierRPC: "http://127.0.0.1:18180",
		XmrRPC:   "",
	}
}
