package app

import (
	crand "crypto/rand"
	"crypto/sha256"
	"encoding/hex"
	"fmt"
	"strconv"
	"strings"
	"time"

	tea "github.com/charmbracelet/bubbletea"
	"github.com/charmbracelet/lipgloss"
)

var (
	headerStyle = lipgloss.NewStyle().
			Foreground(lipgloss.Color("#0AC18E")).
			Bold(true)

	priceStyle = lipgloss.NewStyle().
			Foreground(lipgloss.Color("#FFD700")).
			Bold(true)

	greenStyle = lipgloss.NewStyle().
			Foreground(lipgloss.Color("#00FF7F"))

	redStyle = lipgloss.NewStyle().
			Foreground(lipgloss.Color("#FF4500"))

	dimStyle = lipgloss.NewStyle().
			Foreground(lipgloss.Color("#666666"))

	inputStyle = lipgloss.NewStyle().
			Foreground(lipgloss.Color("#FFFFFF")).
			Bold(true)

	statusStyle = lipgloss.NewStyle().
			Foreground(lipgloss.Color("#FF6347"))

	activeTabStyle = lipgloss.NewStyle().
			Foreground(lipgloss.Color("#0AC18E")).
			Bold(true).
			Underline(true)

	inactiveTabStyle = lipgloss.NewStyle().
			Foreground(lipgloss.Color("#555555"))
)

type activeSwap struct {
	offerID   string
	preimage  string
	hashLock  string
	xfgAmount uint64
	bchNeeded float64
	rate      float64
	startedAt time.Time
	xfgLocked bool
	htlcIndex int
}

type tuiModel struct {
	client    *FuegoClient
	width     int
	height    int
	offers    []SwapOffer
	trades    []SwapTrade
	price     *SwapPriceResponse
	input     string
	status    string
	statusAt  time.Time
	connected bool
	lastFetch time.Time
	tab       int
	swap      *activeSwap
	htlcCount uint32
}

type refreshMsg struct{}
type tickMsg time.Time

func newTuiModel(client *FuegoClient) tuiModel {
	return tuiModel{client: client, tab: 0}
}

func (m tuiModel) Init() tea.Cmd {
	return tea.Batch(
		func() tea.Msg { return refreshMsg{} },
		tea.Tick(5*time.Second, func(t time.Time) tea.Msg { return tickMsg(t) }),
	)
}

func (m tuiModel) Update(msg tea.Msg) (tea.Model, tea.Cmd) {
	switch msg := msg.(type) {
	case tea.KeyMsg:
		switch msg.String() {
		case "ctrl+c", "q":
			return m, tea.Quit
		case "tab":
			m.tab = (m.tab + 1) % 3
			return m, nil
		case "1":
			m.tab = 0
			return m, nil
		case "2":
			m.tab = 1
			return m, nil
		case "3":
			m.tab = 2
			return m, nil
		case "r":
			return m, func() tea.Msg { return refreshMsg{} }
		case "backspace":
			if len(m.input) > 0 {
				m.input = m.input[:len(m.input)-1]
			}
			return m, nil
		case "enter":
			m.processCommand()
			m.input = ""
			return m, nil
		default:
			if len(msg.String()) == 1 && msg.String()[0] >= 32 {
				m.input += msg.String()
			}
		}
		return m, nil
	case tea.WindowSizeMsg:
		m.width = msg.Width
		m.height = msg.Height
		return m, nil
	case refreshMsg:
		m.refresh()
		return m, nil
	case tickMsg:
		if time.Since(m.lastFetch) > 10*time.Second {
			m.refresh()
		}
		return m, tea.Tick(5*time.Second, func(t time.Time) tea.Msg { return tickMsg(t) })
	}
	return m, nil
}

func (m *tuiModel) refresh() {
	offers, err := m.client.GetOffers()
	if err != nil {
		m.connected = false
		m.status = "EFier offline: " + err.Error()
		m.statusAt = time.Now()
		return
	}
	m.offers = offers
	m.connected = true
	price, err := m.client.GetPrice()
	if err == nil {
		m.price = price
	}
	trades, err := m.client.GetTrades(20)
	if err == nil {
		m.trades = trades
	}
	m.monitorSwap()
	m.lastFetch = time.Now()
}

func (m *tuiModel) processCommand() {
	parts := strings.Fields(m.input)
	if len(parts) == 0 {
		return
	}
	cmd := strings.ToLower(parts[0])
	switch cmd {
	case "accept":
		if len(parts) < 2 {
			m.status = "Usage: accept <offer_id>"
		} else {
			m.doAccept(parts[1])
		}
	case "htlc":
		if len(parts) < 2 {
			cnt, err := m.client.GetHtlcCount()
			if err != nil {
				m.status = "HTLC query failed: " + err.Error()
			} else {
				m.status = fmt.Sprintf("HTLC outputs on chain: %d — Usage: htlc <index>", cnt)
			}
		} else {
			m.doQueryHtlc(parts[1])
		}
	case "swap":
		if m.swap != nil {
			m.tab = 2
			m.status = "Swap details on Info tab"
		} else {
			m.status = "No active swap. Use: accept <offer_id>"
		}
	case "help":
		m.status = "accept <id> | htlc [idx] | swap | r=refresh | 1/2/3=tabs | q=quit"
	default:
		m.status = "Unknown: " + cmd + " (type 'help')"
	}
	m.statusAt = time.Now()
}

func (m *tuiModel) doAccept(idPrefix string) {
	var offer *SwapOffer
	for i := range m.offers {
		if strings.HasPrefix(m.offers[i].OfferID, idPrefix) {
			offer = &m.offers[i]
			break
		}
	}
	if offer == nil {
		m.status = "Offer not found: " + idPrefix
		return
	}
	preimage := make([]byte, 32)
	if _, err := crand.Read(preimage); err != nil {
		m.status = "Crypto error: " + err.Error()
		return
	}
	hash := sha256.Sum256(preimage)
	xfg := float64(offer.XfgAmount) / 1e7
	rate := float64(offer.RateNum) / 1e7
	bchNeeded := xfg / rate

	m.swap = &activeSwap{
		offerID:   offer.OfferID,
		preimage:  hex.EncodeToString(preimage),
		hashLock:  hex.EncodeToString(hash[:]),
		xfgAmount: offer.XfgAmount,
		bchNeeded: bchNeeded,
		rate:      rate,
		startedAt: time.Now(),
		htlcIndex: -1,
	}
	m.status = fmt.Sprintf("SWAP INITIATED — Lock %.6f BCH with hashlock %s...", bchNeeded, m.swap.hashLock[:16])
	m.tab = 2
}

func (m *tuiModel) doQueryHtlc(indexStr string) {
	idx, err := strconv.ParseUint(indexStr, 10, 32)
	if err != nil {
		m.status = "Invalid index: " + indexStr
		return
	}
	htlc, err := m.client.GetHtlc(uint32(idx))
	if err != nil {
		m.status = "HTLC query failed: " + err.Error()
		return
	}
	xfg := float64(htlc.Amount) / 1e7
	state := "active"
	if htlc.IsSpent {
		state = "spent"
	}
	hashPfx := htlc.HashLock
	if len(hashPfx) > 16 {
		hashPfx = hashPfx[:16]
	}
	m.status = fmt.Sprintf("HTLC #%d: %.2f XFG, timeout=%d, %s, hash=%s...", idx, xfg, htlc.TimeoutHeight, state, hashPfx)
}

func (m *tuiModel) monitorSwap() {
	if m.swap == nil || m.swap.xfgLocked {
		return
	}
	count, err := m.client.GetHtlcCount()
	if err != nil {
		return
	}
	for i := m.htlcCount; i < count; i++ {
		htlc, err := m.client.GetHtlc(i)
		if err != nil {
			continue
		}
		if htlc.HashLock == m.swap.hashLock && !htlc.IsSpent {
			m.swap.xfgLocked = true
			m.swap.htlcIndex = int(i)
			m.status = fmt.Sprintf("XFG LOCKED! %.2f XFG in HTLC #%d — claim with your preimage!", float64(htlc.Amount)/1e7, i)
			m.statusAt = time.Now()
			break
		}
	}
	m.htlcCount = count
}

func (m tuiModel) View() string {
	if m.width == 0 {
		return ""
	}
	header := m.renderHeader()
	var content string
	switch m.tab {
	case 0:
		content = m.renderOrderBook()
	case 1:
		content = m.renderTrades()
	case 2:
		content = m.renderInfo()
	}
	inputBar := m.renderInput()
	return lipgloss.JoinVertical(lipgloss.Left, header, "", content, "", inputBar)
}

func (m tuiModel) renderHeader() string {
	var connStr string
	if m.connected {
		connStr = greenStyle.Render("●") + dimStyle.Render(" EFier")
	} else {
		connStr = redStyle.Render("●") + dimStyle.Render(" offline")
	}
	var priceStr string
	if m.price != nil {
		rate, _ := strconv.ParseFloat(m.price.CompositeRate, 64)
		if rate <= 0 {
			rate, _ = strconv.ParseFloat(m.price.SeedRate, 64)
		}
		if rate > 0 {
			bchPerXfg := 1.0 / rate
			priceStr = priceStyle.Render(fmt.Sprintf("1 XFG = %.8f BCH", bchPerXfg))
			usdMid, _ := strconv.ParseFloat(m.price.XfgUsdMid, 64)
			if usdMid > 0 {
				priceStr += dimStyle.Render(fmt.Sprintf("  ($%.4f)", usdMid))
			}
		}
	}
	title := headerStyle.Render("₿ XFG⇄BCH")
	tabs := []string{"[1]Book", "[2]Trades", "[3]Info"}
	var tabStr string
	for i, t := range tabs {
		if i == m.tab {
			tabStr += activeTabStyle.Render(t) + " "
		} else {
			tabStr += inactiveTabStyle.Render(t) + " "
		}
	}
	left := title + "  " + priceStr
	right := tabStr + "  " + connStr
	gap := m.width - lipgloss.Width(left) - lipgloss.Width(right)
	if gap < 1 {
		gap = 1
	}
	return left + strings.Repeat(" ", gap) + right
}

func (m tuiModel) renderOrderBook() string {
	if len(m.offers) == 0 {
		return dimStyle.Render("  No offers available. Waiting for XFG holders to post swap offers...")
	}
	hdr := fmt.Sprintf("  %-10s %16s %16s %14s %s", "OFFER", "XFG AMOUNT", "RATE (XFG/BCH)", "BCH VALUE", "AGE")
	lines := []string{dimStyle.Render(hdr)}
	maxLines := m.height - 8
	if maxLines < 3 {
		maxLines = 3
	}
	for i, o := range m.offers {
		if i >= maxLines {
			lines = append(lines, dimStyle.Render(fmt.Sprintf("  ... and %d more", len(m.offers)-i)))
			break
		}
		xfg := float64(o.XfgAmount) / 1e7
		rate := float64(o.RateNum) / 1e7
		bchVal := xfg / rate
		age := formatAge(o.Timestamp)
		id := o.OfferID
		if len(id) > 8 {
			id = id[:8]
		}
		line := fmt.Sprintf("  %-10s %13.2f XFG %13.1f %11.6f BCH %s", id, xfg, rate, bchVal, age)
		lines = append(lines, greenStyle.Render(line))
	}
	return strings.Join(lines, "\n")
}

func (m tuiModel) renderTrades() string {
	if len(m.trades) == 0 {
		return dimStyle.Render("  No recent trades.")
	}
	hdr := fmt.Sprintf("  %16s %16s %14s %s", "XFG VOLUME", "RATE (XFG/BCH)", "BCH VALUE", "AGE")
	lines := []string{dimStyle.Render(hdr)}
	maxLines := m.height - 8
	if maxLines < 3 {
		maxLines = 3
	}
	for i, t := range m.trades {
		if i >= maxLines {
			break
		}
		xfg := float64(t.XfgAmount) / 1e7
		rate, _ := strconv.ParseFloat(t.Rate, 64)
		bchVal := 0.0
		if rate > 0 {
			bchVal = xfg / rate
		}
		age := formatAge(t.Timestamp)
		line := fmt.Sprintf("  %13.2f XFG %13.1f %11.6f BCH %s", xfg, rate, bchVal, age)
		lines = append(lines, line)
	}
	return strings.Join(lines, "\n")
}

func (m tuiModel) renderInfo() string {
	var lines []string
	if m.swap != nil {
		lines = append(lines, headerStyle.Render("  Active Swap"))
		lines = append(lines, "")
		id := m.swap.offerID
		if len(id) > 16 {
			id = id[:16] + "..."
		}
		lines = append(lines, fmt.Sprintf("  Offer:      %s", id))
		lines = append(lines, fmt.Sprintf("  XFG amount: %.2f", float64(m.swap.xfgAmount)/1e7))
		lines = append(lines, fmt.Sprintf("  BCH needed: %.6f", m.swap.bchNeeded))
		lines = append(lines, fmt.Sprintf("  Rate:       %.1f XFG/BCH", m.swap.rate))
		lines = append(lines, "")
		lines = append(lines, priceStyle.Render(fmt.Sprintf("  Hashlock: %s", m.swap.hashLock)))
		lines = append(lines, redStyle.Render(fmt.Sprintf("  Preimage: %s", m.swap.preimage)))
		lines = append(lines, redStyle.Render("  SAVE YOUR PREIMAGE — required to claim XFG"))
		lines = append(lines, "")
		if m.swap.xfgLocked {
			lines = append(lines, greenStyle.Render(fmt.Sprintf("  XFG locked in HTLC #%d — submit preimage to claim", m.swap.htlcIndex)))
		} else {
			lines = append(lines, statusStyle.Render("  Waiting for XFG maker to lock..."))
			lines = append(lines, dimStyle.Render("    1. Lock BCH with hashlock above"))
			lines = append(lines, dimStyle.Render("    2. Maker sees your BCH lock, locks XFG"))
			lines = append(lines, dimStyle.Render("    3. Claim XFG with your preimage"))
		}
		lines = append(lines, "")
	}
	lines = append(lines, headerStyle.Render("  Price Sources"))
	lines = append(lines, "")
	if m.price != nil && len(m.price.Sources) > 0 {
		for _, src := range m.price.Sources {
			rate, _ := strconv.ParseFloat(src.Rate, 64)
			weight, _ := strconv.ParseFloat(src.Weight, 64)
			staleStr := ""
			if src.Stale {
				staleStr = redStyle.Render(" [stale]")
			}
			lines = append(lines, fmt.Sprintf("  %-20s rate=%.1f  weight=%.1f%s", src.Name, rate, weight, staleStr))
		}
	} else {
		lines = append(lines, dimStyle.Render("  No price sources available"))
	}
	lines = append(lines, "")
	lines = append(lines, headerStyle.Render("  Cross-Pair XFG Price"))
	if m.price != nil {
		lo, _ := strconv.ParseFloat(m.price.XfgUsdLow, 64)
		hi, _ := strconv.ParseFloat(m.price.XfgUsdHigh, 64)
		mid, _ := strconv.ParseFloat(m.price.XfgUsdMid, 64)
		if mid > 0 {
			lines = append(lines, fmt.Sprintf("  USD range: $%.4f — $%.4f  (mid: $%.4f)", lo, hi, mid))
		}
		for _, pi := range m.price.PairImplied {
			usd, _ := strconv.ParseFloat(pi.ImpliedUsd, 64)
			name := pairName(pi.Pair)
			lines = append(lines, fmt.Sprintf("    via %s: $%.4f", name, usd))
		}
	}
	lines = append(lines, "")
	lines = append(lines, headerStyle.Render("  Swap Flow"))
	lines = append(lines, dimStyle.Render("  1. Browse offers from XFG holders (Book tab)"))
	lines = append(lines, dimStyle.Render("  2. accept <offer_id> → locks your BCH in HTLC"))
	lines = append(lines, dimStyle.Render("  3. XFG maker sees hashlock, locks XFG"))
	lines = append(lines, dimStyle.Render("  4. You claim XFG (reveals preimage)"))
	lines = append(lines, dimStyle.Render("  5. Maker claims your BCH with revealed preimage"))
	lines = append(lines, "")
	lines = append(lines, dimStyle.Render("  HEAT/BCH pool:  1 XFG = 10,000,000 HEAT (ERC-20)"))
	return strings.Join(lines, "\n")
}

func (m tuiModel) renderInput() string {
	var statusLine string
	if m.status != "" && time.Since(m.statusAt) < 5*time.Second {
		statusLine = statusStyle.Render("  " + m.status)
	}
	prompt := inputStyle.Render("  > " + m.input + "█")
	hint := dimStyle.Render("  accept <id> | htlc [idx] | swap | r=refresh | tab | q")
	if statusLine != "" {
		return lipgloss.JoinVertical(lipgloss.Left, statusLine, prompt, hint)
	}
	return lipgloss.JoinVertical(lipgloss.Left, prompt, hint)
}

func formatAge(ts uint64) string {
	if ts == 0 {
		return "-"
	}
	d := time.Since(time.Unix(int64(ts), 0))
	switch {
	case d < time.Minute:
		return fmt.Sprintf("%ds", int(d.Seconds()))
	case d < time.Hour:
		return fmt.Sprintf("%dm", int(d.Minutes()))
	case d < 24*time.Hour:
		return fmt.Sprintf("%dh", int(d.Hours()))
	default:
		return fmt.Sprintf("%dd", int(d.Hours()/24))
	}
}

func pairName(pair uint8) string {
	switch pair {
	case 0:
		return "XMR"
	case 1:
		return "ETH"
	case 2:
		return "BCH"
	default:
		return "?"
	}
}
