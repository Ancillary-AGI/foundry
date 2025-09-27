#ifndef NEUTRAL_GAMEENGINE_BEHAVIOR_TREE_H
#define NEUTRAL_GAMEENGINE_BEHAVIOR_TREE_H

#include <vector>
#include <memory>
#include <functional>

namespace FoundryEngine {

class BehaviorTree {
public:
    class Node {
    public:
        virtual ~Node() = default;
        virtual bool run() = 0;  // Return true if success
    };

    class Action : public Node {
    public:
        Action(std::function<bool()> func) : function(func) {}
        bool run() override { return function(); }
    private:
        std::function<bool()> function;
    };

    class Condition : public Node {
    public:
        Condition(std::function<bool()> cond) : condition(cond) {}
        bool run() override { return condition(); }
    private:
        std::function<bool()> condition;
    };

    class Sequence : public Node {
    public:
        Sequence(std::vector<std::unique_ptr<Node>> children) : children_(std::move(children)) {}
        bool run() override {
            for (auto& child : children_) {
                if (!child->run()) return false;
            }
            return true;
        }
    private:
        std::vector<std::unique_ptr<Node>> children_;
    };

    class Selector : public Node {
    public:
        Selector(std::vector<std::unique_ptr<Node>> children) : children_(std::move(children)) {}
        bool run() override {
            for (auto& child : children_) {
                if (child->run()) return true;
            }
            return false;
        }
    private:
        std::vector<std::unique_ptr<Node>> children_;
    };

    // Decorators like Inverter, Succeeder could be added

    std::unique_ptr<Node> root;

    void setRoot(std::unique_ptr<Node> r) { root = std::move(r); }

    bool execute() {
        if (root) return root->run();
        return false;
    }
};

} // namespace FoundryEngine

#endif // NEUTRAL_GAMEENGINE_BEHAVIOR_TREE_H
