#ifndef FOUNDRY_GAMEENGINE_FINITE_STATE_MACHINE_H
#define FOUNDRY_GAMEENGINE_FINITE_STATE_MACHINE_H

#include <unordered_map>
#include <functional>
#include <string>

namespace FoundryEngine {

class FiniteStateMachine {
public:
    using State = std::string;
    using TransitionFunction = std::function<bool()>;
    using Action = std::function<void()>;

    void addState(const State& state, Action enter, Action update, Action exit) {
        states[state] = {enter, update, exit};
    }

    void addTransition(const State& from, const State& to, TransitionFunction cond) {
        transitions[{from, to}] = cond;
    }

    void setInitialState(const State& state) {
        currentState = state;
        if (states[currentState].enter) states[currentState].enter();
    }

    void update() {
        // Check for transitions
        for (auto& trans : transitions) {
            if (trans.first.first == currentState && trans.second()) {
                const State& newState = trans.first.second;
                if (states[currentState].exit) states[currentState].exit();
                currentState = newState;
                if (states[currentState].enter) states[currentState].enter();
                break;
            }
        }

        // Update current state
        if (states[currentState].update) states[currentState].update();
    }

    const State& getCurrentState() const { return currentState; }

private:
    struct StateActions {
        Action enter;
        Action update;
        Action exit;
    };

    std::unordered_map<State, StateActions> states;
    std::unordered_map<std::pair<State, State>, TransitionFunction> transitions;
    State currentState;
};

} // namespace FoundryEngine

#endif // FOUNDRY_GAMEENGINE_FINITE_STATE_MACHINE_H
