// swapxfg/app/bridge_sol.go
// Solana bridge page + Go helper methods.
package app

import (
	"encoding/base64"
	"encoding/json"
	"fmt"
	"strconv"
)

// solBridgeHTML returns the inline HTML for the Solana Phantom bridge page.
func solBridgeHTML(port int, network string) string {
	return fmt.Sprintf(`<!DOCTYPE html>
<html>
<head><meta charset="utf-8"><title>swapxfg SOL bridge</title></head>
<body style="background:#111;color:#eee;font-family:monospace;padding:248">
<h2 style="color:#FF5500">&#x26B3;&#xFE0F; swapxfg &middot; SOL bridge</h2>
<p id="status">Connecting to swapxfg&hellip;</p>
<script src="https://cdn.jsdelivr.net/npm/@solana/web3.js@latest/lib/index.iife.min.js"></script>
<script>
 const ws = new WebSocket("ws://127.0.0.1:%d/bridge/ws");
 const st = document.getElementById("status");
 const network = "%s";
 let cluster;
 switch (network) {
   case "devnet":
     cluster = "devnet";
     break;
   case "testnet":
     cluster = "testnet";
     break;
   case "mainnet":
   default:
     cluster = "mainnet-beta";
 }
 const connection = new solanaWeb3.Connection(solanaWeb3.clusterApiUrl(cluster));

ws.onopen = () => { st.textContent = "Connected to swapxfg. Waiting for Phantom..."; };
ws.onclose = () => { st.textContent = "swapxfg disconnected."; };

ws.onmessage = async (ev) => {
  const req = JSON.parse(ev.data);
  let result = null, error = null;
  try {
    const phantom = window.solana;
    if (!phantom || !phantom.isPhantom) throw new Error("Phantom wallet not found");
    await phantom.connect();

    switch (req.action) {
      case "sol_getAddress":
        result = phantom.publicKey.toString();
        break;
      case "sol_getBalance": {
        const pk = new solanaWeb3.PublicKey(req.params.pubkey || phantom.publicKey.toString());
        const lamports = await connection.getBalance(pk);
        result = lamports.toString();
        break;
      }
      case "sol_sendTx": {
        const txBytes = Uint8Array.from(atob(req.params.txBase64), c => c.charCodeAt(0));
        const tx = solanaWeb3.Transaction.from(txBytes);
        const signed = await phantom.signTransaction(tx);
        const sig = await connection.sendRawTransaction(signed.serialize());
        result = sig;
        break;
      }
      default:
        error = "unknown action: " + req.action;
    }
  } catch(e) {
    error = e.message;
  }
  ws.send(JSON.stringify({id: req.id, result: result ? String(result) : "", error: error || ""}));
  st.textContent = "Last: " + req.action + (error ? " ERROR: " + error : " OK: " + (result||""));
};
</script>
</body>
</html>`, port)
}

// SolGetBalance returns the SOL balance in lamports for the given pubkey.
func (b *BridgeServer) SolGetBalance(pubkey string) (uint64, error) {
	log.Printf("Bridge.SolGetBalance: requesting balance for pubkey %s", pubkey)
	params, _ := json.Marshal(map[string]string{"pubkey": pubkey})
	resp, err := b.Send(BridgeRequest{ID: newReqID(), Action: "sol_getBalance", Params: params})
	if err != nil {
		log.Printf("Bridge.SolGetBalance: send error: %v", err)
		return 0, err
	}
	if resp.Error != "" {
		log.Printf("Bridge.SolGetBalance: response error: %s", resp.Error)
		return 0, fmt.Errorf("%s", resp.Error)
	}
	n, err := strconv.ParseUint(resp.Result, 10, 64)
	if err != nil {
		log.Printf("Bridge.SolGetBalance: parse error: %v", err)
		return 0, fmt.Errorf("parse balance: %w", err)
	}
	log.Printf("Bridge.SolGetBalance: balance retrieved: %d lamports", n)
	return n, nil
}

// SolSendTransaction submits a base64-encoded Solana transaction via Phantom.
// Validates parameters before sending to prevent common errors.
// Returns the transaction signature.
func (b *BridgeServer) SolSendTransaction(txBase64 string) (string, error) {
	log.Printf("Bridge.SolSendTransaction: submitting transaction")
	// Validate transaction data
	if txBase64 == "" {
		log.Println("Bridge.SolSendTransaction: missing transaction data")
		return "", fmt.Errorf("missing transaction data")
	}

	// Check if base64 string is valid
	if _, err := base64.StdEncoding.DecodeString(txBase64); err != nil {
		log.Printf("Bridge.SolSendTransaction: invalid base64 transaction data: %s", txBase64)
		return "", fmt.Errorf("invalid base64 transaction data: %s", txBase64)
	}

	params, _ := json.Marshal(map[string]string{"txBase64": txBase64})
	resp, err := b.Send(BridgeRequest{ID: newReqID(), Action: "sol_sendTx", Params: params})
	if err != nil {
		log.Printf("Bridge.SolSendTransaction: send error: %v", err)
		return "", err
	}
	if resp.Error != "" {
		log.Printf("Bridge.SolSendTransaction: response error: %s", resp.Error)
		return "", fmt.Errorf("%s", resp.Error)
	}
	log.Printf("Bridge.SolSendTransaction: transaction submitted: %s", resp.Result)
	return resp.Result, nil
}

// OpenSolBridge opens the SOL bridge page in the default browser.
func (b *BridgeServer) OpenSolBridge() error {
	log.Printf("Bridge.OpenSolBridge: opening bridge page at %s", b.SolURL())
	err := openURL(b.SolURL())
	if err != nil {
		log.Printf("Bridge.OpenSolBridge: failed to open URL %s: %v", b.SolURL(), err)
	} else {
		log.Printf("Bridge.OpenSolBridge: successfully opened bridge page at %s", b.SolURL())
	}
	return err
}
