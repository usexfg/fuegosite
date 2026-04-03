// swapxfg/app/cd_rpc.go
package app

// CdOffer represents a CD sell/buy offer from the fuegod P2P relay.
type CdOffer struct {
	OfferID      string `json:"offerId"`
	IsSell       bool   `json:"isSell"`
	CdAmount     uint64 `json:"cdAmount"`   // atomic units
	CdTerm       uint32 `json:"cdTerm"`     // months
	CdEpoch      uint32 `json:"cdEpoch"`
	CdKeyImage   string `json:"cdKeyImage"`
	AskPrice     uint64 `json:"askPrice"`   // atomic units XFG
	MakerPubKey  string `json:"makerPubKey"`
	Timestamp    uint64 `json:"timestamp"`
	TTLBlocks    uint32 `json:"ttlBlocks"`
	PostedHeight uint32 `json:"postedHeight"`
}

// CdPriceStats holds market stats for a specific CD amount tier.
type CdPriceStats struct {
	Amount       uint64  `json:"cdAmount"`
	FaceValue    uint64  `json:"faceValue"`
	EstInterest  uint64  `json:"estInterest"`  // atomic units, from fee pool
	MedianAsk    uint64  `json:"medianAsk"`
	DiscountPct  float64 `json:"discountPct"`  // negative = below face value
	ActiveOffers int     `json:"activeOffers"`
}

// AcceptCdResponse is returned by /acceptcd.
type AcceptCdResponse struct {
	PartialTx string `json:"partialTx"`
	ExpiresAt uint32 `json:"expiresAt"`
	Status    string `json:"status"`
}

// GetCdOffers fetches all CD offers (amount=0 means all tiers).
func (c *FuegoClient) GetCdOffers(amount uint64) ([]CdOffer, error) {
	req := map[string]interface{}{"amount": amount}
	var resp struct {
		Offers []CdOffer `json:"offers"`
		Status string    `json:"status"`
	}
	if err := c.post("/getcdoffers", req, &resp); err != nil {
		return nil, err
	}
	return resp.Offers, nil
}

// GetCdPrice fetches market stats for a specific CD amount tier.
func (c *FuegoClient) GetCdPrice(amount uint64) (*CdPriceStats, error) {
	req := map[string]interface{}{"amount": amount}
	var resp CdPriceStats
	return &resp, c.post("/getcdprice", req, &resp)
}

// AcceptCdOffer initiates a co-signed CD transfer for the given offer.
func (c *FuegoClient) AcceptCdOffer(offerID, buyerCommitKey string) (*AcceptCdResponse, error) {
	req := map[string]interface{}{
		"offerId":        offerID,
		"buyerCommitKey": buyerCommitKey,
	}
	var resp AcceptCdResponse
	return &resp, c.post("/acceptcd", req, &resp)
}

// SubmitCdOffer posts a new CD offer to the relay.
func (c *FuegoClient) SubmitCdOffer(offer CdOffer) error {
	var resp struct {
		Status string `json:"status"`
	}
	return c.post("/submitcd", offer, &resp)
}

// CancelCdOffer cancels an existing CD offer by offer ID + maker signature.
func (c *FuegoClient) CancelCdOffer(offerID, pubKey, sig string) error {
	req := map[string]interface{}{
		"offerId":     offerID,
		"makerPubKey": pubKey,
		"signature":   sig,
	}
	var resp struct {
		Status string `json:"status"`
	}
	return c.post("/cancelcd", req, &resp)
}

// CdDiscount returns the discount percentage vs face value.
// Negative = selling below face value (discount), positive = premium.
func CdDiscount(cdAmount, askPrice uint64) float64 {
	if cdAmount == 0 {
		return 0
	}
	return (float64(askPrice) - float64(cdAmount)) / float64(cdAmount) * 100.0
}
