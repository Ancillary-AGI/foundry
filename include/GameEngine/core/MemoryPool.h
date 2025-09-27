#ifndef NEUTRAL_GAMEENGINE_MEMORY_POOL_H
#define NEUTRAL_GAMEENGINE_MEMORY_POOL_H

#include <vector>
#include <cstddef>
#include <mutex>
#include <unordered_map>

namespace FoundryEngine {

// Memory pool with fragmentation prevention
class MemoryPool {
public:
    MemoryPool(size_t blockSize = 4096, size_t poolSize = 1024 * 1024); // 1MB default
    ~MemoryPool();

    void* allocate(size_t size);
    void deallocate(void* ptr);

    // Defragmentation
    void defragment();

    // Statistics
    size_t totalAllocated() const;
    size_t totalFree() const;

private:
    struct Block {
        void* start;
        size_t size;
        bool free;
        Block* next;
        Block* prev;
    };

    size_t blockSize_;
    std::vector<void*> poolBlocks_;
    Block* freeList_;
    std::mutex mutex_;
    std::unordered_map<void*, size_t> allocationSize_;

    void expandPool();
    Block* findFreeBlock(size_t size);
    void mergeFreeBlocks();
};

// Lock-free allocator wrapper
template<typename T>
class LockFreeAllocator {
public:
    using value_type = T;

    LockFreeAllocator() = default;

    T* allocate(size_t n) {
        // Use standard allocation for simplicity, but in production use lock-free queues
        return static_cast<T*>(::operator new(n * sizeof(T)));
    }

    void deallocate(T* p, size_t n) {
        ::operator delete(p);
    }
};

} // namespace FoundryEngine

#endif // NEUTRAL_GAMEENGINE_MEMORY_POOL_H
