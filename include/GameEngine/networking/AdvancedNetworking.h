#ifndef NEUTRAL_GAMEENGINE_ADVANCED_NETWORKING_H
#define NEUTRAL_GAMEENGINE_ADVANCED_NETWORKING_H

#include <vector>
#include <unordered_map>
#include <queue>
#include <memory>
#include <chrono>
#include "../../math/Vector3.h"
#include "../../math/Quaternion.h"
#include "../System.h"

namespace NeutralGameEngine {

// Network Reliability System
enum class PacketType {
    UNRELIABLE,
    RELIABLE_ORDERED,
    RELIABLE_UNORDERED,
    STATE_SYNC
};

class NetworkPacket {
public:
    PacketType type;
    uint32_t sequenceNumber;
    uint16_t payloadSize;
    std::vector<uint8_t> payload;
    uint64_t timestamp;
    uint32_t ackBits;

    // Compression support
    bool compressed;
    uint16_t uncompressedSize;

    // Entity state delta compression
    struct DeltaCompression {
        std::vector<uint8_t> baseline;
        std::vector<uint8_t> delta;
        uint32_t baselineSeq;
    };

    DeltaCompression deltaCompressed;
};

// Entity State Synchronization
class EntityStateSync {
public:
    struct NetworkedEntity {
        uint32_t entityId;
        uint32_t ownerId;          // Owning client ID
        Vector3 position;
        Quaternion orientation;
        Vector3 linearVelocity;
        Vector3 angularVelocity;
        uint64_t lastUpdateTime;
        bool authoritative;        // True if this client has authority

        // Prediction history for reconciliation
        struct StateSnapshot {
            uint64_t timestamp;
            Vector3 position;
            Quaternion orientation;
            Vector3 velocity;
            int inputSequence;
        };
        std::vector<StateSnapshot> predictionHistory;
    };

    struct SyncRegion {
        Vector3 center;
        float radius;
        std::vector<uint32_t> entities;
        float updateFrequency;     // Hz
    };

    std::unordered_map<uint32_t, NetworkedEntity> networkedEntities;
    std::vector<SyncRegion> syncRegions;

    // Delta compression for state updates
    NetworkPacket::DeltaCompression compressStateDelta(const NetworkedEntity& current,
                                                      const NetworkedEntity& baseline);

    // Interest management
    void updateSyncRegions(const Vector3& viewPosition);
    std::vector<uint32_t> getRelevantEntities(const Vector3& position, float viewDistance);
};

// Client-side Prediction and Reconciliation
class ClientPrediction {
public:
    struct PredictedState {
        uint32_t sequenceNumber;
        Vector3 position;
        Quaternion orientation;
        Vector3 velocity;
        uint64_t timestamp;
        int serverAck;        // Last acknowledged server state
    };

    struct InputCommand {
        int sequenceNumber;
        Vector3 movementInput;
        Vector3 lookInput;
        bool jumpPressed;
        uint64_t timestamp;

        // Additional inputs for different systems
        std::vector<uint8_t> customInputs;
    };

    std::queue<InputCommand> pendingInputs;
    std::vector<PredictedState> predictionHistory;
    uint32_t nextSequenceNumber = 0;

    // Prediction for different physics systems
    void predictRigidBody(uint32_t entityId, const Vector3& acceleration, float dt);
    void predictCharacter(uint32_t entityId, const Vector3& inputDirection, bool jumping, float dt);
    void predictVehicle(uint32_t entityId, float throttle, float steering, float dt);

    // Reconciliation with server state
    void reconcileState(const EntityStateSync::NetworkedEntity& serverState);

    // Rewind and replay for corrections
    void rewindAndReplay(int correctionSequence);
};

// RELIABLE ORDERED                                                 // last ack received
    struct ReliabilityLayer {
        ReliabilityLayer();

        uint32_t localSequence;         // Sequence number for outgoing packets
        uint32_t remoteSequence;        // Highest sequence number received
        uint32_t ackBits;               // Bitfield of acknowledged packets
        std::unordered_map<uint32_t, NetworkPacket> sentPackets;  // Pending reliable packets
        std::vector<uint32_t> receivedSequences;                  // For duplicate detection

        // Send reliable packet
        bool sendReliable(NetworkPacket& packet, int resendTimeoutMs);

        // Process incoming packet
        void processPacket(const NetworkPacket& packet, std::vector<NetworkPacket>& ackedPackets);

        // Calculate round trip time
        void updateRTT(uint32_t packetSequence, uint64_t sendTime);

        float averageRTT;
        float packetLossRate;
    } reliability;

// Interest Management with Spatial Partitioning
class InterestManagement {
public:
    struct AABB {
        Vector3 min, max;

        bool intersects(const AABB& other) const;
        bool contains(const Vector3& point) const;
    };

    struct InterestCell {
        AABB bounds;
        std::unordered_set<uint32_t> entities;
        std::unordered_set<uint32_t> interestedClients;
        uint32_t cellId;

        // Relevance metrics
        float priority;        // Based on activity or importance
        float updateFrequency; // How often this cell needs updates
    };

    std::vector<InterestCell> spatialGrid;
    static const int GRID_SIZE = 64;  // 64x64x64 grid

    // Assign entities to cells
    void assignEntity(uint32_t entityId, const AABB& entityBounds);

    // Update client interest
    std::vector<uint32_t> getClientInterestList(uint32_t clientId, const Vector3& clientPosition,
                                              const Vector3& viewDirection, float viewRadius);

    // Prioritized updates based on distance and relevance
    void prioritizeUpdates(const Vector3& clientPos,
                          std::vector<uint32_t>& highPriorityEntities,
                          std::vector<uint32_t>& mediumPriorityEntities,
                          std::vector<uint32_t>& lowPriorityEntities);
};

// NAT Punch-through and Relay Systems
class NATTraversal {
public:
    struct STUNServer {
        std::string host;
        int port;
        bool active;
    };

    struct RelayCandidate {
        std::string address;
        int port;
        std::string region;
        float latency;
        bool available;
    };

    std::vector<STUNServer> stunServers;
    std::vector<RelayCandidate> relayCandidates;

    // STUN binding request
    bool performSTUNBinding(const STUNServer& server, std::string& publicAddress, int& publicPort);

    // NAT type detection
    enum class NATType {
        OPEN_INTERNET,
        FULL_CONE,
        RESTRICTED_CONE,
        PORT_RESTRICTED_CONE,
        SYMMETRIC
    };

    NATType detectNATType();

    // Hole punching for P2P connection
    bool initiateHolePunch(uint32_t targetClientId, const std::string& targetPublicAddr, int targetPort);

    // Relay server fallback
    RelayCandidate selectBestRelay(const std::vector<RelayCandidate>& candidates, const std::string& clientRegion);
};

// Anti-cheat with Machine Learning Detection
class AntiCheatSystem {
public:
    struct PlayerStatistics {
        uint32_t playerId;
        std::vector<float> movementSpeeds;
        std::vector<Vector3> positionHistory;
        std::vector<float> reactionTimes;

        // Physics-based detections
        float averageSpeed;
        float maxSpeed;
        float speedVariance;
        int teleportDetections;
        int wallHackFlags;

        // Pattern recognition
        std::vector<std::string> detectedPatterns;
    };

    std::unordered_map<uint32_t, PlayerStatistics> playerStats;
    std::unordered_map<uint32_t, std::vector<float>> movementModel; // ML movement model

    // Statistical anomaly detection
    bool detectSpeedHack(uint32_t playerId, const Vector3& newPosition, float deltaTime);

    // Physics validation
    bool validatePhysics(uint32_t playerId, const Vector3& position, const Vector3& velocity,
                        const Vector3& inputCommand);

    // Machine learning pattern recognition
    struct MLFeatures {
        float positionEntropy;    // Randomness in movement
        float inputConsistency;   // Human-like input patterns
        float reactionTime;       // Response to game events
        float pathEfficiency;     // Navigation effectiveness
        std::vector<float> customFeatures;
    };

    float computeAnomalyScore(const MLFeatures& features, const std::vector<float>& trainedModel);

    // Automated ban system
    enum class ViolationLevel { WARNING, SUSPICIOUS, CHEATING, BANNED };
    void reportViolation(uint32_t playerId, ViolationLevel level, const std::string& evidence);
};

// Advanced Networking System Orchestrator
class NetworkGameEngine : public System {
public:
    EntityStateSync stateSync;
    ClientPrediction prediction;
    InterestManagement interestManager;
    NATTraversal natTraversal;
    AntiCheatSystem antiCheat;

    // Matchmaking variables
    struct MatchMakingCriteria {
        std::string gameMode;
        std::string region;
        int maxPlayers;
        int minSkillLevel;
        int maxSkillLevel;
        std::vector<std::string> requiredFeatures;
    };

    struct PlayerProfile {
        uint32_t playerId;
        int skillRating;
        std::string region;
        std::vector<std::string> capabilities;
        uint64_t lastActive;
    };

    // Player matchmaking with skill-based algorithms
    std::vector<std::pair<uint32_t, uint32_t>> findMatches(const MatchMakingCriteria& criteria,
                                                          const std::vector<PlayerProfile>& players);

    // Skill rating system (Elo-like)
    int calculateNewRating(int currentRating, int opponentRating, bool won, float gameDuration);

    // QoS-aware server selection
    struct ServerInfo {
        std::string address;
        int port;
        std::string region;
        float latency;
        float packetLoss;
        int currentLoad;
        int maxCapacity;
    };

    ServerInfo selectOptimalServer(const std::string& clientRegion,
                                 const std::vector<ServerInfo>& servers);

    // In-game economy simulation
    struct VirtualEconomy {
        std::unordered_map<uint32_t, double> playerBalances;
        std::vector<std::string> tradableItems;
        std::unordered_map<std::string, double> itemPrices;

        // Market simulation with supply/demand
        double getMarketPrice(const std::string& item, int quantity);
        bool executeTrade(uint32_t buyerId, uint32_t sellerId, const std::string& item, int quantity, double price);
    } economy;

    // Guild/clan hierarchical systems
    struct Guild {
        uint32_t guildId;
        std::string name;
        uint32_t leaderId;
        int level;
        double treasury;
        std::vector<uint32_t> members;

        struct Permission {
            uint32_t memberId;
            std::vector<std::string> permissions; // "invite", "kick", "manage_treasury", etc.
        };

        std::vector<Permission> memberPermissions;
    };

    std::unordered_map<uint32_t, Guild> guilds;

    // Voice chat with spatial audio
    struct VoiceChannel {
        uint32_t channelId;
        std::string name;
        std::vector<uint32_t> participants;
        bool spatialAudio;     // 3D positional audio
        float volume;
        std::unordered_map<uint32_t, Vector3> participantPositions;
    };

    std::vector<VoiceChannel> voiceChannels;

    // Spatial audio processing for voice chat
    struct SpatialAudioProcessor {
        Vector3 listenerPosition;
        Quaternion listenerOrientation;
        float masterVolume;

        // HRTF-based 3D audio
        void processSpatialAudio(float* leftBuffer, float* rightBuffer,
                                size_t bufferSize, const Vector3& soundPosition);

        // Occlusion and obstruction
        float calculateAudioOcclusion(const Vector3& soundSource, const Vector3& listener);
    } spatialAudio;

    // Network optimization based on available bandwidth
    struct BandwidthManager {
        float availableBandwidthMbps;
        float currentUsage;
        std::unordered_map<PacketType, float> priorityWeights;
        std::queue<NetworkPacket> packetQueue;

        // Adaptive quality based on bandwidth
        void adjustQuality(float bandwidthDelta);
        bool canSendPacket(const NetworkPacket& packet);
        void prioritizePackets();
    } bandwidthManager;

    void initialize();
    void update(float dt);
};
};
};

#endif // NEUTRAL_GAMEENGINE_ADVANCED_NETWORKING_H
