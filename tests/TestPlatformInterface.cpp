#include "gtest/gtest.h"
#include "GameEngine/platform/TypeSafePlatformInterface.h"
#include <thread>
#include <chrono>
#include <atomic>

namespace FoundryEngine {
namespace Tests {

/**
 * @brief Test fixture for Platform Interface tests
 */
class PlatformInterfaceTest : public ::testing::Test {
protected:
    void SetUp() override {
        capabilities.platformName = "Test Platform";
        capabilities.platformVersion = "1.0.0";
        capabilities.supportsOpenGL = true;
        capabilities.supportsVulkan = false;
        capabilities.maxTextureSize = 4096;
        capabilities.systemMemoryMB = 8192;
        capabilities.availableMemoryMB = 4096;
    }

    TypeSafePlatformCapabilities capabilities;
};

/**
 * @brief Test platform handle type safety
 */
TEST_F(PlatformInterfaceTest, HandleTypeSafety) {
    // Test window handle
    WindowHandle windowHandle(nullptr);
    EXPECT_FALSE(windowHandle.isValid());

    void* nativeWindow = reinterpret_cast<void*>(0x12345678);
    WindowHandle validWindowHandle(nativeWindow);
    EXPECT_TRUE(validWindowHandle.isValid());
    EXPECT_EQ(validWindowHandle.getNative(), nativeWindow);
    EXPECT_EQ(validWindowHandle.getType(), PlatformHandleType::WINDOW);

    // Test socket handle
    SocketHandle socketHandle(nullptr);
    EXPECT_FALSE(socketHandle.isValid());

    void* nativeSocket = reinterpret_cast<void*>(0x87654321);
    SocketHandle validSocketHandle(nativeSocket);
    EXPECT_TRUE(validSocketHandle.isValid());
    EXPECT_EQ(validSocketHandle.getType(), PlatformHandleType::SOCKET);

    // Test handle comparison
    WindowHandle anotherWindowHandle(nativeWindow);
    EXPECT_EQ(windowHandle, validWindowHandle);
    EXPECT_NE(windowHandle, socketHandle);
}

/**
 * @brief Test platform handle registry
 */
TEST_F(PlatformInterfaceTest, HandleRegistry) {
    PlatformHandleRegistry registry;

    // Test initial state
    EXPECT_EQ(registry.size(), 0);

    // Register handles
    WindowHandle windowHandle(reinterpret_cast<void*>(0x1111));
    SocketHandle socketHandle(reinterpret_cast<void*>(0x2222));
    ThreadHandle threadHandle(reinterpret_cast<void*>(0x3333));

    uint64_t windowId = registry.registerHandle(windowHandle, "Main Window");
    uint64_t socketId = registry.registerHandle(socketHandle, "Network Socket");
    uint64_t threadId = registry.registerHandle(threadHandle, "Worker Thread");

    EXPECT_EQ(registry.size(), 3);

    // Test handle validation
    EXPECT_TRUE(registry.isHandleRegistered(windowHandle.getNative()));
    EXPECT_TRUE(registry.isHandleRegistered(socketHandle.getNative()));
    EXPECT_TRUE(registry.isHandleRegistered(threadHandle.getNative()));
    EXPECT_FALSE(registry.isHandleRegistered(reinterpret_cast<void*>(0x9999)));

    // Test handle info retrieval
    const PlatformHandleRegistry::HandleInfo* windowInfo = registry.getHandleInfo(windowId);
    ASSERT_NE(windowInfo, nullptr);
    EXPECT_EQ(windowInfo->type, PlatformHandleType::WINDOW);
    EXPECT_EQ(windowInfo->name, "Main Window");

    // Test type-specific queries
    auto windowHandles = registry.getHandlesByType<PlatformHandleType::WINDOW>();
    EXPECT_EQ(windowHandles.size(), 1);
    EXPECT_EQ(windowHandles[0], windowHandle.getNative());

    // Unregister handles
    registry.unregisterHandle(windowId);
    registry.unregisterHandle(socketId);
    registry.unregisterHandle(threadId);

    EXPECT_EQ(registry.size(), 0);
    EXPECT_FALSE(registry.isHandleRegistered(windowHandle.getNative()));
}

/**
 * @brief Test platform capabilities
 */
TEST_F(PlatformInterfaceTest, PlatformCapabilities) {
    // Test capabilities initialization
    EXPECT_EQ(capabilities.platformName, "Test Platform");
    EXPECT_EQ(capabilities.platformVersion, "1.0.0");
    EXPECT_TRUE(capabilities.supportsOpenGL);
    EXPECT_FALSE(capabilities.supportsVulkan);
    EXPECT_EQ(capabilities.maxTextureSize, 4096);
    EXPECT_EQ(capabilities.systemMemoryMB, 8192);
    EXPECT_EQ(capabilities.availableMemoryMB, 4096);

    // Test extensions and features
    capabilities.extensions["test_extension"] = "enabled";
    capabilities.features["test_feature"] = true;

    EXPECT_EQ(capabilities.extensions.size(), 1);
    EXPECT_EQ(capabilities.features.size(), 1);
    EXPECT_EQ(capabilities.extensions["test_extension"], "enabled");
    EXPECT_TRUE(capabilities.features["test_feature"]);
}

/**
 * @brief Test concurrent handle registry access
 */
TEST_F(PlatformInterfaceTest, ConcurrentRegistryAccess) {
    PlatformHandleRegistry registry;
    const int numThreads = 8;
    const int handlesPerThread = 50;

    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};

    // Launch multiple threads that register and unregister handles
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

    // Verify concurrent access worked
    EXPECT_EQ(successCount.load(), numThreads * handlesPerThread);

    // Registry should be empty after all operations
    EXPECT_EQ(registry.size(), 0);
}

/**
 * @brief Test platform handle move semantics
 */
TEST_F(PlatformInterfaceTest, HandleMoveSemantics) {
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
TEST_F(PlatformInterfaceTest, HandleValidation) {
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
 * @brief Test platform handle registry stress test
 */
TEST_F(PlatformInterfaceTest, RegistryStressTest) {
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
TEST_F(PlatformInterfaceTest, CapabilitiesValidation) {
    // Test default capabilities
    TypeSafePlatformCapabilities defaultCaps;
    EXPECT_FALSE(defaultCaps.platformName.empty()); // Should have some default name
    EXPECT_GE(defaultCaps.maxTextureSize, 256);     // Should have reasonable minimum
    EXPECT_GE(defaultCaps.maxThreadCount, 1);       // Should have at least 1 thread

    // Test capabilities modification
    capabilities.maxTextureSize = 8192;
    capabilities.supportsHDR = true;
    capabilities.extensions["custom_extension"] = "1.0";

    EXPECT_EQ(capabilities.maxTextureSize, 8192);
    EXPECT_TRUE(capabilities.supportsHDR);
    EXPECT_EQ(capabilities.extensions["custom_extension"], "1.0");
}

/**
 * @brief Test platform handle type casting
 */
TEST_F(PlatformInterfaceTest, HandleTypeCasting) {
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
 * @brief Test platform handle registry performance
 */
TEST_F(PlatformInterfaceTest, RegistryPerformance) {
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

} // namespace Tests
} // namespace FoundryEngine
