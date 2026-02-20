package cli

import (
	"flag"
	"fmt"
	"io"
	"net"
	"os"
	"strings"

	"snapsync/internal/discovery"
	apperrors "snapsync/internal/errors"
	"snapsync/internal/store"
	"snapsync/internal/transfer"
)

// RecvCommand returns the recv subcommand.
func RecvCommand(stdout io.Writer, stdin io.Reader) *Command {
	return &Command{
		Name:        "recv",
		Description: "Receive one file over TCP.",
		Run: func(args []string) error {
			fs := flag.NewFlagSet("recv", flag.ContinueOnError)
			fs.SetOutput(io.Discard)
			listen := fs.String("listen", ":45999", "listen address")
			outDir := fs.String("out", "", "output directory")
			overwrite := fs.Bool("overwrite", false, "overwrite existing file")
			autoAccept := fs.Bool("accept", false, "auto-accept incoming offers")
			jsonOut := fs.Bool("json", false, "emit progress as NDJSON")
			name := fs.String("name", "", "advertised receiver name")
			noDiscovery := fs.Bool("no-discovery", false, "disable mDNS advertisement")

			if err := fs.Parse(args); err != nil {
				return fmt.Errorf("parse recv flags: %w: %w", err, apperrors.ErrUsage)
			}
			if len(fs.Args()) != 0 {
				return fmt.Errorf("recv does not accept positional arguments: %w", apperrors.ErrUsage)
			}
			if *outDir == "" {
				return fmt.Errorf("--out is required: %w", apperrors.ErrUsage)
			}
			if stdin == nil {
				stdin = os.Stdin
			}

			err := transfer.ReceiveOnce(transfer.ReceiveOptions{
				Listen:    *listen,
				OutDir:    *outDir,
				Overwrite: *overwrite,
				AcceptAll: *autoAccept,
				JSON:      *jsonOut,
				PromptIn:  stdin,
				Out:       stdout,
				OnListening: func(addr net.Addr) (func(), error) {
					if *noDiscovery {
						return func() {}, nil
					}
					tcpAddr, ok := addr.(*net.TCPAddr)
					if !ok {
						return nil, fmt.Errorf("unexpected listener address type %T", addr)
					}
					localIDPath, pathErr := store.LocalIDPath()
					if pathErr != nil {
						return nil, fmt.Errorf("resolve local id path: %w", pathErr)
					}
					hostname, _ := os.Hostname()
					mac, _ := primaryMAC()
					peerID, idErr := discovery.ResolveLocalPeerID(hostname, mac, localIDPath)
					if idErr != nil {
						return nil, fmt.Errorf("resolve local peer id: %w", idErr)
					}
					instance := strings.TrimSpace(*name)
					if instance == "" {
						instance = hostname
					}
					advertiser, adErr := discovery.Advertise(instance, tcpAddr.Port, peerID)
					if adErr != nil {
						return nil, fmt.Errorf("start mdns advertisement: %w", adErr)
					}
					_, _ = fmt.Fprintf(stdout, "advertising as %s (%s)\n", instance, peerID)
					return advertiser.Stop, nil
				},
			})
			if err != nil {
				return fmt.Errorf("receive transfer failed: %w", err)
			}
			return nil
		},
	}
}

func primaryMAC() (net.HardwareAddr, error) {
	ifaces, err := net.Interfaces()
	if err != nil {
		return nil, fmt.Errorf("list network interfaces: %w", err)
	}
	for _, iface := range ifaces {
		if iface.Flags&net.FlagUp == 0 || iface.Flags&net.FlagLoopback != 0 || len(iface.HardwareAddr) == 0 {
			continue
		}
		return iface.HardwareAddr, nil
	}
	return nil, fmt.Errorf("no primary mac available")
}
