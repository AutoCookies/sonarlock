package buildinfo

import (
	"runtime"
	"testing"
)

func TestCurrentUsesDefaultsWhenUnset(t *testing.T) {
	originalVersion := Version
	originalCommit := Commit
	originalDate := Date
	t.Cleanup(func() {
		Version = originalVersion
		Commit = originalCommit
		Date = originalDate
	})

	Version = ""
	Commit = ""
	Date = ""

	info := Current()
	if info.Version != "dev" {
		t.Fatalf("expected dev version, got %q", info.Version)
	}
	if info.Commit != "unknown" {
		t.Fatalf("expected unknown commit, got %q", info.Commit)
	}
	if info.Date != "unknown" {
		t.Fatalf("expected unknown date, got %q", info.Date)
	}
}

func TestFormatVersionStable(t *testing.T) {
	info := Info{
		Version: "1.2.3",
		Commit:  "abc123",
		Date:    "2026-01-01T00:00:00Z",
		Go:      runtime.Version(),
		OS:      runtime.GOOS,
		Arch:    runtime.GOARCH,
	}

	got := FormatVersion(info)
	want := "SnapSync 1.2.3\n" +
		"commit: abc123\n" +
		"built:  2026-01-01T00:00:00Z\n" +
		"go:     " + runtime.Version() + "\n" +
		"os/arch:" + runtime.GOOS + "/" + runtime.GOARCH + "\n"
	if got != want {
		t.Fatalf("unexpected format\nwant:\n%s\ngot:\n%s", want, got)
	}
}
