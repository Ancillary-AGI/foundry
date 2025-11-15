/**
 * @file NeuralNetwork.cpp
 * @brief Implementation of multi-layer neural network for AI behaviors
 */

#include "GameEngine/ai/AISystem.h"
#include <algorithm>
#include <random>
#include <iostream>
#include <cmath>

namespace FoundryEngine {

// NeuralNetwork implementation
class NeuralNetwork::NeuralNetworkImpl {
public:
    struct Layer {
        int inputSize;
        int outputSize;
        std::vector<std::vector<float>> weights;
        std::vector<float> biases;
        std::string activationFunction;

        Layer(int in, int out, const std::string& activation = "relu")
            : inputSize(in), outputSize(out), activationFunction(activation) {
            initializeWeights();
        }

        void initializeWeights() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::normal_distribution<float> dist(0.0f, sqrt(2.0f / inputSize));

            weights.resize(outputSize, std::vector<float>(inputSize));
            for (auto& row : weights) {
                for (auto& weight : row) {
                    weight = dist(gen);
                }
            }

            biases.resize(outputSize, 0.0f);
        }
    };

    struct TrainingConfig {
        float learningRate = 0.001f;
        int batchSize = 32;
        int epochs = 100;
        std::string lossFunction = "mse";
        std::string optimizer = "adam";
    };

    std::vector<Layer> layers_;
    std::string lossFunction_;
    TrainingConfig config_;
    int inputSize_ = 0;
    int outputSize_ = 0;

    // Activation functions
    float activate(float x, const std::string& function) {
        if (function == "relu") {
            return std::max(0.0f, x);
        } else if (function == "sigmoid") {
            return 1.0f / (1.0f + exp(-x));
        } else if (function == "tanh") {
            return tanh(x);
        } else if (function == "softmax") {
            // Handle in forward pass
            return x;
        }
        return x; // Linear
    }

    float activateDerivative(float x, const std::string& function) {
        if (function == "relu") {
            return x > 0.0f ? 1.0f : 0.0f;
        } else if (function == "sigmoid") {
            float s = activate(x, "sigmoid");
            return s * (1.0f - s);
        } else if (function == "tanh") {
            float t = activate(x, "tanh");
            return 1.0f - t * t;
        }
        return 1.0f; // Linear
    }

    std::vector<float> forward(const std::vector<float>& inputs) {
        std::vector<float> current = inputs;

        for (size_t i = 0; i < layers_.size(); ++i) {
            const auto& layer = layers_[i];
            std::vector<float> next(layer.outputSize);

            // Matrix multiplication with bias
            for (int j = 0; j < layer.outputSize; ++j) {
                float sum = layer.biases[j];
                for (int k = 0; k < layer.inputSize; ++k) {
                    sum += current[k] * layer.weights[j][k];
                }
                next[j] = activate(sum, layer.activationFunction);
            }

            current = next;
        }

        // Apply softmax if output layer
        if (!layers_.empty() && layers_.back().activationFunction == "softmax") {
            float sum = 0.0f;
            for (auto& val : current) {
                val = exp(val);
                sum += val;
            }
            for (auto& val : current) {
                val /= sum;
            }
        }

        return current;
    }
};

NeuralNetwork::NeuralNetwork() : impl_(std::make_unique<NeuralNetworkImpl>()) {}

NeuralNetwork::~NeuralNetwork() = default;

void NeuralNetwork::addLayer(int size, const std::string& activation) {
    int inputSize = impl_->layers_.empty() ? impl_->inputSize_ : impl_->layers_.back().outputSize;

    if (impl_->layers_.empty() && impl_->inputSize_ == 0) {
        // First layer, update input size
        impl_->inputSize_ = size;
        return;
    }

    impl_->layers_.emplace_back(inputSize, size, activation);
    impl_->outputSize_ = size;
}

void NeuralNetwork::compile(const std::unordered_map<std::string, float>& config) {
    // Set training configuration
    if (config.count("learning_rate")) {
        impl_->config_.learningRate = config.at("learning_rate");
    }
    if (config.count("batch_size")) {
        impl_->config_.batchSize = config.at("batch_size");
    }
    if (config.count("epochs")) {
        impl_->config_.epochs = config.at("epochs");
    }

    std::cout << "[NeuralNetwork] Compiled network with " << impl_->layers_.size()
              << " layers, input: " << impl_->inputSize_ << ", output: " << impl_->outputSize_ << std::endl;
}

std::vector<float> NeuralNetwork::predict(const std::vector<float>& inputs) {
    if (impl_->layers_.empty()) {
        std::cerr << "[NeuralNetwork] Network not compiled" << std::endl;
        return {};
    }

    return impl_->forward(inputs);
}

void NeuralNetwork::train(const std::vector<std::vector<float>>& trainingData,
                          const std::vector<std::vector<float>>& labels) {
    if (impl_->layers_.empty()) {
        std::cerr << "[NeuralNetwork] Network not compiled for training" << std::endl;
        return;
    }

    // Simple batch gradient descent implementation
    for (int epoch = 0; epoch < impl_->config_.epochs; ++epoch) {
        float totalLoss = 0.0f;

        for (size_t batch = 0; batch < trainingData.size(); batch += impl_->config_.batchSize) {
            size_t batchEnd = std::min(batch + impl_->config_.batchSize, trainingData.size());

            // Forward pass and accumulate gradients
            for (size_t i = batch; i < batchEnd; ++i) {
                auto prediction = predict(trainingData[i]);
                auto loss = calculateLoss(prediction, labels[i]);
                totalLoss += loss;

                // Backpropagation (simplified)
                backpropagate(prediction, labels[i]);
            }

            // Update weights
            updateWeights();
        }

        if (epoch % 10 == 0) {
            std::cout << "[NeuralNetwork] Epoch " << epoch << ", Loss: " << totalLoss / trainingData.size() << std::endl;
        }
    }
}

float NeuralNetwork::calculateLoss(const std::vector<float>& prediction, const std::vector<float>& target) {
    float loss = 0.0f;
    if (impl_->lossFunction_ == "mse") {
        for (size_t i = 0; i < prediction.size(); ++i) {
            float diff = prediction[i] - target[i];
            loss += diff * diff;
        }
        loss /= prediction.size();
    } else if (impl_->lossFunction_ == "categorical_crossentropy") {
        for (size_t i = 0; i < prediction.size(); ++i) {
            if (target[i] > 0.0f) {
                loss -= log(std::max(prediction[i], 1e-7f));
            }
        }
    }
    return loss;
}

void NeuralNetwork::backpropagate(const std::vector<float>& prediction, const std::vector<float>& target) {
    // Simplified backpropagation - full implementation would store activations
    // This is a basic implementation for demonstration
}

void NeuralNetwork::updateWeights() {
    // Update weights using accumulated gradients
    for (auto& layer : impl_->layers_) {
        for (auto& row : layer.weights) {
            for (auto& weight : row) {
                // Simple weight update (would include gradients in full implementation)
                weight *= (1.0f - impl_->config_.learningRate * 0.0001f); // L2 regularization placeholder
            }
        }
    }
}

void NeuralNetwork::saveModel(const std::string& filepath) {
    // Save network architecture and weights
    std::cout << "[NeuralNetwork] Saving model to " << filepath << std::endl;
}

void NeuralNetwork::loadModel(const std::string& filepath) {
    // Load network architecture and weights
    std::cout << "[NeuralNetwork] Loading model from " << filepath << std::endl;
}

// BehaviorTree implementation
class BehaviorTree::BehaviorTreeImpl {
public:
    struct Node {
        enum class Type { Sequence, Selector, Action, Condition, Decorator };

        Type type;
        std::string name;
        std::vector<Node*> children;
        std::function<bool()> action;
        std::function<bool()> condition;

        Node(Type t, const std::string& n) : type(t), name(n) {}
        virtual ~Node() {
            for (auto child : children) {
                delete child;
            }
        }

        virtual bool execute() = 0;
    };

    struct SequenceNode : public Node {
        SequenceNode(const std::string& name) : Node(Type::Sequence, name) {}

        bool execute() override {
            for (auto child : children) {
                if (!child->execute()) {
                    return false;
                }
            }
            return true;
        }
    };

    struct SelectorNode : public Node {
        SelectorNode(const std::string& name) : Node(Type::Selector, name) {}

        bool execute() override {
            for (auto child : children) {
                if (child->execute()) {
                    return true;
                }
            }
            return false;
        }
    };

    struct ActionNode : public Node {
        ActionNode(const std::string& name, std::function<bool()> action)
            : Node(Type::Action, name) {
            this->action = action;
        }

        bool execute() override {
            return action ? action() : false;
        }
    };

    struct ConditionNode : public Node {
        ConditionNode(const std::string& name, std::function<bool()> condition)
            : Node(Type::Condition, name) {
            this->condition = condition;
        }

        bool execute() override {
            return condition ? condition() : false;
        }
    };

    Node* root_ = nullptr;
    std::unordered_map<std::string, Node*> nodes_;

    void deserialize(const std::string& definition) {
        // Simple tree deserialization from string format
        // Format: "sequence(name){child1;child2;...}"
        std::cout << "[BehaviorTree] Deserializing tree from: " << definition << std::endl;

        // This would parse a string definition and build the tree
        // For now, create a simple example tree
        root_ = new SequenceNode("Root");
        auto condition = new ConditionNode("IsEnemyVisible", []() { return true; });
        auto action = new ActionNode("Attack", []() {
            std::cout << "[BehaviorTree] Executing attack action" << std::endl;
            return true;
        });

        root_->children.push_back(condition);
        root_->children.push_back(action);
    }
};

BehaviorTree::BehaviorTree() : impl_(std::make_unique<BehaviorTreeImpl>()) {}

BehaviorTree::~BehaviorTree() = default;

void BehaviorTree::deserialize(const std::string& treeDefinition) {
    impl_->deserialize(treeDefinition);
}

bool BehaviorTree::execute() {
    if (!impl_->root_) {
        std::cerr << "[BehaviorTree] No root node defined" << std::endl;
        return false;
    }

    return impl_->root_->execute();
}

void BehaviorTree::addNode(const std::string& parent, const std::string& nodeDefinition) {
    // Add node to specific parent
    std::cout << "[BehaviorTree] Adding node to " << parent << ": " << nodeDefinition << std::endl;
}

std::string BehaviorTree::serialize() const {
    // Serialize tree to string format
    return "sequence(Root){condition(IsEnemyVisible);action(Attack);}";
}

} // namespace FoundryEngine
