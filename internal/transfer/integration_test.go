package transfer

import (
	"bufio"
	"bytes"
	"crypto/sha256"
	"io"
	"net"
	"os"
	"path/filepath"
	"testing"
)

func TestSendReceiveIntegration(t *testing.T) {
	srcDir := t.TempDir()
	recvDir := t.TempDir()
	srcPath := filepath.Join(srcDir, "sample.bin")
	srcData := bytes.Repeat([]byte("0123456789abcdef"), 1024*320) // ~5MB
	if err := os.WriteFile(srcPath, srcData, 0o644); err != nil {
		t.Fatalf("write source file: %v", err)
	}

	listener, err := net.Listen("tcp", "127.0.0.1:0")
	if err != nil {
		t.Fatalf("listen: %v", err)
	}
	defer func() { _ = listener.Close() }()

	recvErrCh := make(chan error, 1)
	go func() {
		conn, acceptErr := listener.Accept()
		if acceptErr != nil {
			recvErrCh <- acceptErr
			return
		}
		defer func() { _ = conn.Close() }()
		recvErrCh <- receiveFromConn(conn, ReceiveOptions{OutDir: recvDir, AcceptAll: true, Out: io.Discard})
	}()

	sendErr := SendFile(SendOptions{Path: srcPath, To: listener.Addr().String(), Out: io.Discard})
	if sendErr != nil {
		t.Fatalf("send file: %v", sendErr)
	}

	recvErr := <-recvErrCh
	if recvErr != nil {
		t.Fatalf("receive file: %v", recvErr)
	}

	receivedPath := filepath.Join(recvDir, "sample.bin")
	got, err := os.ReadFile(receivedPath)
	if err != nil {
		t.Fatalf("read output: %v", err)
	}
	if len(got) != len(srcData) {
		t.Fatalf("size mismatch: got %d want %d", len(got), len(srcData))
	}
	if sha256.Sum256(got) != sha256.Sum256(srcData) {
		t.Fatal("content mismatch")
	}
}

func TestReceiveRemovesPartialFileOnEarlyClose(t *testing.T) {
	recvDir := t.TempDir()
	listener, err := net.Listen("tcp", "127.0.0.1:0")
	if err != nil {
		t.Fatalf("listen: %v", err)
	}
	defer func() { _ = listener.Close() }()

	recvErrCh := make(chan error, 1)
	go func() {
		conn, acceptErr := listener.Accept()
		if acceptErr != nil {
			recvErrCh <- acceptErr
			return
		}
		defer func() { _ = conn.Close() }()
		recvErrCh <- receiveFromConn(conn, ReceiveOptions{OutDir: recvDir, AcceptAll: true, Out: io.Discard})
	}()

	conn, err := net.Dial("tcp", listener.Addr().String())
	if err != nil {
		t.Fatalf("dial: %v", err)
	}
	writer := bufio.NewWriter(conn)
	reader := bufio.NewReader(conn)

	if err := WriteFrame(writer, Frame{Type: TypeHello}); err != nil {
		t.Fatalf("send hello: %v", err)
	}
	offerPayload, err := EncodeOffer(Offer{Name: "partial.bin", Size: 1024})
	if err != nil {
		t.Fatalf("offer payload: %v", err)
	}
	if err := WriteFrame(writer, Frame{Type: TypeOffer, Payload: offerPayload}); err != nil {
		t.Fatalf("send offer: %v", err)
	}
	if err := writer.Flush(); err != nil {
		t.Fatalf("flush offer: %v", err)
	}

	frame, err := ReadFrame(reader)
	if err != nil {
		t.Fatalf("read accept: %v", err)
	}
	if frame.Type != TypeAccept {
		t.Fatalf("expected ACCEPT, got %d", frame.Type)
	}

	if err := WriteFrame(writer, Frame{Type: TypeData, Payload: bytes.Repeat([]byte{'x'}, 128)}); err != nil {
		t.Fatalf("send data: %v", err)
	}
	if err := writer.Flush(); err != nil {
		t.Fatalf("flush data: %v", err)
	}
	_ = conn.Close()

	recvErr := <-recvErrCh
	if recvErr == nil {
		t.Fatal("expected receiver error on early close")
	}

	entries, err := os.ReadDir(recvDir)
	if err != nil {
		t.Fatalf("list output directory: %v", err)
	}
	if len(entries) != 0 {
		t.Fatalf("expected partial file cleanup, found %d entries", len(entries))
	}

}
