#include "gtest/gtest.h"
#include "GameEngine/core/Entity.h"
#include "GameEngine/core/Component.h"
#include "GameEngine/core/System.h"
#include "GameEngine/core/World.h"
#include "GameEngine/core/Scene.h"
#include "GameEngine/components/CoreComponents.h"
#include "GameEngine/core/MemoryPool.h"
#include <thread>
#include <chrono>

namespace FoundryEngine {
namespace Tests {

/**
 * @brief Test fixture for Core Systems tests
 */
class CoreSystemsTest : public ::testing::Test {
protected:
    void SetUp() override {
        memoryPool = std::make_unique<MemoryPool>(1024, 8192);
        world = std::make_unique<World>();
        scene = std::make_unique<Scene>("TestScene");
    }

    void TearDown() override {
        scene.reset();
        world.reset();
        memoryPool.reset();
    }

    std::unique_ptr<MemoryPool> memoryPool;
    std::unique_ptr<World> world;
    std::unique_ptr<Scene> scene;
};

/**
 * @brief Test entity creation and management
 */
TEST_F(CoreSystemsTest, EntityManagement) {
    // Test entity creation
    EntityID entity1 = world->createEntity("Player");
    EntityID entity2 = world->createEntity("Enemy");

    EXPECT_NE(entity1, entity2);
    EXPECT_GT(entity1, 0);
    EXPECT_GT(entity2, 0);

    // Test entity lookup
    Entity* foundEntity1 = world->getEntity(entity1);
    Entity* foundEntity2 = world->getEntity(entity2);

    ASSERT_NE(foundEntity1, nullptr);
    ASSERT_NE(foundEntity2, nullptr);
    EXPECT_EQ(foundEntity1->getName(), "Player");
    EXPECT_EQ(foundEntity2->getName(), "Enemy");

    // Test entity destruction
    world->destroyEntity(entity1);
    EXPECT_EQ(world->getEntity(entity1), nullptr);
    EXPECT_NE(world->getEntity(entity2), nullptr); // Other entity should still exist
}

/**
 * @brief Test component system
 */
TEST_F(CoreSystemsTest, ComponentSystem) {
    EntityID entity = world->createEntity("TestEntity");

    // Test transform component
    auto transformComponent = std::make_unique<TransformComponent>();
    transformComponent->position = Vector3(1.0f, 2.0f, 3.0f);
    transformComponent->rotation = Quaternion(0.0f, 1.0f, 0.0f, 1.0f);
    transformComponent->scale = Vector3(2.0f, 2.0f, 2.0f);

    world->addComponent(entity, std::move(transformComponent));

    // Test component retrieval
    TransformComponent* retrievedTransform =
        world->getComponent<TransformComponent>(entity);

    ASSERT_NE(retrievedTransform, nullptr);
    EXPECT_EQ(retrievedTransform->position.x, 1.0f);
    EXPECT_EQ(retrievedTransform->position.y, 2.0f);
    EXPECT_EQ(retrievedTransform->position.z, 3.0f);

    // Test component removal
    world->removeComponent<TransformComponent>(entity);
    EXPECT_EQ(world->getComponent<TransformComponent>(entity), nullptr);
}

/**
 * @brief Test component serialization
 */
TEST_F(CoreSystemsTest, ComponentSerialization) {
    // Create a transform component
    auto transformComponent = std::make_unique<TransformComponent>();
    transformComponent->position = Vector3(10.0f, 20.0f, 30.0f);
    transformComponent->rotation = Quaternion(0.1f, 0.2f, 0.3f, 1.0f);
    transformComponent->scale = Vector3(1.5f, 1.5f, 1.5f);

    // Test serialization (using void* for compatibility with existing interface)
    size_t bufferSize = 1024;
    auto buffer = std::make_unique<std::byte[]>(bufferSize);

    // Serialize component
    transformComponent->serialize(buffer.get());

    // Create new component and deserialize
    auto newTransformComponent = std::make_unique<TransformComponent>();
    newTransformComponent->deserialize(buffer.get());

    // Verify data integrity
    EXPECT_EQ(newTransformComponent->position.x, transformComponent->position.x);
    EXPECT_EQ(newTransformComponent->position.y, transformComponent->position.y);
    EXPECT_EQ(newTransformComponent->position.z, transformComponent->position.z);

    EXPECT_EQ(newTransformComponent->scale.x, transformComponent->scale.x);
    EXPECT_EQ(newTransformComponent->scale.y, transformComponent->scale.y);
    EXPECT_EQ(newTransformComponent->scale.z, transformComponent->scale.z);
}

/**
 * @brief Test system registration and execution
 */
TEST_F(CoreSystemsTest, SystemManagement) {
    // Create a test system
    class TestSystem : public System {
    public:
        TestSystem() : executionCount(0) {}

        void update(float deltaTime) override {
            executionCount++;
            totalDeltaTime += deltaTime;
        }

        int executionCount;
        float totalDeltaTime;
    };

    auto testSystem = std::make_unique<TestSystem>();
    world->registerSystem("TestSystem", std::move(testSystem));

    // Test system update
    world->update(0.016f); // ~60 FPS
    world->update(0.016f);
    world->update(0.016f);

    // Verify system was executed
    TestSystem* retrievedSystem = static_cast<TestSystem*>(
        world->getSystem("TestSystem"));
    ASSERT_NE(retrievedSystem, nullptr);
    EXPECT_EQ(retrievedSystem->executionCount, 3);
    EXPECT_FLOAT_EQ(retrievedSystem->totalDeltaTime, 0.048f);
}

/**
 * @brief Test scene management
 */
TEST_F(CoreSystemsTest, SceneManagement) {
    // Test scene creation
    EXPECT_EQ(scene->getName(), "TestScene");
    EXPECT_TRUE(scene->isActive());

    // Test entity addition to scene
    EntityID entity1 = world->createEntity("SceneEntity1");
    EntityID entity2 = world->createEntity("SceneEntity2");

    scene->addEntity(entity1);
    scene->addEntity(entity2);

    // Test entity retrieval from scene
    auto sceneEntities = scene->getEntities();
    EXPECT_EQ(sceneEntities.size(), 2);

    // Test scene activation/deactivation
    scene->setActive(false);
    EXPECT_FALSE(scene->isActive());

    scene->setActive(true);
    EXPECT_TRUE(scene->isActive());
}

/**
 * @brief Test world-scene integration
 */
TEST_F(CoreSystemsTest, WorldSceneIntegration) {
    // Create entities in world
    EntityID playerEntity = world->createEntity("Player");
    EntityID enemyEntity = world->createEntity("Enemy");

    // Add components to entities
    auto playerTransform = std::make_unique<TransformComponent>();
    playerTransform->position = Vector3(0.0f, 0.0f, 0.0f);
    world->addComponent(playerEntity, std::move(playerTransform));

    auto enemyTransform = std::make_unique<TransformComponent>();
    enemyTransform->position = Vector3(10.0f, 0.0f, 0.0f);
    world->addComponent(enemyEntity, std::move(enemyTransform));

    // Add entities to scene
    scene->addEntity(playerEntity);
    scene->addEntity(enemyEntity);

    // Test that scene contains the entities
    auto sceneEntities = scene->getEntities();
    EXPECT_EQ(sceneEntities.size(), 2);

    // Test that entities are still accessible through world
    Entity* player = world->getEntity(playerEntity);
    Entity* enemy = world->getEntity(enemyEntity);
    ASSERT_NE(player, nullptr);
    ASSERT_NE(enemy, nullptr);

    TransformComponent* playerTransform = world->getComponent<TransformComponent>(playerEntity);
    TransformComponent* enemyTransform = world->getComponent<TransformComponent>(enemyEntity);
    ASSERT_NE(playerTransform, nullptr);
    ASSERT_NE(enemyTransform, nullptr);

    EXPECT_EQ(playerTransform->position.x, 0.0f);
    EXPECT_EQ(enemyTransform->position.x, 10.0f);
}

/**
 * @brief Test concurrent entity operations
 */
TEST_F(CoreSystemsTest, ConcurrentEntityOperations) {
    const int numThreads = 4;
    const int entitiesPerThread = 25;

    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};

    // Launch multiple threads that create entities and add components
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < entitiesPerThread; ++i) {
                EntityID entity = world->createEntity("ThreadEntity_" +
                    std::to_string(t) + "_" + std::to_string(i));

                if (entity > 0) {
                    auto transform = std::make_unique<TransformComponent>();
                    transform->position = Vector3(
                        static_cast<float>(t * 10),
                        static_cast<float>(i),
                        0.0f);

                    world->addComponent(entity, std::move(transform));

                    // Verify component was added
                    if (world->getComponent<TransformComponent>(entity) != nullptr) {
                        successCount++;
                    }
                }
            }
        });
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }

    // Verify concurrent operations worked
    EXPECT_EQ(successCount.load(), numThreads * entitiesPerThread);

    // World should contain all created entities
    EXPECT_EQ(world->getEntityCount(), numThreads * entitiesPerThread);
}

/**
 * @brief Test memory pool integration with core systems
 */
TEST_F(CoreSystemsTest, MemoryPoolIntegration) {
    size_t initialMemory = memoryPool->totalAllocated();

    // Create many entities with components to test memory usage
    std::vector<EntityID> entities;
    for (int i = 0; i < 100; ++i) {
        EntityID entity = world->createEntity("MemoryTestEntity_" + std::to_string(i));
        if (entity > 0) {
            entities.push_back(entity);

            // Add multiple components
            auto transform = std::make_unique<TransformComponent>();
            auto camera = std::make_unique<CameraComponent>();
            auto light = std::make_unique<LightComponent>();

            world->addComponent(entity, std::move(transform));
            world->addComponent(entity, std::move(camera));
            world->addComponent(entity, std::move(light));
        }
    }

    size_t afterAllocationMemory = memoryPool->totalAllocated();
    EXPECT_GT(afterAllocationMemory, initialMemory);

    // Test memory utilization
    float utilization = memoryPool->utilization();
    EXPECT_GT(utilization, 0.0f);
    EXPECT_LE(utilization, 100.0f);

    // Clean up
    for (EntityID entity : entities) {
        world->destroyEntity(entity);
    }
}

/**
 * @brief Test system execution order
 */
TEST_F(CoreSystemsTest, SystemExecutionOrder) {
    // Create multiple test systems
    class FirstSystem : public System {
    public:
        void update(float deltaTime) override {
            executionOrder.push_back(1);
        }
        static std::vector<int> executionOrder;
    };

    class SecondSystem : public System {
    public:
        void update(float deltaTime) override {
            executionOrder.push_back(2);
        }
        static std::vector<int> executionOrder;
    };

    class ThirdSystem : public System {
    public:
        void update(float deltaTime) override {
            executionOrder.push_back(3);
        }
        static std::vector<int> executionOrder;
    };

    std::vector<int> FirstSystem::executionOrder;
    std::vector<int> SecondSystem::executionOrder;
    std::vector<int> ThirdSystem::executionOrder;

    // Register systems in specific order
    world->registerSystem("ThirdSystem", std::make_unique<ThirdSystem>());
    world->registerSystem("FirstSystem", std::make_unique<FirstSystem>());
    world->registerSystem("SecondSystem", std::make_unique<SecondSystem>());

    // Update world multiple times
    for (int i = 0; i < 3; ++i) {
        FirstSystem::executionOrder.clear();
        SecondSystem::executionOrder.clear();
        ThirdSystem::executionOrder.clear();

        world->update(0.016f);

        // Verify execution order (should be registration order)
        ASSERT_GE(FirstSystem::executionOrder.size(), 1);
        ASSERT_GE(SecondSystem::executionOrder.size(), 1);
        ASSERT_GE(ThirdSystem::executionOrder.size(), 1);

        EXPECT_EQ(FirstSystem::executionOrder[0], 1);
        EXPECT_EQ(SecondSystem::executionOrder[0], 2);
        EXPECT_EQ(ThirdSystem::executionOrder[0], 3);
    }
}

/**
 * @brief Test error handling in core systems
 */
TEST_F(CoreSystemsTest, ErrorHandling) {
    // Test invalid entity operations
    EXPECT_EQ(world->getEntity(99999), nullptr); // Non-existent entity

    // Test component operations on invalid entity
    EXPECT_EQ(world->getComponent<TransformComponent>(99999), nullptr);
    EXPECT_NO_THROW(world->removeComponent<TransformComponent>(99999)); // Should handle gracefully

    // Test system operations
    EXPECT_EQ(world->getSystem("NonExistentSystem"), nullptr);

    // Test scene operations
    EXPECT_NO_THROW(scene->addEntity(99999)); // Invalid entity should be handled
    EXPECT_NO_THROW(scene->removeEntity(99999));
}

/**
 * @brief Test core systems performance
 */
TEST_F(CoreSystemsTest, Performance) {
    const int numEntities = 1000;

    // Measure entity creation performance
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < numEntities; ++i) {
        EntityID entity = world->createEntity("PerfEntity_" + std::to_string(i));
        if (entity > 0) {
            auto transform = std::make_unique<TransformComponent>();
            world->addComponent(entity, std::move(transform));
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "Created " << numEntities << " entities with components in "
              << duration.count() << " microseconds" << std::endl;

    // Performance should be reasonable (less than 100ms for 1000 entities)
    EXPECT_LT(duration.count(), 100000);

    // Verify all entities were created
    EXPECT_EQ(world->getEntityCount(), numEntities);
}

/**
 * @brief Test component cloning
 */
TEST_F(CoreSystemsTest, ComponentCloning) {
    // Create original component
    auto originalTransform = std::make_unique<TransformComponent>();
    originalTransform->position = Vector3(5.0f, 10.0f, 15.0f);
    originalTransform->rotation = Quaternion(0.1f, 0.2f, 0.3f, 1.0f);
    originalTransform->scale = Vector3(2.0f, 3.0f, 4.0f);

    // Clone component
    Component* clonedComponent = originalTransform->clone();
    ASSERT_NE(clonedComponent, nullptr);

    TransformComponent* clonedTransform = static_cast<TransformComponent*>(clonedComponent);
    ASSERT_NE(clonedTransform, nullptr);

    // Verify cloned data
    EXPECT_EQ(clonedTransform->position.x, originalTransform->position.x);
    EXPECT_EQ(clonedTransform->position.y, originalTransform->position.y);
    EXPECT_EQ(clonedTransform->position.z, originalTransform->position.z);

    EXPECT_EQ(clonedTransform->scale.x, originalTransform->scale.x);
    EXPECT_EQ(clonedTransform->scale.y, originalTransform->scale.y);
    EXPECT_EQ(clonedTransform->scale.z, originalTransform->scale.z);

    // Clean up
    delete clonedComponent;
}

/**
 * @brief Test transform hierarchy
 */
TEST_F(CoreSystemsTest, TransformHierarchy) {
    EntityID parentEntity = world->createEntity("Parent");
    EntityID childEntity = world->createEntity("Child");

    // Set up parent-child relationship
    TransformComponent* parentTransform = world->getComponent<TransformComponent>(parentEntity);
    TransformComponent* childTransform = world->getComponent<TransformComponent>(childEntity);

    ASSERT_NE(parentTransform, nullptr);
    ASSERT_NE(childTransform, nullptr);

    // Set parent transform
    parentTransform->position = Vector3(10.0f, 0.0f, 0.0f);
    parentTransform->rotation = Quaternion(0.0f, 0.0f, 1.0f, 1.0f); // 180 degree rotation

    // Set child transform relative to parent
    childTransform->position = Vector3(5.0f, 0.0f, 0.0f);

    // Test world matrix calculation
    Matrix4 parentWorldMatrix = parentTransform->getWorldMatrix();
    Matrix4 childWorldMatrix = childTransform->getWorldMatrix();

    // Parent should be at (10, 0, 0)
    EXPECT_FLOAT_EQ(parentWorldMatrix.m[12], 10.0f);
    EXPECT_FLOAT_EQ(parentWorldMatrix.m[13], 0.0f);
    EXPECT_FLOAT_EQ(parentWorldMatrix.m[14], 0.0f);

    // Child should be at (15, 0, 0) relative to parent
    EXPECT_FLOAT_EQ(childWorldMatrix.m[12], 15.0f);
    EXPECT_FLOAT_EQ(childWorldMatrix.m[13], 0.0f);
    EXPECT_FLOAT_EQ(childWorldMatrix.m[14], 0.0f);
}

} // namespace Tests
} // namespace FoundryEngine
