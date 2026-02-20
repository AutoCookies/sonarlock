package cli

import (
	"context"
	"flag"
	"fmt"
	"io"
	"net"
	"strings"
	"time"

	"snapsync/internal/discovery"
	apperrors "snapsync/internal/errors"
	"snapsync/internal/transfer"
)

var sendFile = transfer.SendFile

// SendCommand returns the send subcommand.
func SendCommand(stdout io.Writer, resolver discovery.Resolver) *Command {
	return &Command{
		Name:        "send",
		Description: "Send a file to a receiver over TCP.",
		Run: func(args []string) error {
			fs := flag.NewFlagSet("send", flag.ContinueOnError)
			fs.SetOutput(io.Discard)
			to := fs.String("to", "", "receiver host:port or discovered peer id")
			name := fs.String("name", "", "override destination filename")
			jsonOut := fs.Bool("json", false, "emit progress as NDJSON")
			timeout := fs.Duration("timeout", 2*time.Second, "peer resolution timeout")

			if err := fs.Parse(args); err != nil {
				return fmt.Errorf("parse send flags: %w: %w", err, apperrors.ErrUsage)
			}
			positionals := fs.Args()
			if len(positionals) != 1 {
				return fmt.Errorf("send requires exactly one <path> argument: %w", apperrors.ErrUsage)
			}
			if strings.TrimSpace(*to) == "" {
				return fmt.Errorf("--to is required: %w", apperrors.ErrUsage)
			}

			target, err := resolveSendTarget(context.Background(), resolver, *to, *timeout)
			if err != nil {
				return fmt.Errorf("resolve send target: %w", err)
			}

			err = sendFile(transfer.SendOptions{
				Path: positionals[0],
				To:   target,
				Name: *name,
				JSON: *jsonOut,
				Out:  stdout,
			})
			if err != nil {
				return fmt.Errorf("send transfer failed: %w", err)
			}
			return nil
		},
	}
}

func resolveSendTarget(ctx context.Context, resolver discovery.Resolver, to string, timeout time.Duration) (string, error) {
	if _, _, err := net.SplitHostPort(to); err == nil {
		return to, nil
	}
	if resolver == nil {
		return "", fmt.Errorf("peer id %q requires discovery resolver: %w", to, apperrors.ErrNetwork)
	}
	peer, err := resolver.ResolveByID(ctx, to, timeout)
	if err != nil {
		return "", fmt.Errorf("resolve peer %q: %w: %w", to, err, apperrors.ErrNetwork)
	}
	addr, ok := discovery.PreferredAddress(peer)
	if !ok {
		return "", fmt.Errorf("peer %q has no usable address: %w", to, apperrors.ErrNetwork)
	}
	return net.JoinHostPort(addr, fmt.Sprintf("%d", peer.Port)), nil
}
