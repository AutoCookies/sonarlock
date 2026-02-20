package store

import (
	"os"
	"path/filepath"
	"testing"
)

func TestWriteAndReadID(t *testing.T) {
	path := filepath.Join(t.TempDir(), "peer_id")
	if err := WriteID(path, "abc123def456"); err != nil {
		t.Fatalf("write id: %v", err)
	}
	got, err := ReadID(path)
	if err != nil {
		t.Fatalf("read id: %v", err)
	}
	if got != "abc123def456" {
		t.Fatalf("unexpected id: %q", got)
	}
	if info, err := os.Stat(path); err != nil || info.Size() == 0 {
		t.Fatalf("expected persisted file, err=%v", err)
	}
}
