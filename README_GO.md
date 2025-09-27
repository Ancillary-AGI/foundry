# FoundryEngine Go Backend Server

This directory contains the Go backend server implementation for the FoundryEngine game engine, providing multiplayer networking, asset management, and cloud services.

## 🚀 Features

### Core Server Features

- **High-performance HTTP server** with Gorilla Mux
- **WebSocket support** for real-time communication
- **RESTful API** for game services
- **JWT authentication** and authorization
- **Rate limiting** and DDoS protection
- **CORS support** for cross-origin requests

### Game Services

- **Multiplayer matchmaking** and lobby management
- **Real-time game state synchronization**
- **Player authentication** and profile management
- **Leaderboards** and statistics tracking
- **Asset delivery** and CDN integration
- **Analytics** and telemetry collection

### Networking Features

- **WebSocket connections** for low-latency communication
- **Room-based architecture** for game sessions
- **Player presence** and status tracking
- **Message queuing** with Redis support
- **Load balancing** ready architecture

## 📋 Prerequisites

### System Requirements

- **Go 1.19+** installed
- **Redis** (optional, for advanced features)
- **PostgreSQL** (optional, for persistent storage)
- **4GB+ RAM** recommended
- **Modern CPU** with multiple cores

### Installation

1. **Install Go**
   ```bash
   # On Ubuntu/Debian
   sudo apt-get install golang-go

   # On macOS
   brew install go

   # On Windows
   # Download from https://golang.org/dl/
   ```

2. **Verify Installation**
   ```bash
   go version
   go mod tidy
   ```

## 🔧 Configuration

### Environment Variables

```bash
# Server Configuration
export GO_SERVER_PORT=8080
export GO_SERVER_HOST=localhost
export GO_SERVER_ENV=development

# Database Configuration
export DB_HOST=localhost
export DB_PORT=5432
export DB_USER=foundryengine
export DB_PASSWORD=your_password
export DB_NAME=foundryengine

# Redis Configuration
export REDIS_HOST=localhost
export REDIS_PORT=6379
export REDIS_PASSWORD=

# JWT Configuration
export JWT_SECRET=your_jwt_secret_key
export JWT_EXPIRY=24h

# CORS Configuration
export CORS_ALLOWED_ORIGINS=http://localhost:3000,https://yourdomain.com
```

### Configuration File

Create a `config.yaml` file:

```yaml
server:
  port: 8080
  host: "localhost"
  environment: "development"
  read_timeout: "30s"
  write_timeout: "30s"
  max_header_bytes: 1048576

database:
  host: "localhost"
  port: 5432
  user: "foundryengine"
  password: "your_password"
  dbname: "foundryengine"
  sslmode: "disable"
  max_open_conns: 25
  max_idle_conns: 25

redis:
  host: "localhost"
  port: 6379
  password: ""
  db: 0
  pool_size: 10

jwt:
  secret: "your_jwt_secret_key"
  expiry: "24h"

cors:
  allowed_origins:
    - "http://localhost:3000"
    - "https://yourdomain.com"
  allowed_methods:
    - "GET"
    - "POST"
    - "PUT"
    - "DELETE"
    - "OPTIONS"
  allowed_headers:
    - "Content-Type"
    - "Authorization"
    - "X-Requested-With"
```

## 🚀 Running the Server

### Development Mode

```bash
# Run with hot reload
go run server_go.go

# Or run with specific configuration
go run -config config.yaml server_go.go
```

### Production Mode

```bash
# Build the binary
go build -o foundryengine-server server_go.go

# Run the binary
./foundryengine-server

# Run with systemd (Linux)
sudo systemctl start foundryengine-server
```

### Docker Deployment

```dockerfile
FROM golang:1.19-alpine AS builder
WORKDIR /app
COPY . .
RUN go build -o foundryengine-server server_go.go

FROM alpine:latest
RUN apk --no-cache add ca-certificates
WORKDIR /root/
COPY --from=builder /app/foundryengine-server .
COPY --from=builder /app/config.yaml .
EXPOSE 8080
CMD ["./foundryengine-server"]
```

## 📡 API Endpoints

### Authentication

```
POST   /api/auth/login          - User login
POST   /api/auth/register       - User registration
POST   /api/auth/refresh        - Token refresh
POST   /api/auth/logout         - User logout
GET    /api/auth/profile        - Get user profile
```

### Game Services

```
GET    /api/games               - List available games
POST   /api/games               - Create new game
GET    /api/games/:id           - Get game details
PUT    /api/games/:id           - Update game
DELETE /api/games/:id           - Delete game
```

### Multiplayer

```
GET    /api/rooms               - List game rooms
POST   /api/rooms               - Create room
GET    /api/rooms/:id           - Get room details
POST   /api/rooms/:id/join      - Join room
POST   /api/rooms/:id/leave     - Leave room
DELETE /api/rooms/:id           - Delete room
```

### Leaderboards

```
GET    /api/leaderboards        - Get leaderboards
GET    /api/leaderboards/:game  - Get game leaderboards
POST   /api/leaderboards        - Submit score
```

### Assets

```
GET    /api/assets              - List assets
POST   /api/assets              - Upload asset
GET    /api/assets/:id          - Download asset
DELETE /api/assets/:id          - Delete asset
```

## 🔌 WebSocket Events

### Connection Events

```javascript
// Client connects
socket.emit('join', { roomId: 'room123', playerId: 'player456' });

// Server response
socket.on('joined', (data) => {
    console.log('Joined room:', data.roomId);
});

// Client disconnects
socket.emit('leave', { roomId: 'room123' });
```

### Game Events

```javascript
// Player actions
socket.emit('player_move', {
    playerId: 'player456',
    position: { x: 100, y: 200 },
    rotation: 45
});

// Game state updates
socket.on('game_state', (state) => {
    updateGameState(state);
});

// Chat messages
socket.emit('chat_message', {
    message: 'Hello everyone!',
    timestamp: Date.now()
});
```

## 🗄️ Database Schema

### Users Table

```sql
CREATE TABLE users (
    id SERIAL PRIMARY KEY,
    username VARCHAR(50) UNIQUE NOT NULL,
    email VARCHAR(100) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_login TIMESTAMP,
    is_active BOOLEAN DEFAULT true
);
```

### Games Table

```sql
CREATE TABLE games (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    description TEXT,
    max_players INTEGER DEFAULT 4,
    min_players INTEGER DEFAULT 2,
    game_type VARCHAR(50) NOT NULL,
    created_by INTEGER REFERENCES users(id),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    is_active BOOLEAN DEFAULT true
);
```

### Rooms Table

```sql
CREATE TABLE rooms (
    id SERIAL PRIMARY KEY,
    game_id INTEGER REFERENCES games(id),
    name VARCHAR(100) NOT NULL,
    host_id INTEGER REFERENCES users(id),
    max_players INTEGER DEFAULT 4,
    current_players INTEGER DEFAULT 0,
    status VARCHAR(20) DEFAULT 'waiting',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    started_at TIMESTAMP,
    ended_at TIMESTAMP
);
```

## 🔒 Security Features

### Authentication

- **JWT tokens** with configurable expiry
- **Password hashing** with bcrypt
- **Rate limiting** on login attempts
- **Session management** with Redis

### Authorization

- **Role-based access control** (RBAC)
- **API key authentication** for services
- **CORS protection** with whitelist
- **Input validation** and sanitization

### DDoS Protection

- **Rate limiting** per IP address
- **Request throttling** by endpoint
- **Connection limits** per client
- **Automatic banning** of malicious IPs

## 📊 Monitoring & Analytics

### Health Checks

```
GET /health              - Server health status
GET /metrics             - Prometheus metrics
GET /api/status          - API status endpoint
```

### Logging

- **Structured logging** with JSON format
- **Request/response logging** middleware
- **Error tracking** with stack traces
- **Performance monitoring** with timing

### Analytics

- **Player behavior** tracking
- **Game session** analytics
- **Performance metrics** collection
- **Custom event** tracking

## 🧪 Testing

### Unit Tests

```bash
# Run all tests
go test ./...

# Run tests with coverage
go test -cover ./...

# Run specific test package
go test ./handlers/...
```

### Integration Tests

```bash
# Run integration tests
go test -tags=integration ./tests/...

# Run with test database
go test -tags=integration -db-test ./tests/...
```

### Load Testing

```bash
# Install hey for load testing
go install github.com/rakyll/hey@latest

# Run load test
hey -n 1000 -c 10 http://localhost:8080/api/games
```

## 🚀 Deployment

### Production Checklist

- [ ] **Environment variables** configured
- [ ] **Database** migrations run
- [ ] **SSL/TLS** certificates installed
- [ ] **Load balancer** configured
- [ ] **Monitoring** setup (Prometheus, Grafana)
- [ ] **Logging** aggregation configured
- [ ] **Backup** strategy implemented
- [ ] **Security** headers configured

### Docker Compose

```yaml
version: '3.8'
services:
  foundryengine:
    build: .
    ports:
      - "8080:8080"
    environment:
      - GO_SERVER_ENV=production
      - DB_HOST=postgres
      - REDIS_HOST=redis
    depends_on:
      - postgres
      - redis

  postgres:
    image: postgres:13
    environment:
      - POSTGRES_DB=foundryengine
      - POSTGRES_USER=foundryengine
      - POSTGRES_PASSWORD=your_password

  redis:
    image: redis:6-alpine
    command: redis-server --appendonly yes
```

## 🐛 Troubleshooting

### Common Issues

1. **Port already in use**
   ```bash
   # Check what's using the port
   lsof -i :8080
   # Or use a different port
   export GO_SERVER_PORT=8081
   ```

2. **Database connection failed**
   - Verify database credentials
   - Check database server status
   - Ensure firewall allows connections

3. **High memory usage**
   - Monitor Redis memory usage
   - Check for memory leaks in game loops
   - Configure appropriate connection limits

4. **WebSocket connection issues**
   - Check CORS configuration
   - Verify WebSocket support in proxy/load balancer
   - Monitor connection timeouts

### Performance Tuning

1. **Database optimization**
   - Add appropriate indexes
   - Use connection pooling
   - Optimize query performance

2. **Redis optimization**
   - Configure appropriate memory limits
   - Use Redis clustering for high load
   - Monitor eviction policies

3. **Server tuning**
   - Adjust worker pool sizes
   - Configure appropriate timeouts
   - Use HTTP/2 where possible

## 📚 Resources

- [Go Official Documentation](https://golang.org/doc/)
- [Gorilla WebSocket](https://github.com/gorilla/websocket)
- [Gorilla Mux](https://github.com/gorilla/mux)
- [JWT Go](https://github.com/golang-jwt/jwt)
- [Redis Go Client](https://github.com/go-redis/redis)

## 🤝 Contributing

### Development Setup

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new functionality
5. Run the full test suite
6. Submit a pull request

### Code Style

- Follow Go conventions and best practices
- Use `gofmt` for code formatting
- Add comprehensive documentation
- Include unit tests for new features

---

**FoundryEngine Go Server** - Scalable Backend for Modern Games
