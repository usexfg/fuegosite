package app

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"time"
)

// FuegoClient talks to a fuegod EFier node via JSON-RPC.
type FuegoClient struct {
	endpoint string
	client   *http.Client
}

func NewFuegoClient(endpoint string) *FuegoClient {
	return &FuegoClient{
		endpoint: endpoint,
		client:   &http.Client{Timeout: 10 * time.Second},
	}
}

// --- RPC types (mirror CoreRpcServerCommandsDefinitions.h) ---

type SwapOffer struct {
	OfferID      string `json:"offerId"`
	XfgAmount    uint64 `json:"xfgAmount"`
	RateNum      uint64 `json:"rateNum"`
	Pair         uint8  `json:"pair"`
	MakerPubKey  string `json:"makerPubKey"`
	Timestamp    uint64 `json:"timestamp"`
	TTLBlocks    uint32 `json:"ttlBlocks"`
	PostedHeight uint32 `json:"postedHeight"`
}

type SwapTrade struct {
	Pair      uint8  `json:"pair"`
	XfgAmount uint64 `json:"xfgAmount"`
	CtrAmount uint64 `json:"ctrAmount"`
	Rate      string `json:"rate"`
	Height    uint32 `json:"blockHeight"`
	Timestamp uint64 `json:"timestamp"`
}

type PriceSourceEntry struct {
	Name      string `json:"name"`
	Pair      uint8  `json:"pair"`
	Weight    string `json:"weight"`
	Rate      string `json:"rate"`
	UpdatedAt uint64 `json:"updatedAt"`
	Stale     bool   `json:"stale"`
}

type PairImplied struct {
	Pair       uint8  `json:"pair"`
	ImpliedUsd string `json:"impliedUsd"`
}

type SwapPriceResponse struct {
	Twap          string             `json:"twap"`
	SeedRate      string             `json:"seedRate"`
	CompositeRate string             `json:"compositeRate"`
	SourceCount   uint32             `json:"sourceCount"`
	Sources       []PriceSourceEntry `json:"sources"`
	XfgUsdLow     string            `json:"xfgUsdLow"`
	XfgUsdHigh    string            `json:"xfgUsdHigh"`
	XfgUsdMid     string            `json:"xfgUsdMid"`
	PairImplied   []PairImplied     `json:"pairImplied"`
	Status        string            `json:"status"`
}

// GetOffers fetches swap offers for XMR (pair=0).
func (c *FuegoClient) GetOffers() ([]SwapOffer, error) {
	req := map[string]interface{}{"pair": 0}
	var resp struct {
		Offers []SwapOffer `json:"offers"`
		Status string      `json:"status"`
	}
	if err := c.post("/getswapoffers", req, &resp); err != nil {
		return nil, err
	}
	return resp.Offers, nil
}

// GetPrice fetches composite price for XMR pair.
func (c *FuegoClient) GetPrice() (*SwapPriceResponse, error) {
	req := map[string]interface{}{"pair": 0}
	var resp SwapPriceResponse
	if err := c.post("/getswapprice", req, &resp); err != nil {
		return nil, err
	}
	return &resp, nil
}

// GetTrades fetches recent trades for XMR pair.
func (c *FuegoClient) GetTrades(limit int) ([]SwapTrade, error) {
	req := map[string]interface{}{"pair": 0, "limit": limit}
	var resp struct {
		Trades []SwapTrade `json:"trades"`
		Status string      `json:"status"`
	}
	if err := c.post("/getswaptrades", req, &resp); err != nil {
		return nil, err
	}
	return resp.Trades, nil
}

// HtlcOutput represents an on-chain HTLC output.
type HtlcOutput struct {
	Amount        uint64 `json:"amount"`
	RecipientKey  string `json:"recipientKey"`
	RefundKey     string `json:"refundKey"`
	HashLock      string `json:"hashLock"`
	TimeoutHeight uint32 `json:"timeoutHeight"`
	IsSpent       bool   `json:"isSpent"`
	Status        string `json:"status"`
}

// GetHtlc fetches an HTLC output by index.
func (c *FuegoClient) GetHtlc(index uint32) (*HtlcOutput, error) {
	req := map[string]interface{}{"index": index}
	var resp HtlcOutput
	if err := c.post("/gethtlc", req, &resp); err != nil {
		return nil, err
	}
	return &resp, nil
}

// GetHtlcCount returns the total number of HTLC outputs on chain.
func (c *FuegoClient) GetHtlcCount() (uint32, error) {
	var resp struct {
		Count  uint32 `json:"count"`
		Status string `json:"status"`
	}
	if err := c.post("/gethtlccount", nil, &resp); err != nil {
		return 0, err
	}
	return resp.Count, nil
}

// GetInfo fetches basic node info.
func (c *FuegoClient) GetInfo() (map[string]interface{}, error) {
	var resp map[string]interface{}
	if err := c.post("/getinfo", nil, &resp); err != nil {
		return nil, err
	}
	return resp, nil
}

func (c *FuegoClient) post(path string, reqBody interface{}, result interface{}) error {
	var body io.Reader
	if reqBody != nil {
		data, err := json.Marshal(reqBody)
		if err != nil {
			return fmt.Errorf("marshal: %w", err)
		}
		body = bytes.NewReader(data)
	}

	resp, err := c.client.Post(c.endpoint+path, "application/json", body)
	if err != nil {
		return fmt.Errorf("request %s: %w", path, err)
	}
	defer resp.Body.Close()

	raw, err := io.ReadAll(resp.Body)
	if err != nil {
		return fmt.Errorf("read %s: %w", path, err)
	}

	if err := json.Unmarshal(raw, result); err != nil {
		return fmt.Errorf("decode %s: %w", path, err)
	}
	return nil
}
