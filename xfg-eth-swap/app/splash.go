package app

import (
	"math"
	"math/rand"
	"strings"
	"time"

	tea "github.com/charmbracelet/bubbletea"
	"github.com/charmbracelet/lipgloss"
)

// ── ETH diamond geometry ─────────────────────────────────────────
const (
	dMaxHW = 13            // half-width at equator (total width = 26)
	dTopH  = 13            // top half rows (perfect 2-char step growth)
	dBotH  = 7             // bottom half rows (perfect 4-char step shrink)
	dH     = dTopH + dBotH // 20 total
	dW     = dMaxHW * 2    // 26 total

	fW   = 38 // fire simulation width
	fH   = 10 // fire simulation height
	fLap = 3  // fire rows overlapping diamond bottom
)

// ── Diamond palette (fire-colored ETH diamond) ──────────────────
var (
	dEdge = lipgloss.Color("#FFDD66") // bright gold edge
	dSeam = lipgloss.Color("#FF8C00") // orange equator seam
	dMid  = lipgloss.Color("#FFA030") // amber center line
	dGlow = lipgloss.Color("#FFFFCC") // white-hot shimmer
)

// Top-left facet: 3 bands (white-gold at tip → deep orange near equator)
var tlB = [3]lipgloss.Color{"#FFD060", "#E8A030", "#CC7818"}

// Top-right facet: 3 bands (brighter, light-facing side)
var trB = [3]lipgloss.Color{"#FFE888", "#FFD060", "#FFAA30"}

// Bottom-left facet: 2 bands (deep red/ember, shadow side)
var blB = [2]lipgloss.Color{"#8B2000", "#A03010"}

// Bottom-right facet: 2 bands (dark orange, slight warmth)
var brB = [2]lipgloss.Color{"#CC4400", "#DD5510"}

// ── Fire palette (11 stops: near-black → white-yellow) ───────────
var fPal = [11]lipgloss.Color{
	"#180400", "#3D0A00", "#6D1400", "#9E2000",
	"#D43800", "#FF5500", "#FF7F11", "#FFA940",
	"#FFD066", "#FFE899", "#FFFADD",
}

var fCh = [11]rune{' ', '.', ':', '*', '░', '▒', '▓', '█', '█', '█', '█'}

// ── Title ────────────────────────────────────────────────────────
const splashTitle = `  ██╗  ██╗███████╗ ██████╗     ███████╗████████╗██╗  ██╗
  ╚██╗██╔╝██╔════╝██╔════╝     ██╔════╝╚══██╔══╝██║  ██║
   ╚███╔╝ █████╗  ██║  ███╗    █████╗     ██║   ███████║
   ██╔██╗ ██╔══╝  ██║   ██║    ██╔══╝     ██║   ██╔══██║
  ██╔╝ ██╗██║     ╚██████╔╝    ███████╗   ██║   ██║  ██║
  ╚═╝  ╚═╝╚═╝      ╚═════╝     ╚══════╝   ╚═╝   ╚═╝  ╚═╝`

// ── Model ────────────────────────────────────────────────────────
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

// ── Diamond rendering ────────────────────────────────────────────

func dHW(row int) int {
	if row < dTopH {
		// 1 + 12*row/12 = 1+row  → perfect 2-char steps
		return 1 + (dMaxHW-1)*row/(dTopH-1)
	}
	// 13 - 12*dr/6 = 13-2*dr → perfect 4-char steps
	return dMaxHW - (dMaxHW-1)*(row-dTopH)/(dBotH-1)
}

func dPixel(row, col, frame int) (bool, lipgloss.Color) {
	hw := dHW(row)
	l := dMaxHW - hw
	r := dMaxHW + hw - 1

	if col < l || col > r {
		return false, ""
	}

	// Shimmer wave: single bright row sweeping down the diamond
	if wp := frame % (dH + 8); row == wp && col > l && col < r {
		return true, dGlow
	}

	// Edge pixels
	if col == l || col == r {
		return true, dEdge
	}

	// Equator seam (2 rows at the junction)
	if row == dTopH-1 || row == dTopH {
		return true, dSeam
	}

	// Center vertical line (2 cols)
	if col == dMaxHW-1 || col == dMaxHW {
		return true, dMid
	}

	// Facet color with banded gradient for 3D depth
	if row < dTopH {
		b := row * 3 / dTopH
		if b > 2 {
			b = 2
		}
		if col < dMaxHW {
			return true, tlB[b]
		}
		return true, trB[b]
	}

	b := (row - dTopH) * 2 / dBotH
	if b > 1 {
		b = 1
	}
	if col < dMaxHW {
		return true, blB[b]
	}
	return true, brB[b]
}

// ── View ─────────────────────────────────────────────────────────

func (m splashModel) View() string {
	if m.width == 0 {
		return ""
	}

	artW := fW
	dOff := (artW - dW) / 2
	fStart := dH - fLap
	totalH := fStart + fH

	lines := make([]string, 0, totalH)
	for row := 0; row < totalH; row++ {
		var b strings.Builder

		for col := 0; col < artW; col++ {
			// Diamond pixel?
			dc := col - dOff
			if row < dH && dc >= 0 && dc < dW {
				if ok, c := dPixel(row, dc, m.frame); ok {
					b.WriteString(lipgloss.NewStyle().Foreground(c).Render("█"))
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

	tS := lipgloss.NewStyle().Foreground(lipgloss.Color("#FF6347")).Bold(true)
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
