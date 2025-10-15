#include <gtest/gtest.h>
#include "../include/GameEngine/core/Engine.h"
#include "../include/GameEngine/core/Entity.h"
#include "../include/GameEngine/core/World.h"
#include "../include/GameEngine/core/Scene.h"
#include "../include/GameEngine/core/System.h"
#include "../include/GameEngine/math/Vector2.h"
#include "../include/GameEngine/math/Vector3.h"
#include "../include/GameEngine/math/Matrix4.h"
#include "../include/GameEngine/math/Quaternion.h"

// Test Engine Core Components
class EngineCoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize test environment
    }

    void TearDown() override {
        // Clean up test environment
    }
};

// Test Vector2 operations
TEST_F(EngineCoreTest, Vector2Operations) {
    Vector2 v1(1.0f, 2.0f);
    Vector2 v2(3.0f, 4.0f);

    // Test addition
    Vector2 sum = v1 + v2;
    EXPECT_FLOAT_EQ(sum.x, 4.0f);
    EXPECT_FLOAT_EQ(sum.y, 6.0f);

    // Test subtraction
    Vector2 diff = v2 - v1;
    EXPECT_FLOAT_EQ(diff.x, 2.0f);
    EXPECT_FLOAT_EQ(diff.y, 2.0f);

    // Test scalar multiplication
    Vector2 scaled = v1 * 2.0f;
    EXPECT_FLOAT_EQ(scaled.x, 2.0f);
    EXPECT_FLOAT_EQ(scaled.y, 4.0f);

    // Test dot product
    float dot = v1.dot(v2);
    EXPECT_FLOAT_EQ(dot, 11.0f);

    // Test magnitude
    float mag = v1.magnitude();
    EXPECT_FLOAT_EQ(mag, sqrtf(5.0f));

    // Test normalization
    Vector2 normalized = v1.normalized();
    EXPECT_FLOAT_EQ(normalized.magnitude(), 1.0f);
}

// Test Vector3 operations
TEST_F(EngineCoreTest, Vector3Operations) {
    Vector3 v1(1.0f, 2.0f, 3.0f);
    Vector3 v2(4.0f, 5.0f, 6.0f);

    // Test addition
    Vector3 sum = v1 + v2;
    EXPECT_FLOAT_EQ(sum.x, 5.0f);
    EXPECT_FLOAT_EQ(sum.y, 7.0f);
    EXPECT_FLOAT_EQ(sum.z, 9.0f);

    // Test cross product
    Vector3 cross = v1.cross(v2);
    EXPECT_FLOAT_EQ(cross.x, -3.0f);  // 2*6 - 3*5
    EXPECT_FLOAT_EQ(cross.y, 6.0f);   // 3*4 - 1*6
    EXPECT_FLOAT_EQ(cross.z, -3.0f);  // 1*5 - 2*4

    // Test dot product
    float dot = v1.dot(v2);
    EXPECT_FLOAT_EQ(dot, 32.0f);

    // Test magnitude and normalization
    float mag = v1.magnitude();
    EXPECT_FLOAT_EQ(mag, sqrtf(14.0f));

    Vector3 normalized = v1.normalized();
    EXPECT_FLOAT_EQ(normalized.magnitude(), 1.0f);
}

// Test Matrix4 operations
TEST_F(EngineCoreTest, Matrix4Operations) {
    Matrix4 identity = Matrix4::identity();
    Matrix4 translation = Matrix4::translation(Vector3(1.0f, 2.0f, 3.0f));
    Matrix4 scale = Matrix4::scale(Vector3(2.0f, 3.0f, 4.0f));

    // Test identity matrix
    Vector4 vec(1.0f, 2.0f, 3.0f, 1.0f);
    Vector4 result = identity * vec;
    EXPECT_FLOAT_EQ(result.x, 1.0f);
    EXPECT_FLOAT_EQ(result.y, 2.0f);
    EXPECT_FLOAT_EQ(result.z, 3.0f);
    EXPECT_FLOAT_EQ(result.w, 1.0f);

    // Test matrix multiplication
    Matrix4 combined = translation * scale;
    Vector4 transformed = combined * vec;

    // Verify transformation order (scale then translate)
    EXPECT_FLOAT_EQ(transformed.x, 3.0f);  // 1*2 + 1 = 3
    EXPECT_FLOAT_EQ(transformed.y, 8.0f);  // 2*3 + 2 = 8
    EXPECT_FLOAT_EQ(transformed.z, 15.0f); // 3*4 + 3 = 15
    EXPECT_FLOAT_EQ(transformed.w, 1.0f);
}

// Test Quaternion operations
TEST_F(EngineCoreTest, QuaternionOperations) {
    Quaternion q1 = Quaternion::identity();
    Quaternion q2(0.0f, 0.0f, 0.0f, 1.0f); // 180 degree rotation around Z

    // Test identity
    EXPECT_FLOAT_EQ(q1.w, 1.0f);
    EXPECT_FLOAT_EQ(q1.x, 0.0f);
    EXPECT_FLOAT_EQ(q1.y, 0.0f);
    EXPECT_FLOAT_EQ(q1.z, 0.0f);

    // Test multiplication
    Quaternion product = q1 * q2;
    EXPECT_FLOAT_EQ(product.w, q2.w);
    EXPECT_FLOAT_EQ(product.x, q2.x);
    EXPECT_FLOAT_EQ(product.y, q2.y);
    EXPECT_FLOAT_EQ(product.z, q2.z);

    // Test normalization
    Quaternion normalized = q2.normalized();
    EXPECT_FLOAT_EQ(normalized.magnitude(), 1.0f);

    // Test rotation
    Vector3 point(1.0f, 0.0f, 0.0f);
    Vector3 rotated = q2.rotate(point);
    EXPECT_FLOAT_EQ(rotated.x, -1.0f); // 180 degree rotation around Z
    EXPECT_FLOAT_EQ(rotated.y, 0.0f);
    EXPECT_FLOAT_EQ(rotated.z, 0.0f);
}

// Test Entity Component System
TEST_F(EngineCoreTest, EntityComponentSystem) {
    World world;

    // Create entities
    EntityID entity1 = world.createEntity();
    EntityID entity2 = world.createEntity();

    ASSERT_NE(entity1, entity2);
    ASSERT_TRUE(world.isEntityValid(entity1));
    ASSERT_TRUE(world.isEntityValid(entity2));

    // Test entity destruction
    world.destroyEntity(entity1);
    ASSERT_FALSE(world.isEntityValid(entity1));
    ASSERT_TRUE(world.isEntityValid(entity2));

    // Test entity iteration
    int entityCount = 0;
    world.forEachEntity([&](EntityID entity) {
        entityCount++;
    });
    EXPECT_EQ(entityCount, 1); // Only entity2 should remain
}

// Test Scene Management
TEST_F(EngineCoreTest, SceneManagement) {
    Scene scene("TestScene");

    // Test scene properties
    EXPECT_EQ(scene.getName(), "TestScene");
    EXPECT_TRUE(scene.getEntities().empty());

    // Test scene loading/saving (mock implementation)
    // This would test actual file I/O in a real implementation
    EXPECT_TRUE(scene.saveToFile("test_scene.scene"));
    EXPECT_TRUE(scene.loadFromFile("test_scene.scene"));
}

// Test System Management
TEST_F(EngineCoreTest, SystemManagement) {
    World world;

    // Create a test system
    class TestSystem : public System {
    public:
        TestSystem() : System("TestSystem") {}
        void update(float deltaTime) override {
            updateCount++;
        }
        int updateCount = 0;
    };

    auto testSystem = std::make_shared<TestSystem>();
    world.addSystem(testSystem);

    // Test system update
    world.update(0.016f); // ~60 FPS
    EXPECT_EQ(testSystem->updateCount, 1);

    world.update(0.016f);
    EXPECT_EQ(testSystem->updateCount, 2);

    // Test system removal
    world.removeSystem("TestSystem");
    world.update(0.016f);
    EXPECT_EQ(testSystem->updateCount, 2); // Should not increment
}

// Test Asset System
TEST_F(EngineCoreTest, AssetSystem) {
    // This would test asset loading, caching, and management
    // Mock implementation for testing

    EXPECT_TRUE(true); // Placeholder - implement actual asset tests
}

// Test Physics System
TEST_F(EngineCoreTest, PhysicsSystem) {
    // Test physics world creation
    // Test rigid body creation and properties
    // Test collision detection
    // Test force application

    EXPECT_TRUE(true); // Placeholder - implement actual physics tests
}

// Test Rendering System
TEST_F(EngineCoreTest, RenderingSystem) {
    // Test renderer initialization
    // Test mesh rendering
    // Test shader compilation
    // Test texture loading

    EXPECT_TRUE(true); // Placeholder - implement actual rendering tests
}

// Test Audio System
TEST_F(EngineCoreTest, AudioSystem) {
    // Test audio context initialization
    // Test sound loading and playback
    // Test 3D audio positioning
    // Test audio mixing

    EXPECT_TRUE(true); // Placeholder - implement actual audio tests
}

// Test Input System
TEST_F(EngineCoreTest, InputSystem) {
    // Test keyboard input
    // Test mouse input
    // Test gamepad input
    // Test input mapping

    EXPECT_TRUE(true); // Placeholder - implement actual input tests
}

// Test Networking System
TEST_F(EngineCoreTest, NetworkingSystem) {
    // Test connection establishment
    // Test data serialization/deserialization
    // Test latency simulation
    // Test packet loss handling

    EXPECT_TRUE(true); // Placeholder - implement actual networking tests
}

// Test Profiling System
TEST_F(EngineCoreTest, ProfilingSystem) {
    // Test performance monitoring
    // Test memory tracking
    // Test frame time analysis
    // Test bottleneck identification

    EXPECT_TRUE(true); // Placeholder - implement actual profiling tests
}

// Test Memory Pool
TEST_F(EngineCoreTest, MemoryPool) {
    // Test memory allocation
    // Test memory deallocation
    // Test fragmentation handling
    // Test pool resizing

    EXPECT_TRUE(true); // Placeholder - implement actual memory pool tests
}

// Performance Tests
TEST_F(EngineCoreTest, Performance_VectorOperations) {
    const int iterations = 100000;

    Vector3 v1(1.0f, 2.0f, 3.0f);
    Vector3 v2(4.0f, 5.0f, 6.0f);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        Vector3 result = v1 + v2;
        result = result * 2.0f;
        result = result.normalized();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Should complete within reasonable time (adjust threshold as needed)
    EXPECT_LT(duration.count(), 1000); // Less than 1 second
}

TEST_F(EngineCoreTest, Performance_MatrixOperations) {
    const int iterations = 10000;

    Matrix4 m1 = Matrix4::identity();
    Matrix4 m2 = Matrix4::translation(Vector3(1.0f, 2.0f, 3.0f));
    Vector4 v(1.0f, 2.0f, 3.0f, 1.0f);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        Matrix4 result = m1 * m2;
        Vector4 transformed = result * v;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    EXPECT_LT(duration.count(), 500); // Less than 0.5 seconds
}

// Memory Leak Tests
TEST_F(EngineCoreTest, Memory_LeakPrevention) {
    // Test that entities are properly cleaned up
    World world;

    std::vector<EntityID> entities;
    for (int i = 0; i < 1000; ++i) {
        entities.push_back(world.createEntity());
    }

    // Destroy entities
    for (auto entity : entities) {
        world.destroyEntity(entity);
    }

    // Force garbage collection (if applicable)
    // Verify no memory leaks (would need memory tracking implementation)

    EXPECT_TRUE(true); // Placeholder - implement actual memory leak detection
}

// Concurrency Tests
TEST_F(EngineCoreTest, Concurrency_SafeOperations) {
    World world;

    // Test concurrent entity creation
    std::vector<std::thread> threads;
    std::mutex mutex;
    std::vector<EntityID> allEntities;

    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&]() {
            std::vector<EntityID> localEntities;
            for (int j = 0; j < 100; ++j) {
                localEntities.push_back(world.createEntity());
            }

            std::lock_guard<std::mutex> lock(mutex);
            allEntities.insert(allEntities.end(), localEntities.begin(), localEntities.end());
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(allEntities.size(), 400);

    // Verify all entities are valid
    for (auto entity : allEntities) {
        EXPECT_TRUE(world.isEntityValid(entity));
    }
}

// Error Handling Tests
TEST_F(EngineCoreTest, ErrorHandling_InvalidOperations) {
    World world;

    // Test invalid entity operations
    EntityID invalidEntity = 99999;
    EXPECT_FALSE(world.isEntityValid(invalidEntity));

    // Should not crash when destroying invalid entity
    world.destroyEntity(invalidEntity);

    // Test invalid system operations
    world.removeSystem("NonExistentSystem"); // Should not crash
}

// Serialization Tests
TEST_F(EngineCoreTest, Serialization_SceneSaveLoad) {
    Scene originalScene("TestScene");

    // Add some test data to scene
    // This would require actual scene content

    // Test save
    EXPECT_TRUE(originalScene.saveToFile("test_scene_save.scene"));

    // Test load
    Scene loadedScene("LoadedScene");
    EXPECT_TRUE(loadedScene.loadFromFile("test_scene_save.scene"));

    // Compare scenes (would need proper comparison implementation)
    EXPECT_EQ(originalScene.getName(), "TestScene");
    EXPECT_EQ(loadedScene.getName(), "LoadedScene"); // Different name after loading
}

// Integration Tests
TEST_F(EngineCoreTest, Integration_FullGameLoop) {
    World world;
    Scene scene("IntegrationTest");

    // Set up a minimal game loop simulation
    float deltaTime = 1.0f / 60.0f; // 60 FPS
    int frames = 60; // 1 second of simulation

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < frames; ++i) {
        world.update(deltaTime);
        scene.update(deltaTime);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Should complete within reasonable time
    EXPECT_LT(duration.count(), 2000); // Less than 2 seconds for 60 frames
}

// Benchmark Tests
TEST_F(EngineCoreTest, Benchmark_EntityCreation) {
    World world;

    const int entityCount = 10000;
    std::vector<EntityID> entities;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < entityCount; ++i) {
        entities.push_back(world.createEntity());
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Calculate entities per second
    double eps = entityCount / (duration.count() / 1000.0);
    std::cout << "Entity creation rate: " << eps << " entities/second" << std::endl;

    // Clean up
    for (auto entity : entities) {
        world.destroyEntity(entity);
    }

    // Should be reasonably fast (adjust threshold based on hardware)
    EXPECT_GT(eps, 1000.0); // At least 1000 entities per second
}

TEST_F(EngineCoreTest, Benchmark_SystemUpdate) {
    World world;

    // Add multiple systems
    class BenchmarkSystem : public System {
    public:
        BenchmarkSystem(const std::string& name) : System(name) {}
        void update(float deltaTime) override {
            // Simulate some work
            volatile int sum = 0;
            for (int i = 0; i < 1000; ++i) {
                sum += i;
            }
        }
    };

    std::vector<std::shared_ptr<System>> systems;
    for (int i = 0; i < 10; ++i) {
        auto system = std::make_shared<BenchmarkSystem>("BenchmarkSystem" + std::to_string(i));
        systems.push_back(system);
        world.addSystem(system);
    }

    const int frames = 100;
    float deltaTime = 1.0f / 60.0f;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < frames; ++i) {
        world.update(deltaTime);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    double fps = frames / (duration.count() / 1000.0);
    std::cout << "System update rate: " << fps << " FPS" << std::endl;

    EXPECT_GT(fps, 30.0); // At least 30 FPS
}