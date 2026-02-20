package discovery

import (
	"net"
	"path/filepath"
	"testing"
	"time"
)

func TestDeterministicPeerID(t *testing.T) {
	mac, _ := net.ParseMAC("aa:bb:cc:dd:ee:ff")
	id1, ok1 := DeterministicPeerID("HostA", mac)
	id2, ok2 := DeterministicPeerID("HostA", mac)
	if !ok1 || !ok2 || id1 != id2 {
		t.Fatalf("expected deterministic id, got %q and %q", id1, id2)
	}
	if !IsValidPeerID(id1) {
		t.Fatalf("invalid id format: %q", id1)
	}
}

func TestResolveLocalPeerIDFallbackPersistence(t *testing.T) {
	path := filepath.Join(t.TempDir(), "peer_id")
	id, err := ResolveLocalPeerID("", nil, path)
	if err != nil {
		t.Fatalf("resolve id first time: %v", err)
	}
	id2, err := ResolveLocalPeerID("", nil, path)
	if err != nil {
		t.Fatalf("resolve id second time: %v", err)
	}
	if id != id2 {
		t.Fatalf("expected persisted id reuse, got %q and %q", id, id2)
	}
}

func TestParseTXTAndBuildPeer(t *testing.T) {
	now := time.Now()
	peer, ok := BuildPeer("LivingRoom", "host.local.", []net.IP{net.ParseIP("192.168.1.10")}, 45999,
		[]string{"ver=1", "id=abc123def456", "name=LivingRoomPC", "features=direct"}, now)
	if !ok {
		t.Fatal("expected valid peer")
	}
	if peer.ID != "abc123def456" || peer.Port != 45999 {
		t.Fatalf("unexpected peer: %#v", peer)
	}

	_, ok = BuildPeer("bad", "host", []net.IP{net.ParseIP("192.168.1.10")}, 45999, []string{"ver=2", "id=abc"}, now)
	if ok {
		t.Fatal("expected invalid version to be rejected")
	}
	_, ok = BuildPeer("bad", "host", nil, 0, []string{"ver=1", "id=abc123def456"}, now)
	if ok {
		t.Fatal("expected invalid port to be rejected")
	}
}

func TestPreferredAddressOrdering(t *testing.T) {
	peer := Peer{Addresses: []string{"fe80::1", "8.8.8.8", "192.168.1.5"}}
	addr, ok := PreferredAddress(peer)
	if !ok || addr != "192.168.1.5" {
		t.Fatalf("expected private IPv4 preference, got %q", addr)
	}
}
