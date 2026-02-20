package cli

import (
	"context"
	"encoding/json"
	"flag"
	"fmt"
	"io"
	"sort"
	"strings"
	"time"

	"snapsync/internal/discovery"
	apperrors "snapsync/internal/errors"
)

// ListCommand returns the list subcommand.
func ListCommand(stdout io.Writer, resolver discovery.Resolver) *Command {
	return &Command{
		Name:        "list",
		Description: "List discovered peers on the local network.",
		Run: func(args []string) error {
			fs := flag.NewFlagSet("list", flag.ContinueOnError)
			fs.SetOutput(io.Discard)
			timeout := fs.Duration("timeout", 2*time.Second, "discovery timeout")
			jsonOut := fs.Bool("json", false, "emit NDJSON peer records")
			if err := fs.Parse(args); err != nil {
				return fmt.Errorf("parse list flags: %w: %w", err, apperrors.ErrUsage)
			}
			if len(fs.Args()) != 0 {
				return fmt.Errorf("list accepts no positional arguments: %w", apperrors.ErrUsage)
			}
			if resolver == nil {
				return fmt.Errorf("discovery resolver unavailable: %w", apperrors.ErrNetwork)
			}

			peers, err := resolver.Browse(context.Background(), *timeout)
			if err != nil {
				return fmt.Errorf("browse peers: %w: %w", err, apperrors.ErrNetwork)
			}
			sort.Slice(peers, func(i, j int) bool {
				return peers[i].LastSeen.After(peers[j].LastSeen)
			})

			if *jsonOut {
				enc := json.NewEncoder(stdout)
				for _, peer := range peers {
					if err := enc.Encode(peer); err != nil {
						return fmt.Errorf("encode peer: %w", err)
					}
				}
				return nil
			}

			now := time.Now()
			_, _ = fmt.Fprintln(stdout, "ID           NAME          ADDRESSES              PORT  AGE")
			for _, peer := range peers {
				_, _ = fmt.Fprintf(stdout, "%-12s %-13s %-22s %-5d %s\n",
					peer.ID,
					truncate(peer.Name, 13),
					truncate(strings.Join(peer.Addresses, ", "), 22),
					peer.Port,
					peer.Age(now).Round(100*time.Millisecond),
				)
			}
			return nil
		},
	}
}

func truncate(value string, max int) string {
	if len(value) <= max {
		return value
	}
	if max <= 2 {
		return value[:max]
	}
	return value[:max-2] + ".."
}
