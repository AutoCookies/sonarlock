package transfer

import (
	"encoding/binary"
	"fmt"
	"io"

	apperrors "snapsync/internal/errors"
)

const (
	// ProtocolVersion identifies the current SnapSync wire protocol.
	ProtocolVersion uint16 = 1
	// HeaderLength is the fixed frame header size in bytes.
	HeaderLength = 16
	// MaxChunkSize is the maximum DATA payload size.
	MaxChunkSize = 1024 * 1024
	// MaxPayloadSize is the largest payload accepted by decoder.
	MaxPayloadSize = MaxChunkSize + 1024
)

var protocolMagic = [4]byte{'S', 'S', 'Y', 'N'}

const (
	// TypeHello starts a transfer session.
	TypeHello uint16 = 1
	// TypeOffer offers a file name and size.
	TypeOffer uint16 = 2
	// TypeAccept accepts a received offer.
	TypeAccept uint16 = 3
	// TypeData carries raw file bytes.
	TypeData uint16 = 4
	// TypeDone indicates transfer completion.
	TypeDone uint16 = 5
	// TypeError carries a human-readable rejection or failure reason.
	TypeError uint16 = 6
)

// Frame is a binary protocol frame.
type Frame struct {
	Type    uint16
	Payload []byte
}

// Offer describes a file transfer offer payload.
type Offer struct {
	Name string
	Size uint64
}

// WriteFrame serializes and writes a frame.
func WriteFrame(w io.Writer, frame Frame) error {
	if len(frame.Payload) > MaxPayloadSize {
		return fmt.Errorf("payload too large (%d): %w", len(frame.Payload), apperrors.ErrInvalidProtocol)
	}

	header := make([]byte, HeaderLength)
	copy(header[:4], protocolMagic[:])
	binary.BigEndian.PutUint16(header[4:6], ProtocolVersion)
	binary.BigEndian.PutUint16(header[6:8], frame.Type)
	binary.BigEndian.PutUint32(header[8:12], uint32(len(frame.Payload)))

	if _, err := w.Write(header); err != nil {
		return fmt.Errorf("write frame header: %w", err)
	}
	if len(frame.Payload) == 0 {
		return nil
	}
	if _, err := w.Write(frame.Payload); err != nil {
		return fmt.Errorf("write frame payload: %w", err)
	}
	return nil
}

// ReadFrame decodes one frame from r.
func ReadFrame(r io.Reader) (Frame, error) {
	header := make([]byte, HeaderLength)
	if _, err := io.ReadFull(r, header); err != nil {
		return Frame{}, fmt.Errorf("read frame header: %w", err)
	}
	if string(header[:4]) != string(protocolMagic[:]) {
		return Frame{}, fmt.Errorf("invalid protocol magic: %w", apperrors.ErrInvalidProtocol)
	}
	if got := binary.BigEndian.Uint16(header[4:6]); got != ProtocolVersion {
		return Frame{}, fmt.Errorf("unsupported protocol version %d: %w", got, apperrors.ErrInvalidProtocol)
	}
	if reserved := binary.BigEndian.Uint32(header[12:16]); reserved != 0 {
		return Frame{}, fmt.Errorf("reserved field must be zero: %w", apperrors.ErrInvalidProtocol)
	}

	length := binary.BigEndian.Uint32(header[8:12])
	if length > MaxPayloadSize {
		return Frame{}, fmt.Errorf("payload length %d exceeds limit: %w", length, apperrors.ErrInvalidProtocol)
	}

	payload := make([]byte, int(length))
	if length > 0 {
		if _, err := io.ReadFull(r, payload); err != nil {
			return Frame{}, fmt.Errorf("read frame payload: %w", err)
		}
	}

	return Frame{Type: binary.BigEndian.Uint16(header[6:8]), Payload: payload}, nil
}

// EncodeOffer serializes an offer payload.
func EncodeOffer(offer Offer) ([]byte, error) {
	nameBytes := []byte(offer.Name)
	if len(nameBytes) == 0 || len(nameBytes) > 65535 {
		return nil, fmt.Errorf("invalid offer name length %d: %w", len(nameBytes), apperrors.ErrInvalidProtocol)
	}
	payload := make([]byte, 2+len(nameBytes)+8)
	binary.BigEndian.PutUint16(payload[:2], uint16(len(nameBytes)))
	copy(payload[2:2+len(nameBytes)], nameBytes)
	binary.BigEndian.PutUint64(payload[2+len(nameBytes):], offer.Size)
	return payload, nil
}

// DecodeOffer parses an offer payload.
func DecodeOffer(payload []byte) (Offer, error) {
	if len(payload) < 10 {
		return Offer{}, fmt.Errorf("offer payload too short: %w", apperrors.ErrInvalidProtocol)
	}
	nameLen := int(binary.BigEndian.Uint16(payload[:2]))
	if len(payload) != 2+nameLen+8 {
		return Offer{}, fmt.Errorf("offer payload size mismatch: %w", apperrors.ErrInvalidProtocol)
	}
	if nameLen == 0 {
		return Offer{}, fmt.Errorf("offer filename is empty: %w", apperrors.ErrInvalidProtocol)
	}
	name := string(payload[2 : 2+nameLen])
	size := binary.BigEndian.Uint64(payload[2+nameLen:])
	return Offer{Name: name, Size: size}, nil
}

// EncodeError serializes an ERROR payload message.
func EncodeError(message string) ([]byte, error) {
	msgBytes := []byte(message)
	if len(msgBytes) == 0 || len(msgBytes) > 65535 {
		return nil, fmt.Errorf("invalid error length %d: %w", len(msgBytes), apperrors.ErrInvalidProtocol)
	}
	payload := make([]byte, 2+len(msgBytes))
	binary.BigEndian.PutUint16(payload[:2], uint16(len(msgBytes)))
	copy(payload[2:], msgBytes)
	return payload, nil
}

// DecodeError parses an ERROR payload message.
func DecodeError(payload []byte) (string, error) {
	if len(payload) < 2 {
		return "", fmt.Errorf("error payload too short: %w", apperrors.ErrInvalidProtocol)
	}
	msgLen := int(binary.BigEndian.Uint16(payload[:2]))
	if len(payload) != 2+msgLen {
		return "", fmt.Errorf("error payload size mismatch: %w", apperrors.ErrInvalidProtocol)
	}
	if msgLen == 0 {
		return "", fmt.Errorf("error payload empty: %w", apperrors.ErrInvalidProtocol)
	}
	return string(payload[2:]), nil
}
