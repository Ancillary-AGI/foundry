#ifndef NEUTRAL_GAMEENGINE_WORLD_H
#define NEUTRAL_GAMEENGINE_WORLD_H

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <typeindex>
#include <type_traits>
#include "../math/Vector3.h" // assuming for SIMD
#include <emmintrin.h> // SIMD headers

namespace FoundryEngine {

using EntityID = uint32_t;
using ComponentID = uint32_t;
using ArchetypeID = uint32_t;

const EntityID INVALID_ENTITY = ~0u;

// Type-erased component storage (SoA for SIMD)
struct ComponentStore {
    virtual ~ComponentStore() = default;
    virtual void* get(EntityID entity) = 0;
    virtual const void* get(EntityID entity) const = 0;
    virtual void add(EntityID entity, void* data) = 0;
    virtual void remove(EntityID entity) = 0;
    virtual size_t size() const = 0;
    virtual ComponentID id() const = 0;
};

// Templated component storage
template<typename T>
struct TypedComponentStore : ComponentStore {
    std::vector<EntityID> entities;
    std::vector<T> data;
    ComponentID compID;

    TypedComponentStore(ComponentID id) : compID(id) {}

    void* get(EntityID entity) override {
        for (size_t i = 0; i < entities.size(); ++i) {
            if (entities[i] == entity) return &data[i];
        }
        return nullptr;
    }

    const void* get(EntityID entity) const override {
        for (size_t i = 0; i < entities.size(); ++i) {
            if (entities[i] == entity) return &data[i];
        }
        return nullptr;
    }

    void add(EntityID entity, void* componentData) override {
        entities.push_back(entity);
        data.push_back(*(T*)componentData);
    }

    void remove(EntityID entity) override {
        for (size_t i = 0; i < entities.size(); ++i) {
            if (entities[i] == entity) {
                entities.erase(entities.begin() + i);
                data.erase(data.begin() + i);
                break;
            }
        }
    }

    size_t size() const override { return entities.size(); }
    ComponentID id() const override { return compID; }
};

// Archetype: collection of component types
struct Archetype {
    ArchetypeID id;
    std::unordered_set<ComponentID> componentTypes;
    std::unordered_map<ComponentID, std::unique_ptr<ComponentStore>> stores;
    std::vector<EntityID> entities;

    bool matches(const std::unordered_set<ComponentID>& types) const {
        return componentTypes == types;
    }
};

// Scriptable Component base
class ScriptableComponent {
public:
    virtual ~ScriptableComponent() = default;
    virtual void onUpdate(float dt) {}
    virtual void onInit() {}
    virtual void serialize(std::ostream& os) {}
    virtual void deserialize(std::istream& is) {}
};

// Runtime composition: allow adding components dynamically
class DynamicComponent {
public:
    virtual ~DynamicComponent() = default;
    virtual void update(float dt) = 0;
};

// Component manager for runtime composition and scripting
class ComponentManager {
public:
    static ComponentID getID() {
        static ComponentID nextID = 0;
        return nextID++;
    }

    template<typename T>
    static ComponentID getTypeID() {
        static ComponentID id = getID();
        return id;
    }
};

// ECS World/Registry
class World {
public:
    World();

    EntityID createEntity() {
        EntityID id = nextEntityID_++;
        entities_.insert(id);
        return id;
    }

    void destroyEntity(EntityID entity);

    // Add component to entity
    template<typename T>
    void addComponent(EntityID entity, T component) {
        ComponentID compID = ComponentManager::getTypeID<T>();
        // Find or create archetype
        std::unordered_set<ComponentID> oldTypes = getEntityArchetype(entity);
        std::unordered_set<ComponentID> newTypes = oldTypes;
        newTypes.insert(compID);

        if (oldTypes != newTypes) {
            moveEntityToNewArchetype(entity, oldTypes, newTypes);
        }

        // Add to archetype store
        Archetype* arch = getArchetype(newTypes);
        if (!arch->stores[compID]) {
            arch->stores[compID] = std::make_unique<TypedComponentStore<T>>(compID);
        }
        arch->stores[compID]->add(entity, &component);
        arch->entities.push_back(entity);
    }

    // Get component from entity
    template<typename T>
    T* getComponent(EntityID entity) {
        ComponentID compID = ComponentManager::getTypeID<T>();
        Archetype* arch = getEntityArchetypePtr(entity);
        if (!arch || arch->stores.find(compID) == arch->stores.end()) return nullptr;
        return (T*)arch->stores[compID]->get(entity);
    }

    // Remove component
    template<typename T>
    void removeComponent(EntityID entity) {
        ComponentID compID = ComponentManager::getTypeID<T>();
        std::unordered_set<ComponentID> oldTypes = getEntityArchetype(entity);
        std::unordered_set<ComponentID> newTypes = oldTypes;
        newTypes.erase(compID);

        if (oldTypes != newTypes) {
            moveEntityToNewArchetype(entity, oldTypes, newTypes);
        }
    }

    // Query entities with specific components
    template<typename... Components>
    std::vector<EntityID> query() {
        std::unordered_set<ComponentID> types = {ComponentManager::getTypeID<Components>()...};
        std::vector<EntityID> result;
        for (auto& arch : archetypes_) {
            if (arch.matches(types)) {
                result.insert(result.end(), arch.entities.begin(), arch.entities.end());
            }
        }
        return result;
    }

    // For-each with SIMD-support (example for Vector3)
    template<typename Func>
    void forEachVector3(ComponentID compID, Func func) {
        for (auto& arch : archetypes_) {
            if (arch.stores.find(compID) != arch.stores.end()) {
                TypedComponentStore<Vector3>* store = dynamic_cast<TypedComponentStore<Vector3>*>(arch.stores[compID].get());
                if (store && store->size() >= 4) { // SIMD for vectors of 4
                    // SIMD update example (simplified)
                    for (size_t i = 0; i < store->data.size(); i += 4) {
                        // Load 4 vectors
                        __m128 x = _mm_set_ps(store->data[i].x, store->data[i+1].x, store->data[i+2].x, store->data[i+3].x);
                        __m128 y = _mm_set_ps(store->data[i].y, store->data[i+1].y, store->data[i+2].y, store->data[i+3].y);
                        __m128 z = _mm_set_ps(store->data[i].z, store->data[i+1].z, store->data[i+2].z, store->data[i+3].z);

                        // Apply func (example: add gravity)
                        __m128 dt = _mm_set1_ps(0.016f); // Assume delta time
                        __m128 gravity = _mm_set1_ps(-9.81f * 0.016f);

                        y = _mm_add_ps(y, gravity);

                        // Store back
                        _mm_store_ps(&store->data[i].x, x);
                        _mm_store_ps(&store->data[i].y, y);
                        _mm_store_ps(&store->data[i].z, z);

                        for (size_t j = 0; j < 4 && i + j < store->size(); ++j) {
                            func(store->entities[i + j], store->data[i + j]);
                        }
                    }
                } else {
                    // Non-SIMD fallback
                    for (size_t i = 0; i < store->data.size(); ++i) {
                        func(store->entities[i], store->data[i]);
                    }
                }
            }
        }
    }

private:
    std::unordered_set<EntityID> entities_;
    std::vector<Archetype> archetypes_;
    EntityID nextEntityID_ = 0;
    ArchetypeID nextArchetypeID_ = 0;

    std::unordered_set<ComponentID> getEntityArchetype(EntityID entity) {
        for (auto& arch : archetypes_) {
            for (EntityID e : arch.entities) {
                if (e == entity) return arch.componentTypes;
            }
        }
        return {};
    }

    Archetype* getEntityArchetypePtr(EntityID entity) {
        for (auto& arch : archetypes_) {
            for (EntityID e : arch.entities) {
                if (e == entity) return &arch;
            }
        }
        return nullptr;
    }

    Archetype* getArchetype(const std::unordered_set<ComponentID>& types) {
        for (auto& arch : archetypes_) {
            if (arch.matches(types)) return &arch;
        }
        // Create new archetype
        archetypes_.emplace_back();
        Archetype& newArch = archetypes_.back();
        newArch.id = nextArchetypeID_++;
        newArch.componentTypes = types;
        return &newArch;
    }

    void moveEntityToNewArchetype(EntityID entity, const std::unordered_set<ComponentID>& oldTypes, const std::unordered_set<ComponentID>& newTypes) {
        Archetype* oldArch = nullptr;
        Archetype* newArch = nullptr;

        // Remove from old archetype
        for (auto& arch : archetypes_) {
            if (arch.matches(oldTypes)) {
                for (size_t i = 0; i < arch.entities.size(); ++i) {
                    if (arch.entities[i] == entity) {
                        // Copy component data to new archetype
                        if (newArch) {
                            for (auto& comp : arch.stores) {
                                if (newTypes.count(comp.first)) {
                                    void* data = comp.second->get(entity);
                                    newArch->stores[comp.first]->add(entity, data);
                                }
                            }
                        }
                        // Remove
                        arch.entities.erase(arch.entities.begin() + i);
                        for (auto& comp : arch.stores) {
                            comp.second->remove(entity);
                        }
                        oldArch = &arch;
                        break;
                    }
                }
            }
        }

        // Add to new archetype
        if (!oldArch) return; // Entity wasn't in any archetype
        newArch = getArchetype(newTypes);
        // Data already added above
        newArch->entities.push_back(entity);
    }
};

// Serialization with versioning for prefabs
struct Prefab {
    std::string name;
    uint32_t version;
    std::unordered_map<ComponentID, std::vector<std::byte>> componentData;

    void serialize(std::ostream& os) const;
    void deserialize(std::istream& is);
};

class PrefabManager {
public:
    void savePrefab(const std::string& name, EntityID entity, World& world);
    EntityID loadPrefab(const std::string& name, World& world);

private:
    std::unordered_map<std::string, Prefab> prefabs_;
};

} // namespace FoundryEngine

#endif // NEUTRAL_GAMEENGINE_WORLD_H
