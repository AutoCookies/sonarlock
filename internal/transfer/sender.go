package transfer

import (
	"bufio"
	"fmt"
	"io"
	"net"
	"os"
	"path/filepath"

	apperrors "snapsync/internal/errors"
	"snapsync/internal/progress"
)

// SendOptions controls sender behavior.
type SendOptions struct {
	Path string
	To   string
	Name string
	JSON bool
	Out  io.Writer
}

// SendFile streams a file to a remote SnapSync receiver.
func SendFile(options SendOptions) error {
	if options.Path == "" || options.To == "" {
		return fmt.Errorf("path and --to are required: %w", apperrors.ErrUsage)
	}
	if options.Out == nil {
		options.Out = io.Discard
	}

	file, stat, err := openSendFile(options.Path)
	if err != nil {
		return err
	}
	defer func() { _ = file.Close() }()

	name := options.Name
	if name == "" {
		name = filepath.Base(stat.Name())
	}

	conn, err := net.Dial("tcp", options.To)
	if err != nil {
		return fmt.Errorf("dial %q: %w: %w", options.To, err, apperrors.ErrNetwork)
	}
	defer func() { _ = conn.Close() }()

	reader := bufio.NewReaderSize(conn, MaxChunkSize)
	writer := bufio.NewWriterSize(conn, MaxChunkSize)
	if err := WriteFrame(writer, Frame{Type: TypeHello}); err != nil {
		return fmt.Errorf("send hello: %w: %w", err, apperrors.ErrNetwork)
	}
	offerPayload, err := EncodeOffer(Offer{Name: name, Size: uint64(stat.Size())})
	if err != nil {
		return fmt.Errorf("encode offer: %w", err)
	}
	if err := WriteFrame(writer, Frame{Type: TypeOffer, Payload: offerPayload}); err != nil {
		return fmt.Errorf("send offer: %w: %w", err, apperrors.ErrNetwork)
	}
	if err := writer.Flush(); err != nil {
		return fmt.Errorf("flush offer: %w: %w", err, apperrors.ErrNetwork)
	}

	response, err := ReadFrame(reader)
	if err != nil {
		return fmt.Errorf("read offer response: %w: %w", err, apperrors.ErrNetwork)
	}
	if response.Type == TypeError {
		message, decodeErr := DecodeError(response.Payload)
		if decodeErr != nil {
			return fmt.Errorf("decode rejection: %w", decodeErr)
		}
		return fmt.Errorf("receiver rejected transfer: %s: %w", message, apperrors.ErrRejected)
	}
	if response.Type != TypeAccept {
		return fmt.Errorf("expected ACCEPT, got %d: %w", response.Type, apperrors.ErrInvalidProtocol)
	}

	tracker := progress.NewTracker(stat.Size())
	printer := progress.NewPrinter(options.Out, "send", options.JSON)
	fw := &dataFrameWriter{writer: writer, tracker: tracker, printer: printer}
	buf := make([]byte, MaxChunkSize)
	if _, err := io.CopyBuffer(fw, file, buf); err != nil {
		return err
	}

	if err := WriteFrame(writer, Frame{Type: TypeDone}); err != nil {
		return fmt.Errorf("send done: %w: %w", err, apperrors.ErrNetwork)
	}
	if err := writer.Flush(); err != nil {
		return fmt.Errorf("flush done: %w: %w", err, apperrors.ErrNetwork)
	}
	if err := printer.PrintFinal(tracker.Add(0), ""); err != nil {
		return fmt.Errorf("print sender summary: %w", err)
	}

	return nil
}

type dataFrameWriter struct {
	writer  *bufio.Writer
	tracker *progress.Tracker
	printer *progress.ProgressPrinter
}

func (d *dataFrameWriter) Write(chunk []byte) (int, error) {
	if len(chunk) == 0 {
		return 0, nil
	}
	if len(chunk) > MaxChunkSize {
		return 0, fmt.Errorf("chunk exceeds max size: %w", apperrors.ErrInvalidProtocol)
	}
	if err := WriteFrame(d.writer, Frame{Type: TypeData, Payload: chunk}); err != nil {
		return 0, fmt.Errorf("write data frame: %w: %w", err, apperrors.ErrNetwork)
	}
	if err := d.writer.Flush(); err != nil {
		return 0, fmt.Errorf("flush data frame: %w: %w", err, apperrors.ErrNetwork)
	}
	snapshot := d.tracker.Add(len(chunk))
	if err := d.printer.PrintMaybe(snapshot); err != nil {
		return 0, fmt.Errorf("print sender progress: %w", err)
	}
	return len(chunk), nil
}

func openSendFile(path string) (*os.File, os.FileInfo, error) {
	file, err := os.Open(path)
	if err != nil {
		return nil, nil, fmt.Errorf("open source file %q: %w: %w", path, err, apperrors.ErrIO)
	}
	stat, err := file.Stat()
	if err != nil {
		_ = file.Close()
		return nil, nil, fmt.Errorf("stat source file %q: %w: %w", path, err, apperrors.ErrIO)
	}
	if !stat.Mode().IsRegular() {
		_ = file.Close()
		return nil, nil, fmt.Errorf("source path %q is not a regular file: %w", path, apperrors.ErrUsage)
	}
	return file, stat, nil
}
