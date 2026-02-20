package errors

import sterrors "errors"

var (
	// ErrUsage indicates invalid CLI usage.
	ErrUsage = sterrors.New("usage error")
)

// ExitCode maps a returned error to a stable process exit code.
func ExitCode(err error) int {
	if err == nil {
		return 0
	}

	if sterrors.Is(err, ErrUsage) {
		return 2
	}

	return 1
}
