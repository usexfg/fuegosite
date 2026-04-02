package app

type Config struct {
	EfierRPC string
	BchRPC   string
}

func DefaultConfig() Config {
	return Config{
		EfierRPC: "http://127.0.0.1:18180",
		BchRPC:   "",
	}
}
