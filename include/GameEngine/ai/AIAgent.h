/**
 * @file AIAgent.h
 * @brief Advanced AI agent system with multi-agent coordination
 * @author FoundryEngine Team
 * @date 2024
 * @version 2.0.0
 */

#pragma once

#include "../core/System.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>
#include <functional>

namespace FoundryEngine {

// Forward declarations
class Blackboard;
class MessageBus;
class GoalSystem;
class PersonalitySystem;

/**
 * @enum AgentState
 * @brief States an AI agent can be in
 */
enum class AgentState {
    Idle,           ///< Agent is waiting for tasks
    Planning,       ///< Agent is planning actions
    Executing,      ///< Agent is executing planned actions
    Collaborating,  ///< Agent is coordinating with other agents
    Learning,       ///< Agent is learning from experience
    Blocked,        ///< Agent is blocked and needs help
    Completed       ///< Agent has finished current task
};

/**
 * @enum AgentRole
 * @brief Specialized roles for different agent types
 */
enum class AgentRole {
    Scout,          ///< Exploration and gathering information
    Builder,        ///< Construction and creation tasks
    Defender,       ///< Protection and security
    Analyst,        ///< Data analysis and decision making
    Coordinator,    ///< Managing other agents
    Specialist,     ///< Domain-specific expert
    Generalist      ///< Versatile agent for various tasks
};

/**
 * @struct AgentCapabilities
 * @brief Defines what an agent is capable of doing
 */
struct AgentCapabilities {
    bool canMove = true;                    ///< Can physical movement
    bool canCommunicate = true;             ///< Can send/receive messages
    bool canSense = true;                   ///< Has sensory capabilities
    bool canLearn = true;                   ///< Can learn from experience
    bool canCreate = false;                 ///< Can create new entities
    bool canDestroy = false;                ///< Can destroy entities
    bool canReason = true;                  ///< Can perform reasoning
    bool canPlan = true;                    ///< Can plan complex actions
    bool canCollaborate = true;             ///< Can work with other agents
    float processingPower = 1.0f;           ///< Relative processing capability
    float memoryCapacity = 1.0f;            ///< Relative memory capacity
    std::vector<std::string> skills;        ///< List of specialized skills
};

/**
 * @struct AgentMessage
 * @brief Message structure for inter-agent communication
 */
struct AgentMessage {
    uint32_t senderId;                      ///< ID of sending agent
    uint32_t receiverId;                    ///< ID of receiving agent (0 for broadcast)
    std::string type;                       ///< Message type/category
    std::string content;                    ///< Message content (JSON format)
    float priority = 1.0f;                  ///< Message priority (0-1)
    std::chrono::steady_clock::time_point timestamp;  ///< When message was sent
    std::unordered_map<std::string, std::string> metadata;  ///< Additional data
};

/**
 * @class AIAgent
 * @brief Base class for intelligent AI agents with reasoning and collaboration capabilities
 */
class AIAgent {
public:
    /**
     * @brief Constructor
     * @param id Unique agent identifier
     * @param role Agent's specialized role
     * @param capabilities Agent's capabilities
     */
    AIAgent(uint32_t id, AgentRole role, const AgentCapabilities& capabilities);

    /**
     * @brief Virtual destructor
     */
    virtual ~AIAgent();

    /**
     * @brief Initialize the agent
     * @return true if initialization successful
     */
    virtual bool initialize();

    /**
     * @brief Update agent logic
     * @param deltaTime Time elapsed since last update
     */
    virtual void update(float deltaTime);

    /**
     * @brief Shutdown the agent
     */
    virtual void shutdown();

    /**
     * @brief Assign a task/goal to the agent
     * @param goalId ID of the goal
     * @param goal Description/parameters
     * @return true if goal accepted
     */
    virtual bool assignGoal(uint32_t goalId, const std::string& goal);

    /**
     * @brief Get current agent state
     * @return Current state
     */
    AgentState getState() const { return state_; }

    /**
     * @brief Get agent unique ID
     * @return Agent ID
     */
    uint32_t getId() const { return id_; }

    /**
     * @brief Get agent role
     * @return Agent role
     */
    AgentRole getRole() const { return role_; }

    /**
     * @brief Send message to another agent
     * @param message Message to send
     */
    void sendMessage(const AgentMessage& message);

    /**
     * @brief Receive message from another agent
     * @param message Received message
     */
    void receiveMessage(const AgentMessage& message);

    /**
     * @brief Request collaboration from another agent
     * @param otherAgentId ID of agent to collaborate with
     * @param task Description of collaborative task
     * @return true if collaboration initiated
     */
    bool requestCollaboration(uint32_t otherAgentId, const std::string& task);

    /**
     * @brief Accept collaboration request
     * @param requestorId ID of requesting agent
     * @return true if accepted
     */
    bool acceptCollaboration(uint32_t requestorId);

    /**
     * @brief Decline collaboration request
     * @param requestorId ID of requesting agent
     */
    void declineCollaboration(uint32_t requestorId);

    /**
     * @brief Get agent capabilities
     * @return Reference to capabilities
     */
    const AgentCapabilities& getCapabilities() const { return capabilities_; }

    /**
     * @brief Check if agent has specific skill
     * @param skill Skill name
     * @return true if agent has the skill
     */
    bool hasSkill(const std::string& skill) const;

    /**
     * @brief Learn new skill
     * @param skill Skill to learn
     * @param proficiency Initial proficiency level
     */
    void learnSkill(const std::string& skill, float proficiency = 0.1f);

    /**
     * @brief Get current task progress
     * @return Progress (0.0 to 1.0)
     */
    float getTaskProgress() const { return taskProgress_; }

    /**
     * @brief Get list of current collaborators
     * @return Vector of collaborating agent IDs
     */
    const std::vector<uint32_t>& getCollaborators() const { return collaborators_; }

    /**
     * @brief Get agent trust level towards another agent
     * @param otherAgentId Other agent ID
     * @return Trust level (0.0 to 1.0)
     */
    float getTrustLevel(uint32_t otherAgentId) const;

    /**
     * @brief Update trust level for another agent
     * @param otherAgentId Other agent ID
     * @param deltaTrust Change in trust level
     */
    void updateTrustLevel(uint32_t otherAgentId, float deltaTrust);

protected:
    /**
     * @brief Plan next actions based on goals and situation
     */
    virtual void planActions() = 0;

    /**
     * @brief Execute planned actions
     */
    virtual void executeActions();

    /**
     * @brief Learn from recent experiences
     */
    virtual void learnFromExperience();

    /**
     * @brief Handle incoming message
     * @param message Received message
     */
    virtual void handleMessage(const AgentMessage& message) = 0;

    /**
     * @brief Handle collaboration request
     * @param requestorId ID of requesting agent
     * @param task Task description
     */
    virtual void handleCollaborationRequest(uint32_t requestorId, const std::string& task) = 0;

    // Agent properties
    uint32_t id_;
    AgentRole role_;
    AgentCapabilities capabilities_;
    AgentState state_;
    std::string currentGoal_;
    uint32_t currentGoalId_;
    float taskProgress_;
    bool initialized_;

    // Skills and learning
    std::unordered_map<std::string, float> skills_;  ///< Skill -> proficiency mapping
    std::unordered_map<uint32_t, float> trustLevels_;  ///< Agent ID -> trust level mapping

    // Communication
    std::queue<AgentMessage> messageQueue_;
    std::mutex messageMutex_;
    std::vector<uint32_t> collaborators_;

    // Collaboration state
    std::atomic<bool> awaitingCollaborationResponse_;
    uint32_t pendingCollaborationRequestor_;

    // Timing
    std::chrono::steady_clock::time_point lastUpdateTime_;
    float updateFrequency_;
};

/**
 * @class CollaborativeAgentSystem
 * @brief Manages multiple AI agents with advanced multi-agent coordination
 */
class CollaborativeAgentSystem : public System {
public:
    /**
     * @struct SystemConfig
     * @brief Configuration for the collaborative agent system
     */
    struct SystemConfig {
        int maxAgents = 100;                    ///< Maximum number of agents
        int maxMessagesPerSecond = 1000;        ///< Message rate limiting
        float trustUpdateRate = 0.1f;           ///< Trust update frequency
        bool enableLearning = true;             ///< Enable agent learning
        bool enableCollaboration = true;        ///< Enable inter-agent collaboration
        std::string communicationProtocol = "json";  ///< Message format
    };

    CollaborativeAgentSystem();
    ~CollaborativeAgentSystem();

    bool initialize(const SystemConfig& config = SystemConfig{}) override;
    void shutdown() override;
    void update(float deltaTime) override;

    /**
     * @brief Create a new AI agent
     * @param role Agent role
     * @param capabilities Agent capabilities
     * @return Agent ID, or 0 if failed
     */
    uint32_t createAgent(AgentRole role, const AgentCapabilities& capabilities);

    /**
     * @brief Destroy an agent
     * @param agentId ID of agent to destroy
     */
    void destroyAgent(uint32_t agentId);

    /**
     * @brief Get agent by ID
     * @param agentId Agent ID
     * @return Pointer to agent, or nullptr if not found
     */
    AIAgent* getAgent(uint32_t agentId);

    /**
     * @brief Assign collective goal to multiple agents
     * @param agents List of agent IDs
     * @param goal Goal description
     * @return Goal ID for tracking
     */
    uint32_t assignCollectiveGoal(const std::vector<uint32_t>& agents, const std::string& goal);

    /**
     * @brief Send message between agents (internal system method)
     * @param message Message to send
     */
    void routeMessage(const AgentMessage& message);

    /**
     * @brief Get system statistics
     * @return Statistics about agent performance
     */
    std::unordered_map<std::string, float> getSystemStats() const;

    /**
     * @brief Enable/disable agent collaboration
     * @param enabled Whether to enable collaboration
     */
    void setCollaborationEnabled(bool enabled);

private:
    SystemConfig config_;
    std::unordered_map<uint32_t, std::unique_ptr<AIAgent>> agents_;
    std::queue<AgentMessage> messageQueue_;
    std::mutex systemMutex_;
    std::atomic<uint32_t> nextAgentId_{1};
    std::atomic<uint32_t> nextGoalId_{1};

    // Performance tracking
    uint32_t totalMessages_ = 0;
    uint32_t collaborationsInitiated_ = 0;
    uint32_t goalsCompleted_ = 0;
    float systemEfficiency_ = 0.0f;

    /**
     * @brief Process pending messages
     */
    void processMessages();

    /**
     * @brief Update agent collaborations
     */
    void updateCollaborations();

    /**
     * @brief Monitor and update system statistics
     */
    void updateSystemStats();
};

} // namespace FoundryEngine
