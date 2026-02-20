package logging

import (
	"io"
	"log/slog"
	"strings"
)

// New returns a deterministic text logger configured for the provided level.
func New(level string, out io.Writer) *slog.Logger {
	opts := &slog.HandlerOptions{Level: parseLevel(level)}
	handler := slog.NewTextHandler(out, opts)
	return slog.New(handler)
}

func parseLevel(level string) slog.Level {
	switch strings.ToLower(level) {
	case "debug":
		return slog.LevelDebug
	case "warn":
		return slog.LevelWarn
	case "error":
		return slog.LevelError
	default:
		return slog.LevelInfo
	}
}
