/**
 * @file MemoryManager.h
 * @brief Memory management system with SIMD optimization and pooling
 * @author FoundryEngine Team
 * @date 2024
 * @version 2.0.0
 */

#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <immintrin.h>

namespace FoundryEngine {

/**
 * @class AdvancedMemoryManager
 * @brief High-performance memory manager with SIMD optimization
 */
class AdvancedMemoryManager {
public:
    struct PoolConfig {
        size_t blockSize = 64;
        size_t initialBlocks = 1024;
        size_t maxBlocks = 65536;
        bool useAlignment = true;
        size_t alignment = 32; // AVX2 alignment
    };

    struct MemoryStats {
        size_t totalAllocated = 0;
        size_t totalFreed = 0;
        size_t currentUsage = 0;
        size_t peakUsage = 0;
        size_t poolHits = 0;
        size_t poolMisses = 0;
        double fragmentationRatio = 0.0;
    };

    static AdvancedMemoryManager& getInstance();
    
    // Pool management
    void createPool(const std::string& name, const PoolConfig& config);
    void destroyPool(const std::string& name);
    
    // SIMD-optimized allocation
    void* allocateAligned(size_t size, size_t alignment = 32);
    void* allocateFromPool(const std::string& poolName, size_t size);
    void deallocate(void* ptr);
    void deallocateToPool(const std::string& poolName, void* ptr);
    
    // Bulk operations with SIMD
    void bulkZero(void* ptr, size_t size);
    void bulkCopy(void* dest, const void* src, size_t size);
    void bulkSet(void* ptr, uint8_t value, size_t size);
    
    // Statistics and monitoring
    MemoryStats getStats() const;
    void resetStats();
    double getFragmentation() const;
    
    // Garbage collection
    void collectGarbage();
    void setGCThreshold(size_t threshold);

private:
    class MemoryPool;
    std::unordered_map<std::string, std::unique_ptr<MemoryPool>> pools_;
    mutable std::mutex poolsMutex_;
    std::atomic<MemoryStats> stats_;
    size_t gcThreshold_ = 1024 * 1024 * 100; // 100MB
};

} // namespace FoundryEngine