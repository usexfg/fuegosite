package app

import (
	"fmt"

	tea "github.com/charmbracelet/bubbletea"
)

// Run starts the swap client: splash screen -> main TUI.
func Run(cfg Config) error {
	// Phase 1: splash screen
	splash := newSplashModel()
	p := tea.NewProgram(splash, tea.WithAltScreen())
	_, err := p.Run()
	if err != nil {
		return fmt.Errorf("splash: %w", err)
	}

	// Phase 2: main TUI
	client := NewFuegoClient(cfg.EfierRPC)
	tui := newTuiModel(client)

	p2 := tea.NewProgram(tui, tea.WithAltScreen(), tea.WithMouseCellMotion())
	_, err = p2.Run()
	if err != nil {
		return fmt.Errorf("tui: %w", err)
	}

	return nil
}
