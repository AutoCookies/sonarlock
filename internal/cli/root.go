package cli

import (
	"fmt"
	"io"
	"strings"

	apperrors "snapsync/internal/errors"
)

// Runner executes a command with argv-style arguments.
type Runner func(args []string) error

// Command models a small CLI command tree for Phase 0.
type Command struct {
	Name        string
	Description string
	Run         Runner
	Subcommands []*Command
}

// RootCommand returns the SnapSync root command with its subcommands.
func RootCommand(versionOutput io.Writer) *Command {
	version := VersionCommand(versionOutput)

	return &Command{
		Name:        "snapsync",
		Description: "SnapSync is a LAN file transfer tool (Phase 0 foundation).",
		Subcommands: []*Command{version},
	}
}

// Execute parses arguments and runs a root command.
func Execute(root *Command, args []string, stdout io.Writer) error {
	if root == nil {
		return fmt.Errorf("root command is required: %w", apperrors.ErrUsage)
	}

	if len(args) == 0 || isHelpArg(args[0]) {
		printRootHelp(root, stdout)
		return nil
	}

	name := args[0]
	for _, sub := range root.Subcommands {
		if sub.Name != name {
			continue
		}

		if sub.Run == nil {
			return fmt.Errorf("command %q is not executable: %w", name, apperrors.ErrUsage)
		}

		if len(args) > 1 && isHelpArg(args[1]) {
			printSubcommandHelp(root, sub, stdout)
			return nil
		}

		if err := sub.Run(args[1:]); err != nil {
			return fmt.Errorf("run command %q: %w", sub.Name, err)
		}

		return nil
	}

	return fmt.Errorf("unknown command %q: %w", name, apperrors.ErrUsage)
}

func printRootHelp(root *Command, stdout io.Writer) {
	_, _ = fmt.Fprintf(stdout, "Usage:\n  %s [command]\n\n", root.Name)
	_, _ = fmt.Fprintln(stdout, root.Description)
	_, _ = fmt.Fprintln(stdout)
	_, _ = fmt.Fprintln(stdout, "Available Commands:")
	for _, sub := range root.Subcommands {
		_, _ = fmt.Fprintf(stdout, "  %s\t%s\n", sub.Name, sub.Description)
	}
	_, _ = fmt.Fprintln(stdout)
	_, _ = fmt.Fprintf(stdout, "Use \"%s [command] --help\" for more information about a command.\n", root.Name)
}

func printSubcommandHelp(root, sub *Command, stdout io.Writer) {
	_, _ = fmt.Fprintf(stdout, "Usage:\n  %s %s\n\n", root.Name, sub.Name)
	_, _ = fmt.Fprintln(stdout, sub.Description)
}

func isHelpArg(arg string) bool {
	normalized := strings.TrimSpace(arg)
	return normalized == "-h" || normalized == "--help" || normalized == "help"
}
