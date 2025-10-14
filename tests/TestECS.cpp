#include <gtest/gtest.h>
#include "GameEngine/core/World.h"
#include "GameEngine/components/Component.h"

using namespace FoundryEngine;

// Test components
class TestComponent : public Component {
public:
    float value = 0.0f;
    TestComponent(float v = 0.0f) : value(v) {}
};

class AnotherComponent : public Component {
public:
    int id = 0;
    AnotherComponent(int i = 0) : id(i) {}
};

class ECSTest : public ::testing::Test {
protected:
    void SetUp() override {
        world = std::make_unique<World>();
    }
    
    void TearDown() override {
        world.reset();
    }
    
    std::unique_ptr<World> world;
};

TEST_F(ECSTest, EntityCreation) {
    EntityID entity = world->createEntity();
    
    EXPECT_NE(entity, INVALID_ENTITY);
    EXPECT_GT(entity, 0);
}

TEST_F(ECSTest, EntityDestruction) {
    EntityID entity = world->createEntity();
    
    // Entity should exist
    EXPECT_NE(entity, INVALID_ENTITY);
    
    // Destroy entity
    world->destroyEntity(entity);
    
    // Entity should be destroyed (we can't directly test this without exposing internals)
    // But we can test that we can create a new entity
    EntityID newEntity = world->createEntity();
    EXPECT_NE(newEntity, INVALID_ENTITY);
}

TEST_F(ECSTest, ComponentAddition) {
    EntityID entity = world->createEntity();
    
    // Add component
    world->addComponent<TestComponent>(entity, TestComponent(42.0f));
    
    // Get component
    TestComponent* component = world->getComponent<TestComponent>(entity);
    EXPECT_NE(component, nullptr);
    EXPECT_FLOAT_EQ(component->value, 42.0f);
}

TEST_F(ECSTest, ComponentRemoval) {
    EntityID entity = world->createEntity();
    
    // Add component
    world->addComponent<TestComponent>(entity, TestComponent(42.0f));
    
    // Verify component exists
    TestComponent* component = world->getComponent<TestComponent>(entity);
    EXPECT_NE(component, nullptr);
    
    // Remove component
    world->removeComponent<TestComponent>(entity);
    
    // Component should be removed
    component = world->getComponent<TestComponent>(entity);
    EXPECT_EQ(component, nullptr);
}

TEST_F(ECSTest, MultipleComponents) {
    EntityID entity = world->createEntity();
    
    // Add multiple components
    world->addComponent<TestComponent>(entity, TestComponent(42.0f));
    world->addComponent<AnotherComponent>(entity, AnotherComponent(123));
    
    // Get both components
    TestComponent* testComp = world->getComponent<TestComponent>(entity);
    AnotherComponent* anotherComp = world->getComponent<AnotherComponent>(entity);
    
    EXPECT_NE(testComp, nullptr);
    EXPECT_NE(anotherComp, nullptr);
    EXPECT_FLOAT_EQ(testComp->value, 42.0f);
    EXPECT_EQ(anotherComp->id, 123);
}

TEST_F(ECSTest, ComponentQuery) {
    // Create entities with different component combinations
    EntityID entity1 = world->createEntity();
    EntityID entity2 = world->createEntity();
    EntityID entity3 = world->createEntity();
    
    // Add components
    world->addComponent<TestComponent>(entity1, TestComponent(1.0f));
    world->addComponent<TestComponent>(entity2, TestComponent(2.0f));
    world->addComponent<AnotherComponent>(entity2, AnotherComponent(2));
    world->addComponent<AnotherComponent>(entity3, AnotherComponent(3));
    
    // Query entities with TestComponent
    auto testEntities = world->query<TestComponent>();
    EXPECT_EQ(testEntities.size(), 2);
    
    // Query entities with AnotherComponent
    auto anotherEntities = world->query<AnotherComponent>();
    EXPECT_EQ(anotherEntities.size(), 2);
    
    // Query entities with both components
    auto bothEntities = world->query<TestComponent, AnotherComponent>();
    EXPECT_EQ(bothEntities.size(), 1);
    EXPECT_EQ(bothEntities[0], entity2);
}

TEST_F(ECSTest, ComponentUpdate) {
    EntityID entity = world->createEntity();
    
    // Add component
    world->addComponent<TestComponent>(entity, TestComponent(10.0f));
    
    // Get component and modify it
    TestComponent* component = world->getComponent<TestComponent>(entity);
    EXPECT_NE(component, nullptr);
    
    component->value = 20.0f;
    
    // Verify the change
    TestComponent* updatedComponent = world->getComponent<TestComponent>(entity);
    EXPECT_FLOAT_EQ(updatedComponent->value, 20.0f);
}

TEST_F(ECSTest, MultipleEntitiesWithSameComponents) {
    // Create multiple entities with the same component type
    EntityID entity1 = world->createEntity();
    EntityID entity2 = world->createEntity();
    EntityID entity3 = world->createEntity();
    
    world->addComponent<TestComponent>(entity1, TestComponent(1.0f));
    world->addComponent<TestComponent>(entity2, TestComponent(2.0f));
    world->addComponent<TestComponent>(entity3, TestComponent(3.0f));
    
    // Query all entities with TestComponent
    auto entities = world->query<TestComponent>();
    EXPECT_EQ(entities.size(), 3);
    
    // Verify each entity has the correct component value
    for (EntityID entity : entities) {
        TestComponent* component = world->getComponent<TestComponent>(entity);
        EXPECT_NE(component, nullptr);
        EXPECT_GT(component->value, 0.0f);
        EXPECT_LT(component->value, 4.0f);
    }
}

TEST_F(ECSTest, ComponentTypeIDs) {
    // Test that different component types have different IDs
    ComponentID testID = ComponentManager::getTypeID<TestComponent>();
    ComponentID anotherID = ComponentManager::getTypeID<AnotherComponent>();
    
    EXPECT_NE(testID, anotherID);
    
    // Test that the same component type always has the same ID
    ComponentID testID2 = ComponentManager::getTypeID<TestComponent>();
    EXPECT_EQ(testID, testID2);
}
