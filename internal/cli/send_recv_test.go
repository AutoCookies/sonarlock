package cli

import (
	"bytes"
	"errors"
	"strings"
	"testing"

	apperrors "snapsync/internal/errors"
)

func TestSendRequiresPathAndTo(t *testing.T) {
	cmd := SendCommand(&bytes.Buffer{})
	err := cmd.Run([]string{"--to", "127.0.0.1:1"})
	if !errors.Is(err, apperrors.ErrUsage) {
		t.Fatalf("expected usage error, got %v", err)
	}
}

func TestRecvRequiresOut(t *testing.T) {
	cmd := RecvCommand(&bytes.Buffer{}, strings.NewReader(""))
	err := cmd.Run([]string{"--listen", ":45999"})
	if !errors.Is(err, apperrors.ErrUsage) {
		t.Fatalf("expected usage error, got %v", err)
	}
}
