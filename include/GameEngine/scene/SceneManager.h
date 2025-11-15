/**
 * @file SceneManager.h
 * @brief Scene and level management system
 */

#pragma once

#include "../core/System.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace FoundryEngine {

// Forward declarations
class Scene;

/**
 * @class SceneManager
 * @brief Manages scene loading, saving, and transitions
 */
class SceneManager : public System {
public:
    SceneManager() = default;
    virtual ~SceneManager() = default;

    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update(float deltaTime) = 0;

    // Scene management
    virtual Scene* createScene(const std::string& sceneName) = 0;
    virtual bool destroyScene(const std::string& sceneName) = 0;
    virtual Scene* getScene(const std::string& sceneName) const = 0;
    virtual Scene* getActiveScene() const = 0;
    virtual bool setActiveScene(const std::string& sceneName) = 0;
    virtual bool setActiveScene(Scene* scene) = 0;

    // Scene serialization
    virtual bool loadScene(const std::string& sceneFile) = 0;
    virtual bool saveScene(const std::string& sceneFile, const std::string& sceneName) = 0;

    // Scene transitions
    virtual void transitionToScene(const std::string& sceneName, float transitionTime = 0.0f) = 0;
    virtual bool isTransitioning() const = 0;
    virtual float getTransitionProgress() const = 0;
};

/**
 * @class DefaultSceneManager
 * @brief Default scene manager implementation
 */
class DefaultSceneManager : public SceneManager {
public:
    bool initialize() override { return true; }
    void shutdown() override {}
    void update(float deltaTime) override {}

    Scene* createScene(const std::string& sceneName) override { return nullptr; }
    bool destroyScene(const std::string& sceneName) override { return false; }
    Scene* getScene(const std::string& sceneName) const override { return nullptr; }
    Scene* getActiveScene() const override { return nullptr; }
    bool setActiveScene(const std::string& sceneName) override { return false; }
    bool setActiveScene(Scene* scene) override { return false; }

    bool loadScene(const std::string& sceneFile) override { return false; }
    bool saveScene(const std::string& sceneFile, const std::string& sceneName) override { return false; }

    void transitionToScene(const std::string& sceneName, float transitionTime = 0.0f) override {}
    bool isTransitioning() const override { return false; }
    float getTransitionProgress() const override { return 0.0f; }
};

} // namespace FoundryEngine
