#ifndef FOUNDRY_GAMEENGINE_DECISION_TREE_H
#define FOUNDRY_GAMEENGINE_DECISION_TREE_H

#include <functional>
#include <memory>

namespace FoundryEngine {

class DecisionTree {
public:
    struct Node {
        std::function<bool()> condition;
        std::function<void()> action;
        std::unique_ptr<Node> trueBranch;
        std::unique_ptr<Node> falseBranch;

        Node(std::function<bool()> cond = nullptr, std::function<void()> act = nullptr)
            : condition(cond), action(act), trueBranch(nullptr), falseBranch(nullptr) {}

        void addTrueBranch(std::unique_ptr<Node> child) { trueBranch = std::move(child); }
        void addFalseBranch(std::unique_ptr<Node> child) { falseBranch = std::move(child); }
    };

    std::unique_ptr<Node> root;

    DecisionTree() = default;

    void setRoot(std::unique_ptr<Node> r) { root = std::move(r); }

    void execute() {
        if (root) executeRecursive(root.get());
    }

private:
    void executeRecursive(Node* node) {
        if (!node) return;

        if (node->condition && node->condition()) {
            if (node->action) node->action();
            if (node->trueBranch) executeRecursive(node->trueBranch.get());
        } else {
            if (node->action) node->action();
            if (node->falseBranch) executeRecursive(node->falseBranch.get());
        }
    }
};

} // namespace FoundryEngine

#endif // FOUNDRY_GAMEENGINE_DECISION_TREE_H
