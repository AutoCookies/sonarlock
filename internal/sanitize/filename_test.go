package sanitize

import (
	"os"
	"path/filepath"
	"testing"
)

func TestFilenameSanitizeWindowsInvalidCharacters(t *testing.T) {
	got := Filename(`a<bad>:"|?*.txt`)
	want := "a_bad______.txt"
	if got != want {
		t.Fatalf("expected %q, got %q", want, got)
	}
}

func TestFilenameReservedDeviceName(t *testing.T) {
	got := Filename("CON.txt")
	if got != "CON.txt_" {
		t.Fatalf("expected reserved name to be modified, got %q", got)
	}
}

func TestResolveCollision(t *testing.T) {
	dir := t.TempDir()
	first := filepath.Join(dir, "file.txt")
	if err := os.WriteFile(first, []byte("existing"), 0o644); err != nil {
		t.Fatalf("seed file: %v", err)
	}

	path, err := ResolveCollision(dir, "file.txt", false)
	if err != nil {
		t.Fatalf("resolve collision: %v", err)
	}
	if filepath.Base(path) != "file (1).txt" {
		t.Fatalf("unexpected collision name: %s", path)
	}
}
