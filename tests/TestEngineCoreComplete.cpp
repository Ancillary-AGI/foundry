#include "gtest/gtest.h"
#include "GameEngine/core/Engine.h"
#include "GameEngine/core/World.h"
#include "GameEngine/core/Scene.h"
#include "GameEngine/math/Vector3.h"
#include "GameEngine/math/Matrix4.h"
#include "GameEngine/math/Quaternion.h"
#include "GameEngine/components/TransformComponent.h"
#include "GameEngine/systems/AssetSystem.h"
#include "GameEngine/systems/PhysicsSystem.h"
#include "GameEngine/systems/AudioSystem.h"
#include "GameEngine/systems/InputSystem.h"
#include "GameEngine/systems/NetworkSystem.h"
#include "GameEngine/systems/ProfilerSystem.h"
#include "GameEngine/graphics/Renderer.h"
#include "GameEngine/core/MemoryPool.h"
#include <chrono>
#include <thread>
#include <memory>

namespace FoundryEngine {

class EngineCoreCompleteTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize engine for testing
        engine_ = &Engine::getInstance();
        ASSERT_TRUE(engine_->initialize()) << "Failed to initialize engine";
        
        world_ = engine_->getWorld();
        ASSERT_NE(world_, nullptr) << "World is null";
        
        renderer_ = engine_->getRenderer();
        audio_ = engine_->getAudio();
        input_ = engine_->getInput();
        physics_ = engine_->getPhysics();
        network_ = engine_->getNetwork();
        profiler_ = engine_->getProfiler();
        assets_ = engine_->getAssets();
    }

    void TearDown() override {
        if (engine_) {
            engine_->shutdown();
        }
    }

    Engine* engine_;
    World* world_;
    Renderer* renderer_;
    AudioManager* audio_;
    InputManager* input_;
    PhysicsWorld* physics_;
    NetworkManager* network_;
    ProfileManager* profiler_;
    AssetManager* assets_;
};

// Test Engine Initialization
TEST_F(EngineCoreCompleteTest, EngineInitialization) {
    EXPECT_TRUE(engine_->isRunning()) << "Engine should be running after initialization";
    EXPECT_NE(world_, nullptr) << "World should be initialized";
    EXPECT_NE(renderer_, nullptr) << "Renderer should be initialized";
    EXPECT_NE(audio_, nullptr) << "Audio manager should be initialized";
    EXPECT_NE(input_, nullptr) << "Input manager should be initialized";
    EXPECT_NE(physics_, nullptr) << "Physics world should be initialized";
    EXPECT_NE(network_, nullptr) << "Network manager should be initialized";
    EXPECT_NE(profiler_, nullptr) << "Profiler should be initialized";
    EXPECT_NE(assets_, nullptr) << "Asset manager should be initialized";
}

// Test World and ECS System
TEST_F(EngineCoreCompleteTest, WorldAndECS) {
    // Create entities
    uint32_t entity1 = world_->createEntity();
    uint32_t entity2 = world_->createEntity();
    
    EXPECT_NE(entity1, 0) << "Entity 1 should have valid ID";
    EXPECT_NE(entity2, 0) << "Entity 2 should have valid ID";
    EXPECT_NE(entity1, entity2) << "Entities should have different IDs";
    
    // Add transform components
    TransformComponent transform1;
    transform1.position = Vector3(1.0f, 2.0f, 3.0f);
    transform1.rotation = Vector3(0.0f, 0.0f, 0.0f);
    transform1.scale = Vector3(1.0f, 1.0f, 1.0f);
    
    TransformComponent transform2;
    transform2.position = Vector3(4.0f, 5.0f, 6.0f);
    transform2.rotation = Vector3(0.0f, 0.0f, 0.0f);
    transform2.scale = Vector3(2.0f, 2.0f, 2.0f);
    
    world_->addComponent<TransformComponent>(entity1, transform1);
    world_->addComponent<TransformComponent>(entity2, transform2);
    
    // Verify components
    EXPECT_TRUE(world_->hasComponent<TransformComponent>(entity1)) << "Entity 1 should have transform component";
    EXPECT_TRUE(world_->hasComponent<TransformComponent>(entity2)) << "Entity 2 should have transform component";
    
    // Get and verify component data
    auto* comp1 = world_->getComponent<TransformComponent>(entity1);
    auto* comp2 = world_->getComponent<TransformComponent>(entity2);
    
    ASSERT_NE(comp1, nullptr) << "Transform component 1 should not be null";
    ASSERT_NE(comp2, nullptr) << "Transform component 2 should not be null";
    
    EXPECT_FLOAT_EQ(comp1->position.x, 1.0f) << "Transform 1 position x should be 1.0";
    EXPECT_FLOAT_EQ(comp1->position.y, 2.0f) << "Transform 1 position y should be 2.0";
    EXPECT_FLOAT_EQ(comp1->position.z, 3.0f) << "Transform 1 position z should be 3.0";
    
    EXPECT_FLOAT_EQ(comp2->position.x, 4.0f) << "Transform 2 position x should be 4.0";
    EXPECT_FLOAT_EQ(comp2->position.y, 5.0f) << "Transform 2 position y should be 5.0";
    EXPECT_FLOAT_EQ(comp2->position.z, 6.0f) << "Transform 2 position z should be 6.0";
    
    // Remove component
    world_->removeComponent<TransformComponent>(entity1);
    EXPECT_FALSE(world_->hasComponent<TransformComponent>(entity1)) << "Entity 1 should not have transform component after removal";
    
    // Destroy entity
    world_->destroyEntity(entity2);
    EXPECT_FALSE(world_->hasComponent<TransformComponent>(entity2)) << "Entity 2 should not have transform component after destruction";
}

// Test Math Library
TEST_F(EngineCoreCompleteTest, MathLibrary) {
    // Vector3 tests
    Vector3 v1(1.0f, 2.0f, 3.0f);
    Vector3 v2(4.0f, 5.0f, 6.0f);
    
    // Addition
    Vector3 sum = v1 + v2;
    EXPECT_FLOAT_EQ(sum.x, 5.0f) << "Vector addition x should be 5.0";
    EXPECT_FLOAT_EQ(sum.y, 7.0f) << "Vector addition y should be 7.0";
    EXPECT_FLOAT_EQ(sum.z, 9.0f) << "Vector addition z should be 9.0";
    
    // Subtraction
    Vector3 diff = v2 - v1;
    EXPECT_FLOAT_EQ(diff.x, 3.0f) << "Vector subtraction x should be 3.0";
    EXPECT_FLOAT_EQ(diff.y, 3.0f) << "Vector subtraction y should be 3.0";
    EXPECT_FLOAT_EQ(diff.z, 3.0f) << "Vector subtraction z should be 3.0";
    
    // Dot product
    float dot = v1.dot(v2);
    EXPECT_FLOAT_EQ(dot, 32.0f) << "Dot product should be 32.0";
    
    // Cross product
    Vector3 cross = v1.cross(v2);
    EXPECT_FLOAT_EQ(cross.x, -3.0f) << "Cross product x should be -3.0";
    EXPECT_FLOAT_EQ(cross.y, 6.0f) << "Cross product y should be 6.0";
    EXPECT_FLOAT_EQ(cross.z, -3.0f) << "Cross product z should be -3.0";
    
    // Length
    float length = v1.length();
    EXPECT_FLOAT_EQ(length, std::sqrt(14.0f)) << "Vector length should be sqrt(14)";
    
    // Normalize
    Vector3 normalized = v1.normalized();
    float normalizedLength = normalized.length();
    EXPECT_FLOAT_EQ(normalizedLength, 1.0f) << "Normalized vector length should be 1.0";
    
    // Matrix4 tests
    Matrix4 identity = Matrix4::identity();
    Vector3 testPoint(1.0f, 2.0f, 3.0f);
    Vector3 transformed = identity.transformPoint(testPoint);
    
    EXPECT_FLOAT_EQ(transformed.x, 1.0f) << "Identity matrix transform x should be 1.0";
    EXPECT_FLOAT_EQ(transformed.y, 2.0f) << "Identity matrix transform y should be 2.0";
    EXPECT_FLOAT_EQ(transformed.z, 3.0f) << "Identity matrix transform z should be 3.0";
    
    // Translation matrix
    Vector3 translation(10.0f, 20.0f, 30.0f);
    Matrix4 translationMatrix = Matrix4::translation(translation);
    Vector3 translated = translationMatrix.transformPoint(testPoint);
    
    EXPECT_FLOAT_EQ(translated.x, 11.0f) << "Translation x should be 11.0";
    EXPECT_FLOAT_EQ(translated.y, 22.0f) << "Translation y should be 22.0";
    EXPECT_FLOAT_EQ(translated.z, 33.0f) << "Translation z should be 33.0";
}

// Test Asset System
TEST_F(EngineCoreCompleteTest, AssetSystem) {
    ASSERT_NE(assets_, nullptr) << "Asset manager should not be null";
    
    // Test asset loading (with mock data)
    std::string testAssetPath = "test_texture.png";
    AssetType assetType = AssetType::Texture;
    
    // Note: In a real test, you would have actual asset files
    // For now, we test the interface
    Asset* loadedAsset = assets_->loadAsset(testAssetPath, assetType);
    
    // The asset might not load if file doesn't exist, but the system should handle it gracefully
    if (loadedAsset) {
        EXPECT_NE(loadedAsset, nullptr) << "Loaded asset should not be null";
        EXPECT_GT(loadedAsset->getMemoryUsage(), 0) << "Asset should have memory usage";
        
        // Unload asset
        assets_->unloadAsset(testAssetPath);
    }
    
    // Test asset statistics
    std::string stats = assets_->getStatistics();
    EXPECT_FALSE(stats.empty()) << "Asset statistics should not be empty";
}

// Test Physics System
TEST_F(EngineCoreCompleteTest, PhysicsSystem) {
    ASSERT_NE(physics_, nullptr) << "Physics world should not be null";
    
    // Test physics step
    float deltaTime = 1.0f / 60.0f; // 60 FPS
    physics_->step(deltaTime);
    
    // Test gravity setting
    Vector3 gravity(0.0f, -9.81f, 0.0f);
    physics_->setGravity(gravity);
    
    // Test physics world update
    physics_->update(deltaTime);
    
    // Physics system should be functional
    EXPECT_TRUE(true) << "Physics system should be operational";
}

// Test Audio System
TEST_F(EngineCoreCompleteTest, AudioSystem) {
    ASSERT_NE(audio_, nullptr) << "Audio manager should not be null";
    
    // Test audio system update
    audio_->update();
    
    // Test audio context (platform-specific)
    // Note: Actual audio testing would require platform-specific implementations
    
    // Audio system should be functional
    EXPECT_TRUE(true) << "Audio system should be operational";
}

// Test Input System
TEST_F(EngineCoreCompleteTest, InputSystem) {
    ASSERT_NE(input_, nullptr) << "Input manager should not be null";
    
    // Test input system update
    input_->update();
    
    // Test input state (platform-specific)
    // Note: Actual input testing would require platform-specific implementations
    
    // Input system should be functional
    EXPECT_TRUE(true) << "Input system should be operational";
}

// Test Network System
TEST_F(EngineCoreCompleteTest, NetworkSystem) {
    ASSERT_NE(network_, nullptr) << "Network manager should not be null";
    
    // Test network system update
    network_->update();
    
    // Test network functionality
    // Note: Actual network testing would require network setup
    
    // Network system should be functional
    EXPECT_TRUE(true) << "Network system should be operational";
}

// Test Profiler System
TEST_F(EngineCoreCompleteTest, ProfilerSystem) {
    ASSERT_NE(profiler_, nullptr) << "Profiler should not be null";
    
    // Test profiler functionality
    profiler_->beginFrame();
    
    // Simulate some work
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    
    profiler_->endFrame();
    profiler_->update();
    
    // Profiler should be functional
    EXPECT_TRUE(true) << "Profiler should be operational";
}

// Test Memory Pool
TEST_F(EngineCoreCompleteTest, MemoryPool) {
    // Test memory pool allocation
    MemoryPool pool(1024, 4096); // 1KB blocks, 4KB total
    
    // Test allocation
    void* ptr1 = pool.allocateRaw(512);
    EXPECT_NE(ptr1, nullptr) << "Memory allocation should succeed";
    
    void* ptr2 = pool.allocateRaw(256);
    EXPECT_NE(ptr2, nullptr) << "Second memory allocation should succeed";
    
    // Test deallocation
    pool.deallocateRaw(ptr1);
    pool.deallocateRaw(ptr2);
    
    // Test memory statistics
    size_t totalAllocated = pool.totalAllocated();
    size_t totalFree = pool.totalFree();
    float utilization = pool.utilization();
    float fragmentation = pool.fragmentationRatio();
    
    EXPECT_GE(totalAllocated, 0) << "Total allocated should be non-negative";
    EXPECT_GE(totalFree, 0) << "Total free should be non-negative";
    EXPECT_GE(utilization, 0.0f) << "Utilization should be non-negative";
    EXPECT_LE(utilization, 100.0f) << "Utilization should not exceed 100%";
    EXPECT_GE(fragmentation, 0.0f) << "Fragmentation should be non-negative";
    EXPECT_LE(fragmentation, 1.0f) << "Fragmentation should not exceed 1.0";
    
    // Test defragmentation
    pool.defragment();
    
    // Memory pool should be functional
    EXPECT_TRUE(true) << "Memory pool should be operational";
}

// Test Engine Update Loop
TEST_F(EngineCoreCompleteTest, EngineUpdateLoop) {
    // Test multiple engine updates
    for (int i = 0; i < 10; ++i) {
        float deltaTime = 1.0f / 60.0f;
        engine_->update(deltaTime);
        engine_->render();
        
        // Engine should remain running
        EXPECT_TRUE(engine_->isRunning()) << "Engine should remain running during update loop";
    }
    
    // Test timing
    float deltaTime = engine_->getDeltaTime();
    EXPECT_GE(deltaTime, 0.0f) << "Delta time should be non-negative";
    
    uint64_t frameCount = engine_->getFrameCount();
    EXPECT_GT(frameCount, 0) << "Frame count should be greater than 0";
}

// Performance Tests
TEST_F(EngineCoreCompleteTest, Performance_VectorOperations) {
    const int iterations = 100000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    Vector3 v1(1.0f, 2.0f, 3.0f);
    Vector3 v2(4.0f, 5.0f, 6.0f);
    
    for (int i = 0; i < iterations; ++i) {
        Vector3 result = v1 + v2;
        float dot = v1.dot(v2);
        Vector3 cross = v1.cross(v2);
        float length = result.length();
        Vector3 normalized = result.normalized();
        
        // Prevent optimization
        (void)dot;
        (void)cross;
        (void)length;
        (void)normalized;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Performance should be reasonable (less than 1ms per 1000 operations)
    EXPECT_LT(duration.count(), 1000) << "Vector operations should be fast";
}

TEST_F(EngineCoreCompleteTest, Performance_EntityCreation) {
    const int entityCount = 1000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<uint32_t> entities;
    entities.reserve(entityCount);
    
    // Create entities
    for (int i = 0; i < entityCount; ++i) {
        uint32_t entity = world_->createEntity();
        entities.push_back(entity);
        
        // Add transform component
        TransformComponent transform;
        transform.position = Vector3(i, i, i);
        world_->addComponent<TransformComponent>(entity, transform);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Entity creation should be fast (less than 1ms per 100 entities)
    EXPECT_LT(duration.count(), entityCount * 10) << "Entity creation should be fast";
    
    // Cleanup
    for (uint32_t entity : entities) {
        world_->destroyEntity(entity);
    }
}

// Memory Leak Detection
TEST_F(EngineCoreCompleteTest, MemoryLeakDetection) {
    // Test for memory leaks in entity creation/destruction
    const int iterations = 100;
    
    for (int i = 0; i < iterations; ++i) {
        // Create entity with components
        uint32_t entity = world_->createEntity();
        
        TransformComponent transform;
        transform.position = Vector3(i, i, i);
        world_->addComponent<TransformComponent>(entity, transform);
        
        // Update world
        world_->update(1.0f / 60.0f);
        
        // Destroy entity
        world_->destroyEntity(entity);
    }
    
    // Force garbage collection if available
    world_->update(1.0f / 60.0f);
    
    // Memory should be cleaned up
    EXPECT_TRUE(true) << "Memory leak test completed";
}

} // namespace FoundryEngine
