package buildinfo

import (
	"fmt"
	"runtime"
)

var (
	// Version is the SnapSync version injected at build time via ldflags.
	Version string
	// Commit is the source commit SHA injected at build time via ldflags.
	Commit string
	// Date is the build timestamp injected at build time via ldflags.
	Date string
)

const (
	defaultVersion = "dev"
	defaultUnknown = "unknown"
)

// Info describes build-time and runtime metadata for the executable.
type Info struct {
	Version string
	Commit  string
	Date    string
	Go      string
	OS      string
	Arch    string
}

// Current returns normalized build and runtime metadata for this process.
func Current() Info {
	return Info{
		Version: valueOrDefault(Version, defaultVersion),
		Commit:  valueOrDefault(Commit, defaultUnknown),
		Date:    valueOrDefault(Date, defaultUnknown),
		Go:      runtime.Version(),
		OS:      runtime.GOOS,
		Arch:    runtime.GOARCH,
	}
}

// FormatVersion renders build metadata in the Phase 0 CLI output format.
func FormatVersion(info Info) string {
	return fmt.Sprintf(
		"SnapSync %s\ncommit: %s\nbuilt:  %s\ngo:     %s\nos/arch:%s/%s\n",
		info.Version,
		info.Commit,
		info.Date,
		info.Go,
		info.OS,
		info.Arch,
	)
}

func valueOrDefault(value, fallback string) string {
	if value == "" {
		return fallback
	}

	return value
}
