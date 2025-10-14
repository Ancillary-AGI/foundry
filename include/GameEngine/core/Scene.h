#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "Entity.h"

namespace FoundryEngine {

class Camera;
class Light;
class Environment;

class Scene {
public:
    Scene(const std::string& name);
    ~Scene();
    
    // Entity management
    Entity* createEntity(const std::string& name = "");
    void destroyEntity(Entity* entity);
    Entity* findEntity(const std::string& name);
    std::vector<Entity*> findEntitiesWithTag(const std::string& tag);
    
    // Scene hierarchy
    void setParent(Entity* child, Entity* parent);
    Entity* getParent(Entity* entity);
    std::vector<Entity*> getChildren(Entity* entity);
    
    // Camera management
    void setMainCamera(Camera* camera) { mainCamera_ = camera; }
    Camera* getMainCamera() const { return mainCamera_; }
    
    // Lighting
    void addLight(Light* light);
    void removeLight(Light* light);
    std::vector<Light*> getLights() const { return lights_; }
    
    // Environment
    void setEnvironment(Environment* env) { environment_ = env; }
    Environment* getEnvironment() const { return environment_; }
    
    // Scene properties
    const std::string& getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; }
    
    // Serialization
    void save(const std::string& path);
    bool load(const std::string& path);
    
    // Update
    void update(float deltaTime);
    
private:
    std::string name_;
    std::vector<std::unique_ptr<Entity>> entities_;
    std::unordered_map<std::string, Entity*> namedEntities_;
    std::unordered_map<Entity*, Entity*> parentMap_;
    std::unordered_map<Entity*, std::vector<Entity*>> childrenMap_;
    
    Camera* mainCamera_ = nullptr;
    std::vector<Light*> lights_;
    Environment* environment_ = nullptr;
    
    uint32_t nextEntityId_ = 1;
};

class SceneManager {
public:
    Scene* createScene(const std::string& name);
    void destroyScene(Scene* scene);
    Scene* getScene(const std::string& name);
    
    void setActiveScene(Scene* scene) { activeScene_ = scene; }
    Scene* getActiveScene() const { return activeScene_; }
    
    void loadScene(const std::string& path);
    void loadSceneAsync(const std::string& path, std::function<void(Scene*)> callback);
    
    void update(float deltaTime);
    
private:
    std::vector<std::unique_ptr<Scene>> scenes_;
    std::unordered_map<std::string, Scene*> namedScenes_;
    Scene* activeScene_ = nullptr;
};

} // namespace FoundryEngine
