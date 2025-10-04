package main

import (
	"encoding/binary"
	"fmt"
	"math"
	"net"
	"sync"
	"time"

	"../shared"
)

// Ultra-Fast Multiplayer Client
// Features:
// - Ultra-low latency UDP networking
// - Client-side prediction and reconciliation
// - Automatic server reconciliation
// - Connection quality monitoring
// - Room management and matchmaking

const (
	CLIENT_TICK_RATE     = 60    // Client tick rate (Hz)
	RECONCILIATION_RATE  = 30    // Reconciliation updates per second
	PREDICTION_BUFFER    = 100   // Prediction buffer in milliseconds
	MAX_RECONCILIATION_QUEUE = 32
)

type Client struct {
	// Core networking
	serverAddr *net.UDPAddr
	conn       *net.UDPConn
	playerID   uint64

	// Game state
	player         *Player
	serverState    *Player
	predictionQueue []PredictedState
	reconciliationQueue []ReconciliationData

	// Connection state
	connected      bool
	ping           time.Duration
	lastPing       time.Time
	sequenceNumber uint32
	ackNumber      uint32

	// Room management
	currentRoomID  uint32
	roomPlayers    map[uint64]*Player

	// Synchronization
	stateMutex     sync.RWMutex
	running        bool

	// Performance monitoring
	stats          ClientStats
}

type ClientStats struct {
	messagesSent   int64
	messagesRecv   int64
	bytesSent      int64
	bytesRecv      int64
	reconciliations int64
	predictions    int64
	errors         int64
}

type Player struct {
	ID        uint64
	Name      string
	Position  Vector3
	Rotation  Vector3
	Velocity  Vector3
	Health    float32
	Score     int32
}

type Vector3 struct {
	X, Y, Z float32
}

type PredictedState struct {
	Position     Vector3
	Rotation     Vector3
	Velocity     Vector3
	Timestamp    time.Time
	Sequence     uint32
	Input        PlayerInput
}

type PlayerInput struct {
	MoveForward   float32
	MoveRight     float32
	Turn          float32
	Jump          bool
	Sequence      uint32
}

type ReconciliationData struct {
	ServerState Player
	ClientState Player
	Timestamp   time.Time
}

func NewClient(serverIP string, serverPort int) (*Client, error) {
	serverAddr, err := net.ResolveUDPAddr("udp", fmt.Sprintf("%s:%d", serverIP, serverPort))
	if err != nil {
		return nil, err
	}

	return &Client{
		serverAddr:         serverAddr,
		playerID:           0,
		player:             &Player{},
		serverState:        &Player{},
		predictionQueue:    make([]PredictedState, 0, 64),
		reconciliationQueue: make([]ReconciliationData, 0, MAX_RECONCILIATION_QUEUE),
		connected:          false,
		roomPlayers:        make(map[uint64]*Player),
		running:            true,
	}, nil
}

func (c *Client) Connect(playerName string) error {
	localAddr, err := net.ResolveUDPAddr("udp", ":0")
	if err != nil {
		return fmt.Errorf("failed to resolve local address: %w", err)
	}

	conn, err := net.DialUDP("udp", localAddr, c.serverAddr)
	if err != nil {
		return fmt.Errorf("failed to connect to server: %w", err)
	}

	c.conn = conn
	c.player.Name = playerName
	c.running = true

	fmt.Printf("🔗 Connected to server at %s\n", c.serverAddr.String())

	// Send join message
	joinMsg := &shared.Message{
		Type:      shared.MsgPlayerJoin,
		PlayerID:  0,
		Timestamp: time.Now().UnixNano(),
		Data:      []byte(playerName),
	}

	if err := c.sendMessage(joinMsg); err != nil {
		return fmt.Errorf("failed to send join message: %w", err)
	}

	// Start receive loop
	go c.receiveLoop()

	// Start game loop
	go c.gameLoop()

	return nil
}

func (c *Client) Disconnect() error {
	c.running = false

	if c.conn != nil {
		// Send leave message
		leaveMsg := &shared.Message{
			Type:      shared.MsgPlayerLeave,
			PlayerID:  uint32(c.playerID),
			Timestamp: time.Now().UnixNano(),
			Data:      []byte{},
		}
		c.sendMessage(leaveMsg)

		return c.conn.Close()
	}

	return nil
}

// ========== ROOM MANAGEMENT ==========

func (c *Client) CreateRoom(roomName string) error {
	if !c.connected {
		return fmt.Errorf("not connected to server")
	}

	createMsg := &shared.Message{
		Type:      shared.MsgCreateRoom,
		PlayerID:  uint32(c.playerID),
		Timestamp: time.Now().UnixNano(),
		Data:      []byte(roomName),
	}

	return c.sendMessage(createMsg)
}

func (c *Client) JoinRoom(roomID uint32) error {
	if !c.connected {
		return fmt.Errorf("not connected to server")
	}

	roomData := make([]byte, 4)
	binary.LittleEndian.PutUint32(roomData, roomID)

	joinMsg := &shared.Message{
		Type:      shared.MsgJoinRoom,
		PlayerID:  uint32(c.playerID),
		Timestamp: time.Now().UnixNano(),
		Data:      roomData,
	}

	return c.sendMessage(joinMsg)
}

func (c *Client) LeaveRoom() error {
	if c.currentRoomID == 0 {
		return fmt.Errorf("not in a room")
	}

	leaveMsg := &shared.Message{
		Type:      shared.MsgLeaveRoom,
		PlayerID:  uint32(c.playerID),
		Timestamp: time.Now().UnixNano(),
		Data:      []byte{},
	}

	return c.sendMessage(leaveMsg)
}

// ========== GAMEPLAY ==========

func (c *Client) SendPlayerInput(input PlayerInput) error {
	if !c.connected || c.playerID == 0 {
		return nil
	}

	// Update sequence number
	input.Sequence = c.sequenceNumber
	c.sequenceNumber++

	// Create prediction
	predictedState := c.predictMovement(input)

	// Store prediction for reconciliation
	c.predictionQueue = append(c.predictionQueue, PredictedState{
		Position:  predictedState.Position,
		Rotation:  predictedState.Rotation,
		Velocity:  predictedState.Velocity,
		Timestamp: time.Now(),
		Sequence:  input.Sequence,
		Input:     input,
	})

	// Limit prediction queue
	if len(c.predictionQueue) > 64 {
		c.predictionQueue = c.predictionQueue[1:]
	}

	// Send input to server
	inputData := make([]byte, 20) // 4 floats + 1 bool + 4 sequence = 20 bytes
	binary.LittleEndian.PutUint32(inputData[0:4], math.Float32bits(input.MoveForward))
	binary.LittleEndian.PutUint32(inputData[4:8], math.Float32bits(input.MoveRight))
	binary.LittleEndian.PutUint32(inputData[8:12], math.Float32bits(input.Turn))
	if input.Jump {
		inputData[12] = 1
	}
	binary.LittleEndian.PutUint32(inputData[16:20], input.Sequence)

	inputMsg := &shared.Message{
		Type:      shared.MsgPlayerInput,
		PlayerID:  uint32(c.playerID),
		Timestamp: time.Now().UnixNano(),
		Data:      inputData,
	}

	return c.sendMessage(inputMsg)
}

func (c *Client) predictMovement(input PlayerInput) *Player {
	c.stateMutex.RLock()
	currentState := *c.player
	c.stateMutex.RUnlock()

	// Simple prediction - in a real game, this would include physics simulation
	deltaTime := 1.0 / float32(CLIENT_TICK_RATE)

	// Apply input
	currentState.Position.X += input.MoveForward * deltaTime * 5.0 // Movement speed
	currentState.Position.Z += input.MoveRight * deltaTime * 5.0
	currentState.Rotation.Y += input.Turn * deltaTime * 2.0 // Turn speed

	if input.Jump && currentState.Position.Y <= 0 {
		currentState.Velocity.Y = 10.0 // Jump velocity
	}

	// Apply gravity
	currentState.Velocity.Y -= 20.0 * deltaTime // Gravity
	currentState.Position.Y += currentState.Velocity.Y * deltaTime

	// Ground collision
	if currentState.Position.Y < 0 {
		currentState.Position.Y = 0
		currentState.Velocity.Y = 0
	}

	return &currentState
}

func (c *Client) reconcileState(serverState Player) {
	c.stateMutex.Lock()
	defer c.stateMutex.Unlock()

	// Find corresponding prediction
	var clientPrediction *PredictedState
	for i := len(c.predictionQueue) - 1; i >= 0; i-- {
		if c.predictionQueue[i].Sequence <= uint32(serverState.Score) { // Using Score as sequence for demo
			clientPrediction = &c.predictionQueue[i]
			break
		}
	}

	if clientPrediction != nil {
		// Calculate error
		errorX := serverState.Position.X - clientPrediction.Position.X
		errorY := serverState.Position.Y - clientPrediction.Position.Y
		errorZ := serverState.Position.Z - clientPrediction.Position.Z

		// Apply correction with smoothing
		correctionFactor := 0.1 // Adjust based on network conditions

		c.player.Position.X += errorX * correctionFactor
		c.player.Position.Y += errorY * correctionFactor
		c.player.Position.Z += errorZ * correctionFactor

		// Remove reconciled predictions
		for i, pred := range c.predictionQueue {
			if pred.Sequence <= clientPrediction.Sequence {
				c.predictionQueue = c.predictionQueue[i+1:]
				break
			}
		}

		c.stats.reconciliations++
	}
}

// ========== NETWORKING ==========

func (c *Client) sendMessage(msg *shared.Message) error {
	data, err := msg.Serialize()
	if err != nil {
		c.stats.errors++
		return err
	}

	_, err = c.conn.Write(data)
	if err != nil {
		c.stats.errors++
		return err
	}

	c.stats.messagesSent++
	c.stats.bytesSent += int64(len(data))

	return nil
}

func (c *Client) receiveLoop() {
	buffer := make([]byte, 1472) // Max UDP payload

	for c.running {
		c.conn.SetReadDeadline(time.Now().Add(100 * time.Millisecond))

		n, err := c.conn.Read(buffer)
		if err != nil {
			if netErr, ok := err.(net.Error); ok && netErr.Timeout() {
				continue
			}
			fmt.Printf("Receive error: %v\n", err)
			break
		}

		c.stats.bytesRecv += int64(n)

		msg, err := shared.DeserializeMessage(buffer[:n])
		if err != nil {
			c.stats.errors++
			continue
		}

		c.stats.messagesRecv++
		c.handleMessage(msg)
	}
}

func (c *Client) handleMessage(msg *shared.Message) {
	switch msg.Type {
	case shared.MsgGameState:
		// Extract player ID from welcome message
		if len(msg.Data) >= 8 && c.playerID == 0 {
			c.playerID = binary.LittleEndian.Uint64(msg.Data)
			c.player.ID = c.playerID
			c.connected = true
			fmt.Printf("✅ Connected as player %d\n", c.playerID)
		}

	case shared.MsgPlayerUpdate:
		c.handlePlayerUpdate(msg)

	case shared.MsgSnapshot:
		c.handleSnapshot(msg)

	case shared.MsgReconciliation:
		c.handleReconciliation(msg)

	case shared.MsgRoomCreated:
		c.handleRoomCreated(msg)

	case shared.MsgPlayerJoinedRoom:
		c.handlePlayerJoinedRoom(msg)

	case shared.MsgPlayerLeftRoom:
		c.handlePlayerLeftRoom(msg)

	case shared.MsgGameStart:
		fmt.Println("🎮 Game started!")

	case shared.MsgChat:
		c.handleChat(msg)

	case shared.MsgPong:
		c.updatePing(msg)

	default:
		fmt.Printf("Unknown message type: %d\n", msg.Type)
	}
}

func (c *Client) handlePlayerUpdate(msg *shared.Message) {
	if len(msg.Data) < 32 {
		return
	}

	playerID := binary.LittleEndian.Uint64(msg.Data[0:8])

	c.stateMutex.Lock()
	if playerID == c.playerID {
		// Update our own server state
		c.serverState.Position.X = math.Float32frombits(binary.LittleEndian.Uint32(msg.Data[8:12]))
		c.serverState.Position.Y = math.Float32frombits(binary.LittleEndian.Uint32(msg.Data[12:16]))
		c.serverState.Position.Z = math.Float32frombits(binary.LittleEndian.Uint32(msg.Data[16:20]))
		c.serverState.Rotation.X = math.Float32frombits(binary.LittleEndian.Uint32(msg.Data[20:24]))
		c.serverState.Rotation.Y = math.Float32frombits(binary.LittleEndian.Uint32(msg.Data[24:28]))
		c.serverState.Rotation.Z = math.Float32frombits(binary.LittleEndian.Uint32(msg.Data[28:32]))

		// Reconcile with predictions
		c.reconcileState(*c.serverState)
	} else {
		// Update other player
		if _, exists := c.roomPlayers[playerID]; !exists {
			c.roomPlayers[playerID] = &Player{ID: playerID}
		}
		player := c.roomPlayers[playerID]
		player.Position.X = math.Float32frombits(binary.LittleEndian.Uint32(msg.Data[8:12]))
		player.Position.Y = math.Float32frombits(binary.LittleEndian.Uint32(msg.Data[12:16]))
		player.Position.Z = math.Float32frombits(binary.LittleEndian.Uint32(msg.Data[16:20]))
		player.Rotation.X = math.Float32frombits(binary.LittleEndian.Uint32(msg.Data[20:24]))
		player.Rotation.Y = math.Float32frombits(binary.LittleEndian.Uint32(msg.Data[24:28]))
		player.Rotation.Z = math.Float32frombits(binary.LittleEndian.Uint32(msg.Data[28:32]))
	}
	c.stateMutex.Unlock()
}

func (c *Client) handleSnapshot(msg *shared.Message) {
	// Validate message data length before accessing
	if len(msg.Data) < 4 {
		c.stats.errors++
		return
	}
	
	// Handle world state snapshots
	// This would update all entities in the game world
	fmt.Printf("📸 Received snapshot (ID: %d)\n", binary.LittleEndian.Uint32(msg.Data[0:4]))
}

func (c *Client) handleReconciliation(msg *shared.Message) {
	if len(msg.Data) < 28 {
		return
	}

	// Extract server correction
	sequence := binary.LittleEndian.Uint32(msg.Data[0:4])
	serverPos := Vector3{
		X: math.Float32frombits(binary.LittleEndian.Uint32(msg.Data[4:8])),
		Y: math.Float32frombits(binary.LittleEndian.Uint32(msg.Data[8:12])),
		Z: math.Float32frombits(binary.LittleEndian.Uint32(msg.Data[12:16])),
	}
	serverRot := Vector3{
		X: math.Float32frombits(binary.LittleEndian.Uint32(msg.Data[16:20])),
		Y: math.Float32frombits(binary.LittleEndian.Uint32(msg.Data[20:24])),
		Z: math.Float32frombits(binary.LittleEndian.Uint32(msg.Data[24:28])),
	}

	// Apply correction
	c.stateMutex.Lock()
	c.player.Position = serverPos
	c.player.Rotation = serverRot
	c.stateMutex.Unlock()

	fmt.Printf("🔄 Reconciliation applied (seq: %d)\n", sequence)
}

func (c *Client) handleRoomCreated(msg *shared.Message) {
	if len(msg.Data) >= 8 {
		roomID := binary.LittleEndian.Uint32(msg.Data[0:4])
		playerCount := binary.LittleEndian.Uint32(msg.Data[4:8])

		c.currentRoomID = roomID
		fmt.Printf("🏠 Room created (ID: %d) with %d players\n", roomID, playerCount)
	}
}

func (c *Client) handlePlayerJoinedRoom(msg *shared.Message) {
	if len(msg.Data) >= 8 {
		playerID := binary.LittleEndian.Uint64(msg.Data[0:8])
		playerName := string(msg.Data[8:])

		c.roomPlayers[playerID] = &Player{
			ID:   playerID,
			Name: playerName,
		}

		fmt.Printf("➡️ %s joined the room\n", playerName)
	}
}

func (c *Client) handlePlayerLeftRoom(msg *shared.Message) {
	if len(msg.Data) >= 8 {
		playerID := binary.LittleEndian.Uint64(msg.Data)
		
		// Get player name before deleting
		var playerName string
		if player, exists := c.roomPlayers[playerID]; exists {
			playerName = player.Name
		}
		
		delete(c.roomPlayers, playerID)

		if playerName != "" {
			fmt.Printf("⬅️ %s left the room\n", playerName)
		}
	}
}

func (c *Client) handleChat(msg *shared.Message) {
	if len(msg.Data) >= 8 {
		playerID := binary.LittleEndian.Uint64(msg.Data[0:8])
		message := string(msg.Data[8:])

		if player, exists := c.roomPlayers[playerID]; exists {
			fmt.Printf("💬 %s: %s\n", player.Name, message)
		}
	}
}

func (c *Client) updatePing(msg *shared.Message) {
	pingTime := time.Now().UnixNano() - msg.Timestamp
	c.ping = time.Duration(pingTime)
	c.lastPing = time.Now()
}

// ========== GAME LOOP ==========

func (c *Client) gameLoop() {
	ticker := time.NewTicker(time.Second / time.Duration(CLIENT_TICK_RATE))

	for c.running {
		select {
		case <-ticker.C:
			c.tick()
		}
	}
}

func (c *Client) tick() {
	now := time.Now()

	// Send periodic ping
	if now.Sub(c.lastPing) > 5*time.Second {
		pingMsg := &shared.Message{
			Type:      shared.MsgPing,
			PlayerID:  uint32(c.playerID),
			Timestamp: now.UnixNano(),
			Data:      []byte{},
		}
		c.sendMessage(pingMsg)
	}

	// Update predictions
	c.updatePredictions(now)

	// Process reconciliation queue
	c.processReconciliationQueue(now)
}

func (c *Client) updatePredictions(now time.Time) {
	// Update prediction states based on time
	for i := range c.predictionQueue {
		// Apply time-based corrections to predictions
		age := now.Sub(c.predictionQueue[i].Timestamp)
		if age > PREDICTION_BUFFER*time.Millisecond {
			// Prediction is too old, remove it
			c.predictionQueue = append(c.predictionQueue[:i], c.predictionQueue[i+1:]...)
			i--
		}
	}
}

func (c *Client) processReconciliationQueue(now time.Time) {
	// Process pending reconciliations
	for len(c.reconciliationQueue) > 0 {
		recon := c.reconciliationQueue[0]

		if now.Sub(recon.Timestamp) > 100*time.Millisecond {
			c.reconcileState(recon.ServerState)
			c.reconciliationQueue = c.reconciliationQueue[1:]
		} else {
			break
		}
	}
}

// ========== UTILITIES ==========

func (c *Client) GetPlayer() *Player {
	c.stateMutex.RLock()
	defer c.stateMutex.RUnlock()
	return c.player
}

func (c *Client) GetRoomPlayers() map[uint64]*Player {
	c.stateMutex.RLock()
	defer c.stateMutex.RUnlock()

	// Return a copy
	players := make(map[uint64]*Player)
	for id, player := range c.roomPlayers {
		playerCopy := *player
		players[id] = &playerCopy
	}
	return players
}

func (c *Client) GetStats() ClientStats {
	return c.stats
}

func (c *Client) GetPing() time.Duration {
	return c.ping
}

func (c *Client) IsConnected() bool {
	return c.connected
}

func (c *Client) GetCurrentRoomID() uint32 {
	return c.currentRoomID
}

// ========== MAIN ==========

func main() {
	client, err := NewClient("127.0.0.1", 8080)
	if err != nil {
		fmt.Printf("❌ Failed to create client: %v\n", err)
		return
	}

	if err := client.Connect("TestPlayer"); err != nil {
		fmt.Printf("❌ Failed to connect: %v\n", err)
		return
	}

	// Wait for connection
	time.Sleep(1 * time.Second)

	if err := client.CreateRoom("TestRoom"); err != nil {
		fmt.Printf("❌ Failed to create room: %v\n", err)
	}

	// Simulate gameplay
	ticker := time.NewTicker(16 * time.Millisecond) // ~60 FPS
	defer ticker.Stop()

	inputSequence := uint32(0)

	for i := 0; i < 3600; i++ { // Run for 60 seconds at 60 FPS
		select {
		case <-ticker.C:
			// Generate some test input
			input := PlayerInput{
				MoveForward: float32(math.Sin(float64(i) * 0.01)),
				MoveRight:   float32(math.Cos(float64(i) * 0.01)),
				Turn:        float32(math.Sin(float64(i) * 0.005)),
				Jump:        i%600 == 0, // Jump every 10 seconds
				Sequence:    inputSequence,
			}
			inputSequence++

			if err := client.SendPlayerInput(input); err != nil {
				fmt.Printf("❌ Failed to send input: %v\n", err)
			}

			// Print stats every second
			if i%60 == 0 {
				stats := client.GetStats()
				fmt.Printf("📊 Stats - Ping: %v, Sent: %d, Recv: %d\n",
					client.GetPing(), stats.messagesSent, stats.messagesRecv)
			}
		}
	}

	client.Disconnect()
	fmt.Println("👋 Client disconnected")
}
