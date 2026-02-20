package transfer

import (
	"bufio"
	"fmt"
	"io"
	"net"
	"os"
	"strings"

	apperrors "snapsync/internal/errors"
	"snapsync/internal/progress"
	"snapsync/internal/sanitize"
)

// ReceiveOptions controls receiver behavior.
type ReceiveOptions struct {
	Listen      string
	OutDir      string
	Overwrite   bool
	AcceptAll   bool
	JSON        bool
	PromptIn    io.Reader
	Out         io.Writer
	OnListening func(addr net.Addr) (func(), error)
}

// ReceiveOnce accepts one connection and receives one file.
func ReceiveOnce(options ReceiveOptions) error {
	if options.Listen == "" || options.OutDir == "" {
		return fmt.Errorf("--listen and --out are required: %w", apperrors.ErrUsage)
	}
	if options.PromptIn == nil {
		options.PromptIn = os.Stdin
	}
	if options.Out == nil {
		options.Out = io.Discard
	}

	if err := os.MkdirAll(options.OutDir, 0o755); err != nil {
		return fmt.Errorf("create output directory %q: %w: %w", options.OutDir, err, apperrors.ErrIO)
	}

	listener, err := net.Listen("tcp", options.Listen)
	if err != nil {
		return fmt.Errorf("listen on %q: %w: %w", options.Listen, err, apperrors.ErrNetwork)
	}
	defer func() { _ = listener.Close() }()

	stopAdvertising := func() {}
	if options.OnListening != nil {
		stop, onListenErr := options.OnListening(listener.Addr())
		if onListenErr != nil {
			return fmt.Errorf("on listening callback: %w", onListenErr)
		}
		if stop != nil {
			stopAdvertising = stop
		}
	}
	defer stopAdvertising()

	_, _ = fmt.Fprintf(options.Out, "listening on %s\n", listener.Addr().String())
	conn, err := listener.Accept()
	if err != nil {
		return fmt.Errorf("accept connection: %w: %w", err, apperrors.ErrNetwork)
	}
	defer func() { _ = conn.Close() }()

	return receiveFromConn(conn, options)
}

func receiveFromConn(conn net.Conn, options ReceiveOptions) (retErr error) {
	reader := bufio.NewReaderSize(conn, MaxChunkSize)
	writer := bufio.NewWriterSize(conn, MaxChunkSize)

	hello, err := ReadFrame(reader)
	if err != nil {
		return fmt.Errorf("read hello: %w", err)
	}
	if hello.Type != TypeHello || len(hello.Payload) != 0 {
		sendProtocolError(writer, "expected HELLO")
		return fmt.Errorf("expected HELLO frame: %w", apperrors.ErrInvalidProtocol)
	}
	offerFrame, err := ReadFrame(reader)
	if err != nil {
		return fmt.Errorf("read offer: %w", err)
	}
	if offerFrame.Type != TypeOffer {
		sendProtocolError(writer, "expected OFFER")
		return fmt.Errorf("expected OFFER frame: %w", apperrors.ErrInvalidProtocol)
	}
	offer, err := DecodeOffer(offerFrame.Payload)
	if err != nil {
		sendProtocolError(writer, "invalid OFFER payload")
		return fmt.Errorf("decode offer: %w", err)
	}

	peer := conn.RemoteAddr().String()
	if !options.AcceptAll {
		accepted, promptErr := promptAccept(options.PromptIn, options.Out, offer.Name, offer.Size, peer)
		if promptErr != nil {
			sendProtocolError(writer, "prompt error")
			return fmt.Errorf("prompt for acceptance: %w", promptErr)
		}
		if !accepted {
			if sendErr := sendErrorFrame(writer, "receiver rejected offer"); sendErr != nil {
				return fmt.Errorf("send rejection: %w", sendErr)
			}
			return fmt.Errorf("transfer rejected by user: %w", apperrors.ErrRejected)
		}
	}

	outPath, err := sanitize.ResolveCollision(options.OutDir, offer.Name, options.Overwrite)
	if err != nil {
		sendProtocolError(writer, "resolve output path failed")
		return fmt.Errorf("resolve output path: %w: %w", err, apperrors.ErrIO)
	}

	file, err := os.Create(outPath)
	if err != nil {
		sendProtocolError(writer, "open output file failed")
		return fmt.Errorf("create output file %q: %w: %w", outPath, err, apperrors.ErrIO)
	}
	defer func() {
		closeErr := file.Close()
		if retErr != nil {
			_ = os.Remove(outPath)
		}
		if retErr == nil && closeErr != nil {
			retErr = fmt.Errorf("close output file %q: %w: %w", outPath, closeErr, apperrors.ErrIO)
		}
	}()

	if err := WriteFrame(writer, Frame{Type: TypeAccept}); err != nil {
		return fmt.Errorf("send accept frame: %w: %w", err, apperrors.ErrNetwork)
	}
	if err := writer.Flush(); err != nil {
		return fmt.Errorf("flush accept frame: %w: %w", err, apperrors.ErrNetwork)
	}

	tracker := progress.NewTracker(int64(offer.Size))
	printer := progress.NewPrinter(options.Out, "recv", options.JSON)

	remaining := int64(offer.Size)
	for remaining > 0 {
		frame, readErr := ReadFrame(reader)
		if readErr != nil {
			return fmt.Errorf("read data frame: %w", readErr)
		}
		switch frame.Type {
		case TypeData:
			if int64(len(frame.Payload)) > remaining {
				sendProtocolError(writer, "received more bytes than offered")
				return fmt.Errorf("received %d bytes with %d remaining: %w", len(frame.Payload), remaining, apperrors.ErrInvalidProtocol)
			}
			if _, writeErr := file.Write(frame.Payload); writeErr != nil {
				sendProtocolError(writer, "write output failed")
				return fmt.Errorf("write output data: %w: %w", writeErr, apperrors.ErrIO)
			}
			remaining -= int64(len(frame.Payload))
			snapshot := tracker.Add(len(frame.Payload))
			if printErr := printer.PrintMaybe(snapshot); printErr != nil {
				return fmt.Errorf("print receiver progress: %w", printErr)
			}
		case TypeError:
			message, decodeErr := DecodeError(frame.Payload)
			if decodeErr != nil {
				return fmt.Errorf("decode sender error frame: %w", decodeErr)
			}
			return fmt.Errorf("sender reported error: %s: %w", message, apperrors.ErrNetwork)
		case TypeDone:
			sendProtocolError(writer, "sender finished before declared file size")
			return fmt.Errorf("sender ended early with %d bytes remaining: %w", remaining, apperrors.ErrInvalidProtocol)
		default:
			sendProtocolError(writer, "expected DATA")
			return fmt.Errorf("unexpected frame type %d during data stream: %w", frame.Type, apperrors.ErrInvalidProtocol)
		}
	}

	doneFrame, err := ReadFrame(reader)
	if err != nil {
		return fmt.Errorf("read done frame: %w", err)
	}
	if doneFrame.Type != TypeDone {
		sendProtocolError(writer, "expected DONE")
		return fmt.Errorf("expected DONE frame, got %d: %w", doneFrame.Type, apperrors.ErrInvalidProtocol)
	}

	if err := printer.PrintFinal(tracker.Add(0), outPath); err != nil {
		return fmt.Errorf("print receiver summary: %w", err)
	}
	return nil
}

func sendProtocolError(writer *bufio.Writer, message string) {
	_ = sendErrorFrame(writer, message)
}

func sendErrorFrame(writer *bufio.Writer, message string) error {
	payload, err := EncodeError(message)
	if err != nil {
		return fmt.Errorf("encode protocol error message: %w", err)
	}
	if err := WriteFrame(writer, Frame{Type: TypeError, Payload: payload}); err != nil {
		return fmt.Errorf("write error frame: %w", err)
	}
	if err := writer.Flush(); err != nil {
		return fmt.Errorf("flush error frame: %w", err)
	}
	return nil
}

func promptAccept(in io.Reader, out io.Writer, name string, size uint64, peer string) (bool, error) {
	_, _ = fmt.Fprintf(out, "Accept file %s (%d bytes) from %s? [y/N] ", name, size, peer)
	line, err := bufio.NewReader(in).ReadString('\n')
	if err != nil && err != io.EOF {
		return false, fmt.Errorf("read acceptance input: %w", err)
	}
	response := strings.TrimSpace(strings.ToLower(line))
	return response == "y" || response == "yes", nil
}
