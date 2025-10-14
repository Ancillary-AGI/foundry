#include "../include/GameEngine/core/Scene.h"
#include "../include/GameEngine/core/Entity.h"
#include <algorithm>
#include <fstream>
#include <iostream>

namespace FoundryEngine {

Scene::Scene(const std::string& name) : name_(name), nextEntityId_(1) {
}

Scene::~Scene() {
    // Clean up entities
    entities_.clear();
    namedEntities_.clear();
    parentMap_.clear();
    childrenMap_.clear();
}

Entity* Scene::createEntity(const std::string& name) {
    auto entity = std::make_unique<Entity>(nextEntityId_++);
    Entity* entityPtr = entity.get();
    
    entities_.push_back(std::move(entity));
    
    if (!name.empty()) {
        namedEntities_[name] = entityPtr;
    }
    
    return entityPtr;
}

void Scene::destroyEntity(Entity* entity) {
    if (!entity) return;
    
    // Remove from named entities
    for (auto it = namedEntities_.begin(); it != namedEntities_.end(); ++it) {
        if (it->second == entity) {
            namedEntities_.erase(it);
            break;
        }
    }
    
    // Remove from hierarchy
    Entity* parent = getParent(entity);
    if (parent) {
        auto& children = childrenMap_[parent];
        children.erase(std::remove(children.begin(), children.end(), entity), children.end());
        parentMap_.erase(entity);
    }
    
    // Remove children
    auto childrenIt = childrenMap_.find(entity);
    if (childrenIt != childrenMap_.end()) {
        for (Entity* child : childrenIt->second) {
            parentMap_.erase(child);
        }
        childrenMap_.erase(childrenIt);
    }
    
    // Remove from entities list
    entities_.erase(std::remove_if(entities_.begin(), entities_.end(),
        [entity](const std::unique_ptr<Entity>& e) { return e.get() == entity; }),
        entities_.end());
}

Entity* Scene::findEntity(const std::string& name) {
    auto it = namedEntities_.find(name);
    return (it != namedEntities_.end()) ? it->second : nullptr;
}

std::vector<Entity*> Scene::findEntitiesWithTag(const std::string& tag) {
    std::vector<Entity*> result;
    // This would require adding tag support to Entity class
    // For now, return empty vector
    return result;
}

void Scene::setParent(Entity* child, Entity* parent) {
    if (!child) return;
    
    // Remove from current parent
    Entity* currentParent = getParent(child);
    if (currentParent) {
        auto& children = childrenMap_[currentParent];
        children.erase(std::remove(children.begin(), children.end(), child), children.end());
    }
    
    // Set new parent
    if (parent) {
        parentMap_[child] = parent;
        childrenMap_[parent].push_back(child);
    } else {
        parentMap_.erase(child);
    }
}

Entity* Scene::getParent(Entity* entity) {
    if (!entity) return nullptr;
    auto it = parentMap_.find(entity);
    return (it != parentMap_.end()) ? it->second : nullptr;
}

std::vector<Entity*> Scene::getChildren(Entity* entity) {
    if (!entity) return {};
    auto it = childrenMap_.find(entity);
    return (it != childrenMap_.end()) ? it->second : std::vector<Entity*>();
}

void Scene::addLight(Light* light) {
    if (light) {
        lights_.push_back(light);
    }
}

void Scene::removeLight(Light* light) {
    lights_.erase(std::remove(lights_.begin(), lights_.end(), light), lights_.end());
}

void Scene::save(const std::string& path) {
    // Basic serialization - would need proper implementation
    std::ofstream file(path);
    if (file.is_open()) {
        file << "Scene: " << name_ << std::endl;
        file << "Entities: " << entities_.size() << std::endl;
        file.close();
    }
}

bool Scene::load(const std::string& path) {
    // Basic deserialization - would need proper implementation
    std::ifstream file(path);
    if (file.is_open()) {
        std::string line;
        std::getline(file, line); // Read scene name
        std::getline(file, line); // Read entity count
        file.close();
        return true;
    }
    return false;
}

void Scene::update(float deltaTime) {
    // Update all entities
    for (auto& entity : entities_) {
        // Update entity components
        for (auto& component : entity->components) {
            // Component update logic would go here
        }
    }
}

} // namespace FoundryEngine
