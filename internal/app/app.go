package app

import (
	"fmt"
	"io"
	"os"

	"snapsync/internal/cli"
	apperrors "snapsync/internal/errors"
	"snapsync/internal/logging"
)

// App wires CLI execution and process-level concerns.
type App struct {
	stdout io.Writer
	stderr io.Writer
}

// New constructs the SnapSync app.
func New(stdout, stderr io.Writer) *App {
	return &App{stdout: stdout, stderr: stderr}
}

// Run executes SnapSync with the provided CLI args and returns a process exit code.
func (a *App) Run(args []string) int {
	logger := logging.New("info", a.stderr)
	root := cli.RootCommand(a.stdout, os.Stdin)

	err := cli.Execute(root, args, a.stdout)
	if err == nil {
		return 0
	}

	code := apperrors.ExitCode(err)
	logger.Error("command failed", "error", err, "code", code)
	_, _ = fmt.Fprintf(a.stderr, "error: %v\n", err)

	return code
}

// Run executes SnapSync using process stdio and returns a process exit code.
func Run(args []string) int {
	return New(os.Stdout, os.Stderr).Run(args)
}
