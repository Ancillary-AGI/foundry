#ifndef FOUNDRY_GAMEENGINE_MEMORY_POOL_H
#define FOUNDRY_GAMEENGINE_MEMORY_POOL_H

#include <vector>
#include <cstddef>
#include <mutex>
#include <unordered_map>
#include <memory>
#include <atomic>
#include <stdexcept>
#include <type_traits>

namespace FoundryEngine {

/**
 * @brief Type-safe memory allocation result with ownership semantics
 */
template<typename T = std::byte>
class AllocationResult {
public:
    AllocationResult() = default;
    AllocationResult(T* ptr, size_t size) : data_(ptr), size_(size) {}
    AllocationResult(AllocationResult&& other) noexcept : data_(other.data_), size_(other.size_) {
        other.data_ = nullptr;
        other.size_ = 0;
    }
    AllocationResult& operator=(AllocationResult&& other) noexcept {
        if (this != &other) {
            reset();
            data_ = other.data_;
            size_ = other.size_;
            other.data_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }
    ~AllocationResult() { reset(); }

    T* get() const { return data_; }
    T* release() {
        T* ptr = data_;
        data_ = nullptr;
        size_ = 0;
        return ptr;
    }
    size_t size() const { return size_; }
    explicit operator bool() const { return data_ != nullptr; }
    T& operator*() { return *data_; }
    T* operator->() { return data_; }

private:
    void reset() {
        if (data_) {
            delete[] reinterpret_cast<std::byte*>(data_);
            data_ = nullptr;
            size_ = 0;
        }
    }

    T* data_ = nullptr;
    size_t size_ = 0;
};

/**
 * @brief Thread-safe memory pool with type safety and smart pointer support
 */
class MemoryPool {
public:
    /**
     * @brief Construct a memory pool with specified block and pool sizes
     * @param blockSize Size of each memory block (default 4KB)
     * @param poolSize Total size of the memory pool (default 1MB)
     */
    explicit MemoryPool(size_t blockSize = 4096, size_t poolSize = 1024 * 1024);
    ~MemoryPool();

    // Disable copy operations
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    // Enable move operations
    MemoryPool(MemoryPool&& other) noexcept;
    MemoryPool& operator=(MemoryPool&& other) noexcept;

    /**
     * @brief Allocate memory of specified size with type safety
     * @tparam T Type of objects to allocate
     * @param count Number of objects to allocate
     * @return AllocationResult with ownership of the allocated memory
     * @throws std::bad_alloc if allocation fails
     */
    template<typename T>
    AllocationResult<T> allocateType(size_t count = 1) {
        size_t totalSize = count * sizeof(T);
        if (totalSize == 0) {
            throw std::invalid_argument("Cannot allocate zero-sized array");
        }

        void* ptr = allocateRaw(totalSize);
        if (!ptr) {
            throw std::bad_alloc();
        }

        return AllocationResult<T>(static_cast<T*>(ptr), totalSize);
    }

    /**
     * @brief Allocate raw memory (legacy support)
     * @param size Size in bytes to allocate
     * @return Pointer to allocated memory or nullptr if failed
     */
    void* allocateRaw(size_t size);

    /**
     * @brief Deallocate previously allocated memory
     * @param ptr Pointer to memory to deallocate
     */
    void deallocateRaw(void* ptr);

    /**
     * @brief Defragment the memory pool to reduce fragmentation
     */
    void defragment();

    /**
     * @brief Get total bytes currently allocated
     * @return Total allocated bytes
     */
    size_t totalAllocated() const;

    /**
     * @brief Get total bytes available for allocation
     * @return Total free bytes
     */
    size_t totalFree() const;

    /**
     * @brief Get memory pool utilization percentage
     * @return Utilization as percentage (0.0f to 100.0f)
     */
    float utilization() const;

    /**
     * @brief Get fragmentation ratio
     * @return Fragmentation ratio (0.0f = no fragmentation, 1.0f = maximum fragmentation)
     */
    float fragmentationRatio() const;

private:
    struct Block {
        std::byte* start;
        size_t size;
        bool free;
        Block* next;
        Block* prev;
        size_t allocationId; // For tracking ownership
    };

    size_t blockSize_;
    std::vector<std::unique_ptr<std::byte[]>> poolBlocks_;
    Block* freeList_;
    mutable std::mutex mutex_;
    std::unordered_map<void*, size_t> allocationSize_;
    std::atomic<size_t> totalAllocated_{0};
    std::atomic<size_t> nextAllocationId_{1};

    void expandPool();
    Block* findFreeBlock(size_t size);
    void mergeFreeBlocks();
    void cleanup();
};

/**
 * @brief Thread-safe allocator wrapper with RAII support
 */
template<typename T>
class ThreadSafeAllocator {
public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

    ThreadSafeAllocator() = default;

    template<typename U>
    ThreadSafeAllocator(const ThreadSafeAllocator<U>&) noexcept {}

    /**
     * @brief Allocate memory for n objects
     * @param n Number of objects to allocate
     * @return Pointer to allocated memory
     */
    pointer allocate(size_type n) {
        if (n == 0) return nullptr;

        void* ptr = ::operator new(n * sizeof(T));
        if (!ptr) {
            throw std::bad_alloc();
        }
        return static_cast<pointer>(ptr);
    }

    /**
     * @brief Allocate memory for n objects (nothrow version)
     * @param n Number of objects to allocate
     * @return Pointer to allocated memory or nullptr if failed
     */
    pointer allocate(size_type n, const void* hint) {
        return allocate(n);
    }

    /**
     * @brief Deallocate memory for n objects
     * @param p Pointer to memory to deallocate
     * @param n Number of objects (ignored for single object deallocation)
     */
    void deallocate(pointer p, size_type n) noexcept {
        ::operator delete(p);
    }

    /**
     * @brief Get maximum number of objects that can be allocated
     * @return Maximum allocatable objects
     */
    size_type max_size() const noexcept {
        return std::numeric_limits<size_type>::max() / sizeof(value_type);
    }

    /**
     * @brief Construct object at given location
     * @tparam U Object type
     * @tparam Args Constructor arguments
     * @param p Location to construct object
     * @param args Constructor arguments
     */
    template<typename U, typename... Args>
    void construct(U* p, Args&&... args) {
        ::new(static_cast<void*>(p)) U(std::forward<Args>(args)...);
    }

    /**
     * @brief Destroy object at given location
     * @tparam U Object type
     * @param p Location of object to destroy
     */
    template<typename U>
    void destroy(U* p) {
        p->~U();
    }

    /**
     * @brief Check if allocator is stateless
     */
    bool operator==(const ThreadSafeAllocator&) const noexcept { return true; }
    bool operator!=(const ThreadSafeAllocator&) const noexcept { return false; }
};

/**
 * @brief Smart pointer for memory pool allocations
 */
template<typename T>
class PoolPointer {
public:
    PoolPointer() = default;

    explicit PoolPointer(MemoryPool& pool) : pool_(&pool) {}

    PoolPointer(MemoryPool& pool, AllocationResult<T>&& allocation)
        : pool_(&pool), data_(allocation.release()), size_(allocation.size()) {}

    PoolPointer(PoolPointer&& other) noexcept
        : pool_(other.pool_), data_(other.data_), size_(other.size_) {
        other.data_ = nullptr;
        other.size_ = 0;
    }

    PoolPointer& operator=(PoolPointer&& other) noexcept {
        if (this != &other) {
            reset();
            pool_ = other.pool_;
            data_ = other.data_;
            size_ = other.size_;
            other.data_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    ~PoolPointer() { reset(); }

    T* get() const { return static_cast<T*>(data_); }
    T* release() {
        T* ptr = get();
        data_ = nullptr;
        size_ = 0;
        return ptr;
    }

    size_t size() const { return size_; }
    explicit operator bool() const { return data_ != nullptr; }

    T& operator*() { return *get(); }
    T* operator->() { return get(); }
    T& operator[](size_t index) { return get()[index]; }

private:
    void reset() {
        if (data_ && pool_) {
            pool_->deallocateRaw(data_);
            data_ = nullptr;
            size_ = 0;
        }
    }

    MemoryPool* pool_ = nullptr;
    void* data_ = nullptr;
    size_t size_ = 0;
};

/**
 * @brief RAII wrapper for memory allocations with automatic cleanup
 */
template<typename T>
class ScopedAllocation {
public:
    ScopedAllocation() = default;

    explicit ScopedAllocation(AllocationResult<T>&& allocation)
        : data_(allocation.get()), size_(allocation.size()) {}

    ScopedAllocation(ScopedAllocation&& other) noexcept
        : data_(other.data_), size_(other.size_) {
        other.data_ = nullptr;
        other.size_ = 0;
    }

    ScopedAllocation& operator=(ScopedAllocation&& other) noexcept {
        if (this != &other) {
            reset();
            data_ = other.data_;
            size_ = other.size_;
            other.data_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    ~ScopedAllocation() { reset(); }

    T* get() const { return data_; }
    size_t size() const { return size_; }
    explicit operator bool() const { return data_ != nullptr; }

    T& operator*() { return *data_; }
    T* operator->() { return data_; }

private:
    void reset() {
        if (data_) {
            delete[] reinterpret_cast<std::byte*>(data_);
            data_ = nullptr;
            size_ = 0;
        }
    }

    T* data_ = nullptr;
    size_t size_ = 0;
};

} // namespace FoundryEngine

#endif // FOUNDRY_GAMEENGINE_MEMORY_POOL_H
