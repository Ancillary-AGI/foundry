#ifndef NEUTRAL_GAMEENGINE_ENTITY_H
#define NEUTRAL_GAMEENGINE_ENTITY_H

#include <vector>
#include <memory>
#include "../components/Component.h"

namespace FoundryEngine {

class Entity {
public:
    int id;
    std::vector<std::unique_ptr<Component>> components;

    Entity(int id_) : id(id_) {}

    template<typename T, typename... Args>
    void addComponent(Args&&... args) {
        components.push_back(std::make_unique<T>(std::forward<Args>(args)...));
    }

    template<typename T>
    T* getComponent() {
        for (auto& comp : components) {
            if (dynamic_cast<T*>(comp.get())) {
                return static_cast<T*>(comp.get());
            }
        }
        return nullptr;
    }

    template<typename T>
    bool hasComponent() {
        return getComponent<T>() != nullptr;
    }
};

} // namespace FoundryEngine

#endif // NEUTRAL_GAMEENGINE_ENTITY_H
