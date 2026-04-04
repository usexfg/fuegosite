// swapxfg/app/bridge_eth.go
// ETH bridge page + Go helper methods.
package app

import (
	"encoding/hex"
	"encoding/json"
	"fmt"
	"strconv"
	"strings"
	"sync/atomic"
)

// ethBridgeHTML returns the inline HTML for the ETH MetaMask bridge page.
// The page connects back to the bridge WebSocket and handles eth_* actions.
func ethBridgeHTML(port int, network string) string {
	return fmt.Sprintf(`<!DOCTYPE html>
<html>
<head><meta charset="utf-8"><title>swapxfg ETH bridge</title></head>
<body style="background:#111;color:#eee;font-family:monospace;padding:24px">
<h2 style="color:#FF5500">&#x26B3;&#xFE0F; swapxfg &middot; ETH bridge</h2>
<p id="status">Connecting to swapxfg&hellip;</p>
<script src="https://cdn.ethers.io/lib/ethers-5.7.2.umd.min.js" type="application/javascript" integrity="sha384-OLBgp1GsljhM2TJ+sbHjaiH9txEUvgdDTAzHv2P24donTt6/529l+9Ua0vFImLlb" crossorigin="anonymous"></script>
<script>
const ws = new WebSocket("ws://127.0.0.1:%d/bridge/ws");
const st = document.getElementById("status");
const network = "%s";
let provider;
switch (network) {
  case "goerli":
    provider = new ethers.providers.Web3Provider(window.ethereum, "goerli");
    break;
  case "sepolia":
    provider = new ethers.providers.Web3Provider(window.ethereum, "sepolia");
    break;
  case "mainnet":
  default:
    provider = new ethers.providers.Web3Provider(window.ethereum, "mainnet");
}

ws.onopen = () => { st.textContent = "Connected to swapxfg. Waiting for MetaMask..."; };
ws.onclose = () => { st.textContent = "swapxfg disconnected."; };

ws.onmessage = async (ev) => {
  const req = JSON.parse(ev.data);
  let result = null, error = null;
  try {
    if (!window.ethereum) throw new Error("MetaMask not found");
    await provider.send("eth_requestAccounts", []);
    const signer = provider.getSigner();

    switch (req.action) {
      case "eth_getAddress":
        result = await signer.getAddress();
        break;
      case "eth_getBalance": {
        const addr = req.params.address || (await signer.getAddress());
        const bal = await provider.getBalance(addr);
        result = bal.toString();
        break;
      }
      case "eth_sendTx": {
        const tx = await signer.sendTransaction({
          to:    req.params.to,
          value: ethers.BigNumber.from(req.params.value || "0"),
          data:  req.params.data || "0x",
        });
        result = tx.hash;
        break;
      }
      case "erc20_getBalance": {
        const abi = ["function balanceOf(address) view returns (uint256)"];
        const c = new ethers.Contract(req.params.token, abi, provider);
        result = (await c.balanceOf(req.params.address)).toString();
        break;
      }
      case "erc20_approve": {
        const abi = ["function approve(address,uint256) returns (bool)"];
        const c = new ethers.Contract(req.params.token, abi, signer);
        const tx = await c.approve(req.params.spender, req.params.amount);
        result = tx.hash;
        break;
      }
      case "erc20_transfer": {
        const abi = ["function transfer(address,uint256) returns (bool)"];
        const c = new ethers.Contract(req.params.token, abi, signer);
        const tx = await c.transfer(req.params.to, req.params.amount);
        result = tx.hash;
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

// reqCounter is an atomic counter for unique request IDs.
var reqCounter uint64

func newReqID() string {
	n := atomic.AddUint64(&reqCounter, 1)
	return "req-" + strconv.FormatUint(n, 10)
}

// EthGetAddress retrieves the connected MetaMask wallet address.
func (b *BridgeServer) EthGetAddress() (string, error) {
	log.Println("Bridge.EthGetAddress: requesting wallet address")
	params, _ := json.Marshal(map[string]string{})
	resp, err := b.Send(BridgeRequest{ID: newReqID(), Action: "eth_getAddress", Params: params})
	if err != nil {
		log.Printf("Bridge.EthGetAddress: send error: %v", err)
		return "", err
	}
	if resp.Error != "" {
		log.Printf("Bridge.EthGetAddress: response error: %s", resp.Error)
		return "", fmt.Errorf("%s", resp.Error)
	}
	log.Printf("Bridge.EthGetAddress: address retrieved: %s", resp.Result)
	return resp.Result, nil
}

// EthGetBalance returns the ETH balance (in wei as string) for address.
func (b *BridgeServer) EthGetBalance(address string) (string, error) {
	log.Printf("Bridge.EthGetBalance: requesting balance for address %s", address)
	params, _ := json.Marshal(map[string]string{"address": address})
	resp, err := b.Send(BridgeRequest{ID: newReqID(), Action: "eth_getBalance", Params: params})
	if err != nil {
		log.Printf("Bridge.EthGetBalance: send error: %v", err)
		return "", err
	}
	if resp.Error != "" {
		log.Printf("Bridge.EthGetBalance: response error: %s", resp.Error)
		return "", fmt.Errorf("%s", resp.Error)
	}
	log.Printf("Bridge.EthGetBalance: balance retrieved: %s wei", resp.Result)
	return resp.Result, nil
}

// EthSendTransaction sends a raw ETH transaction via MetaMask. Returns tx hash.
// Validates parameters before sending to prevent common errors.
func (b *BridgeServer) EthSendTransaction(to, value, data string) (string, error) {
	// Validate to address
	if to == "" {
		return "", fmt.Errorf("missing to address")
	}
	if !strings.HasPrefix(to, "0x") || len(to) != 42 {
		return "", fmt.Errorf("invalid Ethereum address format: %s", to)
	}

	// Validate value (if provided)
	if value != "" {
		if !strings.HasPrefix(value, "0x") {
			// Must be a valid number string
			if _, err := strconv.ParseFloat(value, 64); err != nil {
				return "", fmt.Errorf("invalid ETH value: %s", value)
			}
			// Check for negative values
			if strings.HasPrefix(value, "-") {
				return "", fmt.Errorf("ETH value cannot be negative: %s", value)
			}
		}
	}

	// Validate data (if provided)
	if data != "" {
		if !strings.HasPrefix(data, "0x") {
			return "", fmt.Errorf("data must be hex string starting with 0x: %s", data)
		}
		// Check if hex string is valid
		if _, err := hex.DecodeString(data[2:]); err != nil {
			return "", fmt.Errorf("invalid hex data: %s", data)
		}
	}

	params, _ := json.Marshal(map[string]string{"to": to, "value": value, "data": data})
	resp, err := b.Send(BridgeRequest{ID: newReqID(), Action: "eth_sendTx", Params: params})
	if err != nil {
		return "", err
	}
	if resp.Error != "" {
		return "", fmt.Errorf("%s", resp.Error)
	}
	return resp.Result, nil
}

// Erc20Balance returns the ERC-20 token balance for address.
// Validates parameters before sending to prevent common errors.
func (b *BridgeServer) Erc20Balance(token, address string) (string, error) {
	log.Printf("Bridge.Erc20Balance: requesting balance for token %s, address %s", token, address)
	// Validate token address
	if token == "" {
		log.Println("Bridge.Erc20Balance: missing token address")
		return "", fmt.Errorf("missing token address")
	}
	if !strings.HasPrefix(token, "0x") || len(token) != 42 {
		log.Printf("Bridge.Erc20Balance: invalid ERC-20 token address format: %s", token)
		return "", fmt.Errorf("invalid ERC-20 token address format: %s", token)
	}

	// Validate address
	if address == "" {
		log.Println("Bridge.Erc20Balance: missing address")
		return "", fmt.Errorf("missing address")
	}
	if !strings.HasPrefix(address, "0x") || len(address) != 42 {
		log.Printf("Bridge.Erc20Balance: invalid Ethereum address format: %s", address)
		return "", fmt.Errorf("invalid Ethereum address format: %s", address)
	}

	params, _ := json.Marshal(map[string]string{"token": token, "address": address})
	resp, err := b.Send(BridgeRequest{ID: newReqID(), Action: "erc20_getBalance", Params: params})
	if err != nil {
		log.Printf("Bridge.Erc20Balance: send error: %v", err)
		return "", err
	}
	if resp.Error != "" {
		log.Printf("Bridge.Erc20Balance: response error: %s", resp.Error)
		return "", fmt.Errorf("%s", resp.Error)
	}
	log.Printf("Bridge.Erc20Balance: balance retrieved: %s", resp.Result)
	return resp.Result, nil
}

// Erc20Approve approves an ERC-20 token spender.
// Validates parameters before sending to prevent common errors.
func (b *BridgeServer) Erc20Approve(token, spender string, amount string) (string, error) {
	log.Printf("Bridge.Erc20Approve: approving spender %s for token %s with amount %s", spender, token, amount)
	// Validate token address
	if token == "" {
		log.Println("Bridge.Erc20Approve: missing token address")
		return "", fmt.Errorf("missing token address")
	}
	if !strings.HasPrefix(token, "0x") || len(token) != 42 {
		log.Printf("Bridge.Erc20Approve: invalid ERC-20 token address format: %s", token)
		return "", fmt.Errorf("invalid ERC-20 token address format: %s", token)
	}

	// Validate spender address
	if spender == "" {
		log.Println("Bridge.Erc20Approve: missing spender address")
		return "", fmt.Errorf("missing spender address")
	}
	if !strings.HasPrefix(spender, "0x") || len(spender) != 42 {
		log.Printf("Bridge.Erc20Approve: invalid spender address format: %s", spender)
		return "", fmt.Errorf("invalid spender address format: %s", spender)
	}

	// Validate amount
	if amount == "" {
		log.Println("Bridge.Erc20Approve: missing amount")
		return "", fmt.Errorf("missing amount")
	}
	if !strings.HasPrefix(amount, "0x") {
		// Must be a valid non-negative number string
		if _, err := strconv.ParseFloat(amount, 64); err != nil {
			log.Printf("Bridge.Erc20Approve: invalid amount: %s", amount)
			return "", fmt.Errorf("invalid amount: %s", amount)
		}
		if strings.HasPrefix(amount, "-") {
			log.Printf("Bridge.Erc20Approve: amount cannot be negative: %s", amount)
			return "", fmt.Errorf("amount cannot be negative: %s", amount)
		}
	}

	params, _ := json.Marshal(map[string]string{"token": token, "spender": spender, "amount": amount})
	resp, err := b.Send(BridgeRequest{ID: newReqID(), Action: "erc20_approve", Params: params})
	if err != nil {
		log.Printf("Bridge.Erc20Approve: send error: %v", err)
		return "", err
	}
	if resp.Error != "" {
		log.Printf("Bridge.Erc20Approve: response error: %s", resp.Error)
		return "", fmt.Errorf("%s", resp.Error)
	}
	log.Printf("Bridge.Erc20Approve: approval transaction submitted: %s", resp.Result)
	return resp.Result, nil
}

// Erc20Transfer transfers ERC-20 tokens to a recipient.
// Validates parameters before sending to prevent common errors.
func (b *BridgeServer) Erc20Transfer(token, to string, amount string) (string, error) {
	log.Printf("Bridge.Erc20Transfer: transferring %s of token %s to %s", amount, token, to)
	// Validate token address
	if token == "" {
		log.Println("Bridge.Erc20Transfer: missing token address")
		return "", fmt.Errorf("missing token address")
	}
	if !strings.HasPrefix(token, "0x") || len(token) != 42 {
		log.Printf("Bridge.Erc20Transfer: invalid ERC-20 token address format: %s", token)
		return "", fmt.Errorf("invalid ERC-20 token address format: %s", token)
	}

	// Validate to address
	if to == "" {
		log.Println("Bridge.Erc20Transfer: missing to address")
		return "", fmt.Errorf("missing to address")
	}
	if !strings.HasPrefix(to, "0x") || len(to) != 42 {
		log.Printf("Bridge.Erc20Transfer: invalid recipient address format: %s", to)
		return "", fmt.Errorf("invalid recipient address format: %s", to)
	}

	// Validate amount
	if amount == "" {
		log.Println("Bridge.Erc20Transfer: missing amount")
		return "", fmt.Errorf("missing amount")
	}
	if !strings.HasPrefix(amount, "0x") {
		// Must be a valid non-negative number string
		if _, err := strconv.ParseFloat(amount, 64); err != nil {
			log.Printf("Bridge.Erc20Transfer: invalid amount: %s", amount)
			return "", fmt.Errorf("invalid amount: %s", amount)
		}
		if strings.HasPrefix(amount, "-") {
			log.Printf("Bridge.Erc20Transfer: amount cannot be negative: %s", amount)
			return "", fmt.Errorf("amount cannot be negative: %s", amount)
		}
	}

	params, _ := json.Marshal(map[string]string{"token": token, "to": to, "amount": amount})
	resp, err := b.Send(BridgeRequest{ID: newReqID(), Action: "erc20_transfer", Params: params})
	if err != nil {
		log.Printf("Bridge.Erc20Transfer: send error: %v", err)
		return "", err
	}
	if resp.Error != "" {
		log.Printf("Bridge.Erc20Transfer: response error: %s", resp.Error)
		return "", fmt.Errorf("%s", resp.Error)
	}
	log.Printf("Bridge.Erc20Transfer: transfer transaction submitted: %s", resp.Result)
	return resp.Result, nil
}

// OpenEthBridge opens the ETH bridge page in the default browser.
func (b *BridgeServer) OpenEthBridge() error {
	log.Printf("Bridge.OpenEthBridge: opening bridge page at %s", b.EthURL())
	err := openURL(b.EthURL())
	if err != nil {
		log.Printf("Bridge.OpenEthBridge: failed to open URL %s: %v", b.EthURL(), err)
	} else {
		log.Printf("Bridge.OpenEthBridge: successfully opened bridge page at %s", b.EthURL())
	}
	return err
}
