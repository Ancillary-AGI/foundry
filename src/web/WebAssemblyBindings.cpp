#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

#include "../../include/GameEngine/core/Engine.h"
#include "../../include/GameEngine/math/Vector3.h"
#include "../../include/GameEngine/math/Matrix4.h"
#include "../../include/GameEngine/math/Quaternion.h"
#include "../../include/GameEngine/components/TransformComponent.h"
#include "../../include/GameEngine/core/World.h"
#include "../../include/GameEngine/core/Scene.h"

using namespace FoundryEngine;
using namespace emscripten;

// Global engine instance
static Engine* g_engine = nullptr;
static World* g_world = nullptr;

// Memory management for cross-language boundaries
class WASMMemoryManager {
private:
    std::unordered_map<uintptr_t, std::unique_ptr<void, std::function<void(void*)>>> managed_objects_;
    std::atomic<uintptr_t> next_id_{1};

public:
    uintptr_t store_object(std::unique_ptr<void, std::function<void(void*)>> obj) {
        uintptr_t id = next_id_.fetch_add(1);
        managed_objects_[id] = std::move(obj);
        return id;
    }

    void* get_object(uintptr_t id) {
        auto it = managed_objects_.find(id);
        return (it != managed_objects_.end()) ? it->second.get() : nullptr;
    }

    void release_object(uintptr_t id) {
        managed_objects_.erase(id);
    }

    void cleanup() {
        managed_objects_.clear();
    }
};

static WASMMemoryManager g_memory_manager;

// Exception handling wrapper
template<typename Func>
auto safe_call(Func&& func) -> decltype(func()) {
    try {
        return func();
    } catch (const std::exception& e) {
        EM_ASM({
            console.error('C++ Exception:', UTF8ToString($0));
        }, e.what());
        throw;
    } catch (...) {
        EM_ASM({
            console.error('Unknown C++ Exception');
        });
        throw;
    }
}

// Vector3 bindings
class Vector3Wrapper {
private:
    Vector3* vector_;

public:
    Vector3Wrapper() : vector_(new Vector3()) {}
    Vector3Wrapper(float x, float y, float z) : vector_(new Vector3(x, y, z)) {}
    Vector3Wrapper(const Vector3& v) : vector_(new Vector3(v)) {}
    ~Vector3Wrapper() { delete vector_; }

    Vector3Wrapper(const Vector3Wrapper& other) : vector_(new Vector3(*other.vector_)) {}
    Vector3Wrapper& operator=(const Vector3Wrapper& other) {
        if (this != &other) {
            *vector_ = *other.vector_;
        }
        return *this;
    }

    float getX() const { return vector_->x; }
    float getY() const { return vector_->y; }
    float getZ() const { return vector_->z; }
    
    void setX(float x) { vector_->x = x; }
    void setY(float y) { vector_->y = y; }
    void setZ(float z) { vector_->z = z; }

    Vector3Wrapper add(const Vector3Wrapper& other) const {
        return Vector3Wrapper(*vector_ + *other.vector_);
    }

    Vector3Wrapper subtract(const Vector3Wrapper& other) const {
        return Vector3Wrapper(*vector_ - *other.vector_);
    }

    Vector3Wrapper multiply(float scalar) const {
        return Vector3Wrapper(*vector_ * scalar);
    }

    float dot(const Vector3Wrapper& other) const {
        return vector_->dot(*other.vector_);
    }

    Vector3Wrapper cross(const Vector3Wrapper& other) const {
        return Vector3Wrapper(vector_->cross(*other.vector_));
    }

    float length() const {
        return vector_->length();
    }

    Vector3Wrapper normalize() const {
        return Vector3Wrapper(vector_->normalized());
    }

    const Vector3& getVector() const { return *vector_; }
};

// Matrix4 bindings
class Matrix4Wrapper {
private:
    Matrix4* matrix_;

public:
    Matrix4Wrapper() : matrix_(new Matrix4()) {}
    Matrix4Wrapper(const Matrix4& m) : matrix_(new Matrix4(m)) {}
    ~Matrix4Wrapper() { delete matrix_; }

    Matrix4Wrapper(const Matrix4Wrapper& other) : matrix_(new Matrix4(*other.matrix_)) {}
    Matrix4Wrapper& operator=(const Matrix4Wrapper& other) {
        if (this != &other) {
            *matrix_ = *other.matrix_;
        }
        return *this;
    }

    Matrix4Wrapper multiply(const Matrix4Wrapper& other) const {
        return Matrix4Wrapper(*matrix_ * *other.matrix_);
    }

    Vector3Wrapper transformPoint(const Vector3Wrapper& point) const {
        return Vector3Wrapper(matrix_->transformPoint(point.getVector()));
    }

    Vector3Wrapper transformDirection(const Vector3Wrapper& direction) const {
        return Vector3Wrapper(matrix_->transformDirection(direction.getVector()));
    }

    static Matrix4Wrapper identity() {
        return Matrix4Wrapper(Matrix4::identity());
    }

    static Matrix4Wrapper translation(const Vector3Wrapper& translation) {
        return Matrix4Wrapper(Matrix4::translation(translation.getVector()));
    }

    static Matrix4Wrapper rotation(const Vector3Wrapper& axis, float angle) {
        return Matrix4Wrapper(Matrix4::rotation(axis.getVector(), angle));
    }

    static Matrix4Wrapper scale(const Vector3Wrapper& scale) {
        return Matrix4Wrapper(Matrix4::scale(scale.getVector()));
    }

    const Matrix4& getMatrix() const { return *matrix_; }
};

// Engine bindings
class EngineWrapper {
public:
    static bool initialize() {
        return safe_call([]() {
            g_engine = &Engine::getInstance();
            if (g_engine->initialize()) {
                g_world = g_engine->getWorld();
                return true;
            }
            return false;
        });
    }

    static void shutdown() {
        safe_call([]() {
            if (g_engine) {
                g_engine->shutdown();
                g_engine = nullptr;
                g_world = nullptr;
            }
            g_memory_manager.cleanup();
        });
    }

    static void update(float deltaTime) {
        safe_call([deltaTime]() {
            if (g_engine) {
                g_engine->update(deltaTime);
            }
        });
    }

    static void render() {
        safe_call([]() {
            if (g_engine) {
                g_engine->render();
            }
        });
    }

    static float getDeltaTime() {
        return safe_call([]() {
            return g_engine ? g_engine->getDeltaTime() : 0.0f;
        });
    }

    static uint64_t getFrameCount() {
        return safe_call([]() {
            return g_engine ? g_engine->getFrameCount() : 0;
        });
    }

    static bool isRunning() {
        return safe_call([]() {
            return g_engine != nullptr;
        });
    }
};

// World bindings
class WorldWrapper {
public:
    static uint32_t createEntity() {
        return safe_call([]() {
            return g_world ? g_world->createEntity() : 0;
        });
    }

    static void destroyEntity(uint32_t entityId) {
        safe_call([entityId]() {
            if (g_world) {
                g_world->destroyEntity(entityId);
            }
        });
    }

    static bool hasComponent(uint32_t entityId, const std::string& componentType) {
        return safe_call([entityId, &componentType]() {
            if (!g_world) return false;
            
            // This would need to be expanded based on actual component system
            if (componentType == "Transform") {
                return g_world->hasComponent<TransformComponent>(entityId);
            }
            return false;
        });
    }

    static uintptr_t addTransformComponent(uint32_t entityId, float x, float y, float z) {
        return safe_call([entityId, x, y, z]() {
            if (!g_world) return 0;
            
            auto component = std::make_unique<TransformComponent>();
            component->position = Vector3(x, y, z);
            
            g_world->addComponent<TransformComponent>(entityId, *component);
            
            return g_memory_manager.store_object(
                std::unique_ptr<void, std::function<void(void*)>>(
                    component.release(),
                    [](void* ptr) { delete static_cast<TransformComponent*>(ptr); }
                )
            );
        });
    }

    static void updateTransformComponent(uintptr_t componentId, float x, float y, float z) {
        safe_call([componentId, x, y, z]() {
            auto* component = static_cast<TransformComponent*>(g_memory_manager.get_object(componentId));
            if (component) {
                component->position = Vector3(x, y, z);
            }
        });
    }

    static void removeComponent(uint32_t entityId, const std::string& componentType) {
        safe_call([entityId, &componentType]() {
            if (!g_world) return;
            
            if (componentType == "Transform") {
                g_world->removeComponent<TransformComponent>(entityId);
            }
        });
    }
};

// Scene bindings
class SceneWrapper {
public:
    static uintptr_t createScene(const std::string& name) {
        return safe_call([&name]() {
            if (!g_engine) return 0;
            
            auto* sceneManager = g_engine->getScenes();
            if (sceneManager) {
                auto* scene = sceneManager->createScene(name);
                return g_memory_manager.store_object(
                    std::unique_ptr<void, std::function<void(void*)>>(
                        scene,
                        [](void* ptr) { /* Scene cleanup handled by scene manager */ }
                    )
                );
            }
            return 0;
        });
    }

    static void setActiveScene(uintptr_t sceneId) {
        safe_call([sceneId]() {
            if (!g_engine) return;
            
            auto* scene = static_cast<Scene*>(g_memory_manager.get_object(sceneId));
            auto* sceneManager = g_engine->getScenes();
            if (scene && sceneManager) {
                sceneManager->setActiveScene(scene);
            }
        });
    }

    static void addEntityToScene(uintptr_t sceneId, uint32_t entityId) {
        safe_call([sceneId, entityId]() {
            auto* scene = static_cast<Scene*>(g_memory_manager.get_object(sceneId));
            if (scene) {
                scene->addEntity(entityId);
            }
        });
    }

    static void removeEntityFromScene(uintptr_t sceneId, uint32_t entityId) {
        safe_call([sceneId, entityId]() {
            auto* scene = static_cast<Scene*>(g_memory_manager.get_object(sceneId));
            if (scene) {
                scene->removeEntity(entityId);
            }
        });
    }
};

// Memory management bindings
class MemoryWrapper {
public:
    static void releaseObject(uintptr_t objectId) {
        safe_call([objectId]() {
            g_memory_manager.release_object(objectId);
        });
    }

    static size_t getManagedObjectCount() {
        return safe_call([]() {
            // This would need to be implemented in WASMMemoryManager
            return 0; // Placeholder
        });
    }
};

// Emscripten bindings
EMSCRIPTEN_BINDINGS(foundry_engine) {
    // Vector3 bindings
    class_<Vector3Wrapper>("Vector3")
        .constructor<>()
        .constructor<float, float, float>()
        .property("x", &Vector3Wrapper::getX, &Vector3Wrapper::setX)
        .property("y", &Vector3Wrapper::getY, &Vector3Wrapper::setY)
        .property("z", &Vector3Wrapper::getZ, &Vector3Wrapper::setZ)
        .function("add", &Vector3Wrapper::add)
        .function("subtract", &Vector3Wrapper::subtract)
        .function("multiply", &Vector3Wrapper::multiply)
        .function("dot", &Vector3Wrapper::dot)
        .function("cross", &Vector3Wrapper::cross)
        .function("length", &Vector3Wrapper::length)
        .function("normalize", &Vector3Wrapper::normalize);

    // Matrix4 bindings
    class_<Matrix4Wrapper>("Matrix4")
        .constructor<>()
        .function("multiply", &Matrix4Wrapper::multiply)
        .function("transformPoint", &Matrix4Wrapper::transformPoint)
        .function("transformDirection", &Matrix4Wrapper::transformDirection)
        .class_function("identity", &Matrix4Wrapper::identity)
        .class_function("translation", &Matrix4Wrapper::translation)
        .class_function("rotation", &Matrix4Wrapper::rotation)
        .class_function("scale", &Matrix4Wrapper::scale);

    // Engine bindings
    class_<EngineWrapper>("Engine")
        .class_function("initialize", &EngineWrapper::initialize)
        .class_function("shutdown", &EngineWrapper::shutdown)
        .class_function("update", &EngineWrapper::update)
        .class_function("render", &EngineWrapper::render)
        .class_function("getDeltaTime", &EngineWrapper::getDeltaTime)
        .class_function("getFrameCount", &EngineWrapper::getFrameCount)
        .class_function("isRunning", &EngineWrapper::isRunning);

    // World bindings
    class_<WorldWrapper>("World")
        .class_function("createEntity", &WorldWrapper::createEntity)
        .class_function("destroyEntity", &WorldWrapper::destroyEntity)
        .class_function("hasComponent", &WorldWrapper::hasComponent)
        .class_function("addTransformComponent", &WorldWrapper::addTransformComponent)
        .class_function("updateTransformComponent", &WorldWrapper::updateTransformComponent)
        .class_function("removeComponent", &WorldWrapper::removeComponent);

    // Scene bindings
    class_<SceneWrapper>("Scene")
        .class_function("createScene", &SceneWrapper::createScene)
        .class_function("setActiveScene", &SceneWrapper::setActiveScene)
        .class_function("addEntityToScene", &SceneWrapper::addEntityToScene)
        .class_function("removeEntityFromScene", &SceneWrapper::removeEntityFromScene);

    // Memory management bindings
    class_<MemoryWrapper>("Memory")
        .class_function("releaseObject", &MemoryWrapper::releaseObject)
        .class_function("getManagedObjectCount", &MemoryWrapper::getManagedObjectCount);

    // Register value types
    register_vector<uint32_t>("EntityIdVector");
    register_vector<std::string>("StringVector");
}

// Export functions for direct C API access
extern "C" {
    EMSCRIPTEN_KEEPALIVE
    bool initializeEngine() {
        return EngineWrapper::initialize();
    }

    EMSCRIPTEN_KEEPALIVE
    void shutdownEngine() {
        EngineWrapper::shutdown();
    }

    EMSCRIPTEN_KEEPALIVE
    void updateEngine(float deltaTime) {
        EngineWrapper::update(deltaTime);
    }

    EMSCRIPTEN_KEEPALIVE
    void renderFrame() {
        EngineWrapper::render();
    }

    EMSCRIPTEN_KEEPALIVE
    float getEngineDeltaTime() {
        return EngineWrapper::getDeltaTime();
    }

    EMSCRIPTEN_KEEPALIVE
    uint64_t getEngineFrameCount() {
        return EngineWrapper::getFrameCount();
    }

    EMSCRIPTEN_KEEPALIVE
    bool isEngineRunning() {
        return EngineWrapper::isRunning();
    }

    EMSCRIPTEN_KEEPALIVE
    uint32_t createEntity() {
        return WorldWrapper::createEntity();
    }

    EMSCRIPTEN_KEEPALIVE
    void destroyEntity(uint32_t entityId) {
        WorldWrapper::destroyEntity(entityId);
    }

    EMSCRIPTEN_KEEPALIVE
    uintptr_t addTransformComponent(uint32_t entityId, float x, float y, float z) {
        return WorldWrapper::addTransformComponent(entityId, x, y, z);
    }

    EMSCRIPTEN_KEEPALIVE
    void updateTransformComponent(uintptr_t componentId, float x, float y, float z) {
        WorldWrapper::updateTransformComponent(componentId, x, y, z);
    }

    EMSCRIPTEN_KEEPALIVE
    void releaseObject(uintptr_t objectId) {
        MemoryWrapper::releaseObject(objectId);
    }
}
