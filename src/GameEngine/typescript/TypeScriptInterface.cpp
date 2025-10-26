/**
 * @file TypeScriptInterface.cpp
 * @brief Implementation of native TypeScript interface
 */

#include "GameEngine/typescript/TypeScriptInterface.h"
#include "GameEngine/core/ECS.h"
#include "GameEngine/physics/PhysicsSystem.h"
#include "GameEngine/graphics/RenderPipeline.h"
#include <unordered_map>
#include <memory>

namespace FoundryEngine {

std::unique_ptr<NativeTypeScriptInterface::NativeAPI> NativeTypeScriptInterface::nativeAPI_;
bool NativeTypeScriptInterface::isInitialized_ = false;

// Global engine systems for direct access
static EnhancedECS* g_ecs = nullptr;
static AdvancedPhysicsSystem* g_physics = nullptr;
static AdvancedRenderPipeline* g_renderer = nullptr;

// Direct C++ function implementations - no bridge overhead
extern "C" {
    // Entity System - Direct function pointers
    uint32_t native_createEntity() {
        return g_ecs ? g_ecs->createEntity() : 0;
    }
    
    void native_destroyEntity(uint32_t entityId) {
        if (g_ecs) g_ecs->destroyEntity(entityId);
    }
    
    bool native_isEntityValid(uint32_t entityId) {
        return g_ecs ? g_ecs->isEntityValid(entityId) : false;
    }
    
    // Transform System - Direct struct access
    void native_setPosition(uint32_t entityId, float x, float y, float z) {
        if (!g_ecs) return;
        
        struct Transform {
            float position[3];
            float rotation[4];
            float scale[3];
        };
        
        auto* transform = g_ecs->getComponent<Transform>(entityId);
        if (transform) {
            transform->position[0] = x;
            transform->position[1] = y;
            transform->position[2] = z;
        }
    }
    
    void native_getPosition(uint32_t entityId, float* x, float* y, float* z) {
        if (!g_ecs || !x || !y || !z) return;
        
        struct Transform {
            float position[3];
            float rotation[4];
            float scale[3];
        };
        
        auto* transform = g_ecs->getComponent<Transform>(entityId);
        if (transform) {
            *x = transform->position[0];
            *y = transform->position[1];
            *z = transform->position[2];
        }
    }
    
    // Physics System - Direct native calls
    uint32_t native_createRigidBody(float mass, const char* shape, float* params) {
        if (!g_physics) return 0;
        
        AdvancedPhysicsSystem::RigidBodyDesc desc;
        desc.mass = mass;
        desc.position = Vector3(0, 0, 0);
        
        if (strcmp(shape, "sphere") == 0) {
            desc.shapeType = AdvancedPhysicsSystem::ShapeType::Sphere;
            desc.shapeParams.radius = params ? params[0] : 1.0f;
        } else if (strcmp(shape, "box") == 0) {
            desc.shapeType = AdvancedPhysicsSystem::ShapeType::Box;
            desc.shapeParams.dimensions = Vector3(
                params ? params[0] : 1.0f,
                params ? params[1] : 1.0f,
                params ? params[2] : 1.0f
            );
        }
        
        return g_physics->createRigidBody(desc);
    }
    
    void native_applyForce(uint32_t bodyId, float x, float y, float z) {
        if (g_physics) {
            g_physics->applyForce(bodyId, Vector3(x, y, z));
        }
    }
    
    void native_setGravity(float x, float y, float z) {
        if (g_physics) {
            g_physics->setGravity(Vector3(x, y, z));
        }
    }
    
    // Rendering System - Direct GPU access
    uint32_t native_createMesh(float* vertices, uint32_t vertexCount, uint32_t* indices, uint32_t indexCount) {
        if (!g_renderer) return 0;
        
        // Direct mesh creation without serialization
        // Implementation would create GPU buffers directly
        return 1; // Placeholder
    }
    
    uint32_t native_createTexture(uint8_t* data, uint32_t width, uint32_t height, uint32_t format) {
        if (!g_renderer) return 0;
        
        // Direct texture creation
        // Implementation would upload to GPU directly
        return 1; // Placeholder
    }
    
    // Memory Management - Direct allocator access
    void* native_allocateMemory(size_t size, size_t alignment) {
        auto& memManager = AdvancedMemoryManager::getInstance();
        return memManager.allocateAligned(size, alignment);
    }
    
    void native_deallocateMemory(void* ptr) {
        auto& memManager = AdvancedMemoryManager::getInstance();
        memManager.deallocate(ptr);
    }
}

NativeTypeScriptInterface::NativeAPI* NativeTypeScriptInterface::getInstance() {
    if (!nativeAPI_) {
        nativeAPI_ = std::make_unique<NativeAPI>();
        
        // Set up direct function pointers - no bridge overhead
        nativeAPI_->createEntity = native_createEntity;
        nativeAPI_->destroyEntity = native_destroyEntity;
        nativeAPI_->isEntityValid = native_isEntityValid;
        
        nativeAPI_->setPosition = native_setPosition;
        nativeAPI_->getPosition = native_getPosition;
        
        nativeAPI_->createRigidBody = native_createRigidBody;
        nativeAPI_->applyForce = native_applyForce;
        nativeAPI_->setGravity = native_setGravity;
        
        nativeAPI_->createMesh = native_createMesh;
        nativeAPI_->createTexture = native_createTexture;
        
        nativeAPI_->allocateMemory = native_allocateMemory;
        nativeAPI_->deallocateMemory = native_deallocateMemory;
    }
    
    return nativeAPI_.get();
}

void NativeTypeScriptInterface::initialize() {
    if (isInitialized_) return;
    
    // Initialize engine systems for direct access
    static EnhancedECS ecs;
    static AdvancedPhysicsSystem physics;
    static AdvancedRenderPipeline renderer;
    
    g_ecs = &ecs;
    g_physics = &physics;
    g_renderer = &renderer;
    
    // Initialize systems
    AdvancedPhysicsSystem::PhysicsConfig physicsConfig;
    g_physics->initialize(physicsConfig);
    
    AdvancedRenderPipeline::RenderConfig renderConfig;
    g_renderer->initialize(renderConfig);
    
    isInitialized_ = true;
}

void NativeTypeScriptInterface::shutdown() {
    if (g_physics) g_physics->shutdown();
    if (g_renderer) g_renderer->shutdown();
    
    g_ecs = nullptr;
    g_physics = nullptr;
    g_renderer = nullptr;
    
    nativeAPI_.reset();
    isInitialized_ = false;
}

// Shared memory regions for zero-copy data exchange
static std::unordered_map<std::string, void*> sharedMemoryRegions;

void* NativeTypeScriptInterface::getSharedMemoryRegion(const std::string& name, size_t size) {
    auto it = sharedMemoryRegions.find(name);
    if (it != sharedMemoryRegions.end()) {
        return it->second;
    }
    
    // Allocate aligned memory for shared access
    auto& memManager = AdvancedMemoryManager::getInstance();
    void* memory = memManager.allocateAligned(size, 64); // Cache line aligned
    
    if (memory) {
        sharedMemoryRegions[name] = memory;
    }
    
    return memory;
}

void NativeTypeScriptInterface::releaseSharedMemoryRegion(const std::string& name) {
    auto it = sharedMemoryRegions.find(name);
    if (it != sharedMemoryRegions.end()) {
        auto& memManager = AdvancedMemoryManager::getInstance();
        memManager.deallocate(it->second);
        sharedMemoryRegions.erase(it);
    }
}

} // namespace FoundryEngine