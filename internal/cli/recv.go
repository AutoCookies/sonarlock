package cli

import (
	"flag"
	"fmt"
	"io"
	"os"

	apperrors "snapsync/internal/errors"
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
			})
			if err != nil {
				return fmt.Errorf("receive transfer failed: %w", err)
			}
			return nil
		},
	}
}
