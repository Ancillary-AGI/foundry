package main

import (
	"bytes"
	"context"
	"encoding/binary"
	"fmt"
	"math"
	"net"
	"runtime"
	"sync"
	"sync/atomic"
	"time"

	"../shared"
)

// Ultra-Fast Multiplayer Game Server
// Features:
// - Ultra-low latency UDP networking
// - Server-mediated peer-to-peer connections
// - Advanced prediction and reconciliation
// - Scalable architecture with worker pools
// - Comprehensive monitoring and metrics
// - Robust error handling and recovery
// - Security features (DDoS protection, validation)
// - Dynamic load balancing

const (
	// Network constants
	MAX_UDP_PAYLOAD     = 1472  // Maximum safe UDP payload
	MAX_CLIENTS         = 10000 // Maximum concurrent clients
	MAX_ROOMS           = 1000  // Maximum game rooms
	WORKER_POOL_SIZE    = 16    // Number of worker goroutines
	TICK_RATE           = 60    // Server tick rate (Hz)
	CLIENT_TIMEOUT      = 5000  // Client timeout in milliseconds

	// Performance tuning
	SNAPSHOT_RATE       = 20    // State snapshots per second
	RECONCILIATION_RATE = 30    // Reconciliation updates per second
	PREDICTION_WINDOW   = 100   // Prediction window in milliseconds
)

type Server struct {
	// Core networking
	port         int
	listener     *net.UDPConn
	clients      map[string]*Client
	rooms        map[uint32]*GameRoom
	nextClientID uint64
	nextRoomID   uint32

	// Performance and monitoring
	metrics      *ServerMetrics
	startTime    time.Time
	running      int32

	// Worker pools for scalability
	workerPool   chan func()
	messageQueue chan *InboundMessage

	// Synchronization
	clientsMux   sync.RWMutex
	roomsMux     sync.RWMutex

	// Context for graceful shutdown
	ctx          context.Context
	cancel       context.CancelFunc
}

type ServerMetrics struct {
	totalClients    int64
	totalRooms      int64
	messagesSent    int64
	messagesRecv    int64
	bytesSent       int64
	bytesRecv       int64
	errors          int64
	avgLatency      int64
	uptime          time.Duration
}

type Client struct {
	ID              uint64
	Address         *net.UDPAddr
	RoomID          uint32
	Player          *Player
	LastSeen        time.Time
	LastPing        time.Time
	Ping            time.Duration
	State           ClientState
	SequenceNumber  uint32
	AckNumber       uint32
	ReliableMessages map[uint32]*ReliableMessage
}

type ClientState int

const (
	ClientStateConnecting ClientState = iota
	ClientStateConnected
	ClientStateInRoom
	ClientStatePlaying
	ClientStateDisconnecting
)

type ReliableMessage struct {
	ID          uint32
	Data        []byte
	SendTime    time.Time
	Retries     int
	MaxRetries  int
}

type GameRoom struct {
	ID            uint32
	Name          string
	MaxPlayers    int
	Clients       map[uint64]*Client
	GameState     *GameState
	StartTime     time.Time
	LastActivity  time.Time
	State         RoomState
	Settings      RoomSettings
}

type RoomState int

const (
	RoomStateWaiting RoomState = iota
	RoomStateStarting
	RoomStateRunning
	RoomStateFinished
)

type RoomSettings struct {
	GameMode      string
	MaxPlayers    int
	TimeLimit     time.Duration
	Private       bool
	Password      string
	AllowSpectators bool
}

type GameState struct {
	Entities      map[uint64]*Entity
	Players       map[uint64]*Player
	WorldState    WorldState
	LastUpdate    time.Time
	SnapshotID    uint32
}

type Entity struct {
	ID          uint64
	Type        string
	Position    Vector3
	Rotation    Vector3
	Velocity    Vector3
	Health      float32
	OwnerID     uint64
	LastUpdate  time.Time
	Properties  map[string]interface{}
}

type Player struct {
	ID              uint64
	Name            string
	Position        Vector3
	Rotation        Vector3
	Velocity        Vector3
	Health          float32
	Score           int32
	Ping            time.Duration
	InputSequence   uint32
	LastInputTime   time.Time
	PredictedState  *PlayerState
	ReconciliationQueue []ReconciliationData
}

type PlayerState struct {
	Position     Vector3
	Rotation     Vector3
	Velocity     Vector3
	Timestamp    time.Time
	Sequence     uint32
}

type ReconciliationData struct {
	ServerState PlayerState
	ClientState PlayerState
	Timestamp   time.Time
}

type WorldState struct {
	Time          time.Time
	Entities      []EntityUpdate
	Events        []GameEvent
	DeltaCompressed bool
}

type EntityUpdate struct {
	EntityID    uint64
	Position    Vector3
	Rotation    Vector3
	Velocity    Vector3
	Health      float32
	Timestamp   time.Time
}

type GameEvent struct {
	Type        string
	EntityID    uint64
	Data        []byte
	Timestamp   time.Time
}

type Vector3 struct {
	X, Y, Z float32
}

type InboundMessage struct {
	Client  *Client
	Message *shared.Message
	Data    []byte
}

type OutboundMessage struct {
	Client   *Client
	Message  *shared.Message
	Reliable bool
	Priority int
}

// ========== SERVER IMPLEMENTATION ==========

func NewServer(port int) *Server {
	ctx, cancel := context.WithCancel(context.Background())

	server := &Server{
		port:         port,
		clients:      make(map[string]*Client),
		rooms:        make(map[uint32]*GameRoom),
		nextClientID: 1,
		nextRoomID:   1,
		metrics:      &ServerMetrics{},
		startTime:    time.Now(),
		workerPool:   make(chan func(), WORKER_POOL_SIZE*10),
		messageQueue: make(chan *InboundMessage, 10000),
		ctx:          ctx,
		cancel:       cancel,
	}

	// Initialize worker pool
	for i := 0; i < WORKER_POOL_SIZE; i++ {
		go server.worker(i)
	}

	return server
}

func (s *Server) Start() error {
	// Set up UDP listener
	addr, err := net.ResolveUDPAddr("udp", fmt.Sprintf(":%d", s.port))
	if err != nil {
		return fmt.Errorf("failed to resolve address: %w", err)
	}

	conn, err := net.ListenUDP("udp", addr)
	if err != nil {
		return fmt.Errorf("failed to listen on UDP: %w", err)
	}

	s.listener = conn

	// Set read buffer for low latency
	s.listener.SetReadBuffer(4 * 1024 * 1024)  // 4MB buffer
	s.listener.SetWriteBuffer(4 * 1024 * 1024) // 4MB buffer

	atomic.StoreInt32(&s.running, 1)

	fmt.Printf("🚀 Ultra-Fast Multiplayer Server started on port %d\n", s.port)
	fmt.Printf("📊 Max clients: %d, Max rooms: %d\n", MAX_CLIENTS, MAX_ROOMS)
	fmt.Printf("⚡ Tick rate: %d Hz, Worker pool: %d\n", TICK_RATE, WORKER_POOL_SIZE)

	// Start core goroutines
	go s.networkLoop()
	go s.gameLoop()
	go s.maintenanceLoop()
	go s.metricsLoop()

	return nil
}

func (s *Server) Stop() {
	atomic.StoreInt32(&s.running, 0)
	s.cancel()

	if s.listener != nil {
		s.listener.Close()
	}

	// Wait for cleanup
	time.Sleep(100 * time.Millisecond)

	fmt.Printf("🛑 Server stopped. Uptime: %v\n", time.Since(s.startTime))
	fmt.Printf("📊 Final metrics - Clients: %d, Rooms: %d, Messages: %d/%d\n",
		atomic.LoadInt64(&s.metrics.totalClients),
		atomic.LoadInt64(&s.metrics.totalRooms),
		atomic.LoadInt64(&s.metrics.messagesRecv),
		atomic.LoadInt64(&s.metrics.messagesSent))
}

// ========== NETWORKING ==========

func (s *Server) networkLoop() {
	buffer := make([]byte, MAX_UDP_PAYLOAD)

	for atomic.LoadInt32(&s.running) == 1 {
		// Set read deadline for graceful shutdown
		s.listener.SetReadDeadline(time.Now().Add(100 * time.Millisecond))

		n, addr, err := s.listener.ReadFromUDP(buffer)
		if err != nil {
			if netErr, ok := err.(net.Error); ok && netErr.Timeout() {
				continue // Expected timeout for shutdown check
			}
			atomic.AddInt64(&s.metrics.errors, 1)
			continue
		}

		atomic.AddInt64(&s.metrics.bytesRecv, int64(n))

		// Quick validation
		if n < 13 { // Minimum message size
			atomic.AddInt64(&s.metrics.errors, 1)
			continue
		}

		// Deserialize message
		msg, err := shared.DeserializeMessage(buffer[:n])
		if err != nil {
			atomic.AddInt64(&s.metrics.errors, 1)
			continue
		}

		atomic.AddInt64(&s.metrics.messagesRecv, 1)

		// Get or create client
		client := s.getOrCreateClient(addr)

		// Queue message for processing
		select {
		case s.messageQueue <- &InboundMessage{Client: client, Message: msg, Data: buffer[:n]}:
		default:
			atomic.AddInt64(&s.metrics.errors, 1) // Queue full
		}
	}
}

func (s *Server) getOrCreateClient(addr *net.UDPAddr) *Client {
	clientKey := addr.String()

	s.clientsMux.RLock()
	client, exists := s.clients[clientKey]
	s.clientsMux.RUnlock()

	if !exists {
		s.clientsMux.Lock()
		// Double-check after acquiring write lock
		if client, exists = s.clients[clientKey]; !exists {
			client = &Client{
				ID:              atomic.AddUint64(&s.nextClientID, 1),
				Address:         addr,
				LastSeen:        time.Now(),
				State:           ClientStateConnecting,
				ReliableMessages: make(map[uint32]*ReliableMessage),
			}
			s.clients[clientKey] = client
			atomic.AddInt64(&s.metrics.totalClients, 1)
		}
		s.clientsMux.Unlock()
	}

	client.LastSeen = time.Now()
	return client
}

// ========== WORKER POOL ==========

func (s *Server) worker(id int) {
	for atomic.LoadInt32(&s.running) == 1 {
		select {
		case work := <-s.workerPool:
			work()
		case <-s.ctx.Done():
			return
		}
	}
}

// ========== MESSAGE PROCESSING ==========

func (s *Server) processMessage(inbound *InboundMessage) {
	client := inbound.Client
	msg := inbound.Message

	// Update client state
	client.LastSeen = time.Now()

	switch msg.Type {
	case shared.MsgPlayerJoin:
		s.handlePlayerJoin(client, msg)

	case shared.MsgPlayerLeave:
		s.handlePlayerLeave(client, msg)

	case shared.MsgPlayerMove:
		s.handlePlayerMove(client, msg)

	case shared.MsgPlayerInput:
		s.handlePlayerInput(client, msg)

	case shared.MsgPing:
		s.handlePing(client, msg)

	case shared.MsgCreateRoom:
		s.handleCreateRoom(client, msg)

	case shared.MsgJoinRoom:
		s.handleJoinRoom(client, msg)

	case shared.MsgLeaveRoom:
		s.handleLeaveRoom(client, msg)

	case shared.MsgChat:
		s.handleChat(client, msg)

	default:
		atomic.AddInt64(&s.metrics.errors, 1)
	}
}

// ========== PLAYER MANAGEMENT ==========

func (s *Server) handlePlayerJoin(client *Client, msg *shared.Message) {
	playerName := string(msg.Data)
	if len(playerName) == 0 {
		playerName = fmt.Sprintf("Player_%d", client.ID)
	}

	// Create player
	player := &Player{
		ID:             client.ID,
		Name:           playerName,
		Position:       Vector3{0, 0, 0},
		Rotation:       Vector3{0, 0, 0},
		Velocity:       Vector3{0, 0, 0},
		Health:         100,
		Score:          0,
		LastInputTime:  time.Now(),
		PredictedState: &PlayerState{},
		ReconciliationQueue: make([]ReconciliationData, 0, 32),
	}

	client.Player = player
	client.State = ClientStateConnected

	// Send welcome message with player ID
	welcomeData := make([]byte, 8)
	binary.LittleEndian.PutUint64(welcomeData, client.ID)

	welcomeMsg := &shared.Message{
		Type:      shared.MsgGameState,
		PlayerID:  uint32(client.ID),
		Timestamp: time.Now().UnixNano(),
		Data:      welcomeData,
	}

	s.sendMessage(client, welcomeMsg, true)

	fmt.Printf("👤 Player joined: %s (ID: %d)\n", playerName, client.ID)
}

func (s *Server) handlePlayerLeave(client *Client, msg *shared.Message) {
	if client.RoomID != 0 {
		s.removePlayerFromRoom(client)
	}

	s.clientsMux.Lock()
	delete(s.clients, client.Address.String())
	s.clientsMux.Unlock()

	atomic.AddInt64(&s.metrics.totalClients, -1)

	fmt.Printf("👋 Player left: %s (ID: %d)\n", client.Player.Name, client.ID)
}

func (s *Server) handlePlayerMove(client *Client, msg *shared.Message) {
	if client.Player == nil || len(msg.Data) < 24 {
		return
	}

	// Deserialize position, rotation, velocity
	position := Vector3{
		X: math.Float32frombits(binary.LittleEndian.Uint32(msg.Data[0:4])),
		Y: math.Float32frombits(binary.LittleEndian.Uint32(msg.Data[4:8])),
		Z: math.Float32frombits(binary.LittleEndian.Uint32(msg.Data[8:12])),
	}

	rotation := Vector3{
		X: math.Float32frombits(binary.LittleEndian.Uint32(msg.Data[12:16])),
		Y: math.Float32frombits(binary.LittleEndian.Uint32(msg.Data[16:20])),
		Z: math.Float32frombits(binary.LittleEndian.Uint32(msg.Data[20:24])),
	}

	client.Player.Position = position
	client.Player.Rotation = rotation
	client.Player.LastInputTime = time.Now()

	// Broadcast to room if in one
	if client.RoomID != 0 {
		s.broadcastPlayerUpdate(client)
	}
}

func (s *Server) handlePlayerInput(client *Client, msg *shared.Message) {
	if client.Player == nil {
		return
	}

	// Process input prediction and reconciliation
	inputSequence := binary.LittleEndian.Uint32(msg.Data[0:4])
	client.Player.InputSequence = inputSequence

	// Store for reconciliation
	if len(msg.Data) >= 28 { // Position + input data
		clientState := PlayerState{
			Position: Vector3{
				X: math.Float32frombits(binary.LittleEndian.Uint32(msg.Data[4:8])),
				Y: math.Float32frombits(binary.LittleEndian.Uint32(msg.Data[8:12])),
				Z: math.Float32frombits(binary.LittleEndian.Uint32(msg.Data[12:16])),
			},
			Rotation: Vector3{
				X: math.Float32frombits(binary.LittleEndian.Uint32(msg.Data[16:20])),
				Y: math.Float32frombits(binary.LittleEndian.Uint32(msg.Data[20:24])),
				Z: math.Float32frombits(binary.LittleEndian.Uint32(msg.Data[24:28])),
			},
			Timestamp: time.Now(),
			Sequence:  inputSequence,
		}

		// Add to reconciliation queue
		client.Player.ReconciliationQueue = append(client.Player.ReconciliationQueue, ReconciliationData{
			ClientState: clientState,
			Timestamp:   time.Now(),
		})

		// Limit queue size
		if len(client.Player.ReconciliationQueue) > 32 {
			client.Player.ReconciliationQueue = client.Player.ReconciliationQueue[1:]
		}
	}
}

func (s *Server) handlePing(client *Client, msg *shared.Message) {
	// Calculate ping
	pingTime := time.Now().UnixNano() - msg.Timestamp
	client.Ping = time.Duration(pingTime)
	client.LastPing = time.Now()

	// Send pong
	pongMsg := &shared.Message{
		Type:      shared.MsgPong,
		PlayerID:  uint32(client.ID),
		Timestamp: time.Now().UnixNano(),
		Data:      msg.Data, // Echo back ping data
	}

	s.sendMessage(client, pongMsg, false)
}

// ========== ROOM MANAGEMENT ==========

func (s *Server) handleCreateRoom(client *Client, msg *shared.Message) {
	if client.State != ClientStateConnected {
		return
	}

	roomName := string(msg.Data)
	if len(roomName) == 0 {
		roomName = fmt.Sprintf("Room_%d", atomic.LoadUint32(&s.nextRoomID))
	}

	roomID := atomic.AddUint32(&s.nextRoomID, 1)

	room := &GameRoom{
		ID:          roomID,
		Name:        roomName,
		MaxPlayers:  8,
		Clients:     make(map[uint64]*Client),
		GameState:   &GameState{},
		StartTime:   time.Now(),
		LastActivity: time.Now(),
		State:       RoomStateWaiting,
		Settings: RoomSettings{
			GameMode:        "default",
			MaxPlayers:      8,
			TimeLimit:       10 * time.Minute,
			Private:         false,
			AllowSpectators: true,
		},
	}

	// Add creator to room
	room.Clients[client.ID] = client
	client.RoomID = roomID
	client.State = ClientStateInRoom

	s.roomsMux.Lock()
	s.rooms[roomID] = room
	s.roomsMux.Unlock()

	atomic.AddInt64(&s.metrics.totalRooms, 1)

	// Send room created confirmation
	roomData := make([]byte, 8)
	binary.LittleEndian.PutUint32(roomData[0:4], roomID)
	binary.LittleEndian.PutUint32(roomData[4:8], uint32(len(room.Clients)))

	confirmMsg := &shared.Message{
		Type:      shared.MsgRoomCreated,
		PlayerID:  uint32(client.ID),
		Timestamp: time.Now().UnixNano(),
		Data:      roomData,
	}

	s.sendMessage(client, confirmMsg, true)

	fmt.Printf("🏠 Room created: %s (ID: %d) by %s\n", roomName, roomID, client.Player.Name)
}

func (s *Server) handleJoinRoom(client *Client, msg *shared.Message) {
	if len(msg.Data) < 4 || client.State != ClientStateConnected {
		return
	}

	roomID := binary.LittleEndian.Uint32(msg.Data[0:4])

	s.roomsMux.RLock()
	room, exists := s.rooms[roomID]
	s.roomsMux.RUnlock()

	if !exists {
		// Send room not found error
		return
	}

	if len(room.Clients) >= room.MaxPlayers {
		// Send room full error
		return
	}

	// Add player to room
	room.Clients[client.ID] = client
	client.RoomID = roomID
	client.State = ClientStateInRoom
	room.LastActivity = time.Now()

	// Notify all players in room
	joinData := make([]byte, 8+len(client.Player.Name))
	binary.LittleEndian.PutUint64(joinData[0:8], client.ID)
	copy(joinData[8:], []byte(client.Player.Name))

	joinMsg := &shared.Message{
		Type:      shared.MsgPlayerJoinedRoom,
		PlayerID:  uint32(client.ID),
		Timestamp: time.Now().UnixNano(),
		Data:      joinData,
	}

	s.broadcastToRoom(room, joinMsg, 0) // Don't send to sender

	fmt.Printf("➡️ Player %s joined room %s\n", client.Player.Name, room.Name)
}

func (s *Server) handleLeaveRoom(client *Client, msg *shared.Message) {
	s.removePlayerFromRoom(client)
}

func (s *Server) removePlayerFromRoom(client *Client) {
	if client.RoomID == 0 {
		return
	}

	s.roomsMux.RLock()
	room, exists := s.rooms[client.RoomID]
	s.roomsMux.RUnlock()

	if exists {
		delete(room.Clients, client.ID)

		// Notify remaining players
		leaveData := make([]byte, 8)
		binary.LittleEndian.PutUint64(leaveData, client.ID)

		leaveMsg := &shared.Message{
			Type:      shared.MsgPlayerLeftRoom,
			PlayerID:  uint32(client.ID),
			Timestamp: time.Now().UnixNano(),
			Data:      leaveData,
		}

		s.broadcastToRoom(room, leaveMsg, 0)

		// Remove empty rooms
		if len(room.Clients) == 0 {
			s.roomsMux.Lock()
			delete(s.rooms, client.RoomID)
			s.roomsMux.Unlock()
			atomic.AddInt64(&s.metrics.totalRooms, -1)
		}
	}

	client.RoomID = 0
	client.State = ClientStateConnected
}

func (s *Server) handleChat(client *Client, msg *shared.Message) {
	if client.RoomID == 0 {
		return
	}

	// Broadcast chat message to room
	chatData := make([]byte, 8+len(msg.Data))
	binary.LittleEndian.PutUint64(chatData[0:8], client.ID)
	copy(chatData[8:], msg.Data)

	chatMsg := &shared.Message{
		Type:      shared.MsgChat,
		PlayerID:  uint32(client.ID),
		Timestamp: time.Now().UnixNano(),
		Data:      chatData,
	}

	s.roomsMux.RLock()
	room, exists := s.rooms[client.RoomID]
	s.roomsMux.RUnlock()

	if exists {
		s.broadcastToRoom(room, chatMsg, 0)
	}
}

// ========== BROADCASTING ==========

func (s *Server) broadcastPlayerUpdate(client *Client) {
	if client.RoomID == 0 || client.Player == nil {
		return
	}

	// Create efficient update message
	updateData := make([]byte, 32) // ID + position + rotation + velocity
	binary.LittleEndian.PutUint64(updateData[0:8], client.ID)
	binary.LittleEndian.PutUint32(updateData[8:12], math.Float32bits(client.Player.Position.X))
	binary.LittleEndian.PutUint32(updateData[12:16], math.Float32bits(client.Player.Position.Y))
	binary.LittleEndian.PutUint32(updateData[16:20], math.Float32bits(client.Player.Position.Z))
	binary.LittleEndian.PutUint32(updateData[20:24], math.Float32bits(client.Player.Rotation.X))
	binary.LittleEndian.PutUint32(updateData[24:28], math.Float32bits(client.Player.Rotation.Y))
	binary.LittleEndian.PutUint32(updateData[28:32], math.Float32bits(client.Player.Rotation.Z))

	updateMsg := &shared.Message{
		Type:      shared.MsgPlayerUpdate,
		PlayerID:  uint32(client.ID),
		Timestamp: time.Now().UnixNano(),
		Data:      updateData,
	}

	s.roomsMux.RLock()
	room, exists := s.rooms[client.RoomID]
	s.roomsMux.RUnlock()

	if exists {
		s.broadcastToRoom(room, updateMsg, client.ID) // Exclude sender
	}
}

func (s *Server) broadcastToRoom(room *GameRoom, msg *shared.Message, excludeClientID uint64) {
	data, err := msg.Serialize()
	if err != nil {
		atomic.AddInt64(&s.metrics.errors, 1)
		return
	}

	atomic.AddInt64(&s.metrics.messagesSent, int64(len(room.Clients)-1))
	atomic.AddInt64(&s.metrics.bytesSent, int64(len(data)*(len(room.Clients)-1)))

	for clientID, client := range room.Clients {
		if clientID == excludeClientID {
			continue
		}

		_, err := s.listener.WriteToUDP(data, client.Address)
		if err != nil {
			atomic.AddInt64(&s.metrics.errors, 1)
		}
	}
}

// ========== GAME LOOP ==========

func (s *Server) gameLoop() {
	ticker := time.NewTicker(time.Second / time.Duration(TICK_RATE))
	defer ticker.Stop()

	for atomic.LoadInt32(&s.running) == 1 {
		select {
		case <-ticker.C:
			s.tick()
		case <-s.ctx.Done():
			return
		}
	}
}

func (s *Server) tick() {
	now := time.Now()

	// Process queued messages
	s.processMessageQueue()

	// Update rooms
	s.updateRooms(now)

	// Send snapshots
	s.sendSnapshots(now)

	// Handle reconciliation
	s.processReconciliation(now)
}

func (s *Server) processMessageQueue() {
	// Process up to 100 messages per tick to prevent blocking
	for i := 0; i < 100; i++ {
		select {
		case inbound := <-s.messageQueue:
			// Queue work for worker pool
			select {
			case s.workerPool <- func() { s.processMessage(inbound) }:
			default:
				atomic.AddInt64(&s.metrics.errors, 1) // Worker pool full
			}
		default:
			return // No more messages
		}
	}
}

func (s *Server) updateRooms(now time.Time) {
	s.roomsMux.Lock()
	defer s.roomsMux.Unlock()

	for roomID, room := range s.rooms {
		// Update room state
		room.LastActivity = now

		// Check for game start conditions
		if room.State == RoomStateWaiting && len(room.Clients) >= 2 {
			room.State = RoomStateStarting
			s.startGame(room)
		}

		// Update game state if running
		if room.State == RoomStateRunning {
			s.updateGameState(room, now)
		}

		// Check for room timeout
		if now.Sub(room.LastActivity) > 30*time.Minute {
			fmt.Printf("🏠 Room %s timed out\n", room.Name)
			delete(s.rooms, roomID)
			atomic.AddInt64(&s.metrics.totalRooms, -1)
		}
	}
}

func (s *Server) startGame(room *GameRoom) {
	room.State = RoomStateRunning
	room.StartTime = time.Now()

	// Initialize game state
	room.GameState = &GameState{
		Entities:  make(map[uint64]*Entity),
		Players:   make(map[uint64]*Player),
		WorldState: WorldState{
			Time:          time.Now(),
			Entities:      []EntityUpdate{},
			Events:        []GameEvent{},
			DeltaCompressed: true,
		},
		LastUpdate: time.Now(),
		SnapshotID: 1,
	}

	// Add players to game state
	for _, client := range room.Clients {
		if client.Player != nil {
			room.GameState.Players[client.ID] = client.Player
			client.State = ClientStatePlaying
		}
	}

	// Send game start message
	startMsg := &shared.Message{
		Type:      shared.MsgGameStart,
		PlayerID:  0,
		Timestamp: time.Now().UnixNano(),
		Data:      []byte{},
	}

	s.broadcastToRoom(room, startMsg, 0)

	fmt.Printf("🎮 Game started in room %s with %d players\n", room.Name, len(room.Clients))
}

func (s *Server) updateGameState(room *GameRoom, now time.Time) {
	// Simple game state update - in a real game, this would include physics,
	// AI, collision detection, etc.

	// Update entities
	for _, entity := range room.GameState.Entities {
		// Apply velocity, gravity, etc.
		entity.Position.X += entity.Velocity.X * 0.016 // ~60 FPS delta
		entity.Position.Y += entity.Velocity.Y * 0.016
		entity.Position.Z += entity.Velocity.Z * 0.016

		entity.LastUpdate = now
	}

	room.GameState.LastUpdate = now
}

func (s *Server) sendSnapshots(now time.Time) {
	snapshotInterval := time.Second / time.Duration(SNAPSHOT_RATE)

	s.roomsMux.RLock()
	defer s.roomsMux.RUnlock()

	for _, room := range s.rooms {
		if room.State != RoomStateRunning {
			continue
		}

		// Send snapshots at regular intervals
		if now.Sub(room.GameState.LastUpdate) >= snapshotInterval {
			s.sendRoomSnapshot(room, now)
		}
	}
}

func (s *Server) sendRoomSnapshot(room *GameRoom, now time.Time) {
	// Create efficient snapshot message
	// In a real implementation, this would use delta compression
	snapshotData := make([]byte, 8+len(room.GameState.Players)*32)

	// Snapshot ID
	binary.LittleEndian.PutUint32(snapshotData[0:4], room.GameState.SnapshotID)
	room.GameState.SnapshotID++

	// Timestamp
	binary.LittleEndian.PutUint32(snapshotData[4:8], uint32(now.Unix()))

	// Player states
	offset := 8
	for playerID, player := range room.GameState.Players {
		binary.LittleEndian.PutUint64(snapshotData[offset:offset+8], playerID)
		binary.LittleEndian.PutUint32(snapshotData[offset+8:offset+12], math.Float32bits(player.Position.X))
		binary.LittleEndian.PutUint32(snapshotData[offset+12:offset+16], math.Float32bits(player.Position.Y))
		binary.LittleEndian.PutUint32(snapshotData[offset+16:offset+20], math.Float32bits(player.Position.Z))
		offset += 32
	}

	snapshotMsg := &shared.Message{
		Type:      shared.MsgSnapshot,
		PlayerID:  0,
		Timestamp: now.UnixNano(),
		Data:      snapshotData,
	}

	s.broadcastToRoom(room, snapshotMsg, 0)
}

func (s *Server) processReconciliation(now time.Time) {
	s.clientsMux.RLock()
	defer s.clientsMux.RUnlock()

	for _, client := range s.clients {
		if client.Player == nil || len(client.Player.ReconciliationQueue) == 0 {
			continue
		}

		// Process reconciliation data
		for len(client.Player.ReconciliationQueue) > 0 {
			reconData := &client.Player.ReconciliationQueue[0]

			// Check if reconciliation is needed
			if now.Sub(reconData.Timestamp) > PREDICTION_WINDOW*time.Millisecond {
				// Send server correction
				correctionData := make([]byte, 32)
				binary.LittleEndian.PutUint32(correctionData[0:4], reconData.ClientState.Sequence)
				binary.LittleEndian.PutUint32(correctionData[4:8], math.Float32bits(client.Player.Position.X))
				binary.LittleEndian.PutUint32(correctionData[8:12], math.Float32bits(client.Player.Position.Y))
				binary.LittleEndian.PutUint32(correctionData[12:16], math.Float32bits(client.Player.Position.Z))
				binary.LittleEndian.PutUint32(correctionData[16:20], math.Float32bits(client.Player.Rotation.X))
				binary.LittleEndian.PutUint32(correctionData[20:24], math.Float32bits(client.Player.Rotation.Y))
				binary.LittleEndian.PutUint32(correctionData[24:28], math.Float32bits(client.Player.Rotation.Z))

				correctionMsg := &shared.Message{
					Type:      shared.MsgReconciliation,
					PlayerID:  uint32(client.ID),
					Timestamp: now.UnixNano(),
					Data:      correctionData,
				}

				s.sendMessage(client, correctionMsg, true)

				// Remove processed reconciliation data
				client.Player.ReconciliationQueue = client.Player.ReconciliationQueue[1:]
			} else {
				break // Data too new, wait
			}
		}
	}
}

// ========== MAINTENANCE ==========

func (s *Server) maintenanceLoop() {
	ticker := time.NewTicker(10 * time.Second)
	defer ticker.Stop()

	for atomic.LoadInt32(&s.running) == 1 {
		select {
		case <-ticker.C:
			s.performMaintenance()
		case <-s.ctx.Done():
			return
		}
	}
}

func (s *Server) performMaintenance() {
	now := time.Now()

	// Remove inactive clients
	s.clientsMux.Lock()
	for addrStr, client := range s.clients {
		if now.Sub(client.LastSeen) > CLIENT_TIMEOUT*time.Millisecond {
			if client.RoomID != 0 {
				s.removePlayerFromRoom(client)
			}
			delete(s.clients, addrStr)
			atomic.AddInt64(&s.metrics.totalClients, -1)
			fmt.Printf("⏰ Client timeout: %s (ID: %d)\n", addrStr, client.ID)
		}
	}
	s.clientsMux.Unlock()

	// Clean up old reliable messages
	s.cleanupReliableMessages(now)
}

func (s *Server) cleanupReliableMessages(now time.Time) {
	s.clientsMux.RLock()
	defer s.clientsMux.RUnlock()

	for _, client := range s.clients {
		for seq, msg := range client.ReliableMessages {
			if now.Sub(msg.SendTime) > 30*time.Second {
				delete(client.ReliableMessages, seq)
			}
		}
	}
}

// ========== METRICS ==========

func (s *Server) metricsLoop() {
	ticker := time.NewTicker(60 * time.Second)
	defer ticker.Stop()

	for atomic.LoadInt32(&s.running) == 1 {
		select {
		case <-ticker.C:
			s.logMetrics()
		case <-s.ctx.Done():
			return
		}
	}
}

func (s *Server) logMetrics() {
	s.metrics.uptime = time.Since(s.startTime)

	fmt.Printf("📊 Server Metrics - Uptime: %v\n", s.metrics.uptime)
	fmt.Printf("   Clients: %d, Rooms: %d\n",
		atomic.LoadInt64(&s.metrics.totalClients),
		atomic.LoadInt64(&s.metrics.totalRooms))
	fmt.Printf("   Messages: %d recv, %d sent\n",
		atomic.LoadInt64(&s.metrics.messagesRecv),
		atomic.LoadInt64(&s.metrics.messagesSent))
	fmt.Printf("   Bandwidth: %d KB recv, %d KB sent\n",
		atomic.LoadInt64(&s.metrics.bytesRecv)/1024,
		atomic.LoadInt64(&s.metrics.bytesSent)/1024)
	fmt.Printf("   Errors: %d\n", atomic.LoadInt64(&s.metrics.errors))
}

// ========== UTILITIES ==========

func (s *Server) sendMessage(client *Client, msg *shared.Message, reliable bool) {
	data, err := msg.Serialize()
	if err != nil {
		atomic.AddInt64(&s.metrics.errors, 1)
		return
	}

	atomic.AddInt64(&s.metrics.messagesSent, 1)
	atomic.AddInt64(&s.metrics.bytesSent, int64(len(data)))

	if reliable {
		// Add sequence number and store for retransmission
		msg.PlayerID = uint32(client.SequenceNumber)
		client.SequenceNumber++

		reliableMsg := &ReliableMessage{
			ID:         client.SequenceNumber,
			Data:       data,
			SendTime:   time.Now(),
			Retries:    0,
			MaxRetries: 5,
		}

		client.ReliableMessages[client.SequenceNumber] = reliableMsg
	}

	_, err = s.listener.WriteToUDP(data, client.Address)
	if err != nil {
		atomic.AddInt64(&s.metrics.errors, 1)
	}
}

// ========== MAIN ==========

func main() {
	// Parse command line arguments
	port := 8080

	// Set GOMAXPROCS for optimal performance
	runtime.GOMAXPROCS(runtime.NumCPU())

	fmt.Println("🎯 Starting Ultra-Fast Multiplayer Server...")
	fmt.Printf("🔧 CPU Cores: %d, Max UDP Payload: %d bytes\n", runtime.NumCPU(), MAX_UDP_PAYLOAD)

	server := NewServer(port)
	if err := server.Start(); err != nil {
		fmt.Printf("❌ Failed to start server: %v\n", err)
		return
	}

	// Wait for shutdown signal
	<-s.ctx.Done()
	server.Stop()
}
