package sanitize

import (
	"fmt"
	"os"
	"path/filepath"
	"strings"
	"unicode/utf8"
)

var reservedWindowsNames = map[string]struct{}{
	"CON": {}, "PRN": {}, "AUX": {}, "NUL": {},
	"COM1": {}, "COM2": {}, "COM3": {}, "COM4": {}, "COM5": {}, "COM6": {}, "COM7": {}, "COM8": {}, "COM9": {},
	"LPT1": {}, "LPT2": {}, "LPT3": {}, "LPT4": {}, "LPT5": {}, "LPT6": {}, "LPT7": {}, "LPT8": {}, "LPT9": {},
}

// Filename returns a cross-platform safe filename from user-provided input.
func Filename(name string) string {
	base := filepath.Base(strings.TrimSpace(name))
	if base == "." || base == string(filepath.Separator) || base == "" {
		base = "file"
	}

	replacer := strings.NewReplacer(
		"<", "_", ">", "_", ":", "_", "\"", "_",
		"/", "_", "\\", "_", "|", "_", "?", "_", "*", "_",
	)
	base = replacer.Replace(base)

	base = strings.Map(func(r rune) rune {
		if r < 32 {
			return '_'
		}
		if r == utf8.RuneError {
			return '_'
		}
		return r
	}, base)

	base = strings.TrimRight(base, " .")
	if base == "" {
		base = "file"
	}

	prefix := base
	if dot := strings.Index(prefix, "."); dot > 0 {
		prefix = prefix[:dot]
	}
	if _, reserved := reservedWindowsNames[strings.ToUpper(prefix)]; reserved {
		base += "_"
	}

	return base
}

// ResolveCollision returns an output path, adding suffixes if overwrite is disabled.
func ResolveCollision(dir, name string, overwrite bool) (string, error) {
	safe := Filename(name)
	if overwrite {
		return filepath.Join(dir, safe), nil
	}

	ext := filepath.Ext(safe)
	stem := strings.TrimSuffix(safe, ext)
	candidate := filepath.Join(dir, safe)
	if _, err := os.Stat(candidate); err != nil {
		if os.IsNotExist(err) {
			return candidate, nil
		}
		return "", fmt.Errorf("stat candidate %q: %w", candidate, err)
	}

	for i := 1; ; i++ {
		next := filepath.Join(dir, fmt.Sprintf("%s (%d)%s", stem, i, ext))
		if _, err := os.Stat(next); err != nil {
			if os.IsNotExist(err) {
				return next, nil
			}
			return "", fmt.Errorf("stat collision candidate %q: %w", next, err)
		}
	}
}
