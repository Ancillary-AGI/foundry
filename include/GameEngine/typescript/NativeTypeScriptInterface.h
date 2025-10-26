/**
 * @file NativeTypeScriptInterface.h
 * @brief Direct native interface between TypeScript and C++ engine APIs
 * @author FoundryEngine Team
 * @date 2024
 * @version 2.0.0
 */

#pragma once

#include "../core/System.h"
#include "../math/Vector3.h"
#include <memory>
#include <functional>

namespace FoundryEngine {

/**
 * @class NativeTypeScriptInterface
 * @brief Direct native interface - no bridge, direct memory access and function calls
 */
class NativeTypeScriptInterface {
public:
    // Direct native function pointers for TypeScript runtime
    struct NativeAPI {
        // Entity System - Direct C++ function pointers
        uint32_t (*createEntity)();
        void (*destroyEntity)(uint32_t entityId);
        bool (*isEntityValid)(uint32_t entityId);
        
        // Component System - Direct memory access
        void* (*addComponent)(uint32_t entityId, const char* componentType, void* componentData, size_t size);
        void* (*getComponent)(uint32_t entityId, const char* componentType);
        void (*removeComponent)(uint32_t entityId, const char* componentType);
        bool (*hasComponent)(uint32_t entityId, const char* componentType);
        
        // Transform System - Direct struct access
        void (*setPosition)(uint32_t entityId, float x, float y, float z);
        void (*getPosition)(uint32_t entityId, float* x, float* y, float* z);
        void (*setRotation)(uint32_t entityId, float x, float y, float z, float w);
        void (*getRotation)(uint32_t entityId, float* x, float* y, float* z, float* w);
        
        // Physics System - Direct native calls
        uint32_t (*createRigidBody)(float mass, const char* shape, float* params);
        void (*applyForce)(uint32_t bodyId, float x, float y, float z);
        void (*applyImpulse)(uint32_t bodyId, float x, float y, float z);
        void (*setGravity)(float x, float y, float z);
        
        // Rendering System - Direct GPU access
        uint32_t (*createMesh)(float* vertices, uint32_t vertexCount, uint32_t* indices, uint32_t indexCount);
        uint32_t (*createTexture)(uint8_t* data, uint32_t width, uint32_t height, uint32_t format);
        uint32_t (*createShader)(const char* vertexSource, const char* fragmentSource);
        void (*drawMesh)(uint32_t meshId, uint32_t shaderId, float* transform);
        
        // Audio System - Direct audio buffer access
        uint32_t (*createAudioSource)(float* audioData, uint32_t sampleCount, uint32_t sampleRate);
        void (*playAudio)(uint32_t sourceId, bool loop);
        void (*setAudioPosition)(uint32_t sourceId, float x, float y, float z);
        
        // Input System - Direct input state access
        bool (*isKeyPressed)(uint32_t keyCode);
        void (*getMousePosition)(float* x, float* y);
        bool (*isMouseButtonPressed)(uint32_t button);
        
        // Memory Management - Direct allocator access
        void* (*allocateMemory)(size_t size, size_t alignment);
        void (*deallocateMemory)(void* ptr);
        void* (*allocateFromPool)(const char* poolName, size_t size);
        void (*deallocateToPool)(const char* poolName, void* ptr);
    };
    
    static NativeAPI* getInstance();
    static void initialize();
    static void shutdown();
    
    // Direct TypeScript value access - no serialization
    static void registerNativeType(const std::string& typeName, size_t typeSize, 
                                  std::function<void*(void*)> constructor,
                                  std::function<void(void*)> destructor);
    
    // Direct function binding - no wrapper overhead
    template<typename Func>
    static void bindNativeFunction(const std::string& name, Func func);
    
    // Direct memory sharing between TypeScript and C++
    static void* getSharedMemoryRegion(const std::string& name, size_t size);
    static void releaseSharedMemoryRegion(const std::string& name);

private:
    static std::unique_ptr<NativeAPI> nativeAPI_;
    static bool isInitialized_;
};

} // namespace FoundryEngine