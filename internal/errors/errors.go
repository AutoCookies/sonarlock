package errors

import sterrors "errors"

var (
	// ErrUsage indicates invalid CLI usage.
	ErrUsage = sterrors.New("usage error")
	// ErrInvalidProtocol indicates malformed or incompatible transfer protocol data.
	ErrInvalidProtocol = sterrors.New("invalid protocol")
	// ErrRejected indicates an explicit transfer rejection by the remote peer or local user.
	ErrRejected = sterrors.New("transfer rejected")
	// ErrIO indicates local filesystem input/output failure.
	ErrIO = sterrors.New("i/o error")
	// ErrNetwork indicates transport-level network failures.
	ErrNetwork = sterrors.New("network error")
)

// ExitCode maps a returned error to a stable process exit code.
func ExitCode(err error) int {
	if err == nil {
		return 0
	}

	switch {
	case sterrors.Is(err, ErrUsage):
		return 2
	case sterrors.Is(err, ErrRejected):
		return 3
	case sterrors.Is(err, ErrInvalidProtocol):
		return 4
	case sterrors.Is(err, ErrNetwork):
		return 5
	case sterrors.Is(err, ErrIO):
		return 6
	default:
		return 1
	}
}
