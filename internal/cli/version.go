package cli

import (
	"fmt"
	"io"

	"snapsync/internal/buildinfo"
	apperrors "snapsync/internal/errors"
)

// VersionCommand returns the version subcommand.
func VersionCommand(stdout io.Writer) *Command {
	return &Command{
		Name:        "version",
		Description: "Print build and runtime version information.",
		Run: func(args []string) error {
			if len(args) > 0 {
				return fmt.Errorf("version takes no arguments: %w", apperrors.ErrUsage)
			}

			_, err := io.WriteString(stdout, buildinfo.FormatVersion(buildinfo.Current()))
			if err != nil {
				return fmt.Errorf("write version output: %w", err)
			}

			return nil
		},
	}
}
