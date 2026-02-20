package discovery

import (
	"crypto/sha256"
	"encoding/hex"
	"fmt"
	"net"
	"regexp"
	"sort"
	"strings"
	"time"

	"snapsync/internal/store"
)

var peerIDPattern = regexp.MustCompile(`^[a-f0-9]{10,16}$`)

// Peer describes a discovered SnapSync receiver.
type Peer struct {
	ID        string
	Name      string
	Hostname  string
	Addresses []string
	Port      int
	LastSeen  time.Time
	Features  string
}

// Age returns how long ago the peer was seen.
func (p Peer) Age(now time.Time) time.Duration {
	return now.Sub(p.LastSeen)
}

// IsValidPeerID returns true when an ID matches the expected short hex format.
func IsValidPeerID(id string) bool {
	return peerIDPattern.MatchString(strings.ToLower(strings.TrimSpace(id)))
}

// DeterministicPeerID derives a stable short ID from hostname and MAC.
func DeterministicPeerID(hostname string, mac net.HardwareAddr) (string, bool) {
	hostname = strings.TrimSpace(strings.ToLower(hostname))
	if hostname == "" || len(mac) == 0 {
		return "", false
	}
	sum := sha256.Sum256([]byte(hostname + "|" + strings.ToLower(mac.String())))
	return hex.EncodeToString(sum[:])[:12], true
}

// ResolveLocalPeerID derives a deterministic peer ID or falls back to persisted random ID.
func ResolveLocalPeerID(hostname string, mac net.HardwareAddr, persistedPath string) (string, error) {
	if id, ok := DeterministicPeerID(hostname, mac); ok {
		return id, nil
	}

	stored, err := store.ReadID(persistedPath)
	if err == nil && IsValidPeerID(stored) {
		return stored, nil
	}

	random, randomErr := store.RandomID()
	if randomErr != nil {
		return "", fmt.Errorf("generate fallback peer id: %w", randomErr)
	}
	if writeErr := store.WriteID(persistedPath, random); writeErr != nil {
		return "", fmt.Errorf("persist fallback peer id: %w", writeErr)
	}
	return random, nil
}

// BuildTXT returns DNS-SD TXT values for SnapSync discovery.
func BuildTXT(id, name string) []string {
	return []string{"ver=1", "id=" + id, "name=" + name, "features=direct"}
}

// ParseTXT extracts discovery fields from TXT records.
func ParseTXT(txt []string) (id, name, features string, ok bool) {
	fields := make(map[string]string, len(txt))
	for _, item := range txt {
		parts := strings.SplitN(item, "=", 2)
		if len(parts) != 2 {
			continue
		}
		fields[strings.TrimSpace(strings.ToLower(parts[0]))] = strings.TrimSpace(parts[1])
	}
	if fields["ver"] != "1" {
		return "", "", "", false
	}
	id = strings.ToLower(fields["id"])
	if !IsValidPeerID(id) {
		return "", "", "", false
	}
	name = fields["name"]
	features = fields["features"]
	return id, name, features, true
}

// BuildPeer builds a Peer from discovery metadata.
func BuildPeer(instance, host string, addrs []net.IP, port int, txt []string, seen time.Time) (Peer, bool) {
	if port <= 0 {
		return Peer{}, false
	}
	id, name, features, ok := ParseTXT(txt)
	if !ok {
		return Peer{}, false
	}
	if name == "" {
		name = strings.TrimSuffix(instance, ".")
	}
	addresses := make([]string, 0, len(addrs))
	for _, ip := range addrs {
		addresses = append(addresses, ip.String())
	}
	sort.Strings(addresses)
	return Peer{ID: id, Name: name, Hostname: host, Addresses: addresses, Port: port, LastSeen: seen, Features: features}, true
}
