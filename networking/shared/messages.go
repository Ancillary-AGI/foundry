package shared

import (
	"bytes"
	"encoding/binary"
	"errors"
	"io"
)

// MessageType defines the type of network messages
type MessageType uint8

const (
	// Core player messages
	MsgPlayerJoin  MessageType = 1
	MsgPlayerLeave MessageType = 2
	MsgPlayerMove  MessageType = 3
	MsgPlayerInput MessageType = 4

	// Game state messages
	MsgGameState   MessageType = 5
	MsgGameStart   MessageType = 6
	MsgGameEnd     MessageType = 7
	MsgSnapshot    MessageType = 8

	// Room management messages
	MsgCreateRoom      MessageType = 9
	MsgJoinRoom        MessageType = 10
	MsgLeaveRoom       MessageType = 11
	MsgRoomCreated     MessageType = 12
	MsgPlayerJoinedRoom MessageType = 13
	MsgPlayerLeftRoom  MessageType = 14

	// Communication messages
	MsgChat        MessageType = 15
	MsgPing        MessageType = 16
	MsgPong        MessageType = 17

	// Reconciliation messages
	MsgReconciliation MessageType = 18
	MsgPlayerUpdate   MessageType = 19
)

// Message represents a network message
type Message struct {
	Type      MessageType
	PlayerID  uint32
	Timestamp int64
	Data      []byte
}

// Serialize converts a message to a byte slice
func (m *Message) Serialize() ([]byte, error) {
	buf := new(bytes.Buffer)

	// Write message type
	if err := binary.Write(buf, binary.LittleEndian, m.Type); err != nil {
		return nil, err
	}

	// Write player ID
	if err := binary.Write(buf, binary.LittleEndian, m.PlayerID); err != nil {
		return nil, err
	}

	// Write timestamp
	if err := binary.Write(buf, binary.LittleEndian, m.Timestamp); err != nil {
		return nil, err
	}

	// Write data length
	dataLen := uint32(len(m.Data))
	if err := binary.Write(buf, binary.LittleEndian, dataLen); err != nil {
		return nil, err
	}

	// Write data
	if _, err := buf.Write(m.Data); err != nil {
		return nil, err
	}

	return buf.Bytes(), nil
}

// Deserialize converts a byte slice to a message
func DeserializeMessage(data []byte) (*Message, error) {
	if len(data) < 13 { // Minimum size: type(1) + id(4) + timestamp(8) + datalen(4) = 13
		return nil, errors.New("data too short")
	}

	buf := bytes.NewReader(data)
	msg := &Message{}

	// Read message type
	if err := binary.Read(buf, binary.LittleEndian, &msg.Type); err != nil {
		return nil, err
	}

	// Read player ID
	if err := binary.Read(buf, binary.LittleEndian, &msg.PlayerID); err != nil {
		return nil, err
	}

	// Read timestamp
	if err := binary.Read(buf, binary.LittleEndian, &msg.Timestamp); err != nil {
		return nil, err
	}

	// Read data length
	var dataLen uint32
	if err := binary.Read(buf, binary.LittleEndian, &dataLen); err != nil {
		return nil, err
	}

	// Validate data length to prevent memory exhaustion
	const maxDataLen = 1024 * 1024 // 1MB maximum
	if dataLen > maxDataLen {
		return nil, errors.New("data too large")
	}
	
	// Read data
	if dataLen > 0 {
		if uint32(len(data)-13) < dataLen {
			return nil, errors.New("data length mismatch")
		}
		msg.Data = make([]byte, dataLen)
		if _, err := io.ReadFull(buf, msg.Data); err != nil {
			return nil, err
		}
	} else {
		msg.Data = []byte{}
	}

	return msg, nil
}
