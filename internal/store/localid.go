package store

import (
	"crypto/rand"
	"encoding/hex"
	"fmt"
	"os"
	"path/filepath"
	"runtime"
	"strings"
)

// LocalIDPath returns the OS-appropriate path for persisted SnapSync peer IDs.
func LocalIDPath() (string, error) {
	configDir, err := os.UserConfigDir()
	if err != nil {
		return "", fmt.Errorf("resolve user config directory: %w", err)
	}
	dirName := "snapsync"
	if runtime.GOOS == "windows" {
		dirName = "SnapSync"
	}
	return filepath.Join(configDir, dirName, "peer_id"), nil
}

// ReadID loads a persisted peer ID from disk.
func ReadID(path string) (string, error) {
	data, err := os.ReadFile(path)
	if err != nil {
		return "", err
	}
	return strings.TrimSpace(string(data)), nil
}

// WriteID persists a peer ID to disk, creating parent directories as needed.
func WriteID(path, id string) error {
	if err := os.MkdirAll(filepath.Dir(path), 0o755); err != nil {
		return fmt.Errorf("create peer id directory: %w", err)
	}
	if err := os.WriteFile(path, []byte(id+"\n"), 0o600); err != nil {
		return fmt.Errorf("write peer id file: %w", err)
	}
	return nil
}

// RandomID returns a short random peer ID fallback.
func RandomID() (string, error) {
	buf := make([]byte, 12)
	if _, err := rand.Read(buf); err != nil {
		return "", fmt.Errorf("generate random id: %w", err)
	}
	return hex.EncodeToString(buf)[:12], nil
}
