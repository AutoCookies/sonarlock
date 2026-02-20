package cli

import (
	"bytes"
	"context"
	"errors"
	"io"
	"strings"
	"testing"
	"time"

	"snapsync/internal/discovery"
	apperrors "snapsync/internal/errors"
	"snapsync/internal/transfer"
)

type fakeResolver struct {
	peers []discovery.Peer
}

func (f fakeResolver) Browse(_ context.Context, _ time.Duration) ([]discovery.Peer, error) {
	return f.peers, nil
}

func (f fakeResolver) ResolveByID(_ context.Context, id string, _ time.Duration) (discovery.Peer, error) {
	for _, p := range f.peers {
		if strings.EqualFold(p.ID, id) {
			return p, nil
		}
	}
	return discovery.Peer{}, errors.New("not found")
}

func TestSendRequiresPathAndTo(t *testing.T) {
	cmd := SendCommand(&bytes.Buffer{}, fakeResolver{})
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

func TestListPrintsPeers(t *testing.T) {
	buf := &bytes.Buffer{}
	cmd := ListCommand(buf, fakeResolver{peers: []discovery.Peer{{ID: "abc123def456", Name: "Laptop", Addresses: []string{"192.168.1.9"}, Port: 45999, LastSeen: time.Now()}}})
	if err := cmd.Run(nil); err != nil {
		t.Fatalf("list run: %v", err)
	}
	out := buf.String()
	if !strings.Contains(out, "abc123def456") || !strings.Contains(out, "Laptop") {
		t.Fatalf("unexpected list output: %q", out)
	}
}

func TestSendResolvesPeerIDAndBypassesDirectAddress(t *testing.T) {
	old := sendFile
	defer func() { sendFile = old }()

	var got []string
	sendFile = func(options transfer.SendOptions) error {
		got = append(got, options.To)
		return nil
	}

	resolver := fakeResolver{peers: []discovery.Peer{{ID: "peer00000001", Addresses: []string{"fe80::1", "192.168.1.55"}, Port: 45999}}}
	cmd := SendCommand(io.Discard, resolver)

	if err := cmd.Run([]string{"--to", "peer00000001", "/tmp/file"}); err != nil {
		t.Fatalf("send by peer id: %v", err)
	}
	if err := cmd.Run([]string{"--to", "127.0.0.1:45999", "/tmp/file"}); err != nil {
		t.Fatalf("send by host:port: %v", err)
	}
	if len(got) != 2 {
		t.Fatalf("expected 2 send calls, got %d", len(got))
	}
	if got[0] != "192.168.1.55:45999" {
		t.Fatalf("unexpected resolved target %q", got[0])
	}
	if got[1] != "127.0.0.1:45999" {
		t.Fatalf("expected direct bypass, got %q", got[1])
	}
}
