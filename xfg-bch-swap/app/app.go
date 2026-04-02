package app

import (
	"fmt"

	tea "github.com/charmbracelet/bubbletea"
)

func Run(cfg Config) error {
	splash := newSplashModel()
	p := tea.NewProgram(splash, tea.WithAltScreen())
	_, err := p.Run()
	if err != nil {
		return fmt.Errorf("splash: %w", err)
	}

	client := NewFuegoClient(cfg.EfierRPC)
	tui := newTuiModel(client)
	p2 := tea.NewProgram(tui, tea.WithAltScreen(), tea.WithMouseCellMotion())
	_, err = p2.Run()
	if err != nil {
		return fmt.Errorf("tui: %w", err)
	}

	return nil
}
