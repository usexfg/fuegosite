// swapxfg/app/app.go
package app

import (
	"fmt"
	"log"
	"os"

	tea "github.com/charmbracelet/bubbletea"
)

// Run is the main entry point. Shows splash then launches the trading TUI.
func Run(cfg Config) error {
	// Setup logging
	log.SetOutput(os.Stdout)
	log.SetFlags(log.Ldate | log.Ltime | log.Lshortfile)

	log.Println("Starting swapxfg application")
	log.Printf("Configuration: %+v", cfg)

	// Splash screen
	if !cfg.NoSplash {
		log.Println("Showing splash screen")
		splash := newSplashModel()
		p := tea.NewProgram(splash, tea.WithAltScreen())
		if _, err := p.Run(); err != nil {
			log.Printf("Splash screen error: %v", err)
			return fmt.Errorf("splash: %w", err)
		}
		log.Println("Splash screen completed")
	}

	// Bridge server (MetaMask / Phantom)
	var bridge *BridgeServer
	if !cfg.NoBridge {
		log.Println("Starting bridge server")
		b, err := NewBridgeServer(cfg, cfg.BridgePort)
		if err == nil {
			bridge = b
			log.Printf("Bridge server started on port %d", b.Port())
			defer func() {
				log.Println("Stopping bridge server")
				bridge.Stop()
			}()
		} else {
			log.Printf("Failed to start bridge server: %v", err)
		}
		// non-fatal: TUI works without bridge
	}

	// Main trading TUI
	log.Println("Starting TUI")
	tui := newTuiModel(cfg)
	tui.bridge = bridge
	p := tea.NewProgram(tui, tea.WithAltScreen())
	if _, err := p.Run(); err != nil {
		log.Printf("TUI error: %v", err)
		return fmt.Errorf("tui: %w", err)
	}
	log.Println("TUI completed")

	return nil
}
