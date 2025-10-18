// TypeScript backend integration with Go APIs
// This file handles server-side game logic and API calls

import { NetworkManager } from './engine';

// Backend server configuration
interface ServerConfig {
    host: string;
    port: number;
    secure: boolean;
    apiVersion: string;
}

class GameBackend {
    private config: ServerConfig;
    private connected: boolean = false;
    private playerId: string | null = null;
    private gameState: any = {};

    constructor(config: Partial<ServerConfig> = {}) {
        this.config = {
            host: config.host || 'localhost',
            port: config.port || 8080,
            secure: config.secure || false,
            apiVersion: 'v1',
            ...config
        };

        this.initializeBackend();
    }

    private initializeBackend(): void {
        // Set up network event handlers
        NetworkManager.onMessage((data: any) => {
            this.handleServerMessage(data);
        });

        // Connect to Go backend server
        this.connect();
    }

    private async connect(): Promise<void> {
        try {
            const url = `${this.config.secure ? 'wss' : 'ws'}://${this.config.host}:${this.config.port}/game`;
            await NetworkManager.connect(url);
            this.connected = true;
            console.log('Connected to game backend server');

            // Authenticate and join game
            await this.authenticate();
        } catch (error) {
            console.error('Failed to connect to backend:', error);
            // Retry connection after delay
            setTimeout(() => this.connect(), 5000);
        }
    }

    private async authenticate(): Promise<void> {
        try {
            // Get authentication token from local storage or generate guest token
            const token = this.getAuthToken();

            const response = await NetworkManager.post('/auth/login', {
                token: token,
                clientVersion: '1.0.0',
                platform: 'web'
            });

            if (response.success) {
                this.playerId = response.playerId;
                console.log('Authenticated with backend:', this.playerId);
            } else {
                throw new Error(response.error || 'Authentication failed');
            }
        } catch (error) {
            console.error('Authentication failed:', error);
            // Continue with guest mode
            this.playerId = 'guest_' + Date.now();
        }
    }

    private handleServerMessage(data: any): void {
        switch (data.type) {
            case 'game_state_update':
                this.handleGameStateUpdate(data.payload);
                break;
            case 'player_joined':
                this.handlePlayerJoined(data.payload);
                break;
            case 'player_left':
                this.handlePlayerLeft(data.payload);
                break;
            case 'chat_message':
                this.handleChatMessage(data.payload);
                break;
            case 'achievement_unlocked':
                this.handleAchievementUnlocked(data.payload);
                break;
            case 'leaderboard_update':
                this.handleLeaderboardUpdate(data.payload);
                break;
            default:
                console.log('Unknown message type:', data.type);
        }
    }

    private handleGameStateUpdate(payload: any): void {
        this.gameState = { ...this.gameState, ...payload };
        // Emit event for game to handle
        this.emit('gameStateUpdate', payload);
    }

    private handlePlayerJoined(payload: any): void {
        console.log('Player joined:', payload.playerName);
        this.emit('playerJoined', payload);
    }

    private handlePlayerLeft(payload: any): void {
        console.log('Player left:', payload.playerName);
        this.emit('playerLeft', payload);
    }

    private handleChatMessage(payload: any): void {
        console.log(`Chat [${payload.playerName}]: ${payload.message}`);
        this.emit('chatMessage', payload);
    }

    private handleAchievementUnlocked(payload: any): void {
        console.log('Achievement unlocked:', payload.achievementName);
        this.emit('achievementUnlocked', payload);
    }

    private handleLeaderboardUpdate(payload: any): void {
        this.emit('leaderboardUpdate', payload);
    }

    // Public API methods

    public async createGameRoom(settings: GameRoomSettings): Promise<string> {
        const response = await NetworkManager.post('/games/create', {
            playerId: this.playerId,
            settings: settings
        });

        if (response.success) {
            return response.roomId;
        } else {
            throw new Error(response.error || 'Failed to create game room');
        }
    }

    public async joinGameRoom(roomId: string): Promise<void> {
        const response = await NetworkManager.post(`/games/${roomId}/join`, {
            playerId: this.playerId
        });

        if (!response.success) {
            throw new Error(response.error || 'Failed to join game room');
        }
    }

    public async leaveGameRoom(): Promise<void> {
        const response = await NetworkManager.post('/games/leave', {
            playerId: this.playerId
        });

        if (!response.success) {
            throw new Error(response.error || 'Failed to leave game room');
        }
    }

    public async sendPlayerAction(action: PlayerAction): Promise<void> {
        NetworkManager.send({
            type: 'player_action',
            playerId: this.playerId,
            action: action,
            timestamp: Date.now()
        });
    }

    public async sendChatMessage(message: string): Promise<void> {
        NetworkManager.send({
            type: 'chat_message',
            playerId: this.playerId,
            message: message,
            timestamp: Date.now()
        });
    }

    public async getLeaderboard(gameMode: string): Promise<LeaderboardEntry[]> {
        const response = await NetworkManager.get('/leaderboard', {
            gameMode: gameMode,
            limit: 100
        });

        if (response.success) {
            return response.entries;
        } else {
            throw new Error(response.error || 'Failed to get leaderboard');
        }
    }

    public async savePlayerProgress(progress: PlayerProgress): Promise<void> {
        const response = await NetworkManager.post('/player/progress', {
            playerId: this.playerId,
            progress: progress
        });

        if (!response.success) {
            throw new Error(response.error || 'Failed to save progress');
        }
    }

    public async loadPlayerProgress(): Promise<PlayerProgress> {
        const response = await NetworkManager.get('/player/progress', {
            playerId: this.playerId
        });

        if (response.success) {
            return response.progress;
        } else {
            throw new Error(response.error || 'Failed to load progress');
        }
    }

    public async getPlayerStats(): Promise<PlayerStats> {
        const response = await NetworkManager.get('/player/stats', {
            playerId: this.playerId
        });

        if (response.success) {
            return response.stats;
        } else {
            throw new Error(response.error || 'Failed to get player stats');
        }
    }

    public async reportBug(description: string, stackTrace?: string): Promise<void> {
        const response = await NetworkManager.post('/bugs/report', {
            playerId: this.playerId,
            description: description,
            stackTrace: stackTrace,
            userAgent: navigator.userAgent,
            timestamp: Date.now()
        });

        if (!response.success) {
            throw new Error(response.error || 'Failed to report bug');
        }
    }

    public getGameState(): any {
        return this.gameState;
    }

    public isConnected(): boolean {
        return this.connected;
    }

    public getPlayerId(): string | null {
        return this.playerId;
    }

    // Event system
    private eventListeners: { [event: string]: Function[] } = {};

    public on(event: string, callback: Function): void {
        if (!this.eventListeners[event]) {
            this.eventListeners[event] = [];
        }
        this.eventListeners[event].push(callback);
    }

    public off(event: string, callback: Function): void {
        const listeners = this.eventListeners[event];
        if (listeners) {
            const index = listeners.indexOf(callback);
            if (index > -1) {
                listeners.splice(index, 1);
            }
        }
    }

    private emit(event: string, ...args: any[]): void {
        const listeners = this.eventListeners[event];
        if (listeners) {
            listeners.forEach(callback => {
                try {
                    callback(...args);
                } catch (error) {
                    console.error('Error in event listener:', error);
                }
            });
        }
    }

    private getAuthToken(): string {
        // Get token from local storage or generate guest token
        const stored = localStorage.getItem('foundry_auth_token');
        if (stored) {
            return stored;
        }

        // Generate guest token
        const guestToken = 'guest_' + Date.now() + '_' + Math.random().toString(36).substr(2, 9);
        localStorage.setItem('foundry_auth_token', guestToken);
        return guestToken;
    }
}

// Data types
interface GameRoomSettings {
    name: string;
    maxPlayers: number;
    gameMode: string;
    isPrivate: boolean;
    password?: string;
    timeLimit?: number;
    difficulty?: string;
}

interface PlayerAction {
    type: string;
    data: any;
    timestamp: number;
}

interface PlayerProgress {
    level: number;
    experience: number;
    achievements: string[];
    unlockedItems: string[];
    statistics: { [key: string]: number };
}

interface PlayerStats {
    gamesPlayed: number;
    gamesWon: number;
    totalScore: number;
    bestScore: number;
    playTime: number;
    rank: number;
}

interface LeaderboardEntry {
    playerId: string;
    playerName: string;
    score: number;
    rank: number;
    timestamp: number;
}

// Export singleton instance
export const gameBackend = new GameBackend();

// Export class for custom configurations
export { GameBackend };