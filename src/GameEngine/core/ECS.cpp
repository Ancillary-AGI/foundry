/**
 * @file ECS.cpp
 * @brief Implementation of Entity Component System with SIMD optimization
 */

#include "GameEngine/core/ECS.h"
#include "GameEngine/core/MemoryManager.h"
#include <algorithm>
#include <immintrin.h>

namespace FoundryEngine {

class EnhancedECS::EnhancedECSImpl {
public:
    std::atomic<EntityID> nextEntityId_{1};
    std::atomic<ComponentTypeID> nextComponentTypeId_{1};
    
    std::unordered_map<EntityID, EntityInfo> entities_;
    std::unordered_map<ComponentTypeID, std::unique_ptr<ComponentPool>> componentPools_;
    std::unordered_map<std::string, ComponentTypeID> componentTypeNames_;
    
    std::vector<std::unique_ptr<System>> systems_;
    std::unordered_map<SystemTypeID, size_t> systemIndices_;
    
    mutable std::shared_mutex entitiesMutex_;
    mutable std::shared_mutex componentsMutex_;
    mutable std::shared_mutex systemsMutex_;
    
    PerformanceMetrics metrics_;
    
    // SIMD-optimized component arrays
    struct alignas(32) SIMDTransformData {
        float positions[8][4];  // x, y, z, w for 8 entities
        float rotations[8][4];  // x, y, z, w quaternions
        float scales[8][4];     // x, y, z, w scales
    };
    
    std::vector<SIMDTransformData> simdTransforms_;
    std::unordered_map<EntityID, size_t> entityToSIMDIndex_;
};

EnhancedECS::EnhancedECS() : impl_(std::make_unique<EnhancedECSImpl>()) {}

EnhancedECS::~EnhancedECS() = default;

EntityID EnhancedECS::createEntity() {
    EntityID entityId = impl_->nextEntityId_++;
    
    {
        std::unique_lock<std::shared_mutex> lock(impl_->entitiesMutex_);
        impl_->entities_[entityId] = EntityInfo{entityId, true, {}};
    }
    
    impl_->metrics_.entitiesCreated++;
    impl_->metrics_.activeEntities++;
    
    return entityId;
}

void EnhancedECS::destroyEntity(EntityID entityId) {
    std::unique_lock<std::shared_mutex> lock(impl_->entitiesMutex_);
    
    auto it = impl_->entities_.find(entityId);
    if (it != impl_->entities_.end()) {
        // Remove all components
        for (ComponentTypeID typeId : it->second.componentTypes) {
            removeComponentInternal(entityId, typeId);
        }
        
        impl_->entities_.erase(it);
        impl_->metrics_.entitiesDestroyed++;
        impl_->metrics_.activeEntities--;
    }
}

bool EnhancedECS::isEntityValid(EntityID entityId) const {
    std::shared_lock<std::shared_mutex> lock(impl_->entitiesMutex_);
    
    auto it = impl_->entities_.find(entityId);
    return it != impl_->entities_.end() && it->second.active;
}

std::vector<EntityID> EnhancedECS::getEntitiesWithComponent(ComponentTypeID componentType) const {
    std::vector<EntityID> result;
    std::shared_lock<std::shared_mutex> lock(impl_->entitiesMutex_);
    
    for (const auto& [entityId, info] : impl_->entities_) {
        if (info.active && 
            std::find(info.componentTypes.begin(), info.componentTypes.end(), componentType) != info.componentTypes.end()) {
            result.push_back(entityId);
        }
    }
    
    return result;
}

std::vector<EntityID> EnhancedECS::getEntitiesWithComponents(const std::vector<ComponentTypeID>& componentTypes) const {
    std::vector<EntityID> result;
    std::shared_lock<std::shared_mutex> lock(impl_->entitiesMutex_);
    
    for (const auto& [entityId, info] : impl_->entities_) {
        if (!info.active) continue;
        
        bool hasAllComponents = true;
        for (ComponentTypeID typeId : componentTypes) {
            if (std::find(info.componentTypes.begin(), info.componentTypes.end(), typeId) == info.componentTypes.end()) {
                hasAllComponents = false;
                break;
            }
        }
        
        if (hasAllComponents) {
            result.push_back(entityId);
        }
    }
    
    return result;
}

void EnhancedECS::addSystemInternal(std::unique_ptr<System> system, SystemTypeID typeId) {
    std::unique_lock<std::shared_mutex> lock(impl_->systemsMutex_);
    
    size_t index = impl_->systems_.size();
    impl_->systems_.push_back(std::move(system));
    impl_->systemIndices_[typeId] = index;
}

void EnhancedECS::removeSystem(SystemTypeID typeId) {
    std::unique_lock<std::shared_mutex> lock(impl_->systemsMutex_);
    
    auto it = impl_->systemIndices_.find(typeId);
    if (it != impl_->systemIndices_.end()) {
        size_t index = it->second;
        
        // Shutdown system
        if (impl_->systems_[index]) {
            impl_->systems_[index]->shutdown();
        }
        
        // Remove from vector (swap with last element)
        if (index < impl_->systems_.size() - 1) {
            std::swap(impl_->systems_[index], impl_->systems_.back());
            
            // Update index mapping for swapped system
            for (auto& [sysTypeId, sysIndex] : impl_->systemIndices_) {
                if (sysIndex == impl_->systems_.size() - 1) {
                    sysIndex = index;
                    break;
                }
            }
        }
        
        impl_->systems_.pop_back();
        impl_->systemIndices_.erase(it);
    }
}

System* EnhancedECS::getSystem(SystemTypeID typeId) const {
    std::shared_lock<std::shared_mutex> lock(impl_->systemsMutex_);
    
    auto it = impl_->systemIndices_.find(typeId);
    if (it != impl_->systemIndices_.end()) {
        return impl_->systems_[it->second].get();
    }
    
    return nullptr;
}

void EnhancedECS::updateSystems(float deltaTime) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    std::shared_lock<std::shared_mutex> lock(impl_->systemsMutex_);
    
    // Update systems in dependency order
    for (auto& system : impl_->systems_) {
        if (system) {
            auto systemStart = std::chrono::high_resolution_clock::now();
            system->update(deltaTime);
            auto systemEnd = std::chrono::high_resolution_clock::now();
            
            auto systemTime = std::chrono::duration_cast<std::chrono::microseconds>(systemEnd - systemStart);
            impl_->metrics_.systemUpdateTimes[system.get()] = systemTime.count() / 1000.0f;
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto totalTime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    impl_->metrics_.totalUpdateTime = totalTime.count() / 1000.0f;
    impl_->metrics_.updatesPerformed++;
}

void EnhancedECS::updateSystemsSIMD(float deltaTime) {
    // SIMD-optimized system updates for transform components
    updateTransformsSIMD(deltaTime);
    
    // Regular system updates
    updateSystems(deltaTime);
}

void EnhancedECS::updateTransformsSIMD(float deltaTime) {
    // Process transforms in SIMD batches of 8
    for (auto& simdData : impl_->simdTransforms_) {
        // Load 8 positions at once
        __m256 posX = _mm256_load_ps(&simdData.positions[0][0]);
        __m256 posY = _mm256_load_ps(&simdData.positions[0][1]);
        __m256 posZ = _mm256_load_ps(&simdData.positions[0][2]);
        
        // Example: Apply gravity to all entities
        __m256 gravity = _mm256_set1_ps(-9.81f * deltaTime);
        posY = _mm256_add_ps(posY, gravity);
        
        // Store results back
        _mm256_store_ps(&simdData.positions[0][1], posY);
    }
}

ComponentTypeID EnhancedECS::registerComponentTypeInternal(const std::string& typeName, size_t componentSize) {
    std::unique_lock<std::shared_mutex> lock(impl_->componentsMutex_);
    
    auto it = impl_->componentTypeNames_.find(typeName);
    if (it != impl_->componentTypeNames_.end()) {
        return it->second;
    }
    
    ComponentTypeID typeId = impl_->nextComponentTypeId_++;
    impl_->componentTypeNames_[typeName] = typeId;
    
    // Create component pool
    auto pool = std::make_unique<ComponentPool>(componentSize, 1000); // Initial capacity
    impl_->componentPools_[typeId] = std::move(pool);
    
    return typeId;
}

void* EnhancedECS::addComponentInternal(EntityID entityId, ComponentTypeID componentType, const void* componentData, size_t componentSize) {
    {
        std::unique_lock<std::shared_mutex> lock(impl_->entitiesMutex_);
        auto entityIt = impl_->entities_.find(entityId);
        if (entityIt == impl_->entities_.end() || !entityIt->second.active) {
            return nullptr;
        }
        
        // Add component type to entity
        entityIt->second.componentTypes.push_back(componentType);
    }
    
    std::unique_lock<std::shared_mutex> lock(impl_->componentsMutex_);
    auto poolIt = impl_->componentPools_.find(componentType);
    if (poolIt != impl_->componentPools_.end()) {
        void* component = poolIt->second->allocate();
        if (component && componentData) {
            std::memcpy(component, componentData, componentSize);
        }
        
        impl_->metrics_.componentsCreated++;
        return component;
    }
    
    return nullptr;
}

void* EnhancedECS::getComponentInternal(EntityID entityId, ComponentTypeID componentType) const {
    std::shared_lock<std::shared_mutex> lock(impl_->componentsMutex_);
    
    auto poolIt = impl_->componentPools_.find(componentType);
    if (poolIt != impl_->componentPools_.end()) {
        return poolIt->second->get(entityId);
    }
    
    return nullptr;
}

void EnhancedECS::removeComponentInternal(EntityID entityId, ComponentTypeID componentType) {
    {
        std::unique_lock<std::shared_mutex> lock(impl_->entitiesMutex_);
        auto entityIt = impl_->entities_.find(entityId);
        if (entityIt != impl_->entities_.end()) {
            auto& types = entityIt->second.componentTypes;
            types.erase(std::remove(types.begin(), types.end(), componentType), types.end());
        }
    }
    
    std::unique_lock<std::shared_mutex> lock(impl_->componentsMutex_);
    auto poolIt = impl_->componentPools_.find(componentType);
    if (poolIt != impl_->componentPools_.end()) {
        poolIt->second->deallocate(entityId);
        impl_->metrics_.componentsDestroyed++;
    }
}

bool EnhancedECS::hasComponent(EntityID entityId, ComponentTypeID componentType) const {
    std::shared_lock<std::shared_mutex> lock(impl_->entitiesMutex_);
    
    auto entityIt = impl_->entities_.find(entityId);
    if (entityIt != impl_->entities_.end() && entityIt->second.active) {
        const auto& types = entityIt->second.componentTypes;
        return std::find(types.begin(), types.end(), componentType) != types.end();
    }
    
    return false;
}

EnhancedECS::PerformanceMetrics EnhancedECS::getPerformanceMetrics() const {
    return impl_->metrics_;
}

void EnhancedECS::resetMetrics() {
    impl_->metrics_ = PerformanceMetrics{};
}

size_t EnhancedECS::getEntityCount() const {
    std::shared_lock<std::shared_mutex> lock(impl_->entitiesMutex_);
    return impl_->entities_.size();
}

size_t EnhancedECS::getComponentCount() const {
    size_t total = 0;
    std::shared_lock<std::shared_mutex> lock(impl_->componentsMutex_);
    
    for (const auto& [typeId, pool] : impl_->componentPools_) {
        total += pool->getActiveCount();
    }
    
    return total;
}

size_t EnhancedECS::getSystemCount() const {
    std::shared_lock<std::shared_mutex> lock(impl_->systemsMutex_);
    return impl_->systems_.size();
}

// ComponentPool implementation
class ComponentPool {
public:
    ComponentPool(size_t componentSize, size_t initialCapacity)
        : componentSize_(componentSize), capacity_(initialCapacity) {
        
        auto& memManager = AdvancedMemoryManager::getInstance();
        data_ = static_cast<uint8_t*>(memManager.allocateAligned(componentSize_ * capacity_, 32));
        
        // Initialize free list
        for (size_t i = 0; i < capacity_; ++i) {
            freeIndices_.push(i);
        }
    }
    
    ~ComponentPool() {
        auto& memManager = AdvancedMemoryManager::getInstance();
        memManager.deallocate(data_);
    }
    
    void* allocate() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (freeIndices_.empty()) {
            // Expand pool
            expandPool();
        }
        
        if (!freeIndices_.empty()) {
            size_t index = freeIndices_.front();
            freeIndices_.pop();
            activeCount_++;
            return data_ + (index * componentSize_);
        }
        
        return nullptr;
    }
    
    void deallocate(EntityID entityId) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = entityToIndex_.find(entityId);
        if (it != entityToIndex_.end()) {
            freeIndices_.push(it->second);
            entityToIndex_.erase(it);
            activeCount_--;
        }
    }
    
    void* get(EntityID entityId) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = entityToIndex_.find(entityId);
        if (it != entityToIndex_.end()) {
            return data_ + (it->second * componentSize_);
        }
        
        return nullptr;
    }
    
    size_t getActiveCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return activeCount_;
    }
    
private:
    void expandPool() {
        size_t newCapacity = capacity_ * 2;
        
        auto& memManager = AdvancedMemoryManager::getInstance();
        uint8_t* newData = static_cast<uint8_t*>(memManager.allocateAligned(componentSize_ * newCapacity, 32));
        
        // Copy existing data
        memManager.bulkCopy(newData, data_, componentSize_ * capacity_);
        
        // Free old data
        memManager.deallocate(data_);
        
        data_ = newData;
        
        // Add new indices to free list
        for (size_t i = capacity_; i < newCapacity; ++i) {
            freeIndices_.push(i);
        }
        
        capacity_ = newCapacity;
    }
    
    size_t componentSize_;
    size_t capacity_;
    size_t activeCount_ = 0;
    uint8_t* data_;
    
    std::queue<size_t> freeIndices_;
    std::unordered_map<EntityID, size_t> entityToIndex_;
    
    mutable std::mutex mutex_;
};

} // namespace FoundryEngine