/**
 * @file AISystem.h
 * @brief Advanced AI system with neural networks and behavior trees
 * @author FoundryEngine Team
 * @date 2024
 * @version 2.0.0
 */

#pragma once

#include "../core/System.h"
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>

namespace FoundryEngine {

class NeuralNetwork;
class BehaviorTree;
class AIAgent;

/**
 * @class AISystem
 * @brief Comprehensive AI system for intelligent NPCs and procedural content
 */
class AISystem : public System {
public:
    struct AIConfig {
        bool enableNeuralNetworks = true;
        bool enableBehaviorTrees = true;
        bool enablePathfinding = true;
        bool enableProceduralGeneration = true;
        size_t maxAgents = 1000;
        float updateFrequency = 60.0f;
    };

    struct AgentStats {
        uint32_t agentId;
        std::string behaviorState;
        float decisionTime;
        float pathfindingTime;
        float totalProcessingTime;
        size_t memoryUsage;
    };

    AISystem();
    ~AISystem();

    bool initialize(const AIConfig& config = AIConfig{}) override;
    void shutdown() override;
    void update(float deltaTime) override;

    // Agent management
    uint32_t createAgent(const std::string& agentType);
    void destroyAgent(uint32_t agentId);
    AIAgent* getAgent(uint32_t agentId);
    std::vector<uint32_t> getAllAgents() const;

    // Neural network integration
    uint32_t createNeuralNetwork(const std::string& architecture);
    void trainNetwork(uint32_t networkId, const std::vector<float>& inputs, 
                     const std::vector<float>& expectedOutputs);
    std::vector<float> evaluateNetwork(uint32_t networkId, const std::vector<float>& inputs);
    void saveNetwork(uint32_t networkId, const std::string& filepath);
    uint32_t loadNetwork(const std::string& filepath);

    // Behavior trees
    uint32_t createBehaviorTree(const std::string& treeDefinition);
    void updateBehaviorTree(uint32_t treeId, uint32_t agentId, float deltaTime);
    void setBehaviorTreeVariable(uint32_t treeId, const std::string& name, float value);
    float getBehaviorTreeVariable(uint32_t treeId, const std::string& name);

    // Pathfinding
    std::vector<Vector3> findPath(const Vector3& start, const Vector3& end, 
                                 const std::string& agentType = "default");
    void updateNavMesh(const std::vector<Vector3>& vertices, const std::vector<uint32_t>& indices);
    bool isPathValid(const std::vector<Vector3>& path);

    // Procedural content generation
    std::vector<Vector3> generateLevel(const std::string& levelType, 
                                      const std::unordered_map<std::string, float>& parameters);
    std::string generateDialogue(const std::string& context, const std::string& characterType);
    std::vector<std::string> generateQuests(const std::string& questType, int count);

    // Performance monitoring
    std::vector<AgentStats> getAgentStats() const;
    float getAverageProcessingTime() const;
    size_t getTotalMemoryUsage() const;

private:
    class AISystemImpl;
    std::unique_ptr<AISystemImpl> impl_;
};

/**
 * @class BehaviorTree
 * @brief Visual behavior tree system for AI decision making
 */
class BehaviorTree {
public:
    enum class NodeType {
        Composite,
        Decorator,
        Action,
        Condition
    };

    enum class NodeStatus {
        Success,
        Failure,
        Running,
        Invalid
    };

    struct Node {
        uint32_t id;
        NodeType type;
        std::string name;
        std::vector<uint32_t> children;
        std::function<NodeStatus(uint32_t, float)> execute;
        std::unordered_map<std::string, float> parameters;
    };

    BehaviorTree();
    ~BehaviorTree();

    uint32_t addNode(NodeType type, const std::string& name);
    void removeNode(uint32_t nodeId);
    void addChild(uint32_t parentId, uint32_t childId);
    void removeChild(uint32_t parentId, uint32_t childId);
    
    void setNodeFunction(uint32_t nodeId, std::function<NodeStatus(uint32_t, float)> func);
    void setNodeParameter(uint32_t nodeId, const std::string& name, float value);
    float getNodeParameter(uint32_t nodeId, const std::string& name);

    NodeStatus execute(uint32_t agentId, float deltaTime);
    void reset();

    // Serialization
    std::string serialize() const;
    bool deserialize(const std::string& data);

private:
    std::unordered_map<uint32_t, Node> nodes_;
    uint32_t rootNodeId_ = 0;
    uint32_t nextNodeId_ = 1;
    std::unordered_map<uint32_t, NodeStatus> nodeStates_;
};

/**
 * @class NeuralNetwork
 * @brief Deep learning neural network for AI behaviors
 */
class NeuralNetwork {
public:
    struct LayerConfig {
        size_t neurons;
        std::string activation = "relu";
        float dropout = 0.0f;
        bool batchNorm = false;
    };

    struct TrainingConfig {
        float learningRate = 0.001f;
        size_t batchSize = 32;
        size_t epochs = 100;
        float validationSplit = 0.2f;
        std::string optimizer = "adam";
        std::string lossFunction = "mse";
    };

    NeuralNetwork();
    ~NeuralNetwork();

    // Network architecture
    void addLayer(const LayerConfig& config);
    void compile(const TrainingConfig& config);
    
    // Training
    void train(const std::vector<std::vector<float>>& inputs,
              const std::vector<std::vector<float>>& outputs);
    void trainBatch(const std::vector<std::vector<float>>& inputs,
                   const std::vector<std::vector<float>>& outputs);
    
    // Inference
    std::vector<float> predict(const std::vector<float>& input);
    std::vector<std::vector<float>> predictBatch(const std::vector<std::vector<float>>& inputs);
    
    // Model management
    void save(const std::string& filepath);
    bool load(const std::string& filepath);
    void reset();
    
    // Performance
    float getAccuracy() const;
    float getLoss() const;
    size_t getParameterCount() const;

private:
    class NeuralNetworkImpl;
    std::unique_ptr<NeuralNetworkImpl> impl_;
};

} // namespace FoundryEngine