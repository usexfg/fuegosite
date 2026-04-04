// swapxfg/app/bridge.go
// BridgeServer: local HTTP + WebSocket relay between swapxfg TUI and
// a browser tab running the MetaMask (ETH) or Phantom (SOL) extension.
package app

import (
	"context"
	"encoding/json"
	"fmt"
	"net"
	"net/http"
	"sync"
	"time"

	"github.com/gorilla/websocket"
)

// BridgeRequest is a message sent from the TUI to the browser.
type BridgeRequest struct {
	ID     string          `json:"id"`
	Action string          `json:"action"`
	Params json.RawMessage `json:"params"`
}

// BridgeResponse is a message sent from the browser back to the TUI.
type BridgeResponse struct {
	ID     string `json:"id"`
	Result string `json:"result"`
	Error  string `json:"error,omitempty"`
}

type pendingReq struct {
	ch chan BridgeResponse
}

// BridgeServer manages a single browser WebSocket connection and routes
// JSON messages between the TUI goroutines and the browser page.
type BridgeServer struct {
	ethHTML string
	solHTML string
	port    int

	upgrader websocket.Upgrader
	mu       sync.Mutex
	pending  map[string]pendingReq
	conn     *websocket.Conn
	srv      *http.Server
	cancel   context.CancelFunc
	solNetwork string
	ethNetwork string
}

// NewBridgeServer creates and starts a BridgeServer bound to a random
// loopback port (or the port from cfg.BridgePort if non-zero).
func NewBridgeServer(cfg Config, preferredPort int) (*BridgeServer, error) {
	addr := fmt.Sprintf("127.0.0.1:%d", preferredPort)
	l, err := net.Listen("tcp", addr)
	if err != nil {
		// fall back to random port
		l, err = net.Listen("tcp", "127.0.0.1:0")
		if err != nil {
			return nil, fmt.Errorf("bridge listen: %w", err)
		}
	}

	b := &BridgeServer{
		pending: make(map[string]pendingReq),
		upgrader: websocket.Upgrader{
				CheckOrigin: func(r *http.Request) bool {
					origin := r.Header.Get("Origin")
					return origin == "http://127.0.0.1" || origin == "http://localhost"
				},
			},
	}
	b.port = l.Addr().(*net.TCPAddr).Port

	ctx, cancel := context.WithCancel(context.Background())
	b.cancel = cancel

	mux := http.NewServeMux()
	mux.HandleFunc("/bridge/eth", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "text/html; charset=utf-8")
		fmt.Fprint(w, b.ethHTML)
	})
	mux.HandleFunc("/bridge/sol", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "text/html; charset=utf-8")
		fmt.Fprint(w, b.solHTML)
	})
	mux.HandleFunc("/bridge/ws", b.handleWS)

	b.srv = &http.Server{Handler: mux}

	// Store the network configurations and generate HTML with network info
	b.solNetwork = cfg.SolNetwork
	b.ethNetwork = cfg.EthNetwork
	b.ethHTML = ethBridgeHTML(b.port, cfg.EthNetwork)
	b.solHTML = solBridgeHTML(b.port, cfg.SolNetwork)

	go func() {
		_ = b.srv.Serve(l)
	}()

	go func() {
		<-ctx.Done()
		_ = b.srv.Close()
	}()

	return b, nil
}

// Port returns the port the bridge is listening on.
func (b *BridgeServer) Port() int { return b.port }

// EthURL returns the URL for the ETH bridge page.
func (b *BridgeServer) EthURL() string {
	return fmt.Sprintf("http://127.0.0.1:%d/bridge/eth", b.port)
}

// SolURL returns the URL for the SOL bridge page.
func (b *BridgeServer) SolURL() string {
	return fmt.Sprintf("http://127.0.0.1:%d/bridge/sol", b.port)
}

// Stop shuts down the bridge server and closes the WebSocket.
func (b *BridgeServer) Stop() {
	log.Println("Bridge.Stop: shutting down bridge server")
	b.cancel()
	b.mu.Lock()
	if b.conn != nil {
		log.Println("Bridge.Stop: closing WebSocket connection")
		_ = b.conn.Close()
	}
	b.mu.Unlock()
	log.Println("Bridge.Stop: bridge server shutdown complete")
}

// IsConnected reports whether a browser tab is currently connected.
func (b *BridgeServer) IsConnected() bool {
	b.mu.Lock()
	defer b.mu.Unlock()
	return b.conn != nil
}

// Send sends a request to the browser and waits up to 30 s for a response.
func (b *BridgeServer) Send(req BridgeRequest) (BridgeResponse, error) {
	b.mu.Lock()
	conn := b.conn
	if conn == nil {
		b.mu.Unlock()
		log.Printf("Bridge.Send: no browser connected, request=%s", req.Action)
		return BridgeResponse{}, fmt.Errorf("no browser connected")
	}

	ch := make(chan BridgeResponse, 1)
	b.pending[req.ID] = pendingReq{ch: ch}
	data, _ := json.Marshal(req)
	err := conn.WriteMessage(websocket.TextMessage, data)
	b.mu.Unlock()

	if err != nil {
		b.mu.Lock()
		delete(b.pending, req.ID)
		b.mu.Unlock()
		log.Printf("Bridge.Send: write error, request=%s, error=%v", req.Action, err)
		return BridgeResponse{}, fmt.Errorf("write: %w", err)
	}

	log.Printf("Bridge.Send: request sent, request=%s, id=%s", req.Action, req.ID)
	select {
	case resp := <-ch:
		log.Printf("Bridge.Send: response received, request=%s, id=%s, error=%s", req.Action, req.ID, resp.Error)
		return resp, nil
	case <-time.After(30 * time.Second):
		b.mu.Lock()
		delete(b.pending, req.ID)
		b.mu.Unlock()
		log.Printf("Bridge.Send: timeout, request=%s, id=%s", req.Action, req.ID)
		return BridgeResponse{}, fmt.Errorf("timeout waiting for %s", req.ID)
	}
}

// openURL opens a URL in the default browser (non-blocking, best effort).
func openURL(url string) error {
	return openURLOS(url)
}

// handleWS upgrades the HTTP connection to a WebSocket and reads responses.
func (b *BridgeServer) handleWS(w http.ResponseWriter, r *http.Request) {
	log.Printf("Bridge.handleWS: new WebSocket connection attempt from %s", r.RemoteAddr)
	conn, err := b.upgrader.Upgrade(w, r, nil)
	if err != nil {
		log.Printf("Bridge.handleWS: upgrade error from %s: %v", r.RemoteAddr, err)
		return
	}
	log.Printf("Bridge.handleWS: WebSocket connection established with %s", r.RemoteAddr)

	b.mu.Lock()
	if b.conn != nil {
		log.Printf("Bridge.handleWS: closing previous connection from %s", r.RemoteAddr)
		_ = b.conn.Close()
	}
	b.conn = conn
	b.mu.Unlock()

	defer func() {
		b.mu.Lock()
		if b.conn == conn {
			b.conn = nil
		}
		b.mu.Unlock()
		_ = conn.Close()
		log.Printf("Bridge.handleWS: WebSocket connection closed with %s", r.RemoteAddr)
	}()

	for {
		_, msg, err := conn.ReadMessage()
		if err != nil {
			log.Printf("Bridge.handleWS: read error from %s: %v", r.RemoteAddr, err)
			return
		}
		var resp BridgeResponse
		if err := json.Unmarshal(msg, &resp); err != nil {
			log.Printf("Bridge.handleWS: unmarshal error from %s: %v, message=%s", r.RemoteAddr, err, string(msg))
			continue
		}
		log.Printf("Bridge.handleWS: message received, id=%s, result=%s, error=%s", resp.ID, resp.Result, resp.Error)
		b.mu.Lock()
		if pr, ok := b.pending[resp.ID]; ok {
			delete(b.pending, resp.ID)
			pr.ch <- resp
			log.Printf("Bridge.handleWS: response delivered to pending request, id=%s", resp.ID)
		} else {
			log.Printf("Bridge.handleWS: received response for unknown request id=%s", resp.ID)
		}
		b.mu.Unlock()
	}
}
