package discovery

import (
	"context"
	"fmt"
	"net"
	"sort"
	"strconv"
	"strings"
	"time"
)

// Resolver discovers peers and resolves by ID.
type Resolver interface {
	Browse(ctx context.Context, timeout time.Duration) ([]Peer, error)
	ResolveByID(ctx context.Context, id string, timeout time.Duration) (Peer, error)
}

// MDNSResolver discovers SnapSync peers via LAN multicast.
type MDNSResolver struct{}

// NewMDNSResolver constructs a production resolver.
func NewMDNSResolver() *MDNSResolver {
	return &MDNSResolver{}
}

// Browse discovers peers for up to timeout duration.
func (r *MDNSResolver) Browse(ctx context.Context, timeout time.Duration) ([]Peer, error) {
	if timeout <= 0 {
		timeout = 2 * time.Second
	}
	ctx, cancel := context.WithTimeout(ctx, timeout)
	defer cancel()

	listenAddr, err := net.ResolveUDPAddr("udp4", announceAddr)
	if err != nil {
		return nil, fmt.Errorf("resolve browse listen addr: %w", err)
	}
	conn, err := net.ListenMulticastUDP("udp4", nil, listenAddr)
	if err != nil {
		return []Peer{}, nil
	}
	defer func() { _ = conn.Close() }()
	_ = conn.SetReadBuffer(64 * 1024)

	buf := make([]byte, 2048)
	seen := map[string]Peer{}
	for {
		if err := conn.SetReadDeadline(time.Now().Add(200 * time.Millisecond)); err != nil {
			return nil, fmt.Errorf("set read deadline: %w", err)
		}
		n, src, readErr := conn.ReadFromUDP(buf)
		if readErr == nil {
			peer, ok := decodeAnnouncement(buf[:n], src)
			if ok {
				seen[peer.ID] = peer
			}
		}
		select {
		case <-ctx.Done():
			list := make([]Peer, 0, len(seen))
			for _, peer := range seen {
				list = append(list, peer)
			}
			sort.Slice(list, func(i, j int) bool { return list[i].LastSeen.After(list[j].LastSeen) })
			return list, nil
		default:
		}
	}
}

// ResolveByID resolves one peer by ID within timeout.
func (r *MDNSResolver) ResolveByID(ctx context.Context, id string, timeout time.Duration) (Peer, error) {
	peers, err := r.Browse(ctx, timeout)
	if err != nil {
		return Peer{}, err
	}
	needle := strings.ToLower(strings.TrimSpace(id))
	for _, peer := range peers {
		if strings.EqualFold(peer.ID, needle) {
			return peer, nil
		}
	}
	return Peer{}, fmt.Errorf("peer %q not found", id)
}

// PreferredAddress selects an address to connect to.
func PreferredAddress(peer Peer) (string, bool) {
	if len(peer.Addresses) == 0 {
		return "", false
	}
	var firstIPv6 string
	var firstAny string
	for _, addr := range peer.Addresses {
		ip := net.ParseIP(addr)
		if ip == nil {
			continue
		}
		if firstAny == "" {
			firstAny = addr
		}
		if ip4 := ip.To4(); ip4 != nil {
			if isRFC1918(ip4) {
				return addr, true
			}
			continue
		}
		if firstIPv6 == "" && (ip.IsLinkLocalUnicast() || ip.IsPrivate()) {
			firstIPv6 = addr
		}
	}
	if firstIPv6 != "" {
		return firstIPv6, true
	}
	if firstAny != "" {
		return firstAny, true
	}
	return "", false
}

func isRFC1918(ip net.IP) bool {
	if len(ip) != net.IPv4len {
		return false
	}
	return ip[0] == 10 ||
		(ip[0] == 172 && ip[1] >= 16 && ip[1] <= 31) ||
		(ip[0] == 192 && ip[1] == 168)
}

func encodeAnnouncement(peer Peer) []byte {
	return []byte(fmt.Sprintf("%sver=1;type=%s;domain=%s;id=%s;name=%s;features=direct;port=%d",
		packetPrefix, ServiceType, ServiceDomain, peer.ID, strings.ReplaceAll(peer.Name, ";", ""), peer.Port))
}

func decodeAnnouncement(payload []byte, src *net.UDPAddr) (Peer, bool) {
	text := string(payload)
	if !strings.HasPrefix(text, packetPrefix) {
		return Peer{}, false
	}
	fields := map[string]string{}
	for _, part := range strings.Split(strings.TrimPrefix(text, packetPrefix), ";") {
		kv := strings.SplitN(part, "=", 2)
		if len(kv) != 2 {
			continue
		}
		fields[strings.TrimSpace(kv[0])] = strings.TrimSpace(kv[1])
	}
	if fields["ver"] != "1" || fields["type"] != ServiceType {
		return Peer{}, false
	}
	port, err := strconv.Atoi(fields["port"])
	if err != nil || port <= 0 {
		return Peer{}, false
	}
	id := strings.ToLower(fields["id"])
	if !IsValidPeerID(id) {
		return Peer{}, false
	}
	name := fields["name"]
	if name == "" {
		name = src.IP.String()
	}
	return Peer{ID: id, Name: name, Hostname: src.IP.String(), Addresses: []string{src.IP.String()}, Port: port, LastSeen: time.Now(), Features: fields["features"]}, true
}
