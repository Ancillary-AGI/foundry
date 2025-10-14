#ifndef FOUNDRY_GAMEENGINE_AI_COMPONENT_H
#define FOUNDRY_GAMEENGINE_AI_COMPONENT_H

#include "../ai/DecisionTree.h"
#include "../ai/BehaviorTree.h"
#include "../ai/FiniteStateMachine.h"
#include "Component.h"

namespace FoundryEngine {

class AIComponent : public Component {
public:
    std::unique_ptr<DecisionTree> decisionTree;
    std::unique_ptr<BehaviorTree> behaviorTree;
    std::unique_ptr<FiniteStateMachine> stateMachine;

    void setDecisionTree(std::unique_ptr<DecisionTree> dt) { decisionTree = std::move(dt); }
    void setBehaviorTree(std::unique_ptr<BehaviorTree> bt) { behaviorTree = std::move(bt); }
    void setStateMachine(std::unique_ptr<FiniteStateMachine> fsm) { stateMachine = std::move(fsm); }

    void update() {
        if (decisionTree) decisionTree->execute();
        if (behaviorTree) behaviorTree->execute();
        if (stateMachine) stateMachine->update();
    }
};

} // namespace FoundryEngine

#endif // FOUNDRY_GAMEENGINE_AI_COMPONENT_H
