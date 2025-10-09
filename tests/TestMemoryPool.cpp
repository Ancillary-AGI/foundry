#include "gtest/gtest.h"
#include "GameEngine/core/MemoryPool.h"
#include <thread>
#include <chrono>

namespace FoundryEngine {
namespace Tests {

/**
 * @brief Test fixture for MemoryPool tests
 */
class MemoryPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        memoryPool = std::make_unique<MemoryPool>(1024, 8192); // 1KB blocks, 8KB total
    }

    void TearDown() override {
        memoryPool.reset();
    }

    std::unique_ptr<MemoryPool> memoryPool;
};

/**
 * @brief Test basic memory allocation and deallocation
 */
TEST_F(MemoryPoolTest, BasicAllocation) {
    // Test type-safe allocation
    auto allocation = memoryPool->allocateType<int>(5);
    ASSERT_TRUE(allocation);

    int* data = allocation.get();
    ASSERT_NE(data, nullptr);

    // Test that we can write to allocated memory
    for (int i = 0; i < 5; ++i) {
        data[i] = i * 10;
    }

    // Verify data integrity
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(data[i], i * 10);
    }

    // Test utilization
    float utilization = memoryPool->utilization();
    EXPECT_GT(utilization, 0.0f);
    EXPECT_LE(utilization, 100.0f);
}

/**
 * @brief Test memory pool statistics
 */
TEST_F(MemoryPoolTest, MemoryStatistics) {
    size_t initialTotal = memoryPool->totalAllocated();
    size_t initialFree = memoryPool->totalFree();

    // Allocate some memory
    auto alloc1 = memoryPool->allocateType<double>(100);
    auto alloc2 = memoryPool->allocateType<char>(50);

    size_t afterAllocTotal = memoryPool->totalAllocated();
    size_t afterAllocFree = memoryPool->totalFree();

    // Total allocated should increase
    EXPECT_GT(afterAllocTotal, initialTotal);

    // Total free should decrease
    EXPECT_LT(afterAllocFree, initialFree);

    // Test fragmentation ratio
    float fragmentation = memoryPool->fragmentationRatio();
    EXPECT_GE(fragmentation, 0.0f);
    EXPECT_LE(fragmentation, 1.0f);
}

/**
 * @brief Test thread safety of memory pool
 */
TEST_F(MemoryPoolTest, ThreadSafety) {
    const int numThreads = 4;
    const int allocationsPerThread = 100;

    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};

    // Launch multiple threads that allocate and deallocate memory
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < allocationsPerThread; ++i) {
                try {
                    auto allocation = memoryPool->allocateType<int>(10);
                    if (allocation) {
                        int* data = allocation.get();
                        *data = t * 1000 + i;
                        successCount++;
                    }
                } catch (const std::exception& e) {
                    // Allocation might fail under heavy load, that's OK
                }
            }
        });
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }

    // Verify that some allocations succeeded
    EXPECT_GT(successCount.load(), 0);

    // Pool should still be in valid state
    float utilization = memoryPool->utilization();
    EXPECT_GE(utilization, 0.0f);
    EXPECT_LE(utilization, 100.0f);
}

/**
 * @brief Test smart pointer integration
 */
TEST_F(MemoryPoolTest, SmartPointerIntegration) {
    // Test PoolPointer
    auto allocation = memoryPool->allocateType<float>(20);
    ASSERT_TRUE(allocation);

    PoolPointer<float> poolPtr(*memoryPool, std::move(allocation));
    ASSERT_TRUE(poolPtr);

    float* data = poolPtr.get();
    ASSERT_NE(data, nullptr);

    // Test array access
    for (int i = 0; i < 20; ++i) {
        data[i] = static_cast<float>(i) * 1.5f;
    }

    // Verify data
    for (int i = 0; i < 20; ++i) {
        EXPECT_FLOAT_EQ(data[i], static_cast<float>(i) * 1.5f);
    }

    // Test move semantics
    PoolPointer<float> movedPtr = std::move(poolPtr);
    ASSERT_FALSE(poolPtr); // Original should be null
    ASSERT_TRUE(movedPtr);  // Moved should be valid

    // Data should still be accessible
    EXPECT_FLOAT_EQ(movedPtr[5], 7.5f);
}

/**
 * @brief Test ScopedAllocation RAII behavior
 */
TEST_F(MemoryPoolTest, ScopedAllocation) {
    std::vector<float> observedValues;

    {
        ScopedAllocation<float> scopedAlloc(memoryPool->allocateType<float>(10));
        ASSERT_TRUE(scopedAlloc);

        float* data = scopedAlloc.get();
        for (int i = 0; i < 10; ++i) {
            data[i] = static_cast<float>(i * i);
        }

        // Copy values for verification
        observedValues.assign(data, data + 10);
    }
    // scopedAlloc should be automatically destroyed here

    // Verify we captured the values before destruction
    ASSERT_EQ(observedValues.size(), 10);
    for (int i = 0; i < 10; ++i) {
        EXPECT_FLOAT_EQ(observedValues[i], static_cast<float>(i * i));
    }
}

/**
 * @brief Test memory pool defragmentation
 */
TEST_F(MemoryPoolTest, Defragmentation) {
    // Allocate and free memory to create fragmentation
    std::vector<void*> allocations;

    // Allocate several blocks
    for (int i = 0; i < 10; ++i) {
        auto alloc = memoryPool->allocateType<int>(5);
        if (alloc) {
            allocations.push_back(alloc.release());
        }
    }

    size_t fragmentedFree = memoryPool->totalFree();
    float initialFragmentation = memoryPool->fragmentationRatio();

    // Defragment the pool
    memoryPool->defragment();

    size_t defragmentedFree = memoryPool->totalFree();
    float finalFragmentation = memoryPool->fragmentationRatio();

    // Fragmentation should not increase after defragmentation
    EXPECT_LE(finalFragmentation, initialFragmentation);

    // Clean up
    for (void* ptr : allocations) {
        memoryPool->deallocateRaw(ptr);
    }
}

/**
 * @brief Test edge cases and error conditions
 */
TEST_F(MemoryPoolTest, EdgeCases) {
    // Test zero allocation
    auto zeroAlloc = memoryPool->allocateType<int>(0);
    EXPECT_FALSE(zeroAlloc);

    // Test allocation of very large block
    auto largeAlloc = memoryPool->allocateType<char>(10000); // Larger than pool
    // This might fail depending on pool size, which is OK

    // Test double deallocation (should be safe)
    auto normalAlloc = memoryPool->allocateType<int>(5);
    ASSERT_TRUE(normalAlloc);

    void* ptr = normalAlloc.release();
    memoryPool->deallocateRaw(ptr);
    memoryPool->deallocateRaw(ptr); // Should be safe (no-op)

    // Test nullptr deallocation
    memoryPool->deallocateRaw(nullptr); // Should be safe (no-op)
}

/**
 * @brief Performance benchmark test
 */
TEST_F(MemoryPoolTest, PerformanceBenchmark) {
    const int numAllocations = 1000;

    // Measure allocation performance
    auto start = std::chrono::high_resolution_clock::now();

    std::vector<void*> allocations;
    for (int i = 0; i < numAllocations; ++i) {
        auto alloc = memoryPool->allocateType<int>(10);
        if (alloc) {
            allocations.push_back(alloc.release());
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "Allocated " << allocations.size() << " blocks in "
              << duration.count() << " microseconds" << std::endl;

    // Cleanup
    for (void* ptr : allocations) {
        memoryPool->deallocateRaw(ptr);
    }

    // Performance should be reasonable (less than 1ms for 1000 allocations)
    EXPECT_LT(duration.count(), 1000);
}

} // namespace Tests
} // namespace FoundryEngine
