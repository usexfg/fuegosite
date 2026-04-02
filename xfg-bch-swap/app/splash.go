package app

import (
	"math"
	"math/rand"
	"strings"
	"time"

	tea "github.com/charmbracelet/bubbletea"
	"github.com/charmbracelet/lipgloss"
)

// ── BCH circle logo geometry (green circle + ₿) ────────────────
const (
	logoR  = 16
	logoD  = logoR*2 + 1
	logoCX = logoR
	logoCY = logoR

	fW   = 38
	fH   = 10
	fLap = 3
)

// ── BCH palette (green) ────────────────────────────────────────
var (
	bchCircle = lipgloss.Color("#0AC18E") // BCH green
	bchCirclD = lipgloss.Color("#078F68") // darker edge
	bchLetter = lipgloss.Color("#FFFFFF") // white ₿
	bchGlow   = lipgloss.Color("#88FFCC") // shimmer
)

// Fire palette
var fPal = [11]lipgloss.Color{
	"#180400", "#3D0A00", "#6D1400", "#9E2000",
	"#D43800", "#FF5500", "#FF7F11", "#FFA940",
	"#FFD066", "#FFE899", "#FFFADD",
}
var fCh = [11]rune{' ', '.', ':', '*', '░', '▒', '▓', '█', '█', '█', '█'}

const splashTitle = `  ██╗  ██╗███████╗ ██████╗     ██████╗  ██████╗██╗  ██╗
  ╚██╗██╔╝██╔════╝██╔════╝     ██╔══██╗██╔════╝██║  ██║
   ╚███╔╝ █████╗  ██║  ███╗    ██████╔╝██║     ███████║
   ██╔██╗ ██╔══╝  ██║   ██║    ██╔══██╗██║     ██╔══██║
  ██╔╝ ██╗██║     ╚██████╔╝    ██████╔╝╚██████╗██║  ██║
  ╚═╝  ╚═╝╚═╝      ╚═════╝     ╚═════╝  ╚═════╝╚═╝  ╚═╝`

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

func (m *splashModel) stepFire() {
	cx := float64(fW) / 2.0
	for x := 0; x < fW; x++ {
		d := math.Abs(float64(x)-cx) / cx
		pk := 1.0 - d*0.4
		m.fire[fH-1][x] = pk * (0.6 + rand.Float64()*0.4)
		if fH > 1 {
			m.fire[fH-2][x] = pk * (0.25 + rand.Float64()*0.45)
		}
	}
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

// ── Circle with ₿ ──────────────────────────────────────────────

func inCircle(x, y int) bool {
	dx := float64(x-logoCX) * 0.5
	dy := float64(y - logoCY)
	return dx*dx+dy*dy <= float64(logoR)*float64(logoR)*0.25
}

func isEdge(x, y int) bool {
	dx := float64(x-logoCX) * 0.5
	dy := float64(y - logoCY)
	r2 := dx*dx + dy*dy
	outer := float64(logoR) * float64(logoR) * 0.25
	inner := float64(logoR-1) * float64(logoR-1) * 0.25
	return r2 <= outer && r2 > inner
}

// Hand-crafted BCH ₿ bitmap with ~12° clockwise tilt baked in.
// Each row is a set of horizontal spans [left, right] inclusive.
// Tilt: top shifts +3 right, bottom shifts -3 left from center.
// Designed for logoR=16 (33×33 grid, center at 16,16).
type bSpan struct{ l, r int }

var bchB = map[int][]bSpan{
	10: {{14, 15}, {18, 19}},  // top serifs (+3)
	11: {{12, 22}},            // top bar (+2)
	12: {{12, 15}, {19, 23}},  // spine + upper bump (+2)
	13: {{11, 14}, {18, 22}},  // spine + upper bump (+1)
	14: {{11, 14}, {18, 22}},  // spine + upper bump (+1)
	15: {{10, 22}},            // middle bar (0)
	16: {{10, 13}, {17, 24}},  // spine + lower bump (0)
	17: {{9, 12}, {16, 24}},   // spine + lower bump (-1)
	18: {{9, 12}, {16, 23}},   // spine + lower bump (-1)
	19: {{8, 11}, {15, 22}},   // spine + lower bump (-2)
	20: {{8, 23}},             // bottom bar (-2)
	21: {{8, 9}, {12, 13}},   // bottom serifs (-3)
}

func isB(x, y int) bool {
	spans, ok := bchB[y]
	if !ok {
		return false
	}
	for _, s := range spans {
		if x >= s.l && x <= s.r {
			return true
		}
	}
	return false
}

func logoPixel(row, col, frame int) (bool, lipgloss.Color) {
	if !inCircle(col, row) {
		return false, ""
	}

	// Shimmer wave
	if wp := frame % (logoD + 8); row == wp {
		return true, bchGlow
	}

	// ₿ letter
	if isB(col, row) {
		return true, bchLetter
	}

	// Circle interior (solid, no edge ring)
	return true, bchCircle
}

func (m splashModel) View() string {
	if m.width == 0 {
		return ""
	}

	artW := fW
	logoOff := (artW - logoD) / 2
	fStart := logoD - fLap
	totalH := fStart + fH

	lines := make([]string, 0, totalH)
	for row := 0; row < totalH; row++ {
		var b strings.Builder
		for col := 0; col < artW; col++ {
			// Logo circle pixel
			lc := col - logoOff
			if row < logoD && lc >= 0 && lc < logoD {
				if ok, c := logoPixel(row, lc, m.frame); ok {
					b.WriteString(lipgloss.NewStyle().Foreground(c).Render("█"))
					continue
				}
			}

			// Fire pixel
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
	tS := lipgloss.NewStyle().Foreground(lipgloss.Color("#0AC18E")).Bold(true)
	sS := lipgloss.NewStyle().Foreground(lipgloss.Color("#555555"))

	content := lipgloss.JoinVertical(lipgloss.Center,
		art,
		"",
		tS.Render(splashTitle),
		"",
		sS.Render("atomic swap client  ·  press any key"),
	)

	return lipgloss.Place(m.width, m.height,
		lipgloss.Center, lipgloss.Center,
		content,
	)
}
