/**
 * @file ECS.h
 * @brief Entity-Component-System with SIMD optimizations and hierarchical support
 * @author FoundryEngine Team
 * @date 2024
 * @version 2.0.0
 *
 * This file contains the ECS architecture for FoundryEngine.
 * Features include SIMD-optimized component operations, hierarchical entity
 * relationships, advanced memory management, and automatic system parallelization.
 *
 * Key Features:
 * - SIMD-optimized component operations for maximum performance
 * - Hierarchical entity relationships with parent-child support
 * - Advanced component pooling and memory management
 * - Automatic system dependency resolution and parallelization
 * - Entity templates with inheritance and composition
 * - Component versioning for hot-reload support
 * - Advanced query system with filtering and sorting
 */

#pragma once

#include "System.h"
#include "Entity.h"
#include "../math/Vector3.h"
#include "../math/Matrix4.h"
#include "../math/Quaternion.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <typeindex>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>
#include <immintrin.h>  // For SIMD operations

namespace FoundryEngine {

// Forward declarations
class ComponentPool;
class EntityManager;
class SystemManager;
class ComponentQuery;
class EntityTemplate;
class ComponentSerializer;

/**
 * @brief Component type identifier with version support
 */
struct ComponentTypeInfo {
    std::type_index typeIndex;
    size_t typeSize;
    size_t alignment;
    uint32_t version;
    std::string typeName;
    std::function<void(void*)> constructor;
    std::function<void(void*)> destructor;
    std::function<void(void*, const void*)> copyConstructor;
    std::function<void(void*, void*)> moveConstructor;
};

/**
 * @brief Entity handle with generation counter for safe references
 */
struct EntityHandle {
    uint32_t id = 0;
    uint32_t generation = 0;
    
    bool operator==(const EntityHandle& other) const {
        return id == other.id && generation == other.generation;
    }
    
    bool operator!=(const EntityHandle& other) const {
        return !(*this == other);
    }
    
    bool isValid() const { return id != 0; }
    
    struct Hash {
        size_t operator()(const EntityHandle& handle) const {
            return std::hash<uint64_t>{}((uint64_t(handle.generation) << 32) | handle.id);
        }
    };
};

/**
 * @brief Component archetype for efficient storage and iteration
 */
class ComponentArchetype {
public:
    ComponentArchetype(const std::vector<std::type_index>& componentTypes);
    ~ComponentArchetype();

    // Entity management
    size_t addEntity(EntityHandle entity);
    void removeEntity(size_t index);
    void moveEntity(size_t fromIndex, size_t toIndex);

    // Component access
    void* getComponent(size_t entityIndex, std::type_index componentType);
    const void* getComponent(size_t entityIndex, std::type_index componentType) const;
    bool hasComponent(std::type_index componentType) const;

    // Bulk operations
    void* getComponentArray(std::type_index componentType);
    const void* getComponentArray(std::type_index componentType) const;
    size_t getEntityCount() const { return entityCount_; }
    size_t getCapacity() const { return capacity_; }

    // SIMD operations
    void simdTransformComponents(std::type_index componentType, 
                               const std::function<void(__m256*, size_t)>& operation);
    void simdUpdateComponents(std::type_index componentType, float deltaTime);

    // Archetype matching
    bool matches(const std::vector<std::type_index>& requiredTypes,
                const std::vector<std::type_index>& excludedTypes = {}) const;
    const std::vector<std::type_index>& getComponentTypes() const { return componentTypes_; }

private:
    std::vector<std::type_index> componentTypes_;
    std::unordered_map<std::type_index, size_t> componentOffsets_;
    std::unordered_map<std::type_index, size_t> componentSizes_;
    
    std::vector<EntityHandle> entities_;
    std::vector<uint8_t> componentData_;
    
    size_t entityCount_ = 0;
    size_t capacity_ = 0;
    size_t entityStride_ = 0;
    
    void resize(size_t newCapacity);
    void calculateLayout();
};

/**
 * @class EnhancedEntityManager
 * @brief Advanced entity manager with hierarchical support and efficient storage
 */
class EnhancedEntityManager {
public:
    EnhancedEntityManager();
    ~EnhancedEntityManager();

    // Entity lifecycle
    EntityHandle createEntity();
    EntityHandle createEntity(const std::string& name);
    EntityHandle createEntityFromTemplate(const std::string& templateName);
    void destroyEntity(EntityHandle entity);
    bool isEntityValid(EntityHandle entity) const;

    // Hierarchical relationships
    void setParent(EntityHandle child, EntityHandle parent);
    void removeParent(EntityHandle child);
    EntityHandle getParent(EntityHandle entity) const;
    std::vector<EntityHandle> getChildren(EntityHandle entity) const;
    std::vector<EntityHandle> getDescendants(EntityHandle entity) const;
    EntityHandle getRoot(EntityHandle entity) const;
    int getDepth(EntityHandle entity) const;

    // Component management
    template<typename T>
    T* addComponent(EntityHandle entity);
    
    template<typename T>
    T* addComponent(EntityHandle entity, const T& component);
    
    template<typename T>
    void removeComponent(EntityHandle entity);
    
    template<typename T>
    T* getComponent(EntityHandle entity);
    
    template<typename T>
    const T* getComponent(EntityHandle entity) const;
    
    template<typename T>
    bool hasComponent(EntityHandle entity) const;

    // Bulk component operations
    template<typename T>
    std::vector<T*> getAllComponents();
    
    template<typename T>
    void removeAllComponents();
    
    template<typename T>
    size_t getComponentCount() const;

    // Entity queries
    std::vector<EntityHandle> findEntitiesWith(const std::vector<std::type_index>& componentTypes) const;
    std::vector<EntityHandle> findEntitiesWithout(const std::vector<std::type_index>& componentTypes) const;
    std::vector<EntityHandle> findEntitiesByName(const std::string& name) const;
    std::vector<EntityHandle> findEntitiesByTag(const std::string& tag) const;

    // Entity templates
    void registerEntityTemplate(const std::string& name, std::shared_ptr<EntityTemplate> entityTemplate);
    void unregisterEntityTemplate(const std::string& name);
    std::shared_ptr<EntityTemplate> getEntityTemplate(const std::string& name) const;
    std::vector<std::string> getRegisteredTemplates() const;

    // Serialization
    std::string serializeEntity(EntityHandle entity) const;
    EntityHandle deserializeEntity(const std::string& data);
    std::string serializeHierarchy(EntityHandle root) const;
    EntityHandle deserializeHierarchy(const std::string& data);

    // Statistics
    size_t getEntityCount() const { return entityCount_; }
    size_t getArchetypeCount() const { return archetypes_.size(); }
    size_t getTotalComponentCount() const;
    std::unordered_map<std::string, size_t> getComponentStatistics() const;

private:
    struct EntityInfo {
        uint32_t generation = 0;
        std::string name;
        std::vector<std::string> tags;
        EntityHandle parent;
        std::vector<EntityHandle> children;
        size_t archetypeIndex = 0;
        size_t entityIndex = 0;
        bool isActive = true;
    };

    std::vector<EntityInfo> entities_;
    std::vector<uint32_t> freeEntityIds_;
    std::vector<std::unique_ptr<ComponentArchetype>> archetypes_;
    std::unordered_map<std::vector<std::type_index>, size_t> archetypeMap_;
    std::unordered_map<std::string, std::shared_ptr<EntityTemplate>> entityTemplates_;
    
    std::atomic<uint32_t> nextEntityId_{1};
    std::atomic<size_t> entityCount_{0};
    mutable std::mutex entitiesMutex_;
    mutable std::mutex archetypesMutex_;

    // Internal methods
    size_t findOrCreateArchetype(const std::vector<std::type_index>& componentTypes);
    void moveEntityToArchetype(EntityHandle entity, size_t newArchetypeIndex);
    void updateHierarchyOnDestroy(EntityHandle entity);
    
    template<typename T>
    void registerComponentType();
    
    static std::unordered_map<std::type_index, ComponentTypeInfo> componentTypes_;
};

/**
 * @class ComponentQuery
 * @brief Advanced query system for efficient component iteration
 */
class ComponentQuery {
public:
    ComponentQuery();
    ~ComponentQuery();

    // Query building
    template<typename T>
    ComponentQuery& with();
    
    template<typename T>
    ComponentQuery& without();
    
    ComponentQuery& withTag(const std::string& tag);
    ComponentQuery& withoutTag(const std::string& tag);
    ComponentQuery& withName(const std::string& name);
    ComponentQuery& inHierarchy(EntityHandle root);
    ComponentQuery& atDepth(int depth);
    ComponentQuery& maxDepth(int maxDepth);

    // Sorting and filtering
    template<typename T>
    ComponentQuery& sortBy(std::function<bool(const T&, const T&)> comparator);
    
    ComponentQuery& filter(std::function<bool(EntityHandle)> predicate);
    ComponentQuery& limit(size_t maxResults);

    // Execution
    std::vector<EntityHandle> execute(const EnhancedEntityManager& entityManager) const;
    
    template<typename Func>
    void forEach(const EnhancedEntityManager& entityManager, Func&& func) const;
    
    template<typename Func>
    void forEachParallel(const EnhancedEntityManager& entityManager, Func&& func) const;

    // SIMD operations
    template<typename T>
    void simdForEach(const EnhancedEntityManager& entityManager, 
                    std::function<void(__m256*, size_t)> operation) const;

private:
    std::vector<std::type_index> requiredComponents_;
    std::vector<std::type_index> excludedComponents_;
    std::vector<std::string> requiredTags_;
    std::vector<std::string> excludedTags_;
    std::string nameFilter_;
    EntityHandle hierarchyRoot_;
    int depthFilter_ = -1;
    int maxDepthFilter_ = -1;
    size_t limitCount_ = 0;
    
    std::function<bool(EntityHandle, EntityHandle)> sortComparator_;
    std::function<bool(EntityHandle)> filterPredicate_;
};

/**
 * @class EntityTemplate
 * @brief Template system for creating reusable entity configurations
 */
class EntityTemplate {
public:
    EntityTemplate(const std::string& name);
    virtual ~EntityTemplate() = default;

    // Template configuration
    const std::string& getName() const { return name_; }
    void setDescription(const std::string& description) { description_ = description; }
    const std::string& getDescription() const { return description_; }

    // Component specification
    template<typename T>
    EntityTemplate& addComponent(const T& component);
    
    template<typename T>
    EntityTemplate& addComponent();
    
    EntityTemplate& addTag(const std::string& tag);
    EntityTemplate& setName(const std::string& entityName);

    // Inheritance
    EntityTemplate& inheritsFrom(const std::string& parentTemplateName);
    const std::vector<std::string>& getParentTemplates() const { return parentTemplates_; }

    // Instantiation
    virtual EntityHandle instantiate(EnhancedEntityManager& entityManager) const;
    virtual EntityHandle instantiate(EnhancedEntityManager& entityManager, 
                                   const Vector3& position, 
                                   const Quaternion& rotation = Quaternion::identity()) const;

    // Serialization
    std::string serialize() const;
    static std::shared_ptr<EntityTemplate> deserialize(const std::string& data);

protected:
    virtual void onInstantiate(EntityHandle entity, EnhancedEntityManager& entityManager) const {}

private:
    std::string name_;
    std::string description_;
    std::string entityName_;
    std::vector<std::string> tags_;
    std::vector<std::string> parentTemplates_;
    
    struct ComponentData {
        std::type_index typeIndex;
        std::unique_ptr<uint8_t[]> data;
        size_t size;
        std::function<void(EntityHandle, EnhancedEntityManager&, const void*)> applier;
    };
    
    std::vector<ComponentData> components_;
};

/**
 * @class EnhancedSystemManager
 * @brief Advanced system manager with automatic dependency resolution and parallelization
 */
class EnhancedSystemManager {
public:
    /**
     * @brief System execution phase
     */
    enum class ExecutionPhase {
        PreUpdate,      ///< Before main update
        Update,         ///< Main update phase
        PostUpdate,     ///< After main update
        PreRender,      ///< Before rendering
        Render,         ///< Rendering phase
        PostRender,     ///< After rendering
        Cleanup         ///< Cleanup phase
    };

    /**
     * @brief System priority for execution ordering
     */
    enum class SystemPriority {
        Highest = 0,
        High = 100,
        Normal = 500,
        Low = 900,
        Lowest = 1000
    };

    EnhancedSystemManager();
    ~EnhancedSystemManager();

    // System registration
    template<typename T>
    void registerSystem(ExecutionPhase phase = ExecutionPhase::Update, 
                       SystemPriority priority = SystemPriority::Normal);
    
    template<typename T>
    void registerSystem(std::shared_ptr<T> system, 
                       ExecutionPhase phase = ExecutionPhase::Update,
                       SystemPriority priority = SystemPriority::Normal);
    
    template<typename T>
    void unregisterSystem();
    
    template<typename T>
    T* getSystem();
    
    template<typename T>
    const T* getSystem() const;

    // System dependencies
    template<typename T, typename... Dependencies>
    void setSystemDependencies();
    
    void setSystemDependency(std::type_index system, std::type_index dependency);
    void removeSystemDependency(std::type_index system, std::type_index dependency);

    // Execution
    void update(float deltaTime);
    void updatePhase(ExecutionPhase phase, float deltaTime);
    void updateSystem(std::type_index systemType, float deltaTime);

    // Parallel execution
    void enableParallelExecution(bool enable) { parallelExecution_ = enable; }
    bool isParallelExecutionEnabled() const { return parallelExecution_; }
    void setThreadCount(size_t threadCount);
    size_t getThreadCount() const { return threadCount_; }

    // System state
    void enableSystem(std::type_index systemType);
    void disableSystem(std::type_index systemType);
    bool isSystemEnabled(std::type_index systemType) const;

    // Statistics
    std::unordered_map<std::type_index, double> getSystemExecutionTimes() const;
    double getTotalExecutionTime() const;
    size_t getSystemCount() const;

private:
    struct SystemInfo {
        std::shared_ptr<System> system;
        ExecutionPhase phase;
        SystemPriority priority;
        std::vector<std::type_index> dependencies;
        std::vector<std::type_index> dependents;
        bool enabled = true;
        double lastExecutionTime = 0.0;
        std::chrono::high_resolution_clock::time_point lastUpdateTime;
    };

    std::unordered_map<std::type_index, SystemInfo> systems_;
    std::unordered_map<ExecutionPhase, std::vector<std::type_index>> phaseExecutionOrder_;
    
    bool parallelExecution_ = true;
    size_t threadCount_ = std::thread::hardware_concurrency();
    std::vector<std::thread> workerThreads_;
    
    mutable std::mutex systemsMutex_;
    
    // Internal methods
    void calculateExecutionOrder();
    void calculatePhaseExecutionOrder(ExecutionPhase phase);
    std::vector<std::type_index> topologicalSort(const std::vector<std::type_index>& systems) const;
    void executeSystemsParallel(const std::vector<std::type_index>& systems, float deltaTime);
    void executeSystemsSequential(const std::vector<std::type_index>& systems, float deltaTime);
};

/**
 * @brief SIMD-optimized transform operations
 */
namespace SIMDTransforms {
    void transformPositions(__m256* positions, const __m256* matrices, size_t count);
    void transformVectors(__m256* vectors, const __m256* matrices, size_t count);
    void interpolatePositions(__m256* result, const __m256* start, const __m256* end, float t, size_t count);
    void calculateDistances(__m256* distances, const __m256* positions1, const __m256* positions2, size_t count);
    void normalizeVectors(__m256* vectors, size_t count);
    void crossProduct(__m256* result, const __m256* a, const __m256* b, size_t count);
    void dotProduct(__m256* result, const __m256* a, const __m256* b, size_t count);
}

/**
 * @brief Common component types with SIMD optimization
 */
struct alignas(32) SIMDTransformComponent {
    __m256 position;    // x, y, z, w (w unused)
    __m256 rotation;    // quaternion: x, y, z, w
    __m256 scale;       // x, y, z, w (w unused)
    __m256 matrix[4];   // 4x4 transformation matrix
    
    void updateMatrix();
    void setPosition(const Vector3& pos);
    void setRotation(const Quaternion& rot);
    void setScale(const Vector3& scl);
    
    Vector3 getPosition() const;
    Quaternion getRotation() const;
    Vector3 getScale() const;
};

struct alignas(32) SIMDVelocityComponent {
    __m256 linear;      // linear velocity: x, y, z, w (w unused)
    __m256 angular;     // angular velocity: x, y, z, w (w unused)
    
    void setLinear(const Vector3& vel);
    void setAngular(const Vector3& vel);
    Vector3 getLinear() const;
    Vector3 getAngular() const;
};

// Template implementations
template<typename T>
T* EnhancedEntityManager::addComponent(EntityHandle entity) {
    registerComponentType<T>();
    
    if (!isEntityValid(entity)) {
        return nullptr;
    }
    
    std::lock_guard<std::mutex> lock(entitiesMutex_);
    
    auto& entityInfo = entities_[entity.id];
    auto currentArchetype = archetypes_[entityInfo.archetypeIndex].get();
    
    // Check if component already exists
    if (currentArchetype->hasComponent(std::type_index(typeid(T)))) {
        return static_cast<T*>(currentArchetype->getComponent(entityInfo.entityIndex, std::type_index(typeid(T))));
    }
    
    // Create new archetype with additional component
    auto componentTypes = currentArchetype->getComponentTypes();
    componentTypes.push_back(std::type_index(typeid(T)));
    
    size_t newArchetypeIndex = findOrCreateArchetype(componentTypes);
    moveEntityToArchetype(entity, newArchetypeIndex);
    
    // Initialize new component
    auto newArchetype = archetypes_[newArchetypeIndex].get();
    T* component = static_cast<T*>(newArchetype->getComponent(entityInfo.entityIndex, std::type_index(typeid(T))));
    new (component) T();
    
    return component;
}

template<typename T>
T* EnhancedEntityManager::addComponent(EntityHandle entity, const T& component) {
    T* newComponent = addComponent<T>(entity);
    if (newComponent) {
        *newComponent = component;
    }
    return newComponent;
}

template<typename T>
void EnhancedEntityManager::removeComponent(EntityHandle entity) {
    if (!isEntityValid(entity)) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(entitiesMutex_);
    
    auto& entityInfo = entities_[entity.id];
    auto currentArchetype = archetypes_[entityInfo.archetypeIndex].get();
    
    // Check if component exists
    if (!currentArchetype->hasComponent(std::type_index(typeid(T)))) {
        return;
    }
    
    // Destroy component
    T* component = static_cast<T*>(currentArchetype->getComponent(entityInfo.entityIndex, std::type_index(typeid(T))));
    component->~T();
    
    // Create new archetype without component
    auto componentTypes = currentArchetype->getComponentTypes();
    componentTypes.erase(std::remove(componentTypes.begin(), componentTypes.end(), std::type_index(typeid(T))), componentTypes.end());
    
    size_t newArchetypeIndex = findOrCreateArchetype(componentTypes);
    moveEntityToArchetype(entity, newArchetypeIndex);
}

template<typename T>
T* EnhancedEntityManager::getComponent(EntityHandle entity) {
    if (!isEntityValid(entity)) {
        return nullptr;
    }
    
    std::lock_guard<std::mutex> lock(entitiesMutex_);
    
    const auto& entityInfo = entities_[entity.id];
    auto archetype = archetypes_[entityInfo.archetypeIndex].get();
    
    return static_cast<T*>(archetype->getComponent(entityInfo.entityIndex, std::type_index(typeid(T))));
}

template<typename T>
const T* EnhancedEntityManager::getComponent(EntityHandle entity) const {
    if (!isEntityValid(entity)) {
        return nullptr;
    }
    
    std::lock_guard<std::mutex> lock(entitiesMutex_);
    
    const auto& entityInfo = entities_[entity.id];
    auto archetype = archetypes_[entityInfo.archetypeIndex].get();
    
    return static_cast<const T*>(archetype->getComponent(entityInfo.entityIndex, std::type_index(typeid(T))));
}

template<typename T>
bool EnhancedEntityManager::hasComponent(EntityHandle entity) const {
    if (!isEntityValid(entity)) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(entitiesMutex_);
    
    const auto& entityInfo = entities_[entity.id];
    auto archetype = archetypes_[entityInfo.archetypeIndex].get();
    
    return archetype->hasComponent(std::type_index(typeid(T)));
}

} // namespace FoundryEngine