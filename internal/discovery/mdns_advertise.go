package discovery

import (
	"fmt"
	"net"
	"strings"
	"sync"
	"time"
)

const (
	// ServiceType is the DNS-SD service type for SnapSync receivers.
	ServiceType = "_snapsync._tcp"
	// ServiceDomain is the mDNS domain used by SnapSync.
	ServiceDomain = "local."
	announceAddr  = "224.0.0.251:53535"
	packetPrefix  = "SSYNMDNS1|"
)

// Advertiser holds an active discovery advertisement.
type Advertiser struct {
	once sync.Once
	stop chan struct{}
}

// Stop deregisters the active advertisement.
func (a *Advertiser) Stop() {
	if a == nil {
		return
	}
	a.once.Do(func() { close(a.stop) })
}

// Advertise registers a SnapSync receiver announcement on LAN multicast.
func Advertise(instanceName string, port int, peerID string) (*Advertiser, error) {
	name := strings.TrimSpace(instanceName)
	if name == "" {
		name = "SnapSync"
	}

	payload := encodeAnnouncement(Peer{ID: peerID, Name: name, Port: port, LastSeen: time.Now(), Features: "direct"})
	addr, err := net.ResolveUDPAddr("udp4", announceAddr)
	if err != nil {
		return nil, fmt.Errorf("resolve announce addr: %w", err)
	}
	conn, err := net.DialUDP("udp4", nil, addr)
	if err != nil {
		return nil, fmt.Errorf("dial announce multicast: %w", err)
	}

	adv := &Advertiser{stop: make(chan struct{})}
	go func() {
		ticker := time.NewTicker(time.Second)
		defer ticker.Stop()
		defer func() { _ = conn.Close() }()
		for {
			_, _ = conn.Write(payload)
			select {
			case <-ticker.C:
			case <-adv.stop:
				return
			}
		}
	}()
	return adv, nil
}
