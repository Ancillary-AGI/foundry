#ifndef FOUNDRY_GAMEENGINE_SYSTEM_H
#define FOUNDRY_GAMEENGINE_SYSTEM_H

namespace FoundryEngine {

class System {
public:
    virtual void update(float deltaTime) = 0;
    virtual ~System() = default;
};

} // namespace FoundryEngine

#endif // FOUNDRY_GAMEENGINE_SYSTEM_H
