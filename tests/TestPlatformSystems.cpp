#include "gtest/gtest.h"
#include "GameEngine/platform/TypeSafePlatformInterface.h"
#include "GameEngine/core/MemoryPool.h"
#include <thread>
#include <chrono>
#include <atomic>

namespace FoundryEngine {
namespace Tests {

/**
 * @brief Test fixture for Platform Systems tests
 */
class PlatformSystemsTest : public ::testing::Test {
protected:
    void SetUp() override {
        memoryPool = std::make_unique<MemoryPool>(2048, 16384);

        // Setup platform capabilities
        capabilities.platformName = "Test Platform";
        capabilities.platformVersion = "1.0.0";
        capabilities.supportsOpenGL = true;
        capabilities.supportsVulkan = true;
        capabilities.supportsD3D11 = true;
        capabilities.maxTextureSize = 8192;
        capabilities.systemMemoryMB = 16384;
        capabilities.availableMemoryMB = 8192;
    }

    void TearDown() override {
        memoryPool.reset();
    }

    std::unique_ptr<MemoryPool> memoryPool;
    TypeSafePlatformCapabilities capabilities;
};

/**
 * @brief Test platform handle management
 */
TEST_F(PlatformSystemsTest, HandleManagement) {
    PlatformHandleRegistry registry;

    // Test initial state
    EXPECT_EQ(registry.size(), 0);

    // Register various handle types
    WindowHandle windowHandle(reinterpret_cast<void*>(0x1111));
    SocketHandle socketHandle(reinterpret_cast<void*>(0x2222));
    ThreadHandle threadHandle(reinterpret_cast<void*>(0x3333));
    MutexHandle mutexHandle(reinterpret_cast<void*>(0x4444));

    uint64_t windowId = registry.registerHandle(windowHandle, "Main Window");
    uint64_t socketId = registry.registerHandle(socketHandle, "Network Socket");
    uint64_t threadId = registry.registerHandle(threadHandle, "Worker Thread");
    uint64_t mutexId = registry.registerHandle(mutexHandle, "Sync Mutex");

    EXPECT_EQ(registry.size(), 4);

    // Test handle validation
    EXPECT_TRUE(registry.isHandleRegistered(windowHandle.getNative()));
    EXPECT_TRUE(registry.isHandleRegistered(socketHandle.getNative()));
    EXPECT_TRUE(registry.isHandleRegistered(threadHandle.getNative()));
    EXPECT_TRUE(registry.isHandleRegistered(mutexHandle.getNative()));

    // Test handle info retrieval
    const PlatformHandleRegistry::HandleInfo* windowInfo = registry.getHandleInfo(windowId);
    ASSERT_NE(windowInfo, nullptr);
    EXPECT_EQ(windowInfo->type, PlatformHandleType::WINDOW);
    EXPECT_EQ(windowInfo->name, "Main Window");

    // Test type-specific queries
    auto windowHandles = registry.getHandlesByType<PlatformHandleType::WINDOW>();
    EXPECT_EQ(windowHandles.size(), 1);
    EXPECT_EQ(windowHandles[0], windowHandle.getNative());

    auto socketHandles = registry.getHandlesByType<PlatformHandleType::SOCKET>();
    EXPECT_EQ(socketHandles.size(), 1);
    EXPECT_EQ(socketHandles[0], socketHandle.getNative());

    // Test cleanup
    registry.unregisterHandle(windowId);
    registry.unregisterHandle(socketId);
    registry.unregisterHandle(threadId);
    registry.unregisterHandle(mutexId);

    EXPECT_EQ(registry.size(), 0);
}

/**
 * @brief Test platform capabilities
 */
TEST_F(PlatformSystemsTest, PlatformCapabilities) {
    // Test capabilities initialization
    EXPECT_EQ(capabilities.platformName, "Test Platform");
    EXPECT_EQ(capabilities.platformVersion, "1.0.0");
    EXPECT_TRUE(capabilities.supportsOpenGL);
    EXPECT_TRUE(capabilities.supportsVulkan);
    EXPECT_TRUE(capabilities.supportsD3D11);
    EXPECT_EQ(capabilities.maxTextureSize, 8192);
    EXPECT_EQ(capabilities.systemMemoryMB, 16384);
    EXPECT_EQ(capabilities.availableMemoryMB, 8192);

    // Test graphics capabilities
    EXPECT_GE(capabilities.maxRenderTargets, 1);
    EXPECT_GE(capabilities.maxUniformBufferSize, 1024);
    EXPECT_GE(capabilities.maxVertexAttributes, 8);

    // Test compute capabilities
    EXPECT_TRUE(capabilities.supportsComputeShaders);
    EXPECT_TRUE(capabilities.supportsGeometryShaders);
    EXPECT_TRUE(capabilities.supportsTessellationShaders);

    // Test threading capabilities
    EXPECT_TRUE(capabilities.supportsMultithreading);
    EXPECT_GE(capabilities.maxThreadCount, 1);

    // Test texture format support
    EXPECT_TRUE(capabilities.supportsHDR);
    EXPECT_TRUE(capabilities.supportsSRGB);
    EXPECT_TRUE(capabilities.supportsASTC);
    EXPECT_TRUE(capabilities.supportsBC);

    // Test extensions and features
    capabilities.extensions["custom_extension"] = "enabled";
    capabilities.features["custom_feature"] = true;

    EXPECT_EQ(capabilities.extensions.size(), 1);
    EXPECT_EQ(capabilities.features.size(), 1);
    EXPECT_EQ(capabilities.extensions["custom_extension"], "enabled");
    EXPECT_TRUE(capabilities.features["custom_feature"]);
}

/**
 * @brief Test platform interface functionality
 */
TEST_F(PlatformSystemsTest, PlatformInterface) {
    // Note: This would test the actual platform interface implementation
    // For testing purposes, we'll test the interface contract

    // Test window management interface
    WindowHandle window = WindowHandle(reinterpret_cast<void*>(0x1000));
    EXPECT_TRUE(window.isValid());
    EXPECT_EQ(window.getType(), PlatformHandleType::WINDOW);

    // Test graphics context interface
    GraphicsContextHandle context = GraphicsContextHandle(reinterpret_cast<void*>(0x2000));
    EXPECT_TRUE(context.isValid());
    EXPECT_EQ(context.getType(), PlatformHandleType::CONTEXT);

    // Test socket interface
    SocketHandle socket = SocketHandle(reinterpret_cast<void*>(0x3000));
    EXPECT_TRUE(socket.isValid());
    EXPECT_EQ(socket.getType(), PlatformHandleType::SOCKET);

    // Test thread interface
    ThreadHandle thread = ThreadHandle(reinterpret_cast<void*>(0x4000));
    EXPECT_TRUE(thread.isValid());
    EXPECT_EQ(thread.getType(), PlatformHandleType::THREAD);

    // Test synchronization interface
    MutexHandle mutex = MutexHandle(reinterpret_cast<void*>(0x5000));
    EXPECT_TRUE(mutex.isValid());
    EXPECT_EQ(mutex.getType(), PlatformHandleType::MUTEX);

    SemaphoreHandle semaphore = SemaphoreHandle(reinterpret_cast<void*>(0x6000));
    EXPECT_TRUE(semaphore.isValid());
    EXPECT_EQ(semaphore.getType(), PlatformHandleType::SEMAPHORE);

    // Test file interface
    FileHandle file = FileHandle(reinterpret_cast<void*>(0x7000));
    EXPECT_TRUE(file.isValid());
    EXPECT_EQ(file.getType(), PlatformHandleType::FILE);
}

/**
 * @brief Test platform performance
 */
TEST_F(PlatformSystemsTest, Performance) {
    const int numIterations = 100;

    // Measure platform operations performance
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < numIterations; ++i) {
        PlatformHandleRegistry registry;

        // Register multiple handles
        for (int j = 0; j < 10; ++j) {
            void* handlePtr = reinterpret_cast<void*>(static_cast<uintptr_t>(i * 100 + j));
            WindowHandle handle(handlePtr);
            registry.registerHandle(handle);
        }

        // Perform lookups
        for (int j = 0; j < 10; ++j) {
            void* handlePtr = reinterpret_cast<void*>(static_cast<uintptr_t>(i * 100 + j));
            bool isRegistered = registry.isHandleRegistered(handlePtr);
            EXPECT_TRUE(isRegistered);
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "Performed " << numIterations << " platform operations in "
              << duration.count() << " microseconds" << std::endl;

    // Performance should be reasonable (less than 100ms for 100 operations)
    EXPECT_LT(duration.count(), 100000);
}

/**
 * @brief Test platform memory management
 */
TEST_F(PlatformSystemsTest, MemoryManagement) {
    size_t initialMemory = memoryPool->totalAllocated();

    // Create multiple platform systems to test memory usage
    std::vector<PlatformHandleRegistry> registries;

    for (int i = 0; i < 50; ++i) {
        PlatformHandleRegistry registry;

        // Register various handle types
        for (int j = 0; j < 10; ++j) {
            void* handlePtr = reinterpret_cast<void*>(static_cast<uintptr_t>(i * 100 + j));
            WindowHandle window(handlePtr);
            SocketHandle socket(reinterpret_cast<void*>(static_cast<uintptr_t>(i * 100 + j + 50)));
            ThreadHandle thread(reinterpret_cast<void*>(static_cast<uintptr_t>(i * 100 + j + 100)));

            registry.registerHandle(window);
            registry.registerHandle(socket);
            registry.registerHandle(thread);
        }

        registries.push_back(std::move(registry));
    }

    size_t afterAllocationMemory = memoryPool->totalAllocated();
    EXPECT_GT(afterAllocationMemory, initialMemory);

    // Test memory utilization
    float utilization = memoryPool->utilization();
    EXPECT_GT(utilization, 0.0f);
    EXPECT_LE(utilization, 100.0f);

    // Clean up
    registries.clear();
}

/**
 * @brief Test platform error handling
 */
TEST_F(PlatformSystemsTest, ErrorHandling) {
    PlatformHandleRegistry registry;

    // Test invalid operations
    EXPECT_NO_THROW(registry.unregisterHandle(99999)); // Invalid ID should handle gracefully
    EXPECT_FALSE(registry.isHandleRegistered(reinterpret_cast<void*>(0x9999))); // Non-existent handle

    // Test null handle operations
    WindowHandle nullHandle(nullptr);
    EXPECT_FALSE(nullHandle.isValid());
    EXPECT_EQ(nullHandle.getNative(), nullptr);

    uint64_t nullId = registry.registerHandle(nullHandle, "Null Handle");
    EXPECT_FALSE(registry.isHandleRegistered(nullptr));

    registry.unregisterHandle(nullId);

    // Test empty registry operations
    EXPECT_EQ(registry.size(), 0);
    EXPECT_NO_THROW(registry.clear());
}

/**
 * @brief Test platform concurrent operations
 */
TEST_F(PlatformSystemsTest, ConcurrentOperations) {
    PlatformHandleRegistry registry;
    const int numThreads = 8;
    const int handlesPerThread = 50;

    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};

    // Launch multiple threads performing registry operations
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&registry, t, &successCount]() {
            for (int i = 0; i < handlesPerThread; ++i) {
                // Create unique handle for this thread/iteration
                void* handlePtr = reinterpret_cast<void*>(
                    static_cast<uintptr_t>(t) * 10000 + i);

                WindowHandle handle(handlePtr);
                uint64_t id = registry.registerHandle(handle, "Thread" + std::to_string(t));

                if (registry.isHandleRegistered(handlePtr)) {
                    successCount++;
                }

                registry.unregisterHandle(id);
            }
        });
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }

    // Verify concurrent operations worked
    EXPECT_EQ(successCount.load(), numThreads * handlesPerThread);

    // Registry should be empty after all operations
    EXPECT_EQ(registry.size(), 0);
}

/**
 * @brief Test platform handle move semantics
 */
TEST_F(PlatformSystemsTest, HandleMoveSemantics) {
    void* nativePtr = reinterpret_cast<void*>(0xABCDEF);

    // Test move construction
    WindowHandle originalHandle(nativePtr);
    WindowHandle movedHandle(std::move(originalHandle));

    EXPECT_FALSE(originalHandle.isValid()); // Original should be invalid
    EXPECT_TRUE(movedHandle.isValid());     // Moved should be valid
    EXPECT_EQ(movedHandle.getNative(), nativePtr);

    // Test move assignment
    WindowHandle anotherHandle(nullptr);
    anotherHandle = std::move(movedHandle);

    EXPECT_FALSE(movedHandle.isValid());    // Source should be invalid
    EXPECT_TRUE(anotherHandle.isValid());   // Target should be valid
    EXPECT_EQ(anotherHandle.getNative(), nativePtr);
}

/**
 * @brief Test platform handle validation
 */
TEST_F(PlatformSystemsTest, HandleValidation) {
    PlatformHandleRegistry registry;

    // Test null handle registration
    WindowHandle nullHandle(nullptr);
    uint64_t nullId = registry.registerHandle(nullHandle, "Null Handle");
    EXPECT_FALSE(registry.isHandleRegistered(nullptr));

    // Test invalid handle operations
    WindowHandle invalidHandle;
    EXPECT_FALSE(invalidHandle.isValid());
    EXPECT_EQ(invalidHandle.getNative(), nullptr);

    // Clean up
    registry.unregisterHandle(nullId);
}

/**
 * @brief Test platform registry stress test
 */
TEST_F(PlatformSystemsTest, RegistryStressTest) {
    PlatformHandleRegistry registry;
    const int numHandles = 1000;

    // Register many handles
    std::vector<uint64_t> handleIds;
    for (int i = 0; i < numHandles; ++i) {
        void* handlePtr = reinterpret_cast<void*>(static_cast<uintptr_t>(i + 1));
        WindowHandle handle(handlePtr);
        uint64_t id = registry.registerHandle(handle, "Handle " + std::to_string(i));
        handleIds.push_back(id);
    }

    EXPECT_EQ(registry.size(), numHandles);

    // Verify all handles are registered
    for (int i = 0; i < numHandles; ++i) {
        void* handlePtr = reinterpret_cast<void*>(static_cast<uintptr_t>(i + 1));
        EXPECT_TRUE(registry.isHandleRegistered(handlePtr));
    }

    // Unregister all handles
    for (uint64_t id : handleIds) {
        registry.unregisterHandle(id);
    }

    EXPECT_EQ(registry.size(), 0);

    // Verify all handles are unregistered
    for (int i = 0; i < numHandles; ++i) {
        void* handlePtr = reinterpret_cast<void*>(static_cast<uintptr_t>(i + 1));
        EXPECT_FALSE(registry.isHandleRegistered(handlePtr));
    }
}

/**
 * @brief Test platform capabilities validation
 */
TEST_F(PlatformSystemsTest, CapabilitiesValidation) {
    // Test default capabilities
    TypeSafePlatformCapabilities defaultCaps;
    EXPECT_FALSE(defaultCaps.platformName.empty()); // Should have some default name
    EXPECT_GE(defaultCaps.maxTextureSize, 256);     // Should have reasonable minimum
    EXPECT_GE(defaultCaps.maxThreadCount, 1);       // Should have at least 1 thread

    // Test capabilities modification
    capabilities.maxTextureSize = 16384;
    capabilities.supportsHDR = true;
    capabilities.extensions["custom_extension"] = "2.0";

    EXPECT_EQ(capabilities.maxTextureSize, 16384);
    EXPECT_TRUE(capabilities.supportsHDR);
    EXPECT_EQ(capabilities.extensions["custom_extension"], "2.0");
}

/**
 * @brief Test platform handle type casting
 */
TEST_F(PlatformSystemsTest, HandleTypeCasting) {
    void* nativePtr = reinterpret_cast<void*>(0x12345);

    // Test different handle types with same native pointer
    WindowHandle windowHandle(nativePtr);
    SocketHandle socketHandle(nativePtr);

    EXPECT_EQ(windowHandle.getNative(), socketHandle.getNative());
    EXPECT_NE(windowHandle.getType(), socketHandle.getType());
    EXPECT_EQ(windowHandle.getType(), PlatformHandleType::WINDOW);
    EXPECT_EQ(socketHandle.getType(), PlatformHandleType::SOCKET);

    // Test handle comparison with different types
    WindowHandle anotherWindowHandle(nativePtr);
    EXPECT_EQ(windowHandle, anotherWindowHandle);
    EXPECT_NE(windowHandle, socketHandle);
}

/**
 * @brief Test platform registry performance
 */
TEST_F(PlatformSystemsTest, RegistryPerformance) {
    PlatformHandleRegistry registry;
    const int numOperations = 10000;

    // Measure registration performance
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < numOperations; ++i) {
        void* handlePtr = reinterpret_cast<void*>(static_cast<uintptr_t>(i + 1));
        WindowHandle handle(handlePtr);
        uint64_t id = registry.registerHandle(handle);

        if (i % 2 == 0) { // Unregister every other handle
            registry.unregisterHandle(id);
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "Performed " << numOperations << " registry operations in "
              << duration.count() << " microseconds" << std::endl;

    // Performance should be reasonable (less than 100ms for 10k operations)
    EXPECT_LT(duration.count(), 100000);

    // Registry should have roughly half the handles registered
    EXPECT_GT(registry.size(), 0);
    EXPECT_LE(registry.size(), numOperations / 2 + 100); // Allow some margin
}

/**
 * @brief Test platform interface contract
 */
TEST_F(PlatformSystemsTest, InterfaceContract) {
    // Test that all handle types can be created and validated
    std::vector<PlatformHandleType> handleTypes = {
        PlatformHandleType::WINDOW,
        PlatformHandleType::DISPLAY,
        PlatformHandleType::CONTEXT,
        PlatformHandleType::DEVICE,
        PlatformHandleType::SURFACE,
        PlatformHandleType::INSTANCE,
        PlatformHandleType::SOCKET,
        PlatformHandleType::FILE,
        PlatformHandleType::THREAD,
        PlatformHandleType::MUTEX,
        PlatformHandleType::SEMAPHORE
    };

    for (PlatformHandleType type : handleTypes) {
        // Test handle creation for each type
        void* nativePtr = reinterpret_cast<void*>(static_cast<uintptr_t>(type) * 0x1000);

        switch (type) {
            case PlatformHandleType::WINDOW: {
                WindowHandle handle(nativePtr);
                EXPECT_TRUE(handle.isValid());
                EXPECT_EQ(handle.getType(), type);
                break;
            }
            case PlatformHandleType::SOCKET: {
                SocketHandle handle(nativePtr);
                EXPECT_TRUE(handle.isValid());
                EXPECT_EQ(handle.getType(), type);
                break;
            }
            case PlatformHandleType::THREAD: {
                ThreadHandle handle(nativePtr);
                EXPECT_TRUE(handle.isValid());
                EXPECT_EQ(handle.getType(), type);
                break;
            }
            case PlatformHandleType::MUTEX: {
                MutexHandle handle(nativePtr);
                EXPECT_TRUE(handle.isValid());
                EXPECT_EQ(handle.getType(), type);
                break;
            }
            case PlatformHandleType::SEMAPHORE: {
                SemaphoreHandle handle(nativePtr);
                EXPECT_TRUE(handle.isValid());
                EXPECT_EQ(handle.getType(), type);
                break;
            }
            case PlatformHandleType::FILE: {
                FileHandle handle(nativePtr);
                EXPECT_TRUE(handle.isValid());
                EXPECT_EQ(handle.getType(), type);
                break;
            }
            default:
                // Other handle types would need specific creation logic
                break;
        }
    }
}

/**
 * @brief Test platform system integration
 */
TEST_F(PlatformSystemsTest, SystemIntegration) {
    PlatformHandleRegistry registry;

    // Test integrated handle management across different systems
    void* windowPtr = reinterpret_cast<void*>(0x1000);
    void* contextPtr = reinterpret_cast<void*>(0x2000);
    void* socketPtr = reinterpret_cast<void*>(0x3000);

    WindowHandle window(windowPtr);
    GraphicsContextHandle context(contextPtr);
    SocketHandle socket(socketPtr);

    // Register all handles
    uint64_t windowId = registry.registerHandle(window, "Main Window");
    uint64_t contextId = registry.registerHandle(context, "Graphics Context");
    uint64_t socketId = registry.registerHandle(socket, "Network Socket");

    EXPECT_EQ(registry.size(), 3);

    // Test cross-system handle relationships
    EXPECT_TRUE(registry.isHandleRegistered(windowPtr));
    EXPECT_TRUE(registry.isHandleRegistered(contextPtr));
    EXPECT_TRUE(registry.isHandleRegistered(socketPtr));

    // Test type-specific queries
    auto windowHandles = registry.getHandlesByType<PlatformHandleType::WINDOW>();
    auto contextHandles = registry.getHandlesByType<PlatformHandleType::CONTEXT>();
    auto socketHandles = registry.getHandlesByType<PlatformHandleType::SOCKET>();

    EXPECT_EQ(windowHandles.size(), 1);
    EXPECT_EQ(contextHandles.size(), 1);
    EXPECT_EQ(socketHandles.size(), 1);

    // Test handle info retrieval
    const PlatformHandleRegistry::HandleInfo* windowInfo = registry.getHandleInfo(windowId);
    const PlatformHandleRegistry::HandleInfo* contextInfo = registry.getHandleInfo(contextId);
    const PlatformHandleRegistry::HandleInfo* socketInfo = registry.getHandleInfo(socketId);

    ASSERT_NE(windowInfo, nullptr);
    ASSERT_NE(contextInfo, nullptr);
    ASSERT_NE(socketInfo, nullptr);

    EXPECT_EQ(windowInfo->type, PlatformHandleType::WINDOW);
    EXPECT_EQ(contextInfo->type, PlatformHandleType::CONTEXT);
    EXPECT_EQ(socketInfo->type, PlatformHandleType::SOCKET);

    // Clean up
    registry.unregisterHandle(windowId);
    registry.unregisterHandle(contextId);
    registry.unregisterHandle(socketId);

    EXPECT_EQ(registry.size(), 0);
}

} // namespace Tests
} // namespace FoundryEngine
