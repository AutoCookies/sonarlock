package transfer

import (
	"bytes"
	"encoding/binary"
	"errors"
	"testing"

	apperrors "snapsync/internal/errors"
)

func TestFrameRoundTrip(t *testing.T) {
	original := Frame{Type: TypeData, Payload: []byte("hello")}
	var buf bytes.Buffer
	if err := WriteFrame(&buf, original); err != nil {
		t.Fatalf("write frame: %v", err)
	}
	got, err := ReadFrame(&buf)
	if err != nil {
		t.Fatalf("read frame: %v", err)
	}
	if got.Type != original.Type || string(got.Payload) != string(original.Payload) {
		t.Fatalf("unexpected roundtrip frame: %#v", got)
	}
}

func TestReadFrameRejectsInvalidMagicOrVersion(t *testing.T) {
	header := make([]byte, HeaderLength)
	copy(header[:4], []byte("NOPE"))
	binary.BigEndian.PutUint16(header[4:6], ProtocolVersion)
	binary.BigEndian.PutUint16(header[6:8], TypeHello)

	_, err := ReadFrame(bytes.NewReader(header))
	if !errors.Is(err, apperrors.ErrInvalidProtocol) {
		t.Fatalf("expected invalid protocol, got %v", err)
	}

	copy(header[:4], []byte("SSYN"))
	binary.BigEndian.PutUint16(header[4:6], ProtocolVersion+1)
	_, err = ReadFrame(bytes.NewReader(header))
	if !errors.Is(err, apperrors.ErrInvalidProtocol) {
		t.Fatalf("expected invalid protocol on version mismatch, got %v", err)
	}
}

func TestReadFrameRejectsExcessiveLength(t *testing.T) {
	header := make([]byte, HeaderLength)
	copy(header[:4], []byte("SSYN"))
	binary.BigEndian.PutUint16(header[4:6], ProtocolVersion)
	binary.BigEndian.PutUint16(header[6:8], TypeData)
	binary.BigEndian.PutUint32(header[8:12], MaxPayloadSize+1)

	_, err := ReadFrame(bytes.NewReader(header))
	if !errors.Is(err, apperrors.ErrInvalidProtocol) {
		t.Fatalf("expected invalid protocol for long payload, got %v", err)
	}
}

func TestOfferAndErrorPayloadRoundTrip(t *testing.T) {
	offerPayload, err := EncodeOffer(Offer{Name: "file.bin", Size: 42})
	if err != nil {
		t.Fatalf("encode offer: %v", err)
	}
	offer, err := DecodeOffer(offerPayload)
	if err != nil {
		t.Fatalf("decode offer: %v", err)
	}
	if offer.Name != "file.bin" || offer.Size != 42 {
		t.Fatalf("unexpected offer: %#v", offer)
	}

	errPayload, err := EncodeError("nope")
	if err != nil {
		t.Fatalf("encode error: %v", err)
	}
	message, err := DecodeError(errPayload)
	if err != nil {
		t.Fatalf("decode error: %v", err)
	}
	if message != "nope" {
		t.Fatalf("unexpected message: %q", message)
	}
}
