/**
 * @file AdvancedMemoryManager.cpp
 * @brief Implementation of advanced memory management system
 */

#include "GameEngine/core/AdvancedMemoryManager.h"
#include <algorithm>
#include <cstring>
#include <immintrin.h>

namespace FoundryEngine {

class AdvancedMemoryManager::MemoryPool {
public:
    MemoryPool(const PoolConfig& config) : config_(config) {
        blocks_.reserve(config.initialBlocks);
        freeBlocks_.reserve(config.initialBlocks);
        
        // Pre-allocate initial blocks
        for (size_t i = 0; i < config.initialBlocks; ++i) {
            allocateNewBlock();
        }
    }
    
    ~MemoryPool() {
        for (auto* block : blocks_) {
            _aligned_free(block);
        }
    }
    
    void* allocate(size_t size) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (size > config_.blockSize) {
            // Allocate directly for large requests
            return _aligned_malloc(size, config_.alignment);
        }
        
        if (freeBlocks_.empty() && blocks_.size() < config_.maxBlocks) {
            allocateNewBlock();
        }
        
        if (!freeBlocks_.empty()) {
            void* block = freeBlocks_.back();
            freeBlocks_.pop_back();
            return block;
        }
        
        return nullptr; // Pool exhausted
    }
    
    void deallocate(void* ptr) {
        if (!ptr) return;
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Check if this pointer belongs to our pool
        auto it = std::find(blocks_.begin(), blocks_.end(), ptr);
        if (it != blocks_.end()) {
            freeBlocks_.push_back(ptr);
        } else {
            // Direct allocation, free normally
            _aligned_free(ptr);
        }
    }
    
private:
    void allocateNewBlock() {
        void* block = _aligned_malloc(config_.blockSize, config_.alignment);
        if (block) {
            blocks_.push_back(block);
            freeBlocks_.push_back(block);
        }
    }
    
    PoolConfig config_;
    std::vector<void*> blocks_;
    std::vector<void*> freeBlocks_;
    std::mutex mutex_;
};

AdvancedMemoryManager& AdvancedMemoryManager::getInstance() {
    static AdvancedMemoryManager instance;
    return instance;
}

void AdvancedMemoryManager::createPool(const std::string& name, const PoolConfig& config) {
    std::lock_guard<std::mutex> lock(poolsMutex_);
    pools_[name] = std::make_unique<MemoryPool>(config);
}

void AdvancedMemoryManager::destroyPool(const std::string& name) {
    std::lock_guard<std::mutex> lock(poolsMutex_);
    pools_.erase(name);
}

void* AdvancedMemoryManager::allocateAligned(size_t size, size_t alignment) {
    void* ptr = _aligned_malloc(size, alignment);
    if (ptr) {
        auto stats = stats_.load();
        stats.totalAllocated += size;
        stats.currentUsage += size;
        stats.peakUsage = std::max(stats.peakUsage, stats.currentUsage);
        stats_.store(stats);
    }
    return ptr;
}

void* AdvancedMemoryManager::allocateFromPool(const std::string& poolName, size_t size) {
    std::lock_guard<std::mutex> lock(poolsMutex_);
    
    auto it = pools_.find(poolName);
    if (it != pools_.end()) {
        void* ptr = it->second->allocate(size);
        if (ptr) {
            auto stats = stats_.load();
            stats.poolHits++;
            stats_.store(stats);
        } else {
            auto stats = stats_.load();
            stats.poolMisses++;
            stats_.store(stats);
        }
        return ptr;
    }
    
    auto stats = stats_.load();
    stats.poolMisses++;
    stats_.store(stats);
    return nullptr;
}

void AdvancedMemoryManager::deallocate(void* ptr) {
    if (!ptr) return;
    
    _aligned_free(ptr);
    
    auto stats = stats_.load();
    stats.totalFreed += _msize(ptr);
    stats.currentUsage -= _msize(ptr);
    stats_.store(stats);
}

void AdvancedMemoryManager::bulkZero(void* ptr, size_t size) {
    if (!ptr || size == 0) return;
    
    // Use SIMD for large blocks
    if (size >= 32 && reinterpret_cast<uintptr_t>(ptr) % 32 == 0) {
        __m256i zero = _mm256_setzero_si256();
        uint8_t* aligned_ptr = static_cast<uint8_t*>(ptr);
        
        size_t simd_size = size & ~31; // Round down to multiple of 32
        for (size_t i = 0; i < simd_size; i += 32) {
            _mm256_store_si256(reinterpret_cast<__m256i*>(aligned_ptr + i), zero);
        }
        
        // Handle remaining bytes
        std::memset(aligned_ptr + simd_size, 0, size - simd_size);
    } else {
        std::memset(ptr, 0, size);
    }
}

void AdvancedMemoryManager::bulkCopy(void* dest, const void* src, size_t size) {
    if (!dest || !src || size == 0) return;
    
    // Use SIMD for large aligned blocks
    if (size >= 32 && 
        reinterpret_cast<uintptr_t>(dest) % 32 == 0 && 
        reinterpret_cast<uintptr_t>(src) % 32 == 0) {
        
        const uint8_t* src_ptr = static_cast<const uint8_t*>(src);
        uint8_t* dest_ptr = static_cast<uint8_t*>(dest);
        
        size_t simd_size = size & ~31;
        for (size_t i = 0; i < simd_size; i += 32) {
            __m256i data = _mm256_load_si256(reinterpret_cast<const __m256i*>(src_ptr + i));
            _mm256_store_si256(reinterpret_cast<__m256i*>(dest_ptr + i), data);
        }
        
        // Handle remaining bytes
        std::memcpy(dest_ptr + simd_size, src_ptr + simd_size, size - simd_size);
    } else {
        std::memcpy(dest, src, size);
    }
}

void AdvancedMemoryManager::bulkSet(void* ptr, uint8_t value, size_t size) {
    if (!ptr || size == 0) return;
    
    // Use SIMD for large blocks
    if (size >= 32 && reinterpret_cast<uintptr_t>(ptr) % 32 == 0) {
        __m256i pattern = _mm256_set1_epi8(value);
        uint8_t* aligned_ptr = static_cast<uint8_t*>(ptr);
        
        size_t simd_size = size & ~31;
        for (size_t i = 0; i < simd_size; i += 32) {
            _mm256_store_si256(reinterpret_cast<__m256i*>(aligned_ptr + i), pattern);
        }
        
        // Handle remaining bytes
        std::memset(aligned_ptr + simd_size, value, size - simd_size);
    } else {
        std::memset(ptr, value, size);
    }
}

AdvancedMemoryManager::MemoryStats AdvancedMemoryManager::getStats() const {
    return stats_.load();
}

void AdvancedMemoryManager::resetStats() {
    MemoryStats empty{};
    stats_.store(empty);
}

double AdvancedMemoryManager::getFragmentation() const {
    auto stats = stats_.load();
    if (stats.totalAllocated == 0) return 0.0;
    
    return static_cast<double>(stats.totalAllocated - stats.currentUsage) / stats.totalAllocated;
}

void AdvancedMemoryManager::collectGarbage() {
    // Trigger garbage collection for pools
    std::lock_guard<std::mutex> lock(poolsMutex_);
    
    for (auto& [name, pool] : pools_) {
        // Pool-specific garbage collection would go here
        // For now, this is a placeholder
    }
}

void AdvancedMemoryManager::setGCThreshold(size_t threshold) {
    gcThreshold_ = threshold;
}

} // namespace FoundryEngine