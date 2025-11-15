/**
 * @file AIAgent.cpp
 * @brief Implementation of advanced AI agent system with multi-agent coordination
 */

#include "GameEngine/ai/AIAgent.h"
#include <algorithm>
#include <random>
#include <iostream>
#include <thread>
#include <chrono>
#include <queue>
#include <mutex>
#include <unordered_map>

namespace FoundryEngine {

// AIAgent implementation

AIAgent::AIAgent(uint32_t id, AgentRole role, const AgentCapabilities& capabilities)
    : id_(id),
      role_(role),
      capabilities_(capabilities),
      state_(AgentState::Idle),
      taskProgress_(0.0f),
      initialized_(false),
      updateFrequency_(0.1f),
      awaitingCollaborationResponse_(false),
      pendingCollaborationRequestor_(0) {
    lastUpdateTime_ = std::chrono::steady_clock::now();
}

AIAgent::~AIAgent() {
    shutdown();
}

bool AIAgent::initialize() {
    if (initialized_) {
        return true;
    }

    // Initialize skills based on role and capabilities
    switch (role_) {
        case AgentRole::Scout:
            skills_["exploration"] = 0.8f;
            skills_["surveillance"] = 0.7f;
            skills_["navigation"] = 0.6f;
            break;

        case AgentRole::Builder:
            skills_["construction"] = 0.9f;
            skills_["resource_management"] = 0.7f;
            skills_["blueprint_design"] = 0.6f;
            break;

        case AgentRole::Defender:
            skills_["combat"] = 0.9f;
            skills_["strategy"] = 0.7f;
            skills_["threat_analysis"] = 0.8f;
            break;

        case AgentRole::Analyst:
            skills_["data_analysis"] = 0.9f;
            skills_["pattern_recognition"] = 0.8f;
            skills_["prediction"] = 0.7f;
            break;

        case AgentRole::Coordinator:
            skills_["leadership"] = 0.9f;
            skills_["communication"] = 0.8f;
            skills_["task_distribution"] = 0.8f;
            break;

        case AgentRole::Specialist:
            // Skills set by capability configuration
            for (const auto& skill : capabilities_.skills) {
                skills_[skill] = 0.9f; // High proficiency for specialists
            }
            break;

        case AgentRole::Generalist:
        default:
            skills_["general_task"] = 0.6f;
            skills_["adaptability"] = 0.7f;
            break;
    }

    initialized_ = true;
    state_ = AgentState::Idle;

    std::cout << "[AIAgent] Agent " << id_ << " initialized with role: "
              << static_cast<int>(role_) << std::endl;

    return true;
}

void AIAgent::update(float deltaTime) {
    if (!initialized_) {
        return;
    }

    auto now = std::chrono::steady_clock::now();
    auto timeSinceLastUpdate = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - lastUpdateTime_).count() / 1000.0f;

    // Rate limit updates
    if (timeSinceLastUpdate < updateFrequency_) {
        return;
    }

    lastUpdateTime_ = now;

    // Process incoming messages
    processMessages();

    // Handle collaboration responses
    if (awaitingCollaborationResponse_) {
        // Timeout after 10 seconds
        if (timeSinceLastUpdate > 10.0f) {
            awaitingCollaborationResponse_ = false;
            std::cout << "[AIAgent] Agent " << id_ << " collaboration timeout" << std::endl;
        }
    }

    // Execute current goal if active
    if (!currentGoal_.empty() && state_ != AgentState::Completed) {
        planActions();
        executeActions();
        learnFromExperience();

        // Simulate progress
        taskProgress_ = std::min(1.0f, taskProgress_ + deltaTime * 0.1f);

        if (taskProgress_ >= 1.0f) {
            state_ = AgentState::Completed;
            std::cout << "[AIAgent] Agent " << id_ << " completed goal: " << currentGoal_ << std::endl;
        }
    }
}

void AIAgent::shutdown() {
    if (!initialized_) {
        return;
    }

    // Clear collaborators
    collaborators_.clear();

    // Clear message queue
    {
        std::lock_guard<std::mutex> lock(messageMutex_);
        while (!messageQueue_.empty()) {
            messageQueue_.pop();
        }
    }

    initialized_ = false;
    state_ = AgentState::Idle;

    std::cout << "[AIAgent] Agent " << id_ << " shutdown" << std::endl;
}

bool AIAgent::assignGoal(uint32_t goalId, const std::string& goal) {
    if (!initialized_ || goal.empty()) {
        return false;
    }

    currentGoal_ = goal;
    currentGoalId_ = goalId;
    state_ = AgentState::Planning;
    taskProgress_ = 0.0f;

    std::cout << "[AIAgent] Agent " << id_ << " assigned goal: " << goal << std::endl;
    return true;
}

void AIAgent::sendMessage(const AgentMessage& message) {
    if (!initialized_ || !capabilities_.canCommunicate) {
        return;
    }

    std::cout << "[AIAgent] Agent " << id_ << " sending message to " << message.receiverId
              << " type: " << message.type << std::endl;

    // This would normally send to the CollaborativeAgentSystem
    // For now, just log it
}

void AIAgent::receiveMessage(const AgentMessage& message) {
    if (!initialized_ || !capabilities_.canCommunicate) {
        return;
    }

    std::lock_guard<std::mutex> lock(messageMutex_);
    messageQueue_.push(message);
}

bool AIAgent::requestCollaboration(uint32_t otherAgentId, const std::string& task) {
    if (!initialized_ || !capabilities_.canCollaborate) {
        return false;
    }

    if (awaitingCollaborationResponse_) {
        return false; // Already waiting for a response
    }

    pendingCollaborationRequestor_ = otherAgentId;
    awaitingCollaborationResponse_ = true;

    AgentMessage msg;
    msg.senderId = id_;
    msg.receiverId = otherAgentId;
    msg.type = "collaboration_request";
    msg.content = task;
    msg.priority = 0.8f;
    msg.timestamp = std::chrono::steady_clock::now();

    sendMessage(msg);

    std::cout << "[AIAgent] Agent " << id_ << " requesting collaboration from agent "
              << otherAgentId << " for task: " << task << std::endl;

    return true;
}

bool AIAgent::acceptCollaboration(uint32_t requestorId) {
    if (!initialized_ || !capabilities_.canCollaborate) {
        return false;
    }

    // Add to collaborators
    if (std::find(collaborators_.begin(), collaborators_.end(), requestorId) == collaborators_.end()) {
        collaborators_.push_back(requestorId);
    }

    // Send acceptance response
    AgentMessage msg;
    msg.senderId = id_;
    msg.receiverId = requestorId;
    msg.type = "collaboration_accepted";
    msg.content = "{\"accepted\": true}";
    msg.priority = 0.7f;
    msg.timestamp = std::chrono::steady_clock::now();

    sendMessage(msg);

    // Increase trust
    updateTrustLevel(requestorId, 0.1f);

    std::cout << "[AIAgent] Agent " << id_ << " accepted collaboration from agent " << requestorId << std::endl;
    return true;
}

void AIAgent::declineCollaboration(uint32_t requestorId) {
    if (!initialized_) {
        return;
    }

    // Send decline response
    AgentMessage msg;
    msg.senderId = id_;
    msg.receiverId = requestorId;
    msg.type = "collaboration_declined";
    msg.content = "{\"accepted\": false}";
    msg.priority = 0.7f;
    msg.timestamp = std::chrono::steady_clock::now();

    sendMessage(msg);

    // Decrease trust slightly
    updateTrustLevel(requestorId, -0.05f);

    std::cout << "[AIAgent] Agent " << id_ << " declined collaboration from agent " << requestorId << std::endl;
}

bool AIAgent::hasSkill(const std::string& skill) const {
    if (!capabilities_.canLearn) {
        return false;
    }

    return skills_.find(skill) != skills_.end();
}

void AIAgent::learnSkill(const std::string& skill, float proficiency) {
    if (!capabilities_.canLearn) {
        return;
    }

    skills_[skill] = std::clamp(proficiency, 0.0f, 1.0f);
    std::cout << "[AIAgent] Agent " << id_ << " learned skill: " << skill
              << " (proficiency: " << proficiency << ")" << std::endl;
}

void AIAgent::processMessages() {
    std::lock_guard<std::mutex> lock(messageMutex_);

    while (!messageQueue_.empty()) {
        AgentMessage msg = messageQueue_.front();
        messageQueue_.pop();

        handleMessage(msg);
    }
}

void AIAgent::executeActions() {
    if (state_ == AgentState::Planning) {
        state_ = AgentState::Executing;
    }

    // Default implementation - override in derived classes
}

void AIAgent::learnFromExperience() {
    // Default learning implementation - could be enhanced with reinforcement learning
    if (capabilities_.canLearn && !currentGoal_.empty()) {
        // Slight improvement in related skills
        for (auto& skill : skills_) {
            skill.second = std::min(1.0f, skill.second + 0.001f);
        }
    }
}

float AIAgent::getTrustLevel(uint32_t otherAgentId) const {
    auto it = trustLevels_.find(otherAgentId);
    return it != trustLevels_.end() ? it->second : 0.5f; // Default neutral trust
}

void AIAgent::updateTrustLevel(uint32_t otherAgentId, float deltaTrust) {
    float& trust = trustLevels_[otherAgentId];
    trust = std::clamp(trust + deltaTrust, 0.0f, 1.0f);
}

// Concrete Agent Implementations

/**
 * @class ScoutAgent
 * @brief Agent specialized in exploration and information gathering
 */
class ScoutAgent : public AIAgent {
public:
    ScoutAgent(uint32_t id, const AgentCapabilities& capabilities)
        : AIAgent(id, AgentRole::Scout, capabilities),
          explorationRadius_(50.0f),
          discoveredLocations_(0) {}

protected:
    void planActions() override {
        if (currentGoal_.find("explore") != std::string::npos) {
            // Plan exploration routes
            std::cout << "[ScoutAgent] Agent " << getId() << " planning exploration routes" << std::endl;
        } else if (currentGoal_.find("surveil") != std::string::npos) {
            // Plan surveillance positions
            std::cout << "[ScoutAgent] Agent " << getId() << " planning surveillance" << std::endl;
        }
    }

    void handleMessage(const AgentMessage& message) override {
        if (message.type == "collaboration_request") {
            handleCollaborationRequest(message.senderId, message.content);
        } else if (message.type == "location_data") {
            // Process shared location data
            discoveredLocations_++;
        }
    }

    void handleCollaborationRequest(uint32_t requestorId, const std::string& task) override {
        // Scout agents are willing to collaborate on exploration tasks
        if (task.find("explore") != std::string::npos || getTrustLevel(requestorId) > 0.7f) {
            acceptCollaboration(requestorId);
        } else {
            declineCollaboration(requestorId);
        }
    }

private:
    float explorationRadius_;
    int discoveredLocations_;
};

/**
 * @class BuilderAgent
 * @brief Agent specialized in construction and resource management
 */
class BuilderAgent : public AIAgent {
public:
    BuilderAgent(uint32_t id, const AgentCapabilities& capabilities)
        : AIAgent(id, AgentRole::Builder, capabilities),
          constructedBuildings_(0),
          resourceEfficiency_(0.8f) {}

protected:
    void planActions() override {
        if (currentGoal_.find("build") != std::string::npos) {
            // Plan construction sequence
            std::cout << "[BuilderAgent] Agent " << getId() << " planning construction" << std::endl;
        } else if (currentGoal_.find("repair") != std::string::npos) {
            // Plan repair operations
            std::cout << "[BuilderAgent] Agent " << getId() << " planning repairs" << std::endl;
        }
    }

    void handleMessage(const AgentMessage& message) override {
        if (message.type == "collaboration_request") {
            handleCollaborationRequest(message.senderId, message.content);
        } else if (message.type == "resource_available") {
            // Update resource knowledge
            resourceEfficiency_ += 0.01f;
        }
    }

    void handleCollaborationRequest(uint32_t requestorId, const std::string& task) override {
        // Builder agents prioritize construction-related collaboration
        if (task.find("build") != std::string::npos || task.find("construct") != std::string::npos) {
            acceptCollaboration(requestorId);
        } else if (getTrustLevel(requestorId) > 0.8f) {
            acceptCollaboration(requestorId);
        } else {
            declineCollaboration(requestorId);
        }
    }

private:
    int constructedBuildings_;
    float resourceEfficiency_;
};

// CollaborativeAgentSystem implementation

CollaborativeAgentSystem::CollaborativeAgentSystem()
    : nextAgentId_(1), nextGoalId_(1) {}

CollaborativeAgentSystem::~CollaborativeAgentSystem() = default;

bool CollaborativeAgentSystem::initialize(const SystemConfig& config) {
    config_ = config;

    totalMessages_ = 0;
    collaborationsInitiated_ = 0;
    goalsCompleted_ = 0;
    systemEfficiency_ = 1.0f;

    std::cout << "[CollaborativeAgentSystem] Initialized with config: maxAgents="
              << config_.maxAgents << ", collaboration=" << config_.enableCollaboration << std::endl;

    return true;
}

void CollaborativeAgentSystem::shutdown() {
    std::lock_guard<std::mutex> lock(systemMutex_);

    // Clear message queue
    while (!messageQueue_.empty()) {
        messageQueue_.pop();
    }

    // Shutdown all agents
    for (auto& pair : agents_) {
        if (pair.second) {
            pair.second->shutdown();
        }
    }

    agents_.clear();
    std::cout << "[CollaborativeAgentSystem] Shutdown complete" << std::endl;
}

void CollaborativeAgentSystem::update(float deltaTime) {
    std::lock_guard<std::mutex> lock(systemMutex_);

    // Process system messages
    processMessages();

    // Update all active agents
    for (auto& pair : agents_) {
        if (pair.second && pair.second->getState() != AgentState::Completed) {
            pair.second->update(deltaTime);
        }
    }

    // Update collaborations
    if (config_.enableCollaboration) {
        updateCollaborations();
    }

    // Update system statistics
    updateSystemStats();
}

uint32_t CollaborativeAgentSystem::createAgent(AgentRole role, const AgentCapabilities& capabilities) {
    std::lock_guard<std::mutex> lock(systemMutex_);

    if (agents_.size() >= static_cast<size_t>(config_.maxAgents)) {
        std::cerr << "[CollaborativeAgentSystem] Cannot create agent: max agents reached" << std::endl;
        return 0;
    }

    uint32_t agentId = nextAgentId_++;

    std::unique_ptr<AIAgent> agent;

    // Create appropriate agent type based on role
    switch (role) {
        case AgentRole::Scout:
            agent = std::make_unique<ScoutAgent>(agentId, capabilities);
            break;
        case AgentRole::Builder:
            agent = std::make_unique<BuilderAgent>(agentId, capabilities);
            break;
        default:
            // Create generic agent
            agent = std::make_unique<ScoutAgent>(agentId, capabilities);
            std::cout << "[CollaborativeAgentSystem] Created generic agent for unsupported role "
                      << static_cast<int>(role) << std::endl;
            break;
    }

    if (agent && agent->initialize()) {
        agents_[agentId] = std::move(agent);
        std::cout << "[CollaborativeAgentSystem] Created agent " << agentId
                  << " with role " << static_cast<int>(role) << std::endl;
        return agentId;
    }

    std::cerr << "[CollaborativeAgentSystem] Failed to create agent" << std::endl;
    return 0;
}

void CollaborativeAgentSystem::destroyAgent(uint32_t agentId) {
    std::lock_guard<std::mutex> lock(systemMutex_);

    auto it = agents_.find(agentId);
    if (it != agents_.end()) {
        if (it->second) {
            it->second->shutdown();
        }
        agents_.erase(it);
        std::cout << "[CollaborativeAgentSystem] Destroyed agent " << agentId << std::endl;
    }
}

AIAgent* CollaborativeAgentSystem::getAgent(uint32_t agentId) {
    auto it = agents_.find(agentId);
    return it != agents_.end() ? it->second.get() : nullptr;
}

uint32_t CollaborativeAgentSystem::assignCollectiveGoal(const std::vector<uint32_t>& agents, const std::string& goal) {
    std::lock_guard<std::mutex> lock(systemMutex_);

    uint32_t goalId = nextGoalId_++;

    for (uint32_t agentId : agents) {
        auto agent = getAgent(agentId);
        if (agent) {
            agent->assignGoal(goalId, goal);

            // If multiple agents, enable collaboration
            if (agents.size() > 1 && config_.enableCollaboration) {
                for (uint32_t otherId : agents) {
                    if (otherId != agentId) {
                        agent->requestCollaboration(otherId, goal);
                    }
                }
            }
        }
    }

    if (agents.size() > 1) {
        collaborationsInitiated_++;
    }

    std::cout << "[CollaborativeAgentSystem] Assigned collective goal '" << goal
              << "' to " << agents.size() << " agents (goalId: " << goalId << ")" << std::endl;

    return goalId;
}

void CollaborativeAgentSystem::routeMessage(const AgentMessage& message) {
    std::lock_guard<std::mutex> lock(systemMutex_);

    totalMessages_++;

    // Route to recipient
    if (message.receiverId == 0) {
        // Broadcast to all agents
        for (auto& pair : agents_) {
            if (pair.first != message.senderId && pair.second) {
                pair.second->receiveMessage(message);
            }
        }
    } else {
        // Send to specific agent
        auto recipient = getAgent(message.receiverId);
        if (recipient) {
            recipient->receiveMessage(message);
        }
    }
}

std::unordered_map<std::string, float> CollaborativeAgentSystem::getSystemStats() const {
    std::unordered_map<std::string, float> stats;
    stats["total_agents"] = static_cast<float>(agents_.size());
    stats["total_messages"] = static_cast<float>(totalMessages_);
    stats["collaborations_initiated"] = static_cast<float>(collaborationsInitiated_);
    stats["goals_completed"] = static_cast<float>(goalsCompleted_);
    stats["system_efficiency"] = systemEfficiency_;

    return stats;
}

void CollaborativeAgentSystem::setCollaborationEnabled(bool enabled) {
    config_.enableCollaboration = enabled;
    std::cout << "[CollaborativeAgentSystem] Collaboration " << (enabled ? "enabled" : "disabled") << std::endl;
}

void CollaborativeAgentSystem::processMessages() {
    std::lock_guard<std::mutex> lock(systemMutex_);

    // Rate limit message processing
    static int messagesProcessedThisFrame = 0;
    const int maxMessagesPerFrame = 100;

    while (!messageQueue_.empty() && messagesProcessedThisFrame < maxMessagesPerFrame) {
        AgentMessage msg = messageQueue_.front();
        messageQueue_.pop();

        routeMessage(msg);
        messagesProcessedThisFrame++;
    }

    messagesProcessedThisFrame = 0;
}

void CollaborativeAgentSystem::updateCollaborations() {
    std::lock_guard<std::mutex> lock(systemMutex_);

    // Monitor collaboration effectiveness
    int activeCollaborations = 0;
    int completedCollaborations = 0;

    for (const auto& pair : agents_) {
        if (pair.second) {
            const auto& collaborators = pair.second->getCollaborators();
            activeCollaborations += collaborators.size();
        }
    }

    // Calculate collaboration efficiency
    if (collaborationsInitiated_ > 0) {
        systemEfficiency_ = static_cast<float>(completedCollaborations) / collaborationsInitiated_;
    }
}

void CollaborativeAgentSystem::updateSystemStats() {
    // Calculate overall system performance metrics
    int totalGoals = 0;
    int completedGoals = 0;

    for (const auto& pair : agents_) {
        if (pair.second) {
            totalGoals++;
            if (pair.second->getState() == AgentState::Completed) {
                completedGoals++;
            }
        }
    }

    float completionRate = totalGoals > 0 ? static_cast<float>(completedGoals) / totalGoals : 1.0f;
    systemEfficiency_ *= 0.95f; // Slight decay
    systemEfficiency_ += completionRate * 0.05f; // Improvement factor
    systemEfficiency_ = std::clamp(systemEfficiency_, 0.0f, 1.0f);
}

} // namespace FoundryEngine
