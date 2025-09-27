#ifndef NEUTRAL_GAMEENGINE_AI_SYSTEM_H
#define NEUTRAL_GAMEENGINE_AI_SYSTEM_H

#include <vector>
#include <queue>
#include <map>
#include <functional>
#include "../../core/System.h"
#include "../../math/Vector3.h"
#include "../components/AIComponent.h"

namespace FoundryEngine {

class AISystem : public System {
public:
    // A* Node for pathfinding
    struct Node {
        Vector3 position;
        float gCost, hCost, fCost;
        Node* parent;
        bool walkable;

        Node(const Vector3& pos) : position(pos), gCost(0), hCost(0), fCost(0), parent(nullptr), walkable(true) {}

        float distance(const Node* other) const {
            return (position - other->position).magnitude();
        }
    };

    // Decision Tree Node
    struct DecisionNode {
        std::function<bool()> condition;
        DecisionNode* trueBranch;
        DecisionNode* falseBranch;
        std::function<void()> action;

        DecisionNode(std::function<bool()> cond, std::function<void()> act = nullptr)
            : condition(cond), trueBranch(nullptr), falseBranch(nullptr), action(act) {}
    };

    // A* pathfinding
    std::vector<Node> grid_;
    int gridWidth_, gridHeight_;

    std::vector<AIComponent*> aiComponents;

    void registerAIComponent(AIComponent* aiComp) {
        aiComponents.push_back(aiComp);
    }

    void createGrid(int width, int height, float cellSize) {
        gridWidth_ = width;
        gridHeight_ = height;
        grid_.clear();
        for (int h = 0; h < height; ++h) {
            for (int w = 0; w < width; ++w) {
                Vector3 pos(w * cellSize, 0, h * cellSize);
                grid_.emplace_back(pos);
            }
        }
    }

    std::vector<Vector3> findPath(const Vector3& start, const Vector3& goal) {
        // Simplified A* (assuming start and goal on grid)
        auto getNodeIndex = [&](const Vector3& pos) {
            int x = static_cast<int>(pos.x / 1.0f); // cellSize=1
            int y = static_cast<int>(pos.z / 1.0f);
            return y * gridWidth_ + x;
        };

        int startIdx = getNodeIndex(start);
        int goalIdx = getNodeIndex(goal);
        if (startIdx < 0 || goalIdx >= grid_.size()) return {};

        std::vector<Node*> openSet;
        std::vector<Node*> closedSet;
        openSet.push_back(&grid_[startIdx]);

        while (!openSet.empty()) {
            // Find node with lowest fCost
            Node* current = openSet[0];
            for (auto* n : openSet) {
                if (n->fCost < current->fCost || (n->fCost == current->fCost && n->hCost < current->hCost)) {
                    current = n;
                }
            }

            if (current == &grid_[goalIdx]) {
                // Reconstruct path
                std::vector<Vector3> path;
                Node* n = current;
                while (n != nullptr) {
                    path.push_back(n->position);
                    n = n->parent;
                }
                std::reverse(path.begin(), path.end());
                return path;
            }

            openSet.erase(std::remove(openSet.begin(), openSet.end(), current), openSet.end());
            closedSet.push_back(current);

            // Check neighbors (4-directional)
            std::vector<int> neighbors = { -gridWidth_, 1, gridWidth_, -1 };
            for (int offset : neighbors) {
                int neighborIdx = (current - &grid_[0]) + offset;
                if (neighborIdx < 0 || neighborIdx >= grid_.size()) continue;
                if (!grid_[neighborIdx].walkable || std::find(closedSet.begin(), closedSet.end(), &grid_[neighborIdx]) != closedSet.end()) continue;

                float tentativeGCost = current->gCost + current->distance(&grid_[neighborIdx]);
                if (tentativeGCost < grid_[neighborIdx].gCost || std::find(openSet.begin(), openSet.end(), &grid_[neighborIdx]) == openSet.end()) {
                    grid_[neighborIdx].gCost = tentativeGCost;
                    grid_[neighborIdx].hCost = grid_[neighborIdx].distance(&grid_[goalIdx]);
                    grid_[neighborIdx].fCost = grid_[neighborIdx].gCost + grid_[neighborIdx].hCost;
                    grid_[neighborIdx].parent = current;
                    if (std::find(openSet.begin(), openSet.end(), &grid_[neighborIdx]) == openSet.end()) {
                        openSet.push_back(&grid_[neighborIdx]);
                    }
                }
            }
        }
        return {}; // No path found
    }

    // Decision tree evaluation
    void evaluateDecisionTree(DecisionNode* root) {
        if (root == nullptr) return;
        if (root->condition && root->condition()) {
            if (root->trueBranch) evaluateDecisionTree(root->trueBranch);
            if (root->action) root->action();
        } else {
            if (root->falseBranch) evaluateDecisionTree(root->falseBranch);
            if (root->action) root->action();
        }
    }

    // Simple Perceptron for machine learning
    struct Perceptron {
        std::vector<float> weights;
        float bias;
        float learningRate;

        Perceptron(size_t inputSize, float lr = 0.01f) : bias(0), learningRate(lr) {
            weights.resize(inputSize, 0.0f);
        }

        float activate(float x) { return x >= 0 ? 1 : 0; }  // Step function

        float predict(const std::vector<float>& inputs) {
            float sum = bias;
            for (size_t i = 0; i < inputs.size(); ++i) {
                sum += inputs[i] * weights[i];
            }
            return activate(sum);
        }

        void train(const std::vector<std::vector<float>>& trainingInputs, const std::vector<float>& labels) {
            for (size_t epoch = 0; epoch < 100; ++epoch) {  // Simple iterative training
                for (size_t i = 0; i < trainingInputs.size(); ++i) {
                    float prediction = predict(trainingInputs[i]);
                    float error = labels[i] - prediction;
                    for (size_t j = 0; j < weights.size(); ++j) {
                        weights[j] += learningRate * error * trainingInputs[i][j];
                    }
                    bias += learningRate * error;
                }
            }
        }
    };

    // Boid for flocking
    struct Boid {
        Vector3 position, velocity;
        Vector3 acceleration;
        float maxSpeed, maxForce;

        Boid(const Vector3& pos, const Vector3& vel, float mSpeed = 5.0f, float mForce = 1.0f)
            : position(pos), velocity(vel), acceleration(Vector3(0,0,0)), maxSpeed(mSpeed), maxForce(mForce) {}

        void applyForce(const Vector3& force) {
            acceleration += force;
        }

        void update(float deltaTime) {
            velocity += acceleration * deltaTime;
            if (velocity.magnitude() > maxSpeed) {
                velocity = velocity.normalized() * maxSpeed;
            }
            position += velocity * deltaTime;
            acceleration *= 0; // Reset
        }

        // Seek a target
        Vector3 seek(const Vector3& target) {
            Vector3 desired = target - position;
            desired = desired.normalized() * maxSpeed;
            Vector3 steer = desired - velocity;
            if (steer.magnitude() > maxForce) {
                steer = steer.normalized() * maxForce;
            }
            return steer;
        }

        // Flee from target
        Vector3 flee(const Vector3& target) {
            return -seek(target);
        }

        // Separate from nearby boids
        Vector3 separate(const std::vector<Boid>& boids) {
            float desiredSeparation = 25.0f;
            Vector3 steer(0, 0, 0);
            int count = 0;
            for (const auto& other : boids) {
                float d = (position - other.position).magnitude();
                if (d > 0 && d < desiredSeparation) {
                    Vector3 diff = position - other.position;
                    diff.normalize();
                    diff /= d; // Weight by distance
                    steer += diff;
                    count++;
                }
            }
            if (count > 0) {
                steer /= count;
                steer.normalize();
                steer *= maxSpeed;
                steer -= velocity;
                if (steer.magnitude() > maxForce) {
                    steer.normalize();
                    steer *= maxForce;
                }
            }
            return steer;
        }

        // Align velocity with nearby boids
        Vector3 align(const std::vector<Boid>& boids) {
            float neighborDist = 50.0f;
            Vector3 sum(0, 0, 0);
            int count = 0;
            for (const auto& other : boids) {
                float d = (position - other.position).magnitude();
                if (d > 0 && d < neighborDist) {
                    sum += other.velocity;
                    count++;
                }
            }
            if (count > 0) {
                sum /= count;
                sum.normalize();
                sum *= maxSpeed;
                Vector3 steer = sum - velocity;
                if (steer.magnitude() > maxForce) {
                    steer.normalize();
                    steer *= maxForce;
                }
                return steer;
            }
            return Vector3(0, 0, 0);
        }

        // Cohesion: steer towards average position
        Vector3 cohesion(const std::vector<Boid>& boids) {
            float neighborDist = 50.0f;
            Vector3 sum(0, 0, 0);
            int count = 0;
            for (const auto& other : boids) {
                float d = (position - other.position).magnitude();
                if (d > 0 && d < neighborDist) {
                    sum += other.position;
                    count++;
                }
            }
            if (count > 0) {
                sum /= count;
                return seek(sum);
            }
            return Vector3(0, 0, 0);
        }

        // Flock behavior: combine separation, alignment, cohesion
        void flock(const std::vector<Boid>& boids) {
            Vector3 sep = separate(boids);
            Vector3 ali = align(boids);
            Vector3 coh = cohesion(boids);
            sep *= 1.5f; // Weight separation more
            ali *= 1.0f;
            coh *= 1.0f;
            applyForce(sep);
            applyForce(ali);
            applyForce(coh);
        }
    };

    std::vector<Boid> boids;

    void addBoid(const Vector3& pos, const Vector3& vel) {
        boids.emplace_back(pos, vel);
    }

    void updateFlocking(float deltaTime) {
        for (auto& boid : boids) {
            boid.flock(boids);
            boid.update(deltaTime);
        }
    }

    // Simple Feedforward Neural Network
    struct NeuralNetwork {
        std::vector<std::vector<std::vector<float>>> weights; // Layer -> Neuron -> Incoming weights
        std::vector<std::vector<float>> biases; // Layer -> Neuron biases
        std::vector<std::vector<float>> activations; // For forward/backward pass
        std::vector<int> layers; // Sizes of layers

        NeuralNetwork(const std::vector<int>& layerSizes) : layers(layerSizes) {
            // Initialize weights and biases
            weights.resize(layers.size() - 1);
            biases.resize(layers.size() - 1);
            for (size_t i = 0; i < layers.size() - 1; ++i) {
                weights[i].resize(layers[i + 1], std::vector<float>(layers[i], 0.0f));
                biases[i].resize(layers[i + 1], 0.0f);
                // Randomize weights and biases (simple random)
                for (auto& neuron : weights[i]) {
                    for (float& w : neuron) w = (rand() % 2000 - 1000) / 1000.0f; // -1 to 1
                }
                for (float& b : biases[i]) b = (rand() % 2000 - 1000) / 1000.0f;
            }
            activations.resize(layers.size());
        }

        // Sigmoid activation
        float sigmoid(float x) { return 1.0f / (1.0f + exp(-x)); }

        // Derivative of sigmoid
        float sigmoidDeriv(float x) { float s = sigmoid(x); return s * (1 - s); }

        // Forward pass
        std::vector<float> feedforward(const std::vector<float>& inputs) {
            activations[0] = inputs;
            for (size_t i = 0; i < weights.size(); ++i) {
                activations[i + 1].resize(layers[i + 1]);
                for (size_t j = 0; j < weights[i].size(); ++j) {
                    float sum = biases[i][j];
                    for (size_t k = 0; k < weights[i][j].size(); ++k) {
                        sum += weights[i][j][k] * activations[i][k];
                    }
                    activations[i + 1][j] = sigmoid(sum);
                }
            }
            return activations.back();
        }

        // Train with backpropagation
        void train(const std::vector<float>& inputs, const std::vector<float>& targets, float learningRate = 0.1f) {
            feedforward(inputs);

            // Calculate output errors
            std::vector<std::vector<float>> errors(weights.size());
            errors.back().resize(layers.back());
            for (size_t i = 0; i < layers.back(); ++i) {
                float output = activations.back()[i];
                errors.back()[i] = (targets[i] - output) * sigmoidDeriv(output); // Assuming sigmoid
            }

            // Backpropagate errors
            for (int i = weights.size() - 2; i >= 0; --i) {
                errors[i].resize(layers[i + 1]);
                for (size_t j = 0; j < layers[i + 1]; ++j) {
                    float error = 0.0f;
                    for (size_t k = 0; k < layers[i + 2]; ++k) {
                        error += errors[i + 1][k] * weights[i + 1][k][j];
                    }
                    errors[i][j] = error * sigmoidDeriv(activations[i + 1][j]);
                }
            }

            // Update weights and biases
            for (size_t i = 0; i < weights.size(); ++i) {
                for (size_t j = 0; j < weights[i].size(); ++j) {
                    biases[i][j] += learningRate * errors[i][j];
                    for (size_t k = 0; k < weights[i][j].size(); ++k) {
                        weights[i][j][k] += learningRate * errors[i][j] * activations[i][k];
                    }
                }
            }
        }
    };

    NeuralNetwork neuralNetwork{{2, 4, 1}}; // Example: 2 inputs, 1 hidden (4 neurons), 1 output

    // Simple Genetic Algorithm
    struct Individual {
        std::vector<float> genes;
        float fitness;

        Individual(size_t geneCount) : genes(geneCount), fitness(0.0f) {
            for (float& g : genes) g = (rand() % 2000 - 1000) / 1000.0f; // -1 to 1
        }

        void mutate(float mutationRate = 0.01f) {
            for (float& g : genes) {
                if ((rand() % 100) < mutationRate * 100) {
                    g += (rand() % 200 - 100) / 1000.0f; // Small change
                }
            }
        }

        static Individual crossover(const Individual& p1, const Individual& p2) {
            Individual child(p1.genes.size());
            size_t crossoverPoint = rand() % p1.genes.size();
            for (size_t i = 0; i < p1.genes.size(); ++i) {
                child.genes[i] = (i < crossoverPoint ? p1.genes[i] : p2.genes[i]);
            }
            return child;
        }
    };

    std::vector<Individual> population;

    void initializePopulation(size_t popSize, size_t geneCount) {
        population.clear();
        for (size_t i = 0; i < popSize; ++i) {
            population.emplace_back(geneCount);
        }
    }

    void evaluateFitness(std::function<float(const std::vector<float>&)> fitnessFunc) {
        for (auto& ind : population) {
            ind.fitness = fitnessFunc(ind.genes);
        }
        std::sort(population.begin(), population.end(), [](const Individual& a, const Individual& b) {
            return a.fitness > b.fitness; // Descending
        });
    }

    void nextGeneration(float mutationRate = 0.01f) {
        size_t popSize = population.size();
        std::vector<Individual> newPopulation;
        // Elitism: keep top 20%
        size_t eliteCount = popSize / 5;
        for (size_t i = 0; i < eliteCount; ++i) {
            newPopulation.push_back(population[i]);
        }
        // Crossover
        while (newPopulation.size() < popSize) {
            Individual parent1 = population[rand() % (popSize / 2)];
            Individual parent2 = population[rand() % (popSize / 2)];
            Individual child = Individual::crossover(parent1, parent2);
            child.mutate(mutationRate);
            newPopulation.push_back(child);
        }
        population = newPopulation;
    }

    // Simple Q-Learning Agent
    struct QLearningAgent {
        std::vector<std::vector<float>> qTable; // State x Action values
        size_t numStates, numActions;
        float learningRate, discountFactor, explorationRate;

        QLearningAgent(size_t states, size_t actions, float lr = 0.1f, float df = 0.9f, float er = 0.1f)
            : numStates(states), numActions(actions), learningRate(lr), discountFactor(df), explorationRate(er),
              qTable(states, std::vector<float>(actions, 0.0f)) {}

        size_t chooseAction(size_t state) {
            if ((rand() % 100) / 100.0f < explorationRate) {
                return rand() % numActions; // Explore
            } else {
                // Exploit: choose best action
                size_t bestAction = 0;
                float bestValue = qTable[state][0];
                for (size_t a = 1; a < numActions; ++a) {
                    if (qTable[state][a] > bestValue) {
                        bestValue = qTable[state][a];
                        bestAction = a;
                    }
                }
                return bestAction;
            }
        }

        void learn(size_t state, size_t action, size_t nextState, float reward) {
            float maxNextQ = *std::max_element(qTable[nextState].begin(), qTable[nextState].end());
            qTable[state][action] += learningRate * (reward + discountFactor * maxNextQ - qTable[state][action]);
        }

        void decreaseExploration(float decay = 0.995f) {
            explorationRate *= decay;
            if (explorationRate < 0.01f) explorationRate = 0.01f;
        }
    };

    QLearningAgent qAgent{100, 4}; // Example: 100 states, 4 actions

    Perceptron perceptron;

    void update(float deltaTime) override {
        // Update AI agents, path following, etc.
        for (auto* aiComp : aiComponents) {
            if (aiComp) aiComp->update();
        }
        // Update flocking
        updateFlocking(deltaTime);
    }
};

} // namespace FoundryEngine

#endif // NEUTRAL_GAMEENGINE_AI_SYSTEM_H
