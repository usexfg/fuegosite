package app

import (
	"math"
	"math/rand"
	"strings"
	"time"

	tea "github.com/charmbracelet/bubbletea"
	"github.com/charmbracelet/lipgloss"
)

// ── Monero "M" circle logo geometry ─────────────────────────────
const (
	logoR = 10 // circle radius in character cells

	fW   = 38 // fire simulation width
	fH   = 10 // fire simulation height
	fLap = 3  // fire rows overlapping circle bottom
)

// Logo diameter (pixel grid)
var logoD = logoR * 2

// ── Monero palette ──────────────────────────────────────────────
var (
	circleColor  = lipgloss.Color("#FF6600") // Monero orange
	circleDark   = lipgloss.Color("#CC5200") // dark edge
	mLetterFg    = lipgloss.Color("#FFFFFF") // white M
	mLetterBg    = lipgloss.Color("#FF6600") // orange background for M
	shimmerColor = lipgloss.Color("#FFCC88") // shimmer highlight
)

// ── Fire palette (11 stops: near-black -> white-yellow) ─────────
var fPal = [11]lipgloss.Color{
	"#180400", "#3D0A00", "#6D1400", "#9E2000",
	"#D43800", "#FF5500", "#FF7F11", "#FFA940",
	"#FFD066", "#FFE899", "#FFFADD",
}

var fCh = [11]rune{' ', '.', ':', '*', '\u2591', '\u2592', '\u2593', '\u2588', '\u2588', '\u2588', '\u2588'}

// ── Title ───────────────────────────────────────────────────────
const splashTitle = `  ██╗  ██╗███████╗ ██████╗     ██╗  ██╗███╗   ███╗██████╗
  ╚██╗██╔╝██╔════╝██╔════╝     ╚██╗██╔╝████╗ ████║██╔══██╗
   ╚███╔╝ █████╗  ██║  ███╗     ╚███╔╝ ██╔████╔██║██████╔╝
   ██╔██╗ ██╔══╝  ██║   ██║     ██╔██╗ ██║╚██╔╝██║██╔══██╗
  ██╔╝ ██╗██║     ╚██████╔╝    ██╔╝ ██╗██║ ╚═╝ ██║██║  ██║
  ╚═╝  ╚═╝╚═╝      ╚═════╝     ╚═╝  ╚═╝╚═╝     ╚═╝╚═╝  ╚═╝`

// ── Model ───────────────────────────────────────────────────────
type splashModel struct {
	width, height int
	frame         int
	fire          [][]float64
}

type splashTickMsg time.Time

func splashTick() tea.Cmd {
	return tea.Tick(80*time.Millisecond, func(t time.Time) tea.Msg {
		return splashTickMsg(t)
	})
}

func newSplashModel() splashModel {
	f := make([][]float64, fH)
	for i := range f {
		f[i] = make([]float64, fW)
	}
	return splashModel{fire: f}
}

func (m splashModel) Init() tea.Cmd {
	return splashTick()
}

func (m splashModel) Update(msg tea.Msg) (tea.Model, tea.Cmd) {
	switch msg := msg.(type) {
	case tea.KeyMsg:
		return m, tea.Quit
	case tea.MouseMsg:
		_ = msg
	case tea.WindowSizeMsg:
		m.width = msg.Width
		m.height = msg.Height
	case splashTickMsg:
		m.frame++
		if m.frame > 40 {
			return m, tea.Quit
		}
		m.stepFire()
		return m, splashTick()
	}
	return m, nil
}

// stepFire runs one iteration of the doom fire algorithm.
func (m *splashModel) stepFire() {
	cx := float64(fW) / 2.0
	// Seed bottom rows: hotter in center, cooler at edges
	for x := 0; x < fW; x++ {
		d := math.Abs(float64(x)-cx) / cx
		pk := 1.0 - d*0.4
		m.fire[fH-1][x] = pk * (0.6 + rand.Float64()*0.4)
		if fH > 1 {
			m.fire[fH-2][x] = pk * (0.25 + rand.Float64()*0.45)
		}
	}
	// Propagate upward
	for y := 0; y < fH-2; y++ {
		for x := 0; x < fW; x++ {
			s := m.fire[y+1][x] * 3.0
			if x > 0 {
				s += m.fire[y+1][x-1]
			} else {
				s += m.fire[y+1][x]
			}
			if x < fW-1 {
				s += m.fire[y+1][x+1]
			} else {
				s += m.fire[y+1][x]
			}
			if y+2 < fH {
				s += m.fire[y+2][x]
			} else {
				s += m.fire[y+1][x]
			}
			m.fire[y][x] = math.Max(0, s/6.0-0.05-rand.Float64()*0.07)
		}
	}
}

// ── Circle + "M" rendering ──────────────────────────────────────

// inCircle tests if (x,y) is inside the circle centered at (cx,cy)
// with radius r. The 0.5 X-axis correction compensates for terminal
// characters being ~2x taller than wide.
func inCircle(x, y, cx, cy, r float64) bool {
	dx := (x - cx) * 0.5
	dy := y - cy
	return dx*dx+dy*dy <= r*r
}

// inM tests if a position within the circle is part of the "M" letter.
// The M has two vertical legs (width 2 chars) and two diagonals meeting
// at the vertical center.
func inM(col, row, diam int) bool {
	// M letter occupies roughly the inner 60% vertically and 80% horizontally
	mTop := diam/2 - logoR*6/10
	mBot := diam/2 + logoR*6/10
	mLeft := diam/2 - logoR*8/10
	mRight := diam/2 + logoR*8/10

	if row < mTop || row > mBot || col < mLeft || col > mRight {
		return false
	}

	mW := mRight - mLeft
	mH := mBot - mTop
	lc := col - mLeft // local col within M bounding box
	lr := row - mTop  // local row within M bounding box

	// Left leg (2 chars wide)
	if lc <= 1 {
		return true
	}
	// Right leg (2 chars wide)
	if lc >= mW-1 {
		return true
	}
	// Left diagonal: from top-left going to center
	if mW > 0 {
		diag := lc * mH / (mW / 2)
		if lr >= diag-1 && lr <= diag+1 && lc <= mW/2 {
			return true
		}
		// Right diagonal: from top-right going to center
		diag2 := (mW - lc) * mH / (mW / 2)
		if lr >= diag2-1 && lr <= diag2+1 && lc >= mW/2 {
			return true
		}
	}

	return false
}

// ── View ────────────────────────────────────────────────────────

func (m splashModel) View() string {
	if m.width == 0 {
		return ""
	}

	artW := fW
	logoOff := (artW - logoD) / 2
	fStart := logoD - fLap
	totalH := fStart + fH

	cx := float64(logoD) / 2.0
	cy := float64(logoD) / 2.0

	mStyle := lipgloss.NewStyle().Foreground(mLetterFg).Background(mLetterBg)

	// Shimmer row sweeps down through the circle
	shimmerRow := m.frame % (logoD + 8)

	lines := make([]string, 0, totalH)
	for row := 0; row < totalH; row++ {
		var b strings.Builder

		for col := 0; col < artW; col++ {
			// Circle pixel?
			cc := col - logoOff
			if row < logoD && cc >= 0 && cc < logoD {
				if inCircle(float64(cc), float64(row), cx, cy, float64(logoR)) {
					// Is this part of the "M" letter?
					if inM(cc, row, logoD) {
						b.WriteString(mStyle.Render("\u2588"))
						continue
					}

					// Shimmer highlight
					if row == shimmerRow && cc > 1 && cc < logoD-2 {
						b.WriteString(lipgloss.NewStyle().Foreground(shimmerColor).Render("\u2588"))
						continue
					}

					// Edge detection: just barely inside circle -> dark edge
					if !inCircle(float64(cc), float64(row), cx, cy, float64(logoR)-1.2) {
						b.WriteString(lipgloss.NewStyle().Foreground(circleDark).Render("\u2588"))
						continue
					}

					b.WriteString(lipgloss.NewStyle().Foreground(circleColor).Render("\u2588"))
					continue
				}
			}

			// Fire pixel?
			fr := row - fStart
			if fr >= 0 && fr < fH && col >= 0 && col < fW {
				heat := m.fire[fr][col]
				if heat > 0.04 {
					idx := int(heat * 10)
					if idx > 10 {
						idx = 10
					}
					if fCh[idx] != ' ' {
						b.WriteString(lipgloss.NewStyle().Foreground(fPal[idx]).Render(string(fCh[idx])))
						continue
					}
				}
			}

			b.WriteString(" ")
		}

		lines = append(lines, b.String())
	}

	art := strings.Join(lines, "\n")

	tS := lipgloss.NewStyle().Foreground(lipgloss.Color("#FF6600")).Bold(true)
	sS := lipgloss.NewStyle().Foreground(lipgloss.Color("#555555"))

	content := lipgloss.JoinVertical(lipgloss.Center,
		art,
		"",
		tS.Render(splashTitle),
		"",
		sS.Render("atomic swap client  \u00b7  press any key"),
	)

	return lipgloss.Place(m.width, m.height,
		lipgloss.Center, lipgloss.Center,
		content,
	)
}
