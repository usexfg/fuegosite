// swapxfg/app/app.go
package app

import (
	"fmt"

	tea "github.com/charmbracelet/bubbletea"
)

// Run is the main entry point. Shows splash then launches the trading TUI.
func Run(cfg Config) error {
	// Splash screen
	if !cfg.NoSplash {
		splash := newSplashModel()
		p := tea.NewProgram(splash, tea.WithAltScreen())
		if _, err := p.Run(); err != nil {
			return fmt.Errorf("splash: %w", err)
		}
	}

	// Main trading TUI
	tui := newTuiModel(cfg)
	p := tea.NewProgram(tui, tea.WithAltScreen())
	if _, err := p.Run(); err != nil {
		return fmt.Errorf("tui: %w", err)
	}

	return nil
}
