package cli

import (
	"flag"
	"fmt"
	"io"

	apperrors "snapsync/internal/errors"
	"snapsync/internal/transfer"
)

// SendCommand returns the send subcommand.
func SendCommand(stdout io.Writer) *Command {
	return &Command{
		Name:        "send",
		Description: "Send a file to a receiver over TCP.",
		Run: func(args []string) error {
			fs := flag.NewFlagSet("send", flag.ContinueOnError)
			fs.SetOutput(io.Discard)
			to := fs.String("to", "", "receiver address host:port")
			name := fs.String("name", "", "override destination filename")
			jsonOut := fs.Bool("json", false, "emit progress as NDJSON")

			if err := fs.Parse(args); err != nil {
				return fmt.Errorf("parse send flags: %w: %w", err, apperrors.ErrUsage)
			}
			positionals := fs.Args()
			if len(positionals) != 1 {
				return fmt.Errorf("send requires exactly one <path> argument: %w", apperrors.ErrUsage)
			}

			err := transfer.SendFile(transfer.SendOptions{
				Path: positionals[0],
				To:   *to,
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
