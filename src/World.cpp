#include "../include/GameEngine/core/World.h"
#include <algorithm>
#include <stdexcept>
#include <iostream>

namespace FoundryEngine {

World::World() : nextEntityID_(1), nextArchetypeID_(1) {
    // Initialize with empty archetype
    archetypes_.emplace_back();
    archetypes_.back().id = nextArchetypeID_++;
    metrics_.archetypeCount = 1;
}

World::~World() {
    // Clean up all archetypes and their stores
    archetypes_.clear();
    entities_.clear();
}

EntityID World::createEntity() {
    EntityID id = nextEntityID_++;
    entities_.insert(id);
    metrics_.entityCount++;
    return id;
}

bool World::destroyEntity(EntityID entity) {
    if (entities_.find(entity) == entities_.end()) {
        return false; // Entity doesn't exist
    }

    // Find and remove entity from all archetypes
    for (auto& arch : archetypes_) {
        auto it = std::find(arch.entities.begin(), arch.entities.end(), entity);
        if (it != arch.entities.end()) {
            // Remove from all component stores
            for (auto& store : arch.stores) {
                store.second->remove(entity);
                metrics_.componentCount--;
            }
            arch.entities.erase(it);
            break;
        }
    }

    // Remove from entities set
    entities_.erase(entity);
    metrics_.entityCount--;

    return true;
}

} // namespace FoundryEngine
