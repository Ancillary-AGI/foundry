#include "gtest/gtest.h"
#include "GameEngine/optimization/SpatialPartition.h"
#include "GameEngine/core/MemoryPool.h"
#include "GameEngine/math/Vector3.h"
#include <thread>
#include <chrono>

namespace FoundryEngine {
namespace Tests {

/**
 * @brief Test fixture for Optimization Systems tests
 */
class OptimizationSystemsTest : public ::testing::Test {
protected:
    void SetUp() override {
        memoryPool = std::make_unique<MemoryPool>(2048, 16384);
    }

    void TearDown() override {
        memoryPool.reset();
    }

    std::unique_ptr<MemoryPool> memoryPool;
};

/**
 * @brief Test spatial partition system
 */
TEST_F(OptimizationSystemsTest, SpatialPartition) {
    SpatialPartition partition;

    // Test spatial partition initialization
    EXPECT_TRUE(partition.initialize());
    EXPECT_TRUE(partition.isInitialized());

    // Test world bounds setup
    partition.setWorldBounds(Vector3(-100.0f, -100.0f, -100.0f), Vector3(100.0f, 100.0f, 100.0f));
    Vector3 minBounds, maxBounds;
    partition.getWorldBounds(minBounds, maxBounds);
    EXPECT_EQ(minBounds, Vector3(-100.0f, -100.0f, -100.0f));
    EXPECT_EQ(maxBounds, Vector3(100.0f, 100.0f, 100.0f));

    // Test object registration
    ObjectID obj1 = partition.registerObject(Vector3(0.0f, 0.0f, 0.0f), 1.0f);
    ObjectID obj2 = partition.registerObject(Vector3(10.0f, 0.0f, 0.0f), 2.0f);
    ObjectID obj3 = partition.registerObject(Vector3(50.0f, 50.0f, 50.0f), 1.5f);

    EXPECT_GT(obj1, 0);
    EXPECT_GT(obj2, 0);
    EXPECT_GT(obj3, 0);

    // Test object updates
    partition.updateObjectPosition(obj1, Vector3(1.0f, 1.0f, 1.0f));
    partition.updateObjectBounds(obj1, 1.5f);

    Vector3 obj1Pos = partition.getObjectPosition(obj1);
    float obj1Bounds = partition.getObjectBounds(obj1);
    EXPECT_EQ(obj1Pos, Vector3(1.0f, 1.0f, 1.0f));
    EXPECT_FLOAT_EQ(obj1Bounds, 1.5f);

    // Test spatial queries
    std::vector<ObjectID> nearbyObjects = partition.getObjectsInRadius(Vector3(0.0f, 0.0f, 0.0f), 5.0f);
    EXPECT_GT(nearbyObjects.size(), 0);

    std::vector<ObjectID> boxObjects = partition.getObjectsInBox(
        Vector3(-5.0f, -5.0f, -5.0f),
        Vector3(5.0f, 5.0f, 5.0f));
    EXPECT_GT(boxObjects.size(), 0);

    // Test frustum culling
    partition.enableFrustumCulling(true);
    EXPECT_TRUE(partition.isFrustumCullingEnabled());

    // Set up frustum planes (simplified)
    std::vector<Vector4> frustumPlanes = {
        Vector4(1.0f, 0.0f, 0.0f, 5.0f),  // Left plane
        Vector4(-1.0f, 0.0f, 0.0f, 5.0f), // Right plane
        Vector4(0.0f, 1.0f, 0.0f, 5.0f),  // Bottom plane
        Vector4(0.0f, -1.0f, 0.0f, 5.0f), // Top plane
        Vector4(0.0f, 0.0f, 1.0f, 5.0f),  // Near plane
        Vector4(0.0f, 0.0f, -1.0f, 5.0f)  // Far plane
    };

    partition.setFrustumPlanes(frustumPlanes);
    std::vector<ObjectID> visibleObjects = partition.getVisibleObjects();
    EXPECT_GE(visibleObjects.size(), 0);

    // Test occlusion culling
    partition.enableOcclusionCulling(true);
    EXPECT_TRUE(partition.isOcclusionCullingEnabled());

    partition.setOcclusionThreshold(0.1f);
    EXPECT_FLOAT_EQ(partition.getOcclusionThreshold(), 0.1f);

    // Test LOD management
    partition.enableLOD(true);
    EXPECT_TRUE(partition.isLODEnabled());

    partition.setLODLevels(4);
    EXPECT_EQ(partition.getLODLevels(), 4);

    partition.setLODTransitionDistance(0, 10.0f);
    partition.setLODTransitionDistance(1, 25.0f);
    partition.setLODTransitionDistance(2, 50.0f);
    partition.setLODTransitionDistance(3, 100.0f);

    // Test performance optimization
    partition.enableSpatialOptimization(true);
    EXPECT_TRUE(partition.isSpatialOptimizationEnabled());

    partition.setOptimizationFrequency(30); // 30 FPS
    EXPECT_EQ(partition.getOptimizationFrequency(), 30);

    // Test object removal
    partition.unregisterObject(obj3);
    partition.unregisterObject(obj2);
    partition.unregisterObject(obj1);

    EXPECT_EQ(partition.getObjectCount(), 0);

    // Test cleanup
    partition.shutdown();
    EXPECT_FALSE(partition.isInitialized());
}

/**
 * @brief Test optimization performance
 */
TEST_F(OptimizationSystemsTest, Performance) {
    const int numIterations = 100;

    // Measure optimization operations performance
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < numIterations; ++i) {
        SpatialPartition partition;
        partition.initialize();

        partition.setWorldBounds(Vector3(-50.0f, -50.0f, -50.0f), Vector3(50.0f, 50.0f, 50.0f));

        // Register many objects
        std::vector<ObjectID> objects;
        for (int j = 0; j < 100; ++j) {
            Vector3 position(
                static_cast<float>(j % 10) * 5.0f,
                static_cast<float>(j / 10) * 5.0f,
                0.0f);
            ObjectID obj = partition.registerObject(position, 1.0f);
            objects.push_back(obj);
        }

        // Perform spatial queries
        std::vector<ObjectID> nearby = partition.getObjectsInRadius(Vector3(0.0f, 0.0f, 0.0f), 10.0f);
        std::vector<ObjectID> visible = partition.getVisibleObjects();

        // Clean up
        for (ObjectID obj : objects) {
            partition.unregisterObject(obj);
        }
        partition.shutdown();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "Performed " << numIterations << " optimization operations in "
              << duration.count() << " microseconds" << std::endl;

    // Performance should be reasonable (less than 200ms for 100 operations)
    EXPECT_LT(duration.count(), 200000);
}

/**
 * @brief Test optimization memory management
 */
TEST_F(OptimizationSystemsTest, MemoryManagement) {
    size_t initialMemory = memoryPool->totalAllocated();

    // Create multiple optimization systems to test memory usage
    std::vector<std::unique_ptr<SpatialPartition>> partitions;

    for (int i = 0; i < 25; ++i) {
        auto partition = std::make_unique<SpatialPartition>();
        partition->initialize();

        partition->setWorldBounds(
            Vector3(-50.0f, -50.0f, -50.0f),
            Vector3(50.0f, 50.0f, 50.0f));

        // Register objects in a grid pattern
        for (int x = 0; x < 5; ++x) {
            for (int y = 0; y < 5; ++y) {
                for (int z = 0; z < 5; ++z) {
                    Vector3 position(
                        static_cast<float>(x) * 10.0f - 25.0f,
                        static_cast<float>(y) * 10.0f - 25.0f,
                        static_cast<float>(z) * 10.0f - 25.0f);
                    partition->registerObject(position, 2.0f);
                }
            }
        }

        partitions.push_back(std::move(partition));
    }

    size_t afterAllocationMemory = memoryPool->totalAllocated();
    EXPECT_GT(afterAllocationMemory, initialMemory);

    // Test memory utilization
    float utilization = memoryPool->utilization();
    EXPECT_GT(utilization, 0.0f);
    EXPECT_LE(utilization, 100.0f);

    // Clean up
    partitions.clear();
}

/**
 * @brief Test optimization error handling
 */
TEST_F(OptimizationSystemsTest, ErrorHandling) {
    SpatialPartition partition;

    // Test invalid operations
    EXPECT_NO_THROW(partition.registerObject(Vector3(0.0f, 0.0f, 0.0f), -1.0f)); // Negative bounds should handle gracefully
    EXPECT_NO_THROW(partition.updateObjectPosition(99999, Vector3(0.0f, 0.0f, 0.0f))); // Invalid object ID should handle gracefully
    EXPECT_NO_THROW(partition.unregisterObject(99999)); // Invalid object ID should handle gracefully

    // Test uninitialized operations
    EXPECT_FALSE(partition.isInitialized());
    EXPECT_NO_THROW(partition.shutdown()); // Should handle multiple shutdowns

    // Test empty partition queries
    EXPECT_NO_THROW(partition.getObjectsInRadius(Vector3(0.0f, 0.0f, 0.0f), 10.0f));
    EXPECT_NO_THROW(partition.getObjectsInBox(Vector3(-10.0f, -10.0f, -10.0f), Vector3(10.0f, 10.0f, 10.0f)));
    EXPECT_NO_THROW(partition.getVisibleObjects());
}

/**
 * @brief Test optimization concurrent operations
 */
TEST_F(OptimizationSystemsTest, ConcurrentOperations) {
    SpatialPartition partition;
    partition.initialize();

    partition.setWorldBounds(Vector3(-100.0f, -100.0f, -100.0f), Vector3(100.0f, 100.0f, 100.0f));

    const int numThreads = 4;
    const int objectsPerThread = 50;

    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};

    // Launch multiple threads performing spatial operations
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&partition, t, &successCount]() {
            for (int i = 0; i < objectsPerThread; ++i) {
                Vector3 position(
                    static_cast<float>(t * 20),
                    static_cast<float>(i * 2),
                    0.0f);

                ObjectID obj = partition.registerObject(position, 1.0f);
                if (obj > 0) {
                    // Perform spatial queries
                    std::vector<ObjectID> nearby = partition.getObjectsInRadius(position, 5.0f);
                    std::vector<ObjectID> visible = partition.getVisibleObjects();

                    partition.unregisterObject(obj);

                    if (!nearby.empty() || !visible.empty()) {
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
    EXPECT_GT(successCount.load(), 0);

    // Partition should be empty after all operations
    EXPECT_EQ(partition.getObjectCount(), 0);

    // Memory pool should still be in valid state
    float utilization = memoryPool->utilization();
    EXPECT_GE(utilization, 0.0f);
    EXPECT_LE(utilization, 100.0f);

    partition.shutdown();
}

/**
 * @brief Test spatial partitioning algorithms
 */
TEST_F(OptimizationSystemsTest, SpatialPartitioningAlgorithms) {
    SpatialPartition partition;
    partition.initialize();

    partition.setWorldBounds(Vector3(-50.0f, -50.0f, -50.0f), Vector3(50.0f, 50.0f, 50.0f));

    // Test different partitioning methods
    partition.setPartitioningMethod(PartitioningMethod::Octree);
    EXPECT_EQ(partition.getPartitioningMethod(), PartitioningMethod::Octree);

    partition.setPartitioningMethod(PartitioningMethod::KDTree);
    EXPECT_EQ(partition.getPartitioningMethod(), PartitioningMethod::KDTree);

    partition.setPartitioningMethod(PartitioningMethod::BSPTree);
    EXPECT_EQ(partition.getPartitioningMethod(), PartitioningMethod::BSPTree);

    // Test partition parameters
    partition.setMaxObjectsPerNode(16);
    EXPECT_EQ(partition.getMaxObjectsPerNode(), 16);

    partition.setMaxTreeDepth(10);
    EXPECT_EQ(partition.getMaxTreeDepth(), 10);

    // Test tree balancing
    partition.enableAutoBalancing(true);
    EXPECT_TRUE(partition.isAutoBalancingEnabled());

    partition.setBalanceThreshold(0.7f);
    EXPECT_FLOAT_EQ(partition.getBalanceThreshold(), 0.7f);

    // Test spatial hashing
    partition.enableSpatialHashing(true);
    EXPECT_TRUE(partition.isSpatialHashingEnabled());

    partition.setHashTableSize(1024);
    EXPECT_EQ(partition.getHashTableSize(), 1024);

    partition.setHashCellSize(2.0f);
    EXPECT_FLOAT_EQ(partition.getHashCellSize(), 2.0f);

    partition.shutdown();
}

/**
 * @brief Test culling optimization
 */
TEST_F(OptimizationSystemsTest, CullingOptimization) {
    SpatialPartition partition;
    partition.initialize();

    partition.setWorldBounds(Vector3(-100.0f, -100.0f, -100.0f), Vector3(100.0f, 100.0f, 100.0f));

    // Register test objects
    std::vector<ObjectID> objects;
    for (int i = 0; i < 100; ++i) {
        Vector3 position(
            static_cast<float>(i % 10) * 10.0f - 50.0f,
            static_cast<float>(i / 10) * 10.0f - 50.0f,
            0.0f);
        ObjectID obj = partition.registerObject(position, 2.0f);
        objects.push_back(obj);
    }

    // Test view frustum culling
    partition.enableFrustumCulling(true);

    std::vector<Vector4> frustumPlanes = {
        Vector4(1.0f, 0.0f, 0.0f, 10.0f),   // Left
        Vector4(-1.0f, 0.0f, 0.0f, 10.0f),  // Right
        Vector4(0.0f, 1.0f, 0.0f, 10.0f),   // Bottom
        Vector4(0.0f, -1.0f, 0.0f, 10.0f),  // Top
        Vector4(0.0f, 0.0f, 1.0f, 10.0f),   // Near
        Vector4(0.0f, 0.0f, -1.0f, 10.0f)   // Far
    };

    partition.setFrustumPlanes(frustumPlanes);
    std::vector<ObjectID> visibleObjects = partition.getVisibleObjects();

    // Should cull objects outside frustum
    EXPECT_LE(visibleObjects.size(), objects.size());

    // Test occlusion culling
    partition.enableOcclusionCulling(true);

    // Set up occlusion geometry (simplified)
    partition.addOcclusionGeometry(Vector3(0.0f, 0.0f, -5.0f), Vector3(10.0f, 10.0f, 1.0f));
    EXPECT_GT(partition.getOcclusionGeometryCount(), 0);

    std::vector<ObjectID> nonOccludedObjects = partition.getVisibleObjects();
    // Should have fewer objects after occlusion culling
    EXPECT_LE(nonOccludedObjects.size(), visibleObjects.size());

    // Test distance culling
    partition.enableDistanceCulling(true);
    EXPECT_TRUE(partition.isDistanceCullingEnabled());

    partition.setCullDistance(25.0f);
    EXPECT_FLOAT_EQ(partition.getCullDistance(), 25.0f);

    std::vector<ObjectID> nearbyObjects = partition.getObjectsInRadius(Vector3(0.0f, 0.0f, 0.0f), 25.0f);
    EXPECT_LE(nearbyObjects.size(), objects.size());

    // Clean up
    for (ObjectID obj : objects) {
        partition.unregisterObject(obj);
    }
    partition.clearOcclusionGeometry();
    partition.shutdown();
}

/**
 * @brief Test LOD optimization
 */
TEST_F(OptimizationSystemsTest, LODOptimization) {
    SpatialPartition partition;
    partition.initialize();

    partition.setWorldBounds(Vector3(-100.0f, -100.0f, -100.0f), Vector3(100.0f, 100.0f, 100.0f));

    // Register objects with different sizes for LOD testing
    std::vector<ObjectID> smallObjects, mediumObjects, largeObjects;

    for (int i = 0; i < 20; ++i) {
        Vector3 position(
            static_cast<float>(i) * 5.0f - 50.0f,
            0.0f,
            static_cast<float>(i % 5) * 5.0f);

        ObjectID obj = partition.registerObject(position, 1.0f);
        smallObjects.push_back(obj);

        obj = partition.registerObject(position + Vector3(0.0f, 10.0f, 0.0f), 3.0f);
        mediumObjects.push_back(obj);

        obj = partition.registerObject(position + Vector3(0.0f, 20.0f, 0.0f), 5.0f);
        largeObjects.push_back(obj);
    }

    // Test LOD level calculation
    partition.enableLOD(true);
    partition.setLODLevels(3);

    partition.setLODTransitionDistance(0, 10.0f); // High detail
    partition.setLODTransitionDistance(1, 25.0f); // Medium detail
    partition.setLODTransitionDistance(2, 50.0f); // Low detail

    // Test LOD updates based on camera position
    partition.updateLOD(Vector3(0.0f, 0.0f, -60.0f)); // Far from objects

    int highDetailCount = partition.getLODObjectCount(0);
    int mediumDetailCount = partition.getLODObjectCount(1);
    int lowDetailCount = partition.getLODObjectCount(2);

    // At far distance, should have more low detail objects
    EXPECT_GE(lowDetailCount, highDetailCount);

    // Test LOD hysteresis
    partition.enableLODHysteresis(true);
    EXPECT_TRUE(partition.isLODHysteresisEnabled());

    partition.setLODHysteresisThreshold(0.1f);
    EXPECT_FLOAT_EQ(partition.getLODHysteresisThreshold(), 0.1f);

    // Clean up
    for (ObjectID obj : smallObjects) partition.unregisterObject(obj);
    for (ObjectID obj : mediumObjects) partition.unregisterObject(obj);
    for (ObjectID obj : largeObjects) partition.unregisterObject(obj);

    partition.shutdown();
}

/**
 * @brief Test performance monitoring
 */
TEST_F(OptimizationSystemsTest, PerformanceMonitoring) {
    SpatialPartition partition;
    partition.initialize();

    partition.setWorldBounds(Vector3(-50.0f, -50.0f, -50.0f), Vector3(50.0f, 50.0f, 50.0f));

    // Test performance monitoring
    partition.enablePerformanceMonitoring(true);
    EXPECT_TRUE(partition.isPerformanceMonitoringEnabled());

    partition.setMonitoringInterval(1000); // 1 second
    EXPECT_EQ(partition.getMonitoringInterval(), 1000);

    // Register objects and perform operations
    std::vector<ObjectID> objects;
    for (int i = 0; i < 100; ++i) {
        Vector3 position(static_cast<float>(i % 10) * 5.0f, static_cast<float>(i / 10) * 5.0f, 0.0f);
        ObjectID obj = partition.registerObject(position, 1.0f);
        objects.push_back(obj);
    }

    // Perform queries to generate performance data
    for (int i = 0; i < 50; ++i) {
        Vector3 queryPos(static_cast<float>(i % 10) * 5.0f, static_cast<float>(i / 10) * 5.0f, 0.0f);
        partition.getObjectsInRadius(queryPos, 10.0f);
        partition.getVisibleObjects();
    }

    // Test performance statistics
    PerformanceStats stats = partition.getPerformanceStats();
    EXPECT_GE(stats.totalQueries, 0);
    EXPECT_GE(stats.averageQueryTime, 0.0f);
    EXPECT_GE(stats.maxQueryTime, 0.0f);
    EXPECT_GE(stats.minQueryTime, 0.0f);

    // Test performance optimization suggestions
    std::vector<std::string> suggestions = partition.getOptimizationSuggestions();
    // Should provide suggestions based on performance data

    // Clean up
    for (ObjectID obj : objects) {
        partition.unregisterObject(obj);
    }
    partition.shutdown();
}

/**
 * @brief Test batch optimization
 */
TEST_F(OptimizationSystemsTest, BatchOptimization) {
    SpatialPartition partition;
    partition.initialize();

    partition.setWorldBounds(Vector3(-100.0f, -100.0f, -100.0f), Vector3(100.0f, 100.0f, 100.0f));

    // Test batch operations for performance
    std::vector<Vector3> positions;
    std::vector<float> bounds;

    for (int i = 0; i < 1000; ++i) {
        positions.push_back(Vector3(
            static_cast<float>(i % 20) * 5.0f - 50.0f,
            static_cast<float>(i / 20 % 20) * 5.0f - 50.0f,
            static_cast<float>(i / 400) * 5.0f - 50.0f));
        bounds.push_back(1.0f + static_cast<float>(i % 5));
    }

    // Test batch registration
    partition.registerObjectsBatch(positions, bounds);
    EXPECT_EQ(partition.getObjectCount(), 1000);

    // Test batch updates
    for (size_t i = 0; i < positions.size(); ++i) {
        positions[i].x += 1.0f; // Move all objects
    }
    partition.updateObjectsBatch(positions);

    // Verify batch update
    Vector3 firstObjectPos = partition.getObjectPosition(1); // First registered object
    EXPECT_NEAR(firstObjectPos.x, positions[0].x, 0.1f);

    // Test batch queries
    std::vector<Vector3> queryPositions = {
        Vector3(0.0f, 0.0f, 0.0f),
        Vector3(50.0f, 0.0f, 0.0f),
        Vector3(0.0f, 50.0f, 0.0f)
    };

    std::vector<float> queryRadii = {10.0f, 15.0f, 20.0f};

    std::vector<std::vector<ObjectID>> batchResults = partition.batchQueryRadius(queryPositions, queryRadii);
    EXPECT_EQ(batchResults.size(), queryPositions.size());

    // Test batch removal
    partition.unregisterObjectsBatch(); // Remove all objects
    EXPECT_EQ(partition.getObjectCount(), 0);

    partition.shutdown();
}

/**
 * @brief Test optimization integration
 */
TEST_F(OptimizationSystemsTest, Integration) {
    SpatialPartition partition;
    partition.initialize();

    partition.setWorldBounds(Vector3(-100.0f, -100.0f, -100.0f), Vector3(100.0f, 100.0f, 100.0f));

    // Register objects in a complex scene
    std::vector<ObjectID> staticObjects, dynamicObjects;

    // Static objects (buildings, terrain)
    for (int i = 0; i < 50; ++i) {
        Vector3 position(
            static_cast<float>(i % 10) * 20.0f - 100.0f,
            0.0f,
            static_cast<float>(i / 10) * 20.0f - 100.0f);
        ObjectID obj = partition.registerObject(position, 5.0f);
        staticObjects.push_back(obj);
    }

    // Dynamic objects (characters, vehicles)
    for (int i = 0; i < 20; ++i) {
        Vector3 position(
            static_cast<float>(i) * 3.0f,
            0.0f,
            0.0f);
        ObjectID obj = partition.registerObject(position, 1.0f);
        dynamicObjects.push_back(obj);
    }

    // Test integrated culling
    partition.enableFrustumCulling(true);
    partition.enableOcclusionCulling(true);
    partition.enableDistanceCulling(true);
    partition.enableLOD(true);

    // Set up frustum for typical game view
    std::vector<Vector4> gameFrustum = {
        Vector4(0.5f, 0.0f, 0.0f, 20.0f),   // Left
        Vector4(-0.5f, 0.0f, 0.0f, 20.0f),  // Right
        Vector4(0.0f, 0.5f, 0.0f, 15.0f),   // Bottom
        Vector4(0.0f, -0.5f, 0.0f, 15.0f),  // Top
        Vector4(0.0f, 0.0f, 0.5f, 5.0f),    // Near
        Vector4(0.0f, 0.0f, -0.5f, 100.0f)  // Far
    };

    partition.setFrustumPlanes(gameFrustum);

    // Add occlusion geometry (buildings that can occlude)
    for (int i = 0; i < 5; ++i) {
        Vector3 buildingPos(static_cast<float>(i) * 20.0f - 40.0f, 0.0f, -30.0f);
        partition.addOcclusionGeometry(buildingPos, Vector3(8.0f, 20.0f, 8.0f));
    }

    // Test comprehensive culling
    std::vector<ObjectID> visibleObjects = partition.getVisibleObjects();
    EXPECT_LE(visibleObjects.size(), staticObjects.size() + dynamicObjects.size());

    // Test LOD distribution
    partition.updateLOD(Vector3(0.0f, 10.0f, 50.0f)); // Camera looking down

    int highDetail = partition.getLODObjectCount(0);
    int mediumDetail = partition.getLODObjectCount(1);
    int lowDetail = partition.getLODObjectCount(2);

    // Should have reasonable LOD distribution
    EXPECT_GE(highDetail + mediumDetail + lowDetail, visibleObjects.size() * 0.8f);

    // Test dynamic object updates
    for (int i = 0; i < 10; ++i) {
        Vector3 newPos(static_cast<float>(i) * 3.0f + 10.0f, 0.0f, 0.0f);
        partition.updateObjectPosition(dynamicObjects[i], newPos);
    }

    // Verify updates
    Vector3 updatedPos = partition.getObjectPosition(dynamicObjects[5]);
    EXPECT_NEAR(updatedPos.x, 25.0f, 0.1f); // 5 * 3 + 10

    // Clean up
    for (ObjectID obj : dynamicObjects) partition.unregisterObject(obj);
    for (ObjectID obj : staticObjects) partition.unregisterObject(obj);
    partition.clearOcclusionGeometry();
    partition.shutdown();
}

} // namespace Tests
} // namespace FoundryEngine
