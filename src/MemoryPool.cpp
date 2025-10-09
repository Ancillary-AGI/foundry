#include "GameEngine/core/MemoryPool.h"
#include <algorithm>
#include <cstring>
#include <iostream>

namespace FoundryEngine {

// MemoryPool Implementation
MemoryPool::MemoryPool(size_t blockSize, size_t poolSize)
    : blockSize_(blockSize) {
    // Calculate number of blocks needed
    size_t numBlocks = (poolSize + blockSize - 1) / blockSize; // Ceiling division

    // Allocate initial pool
    poolBlocks_.reserve(numBlocks);
    for (size_t i = 0; i < numBlocks; ++i) {
        expandPool();
    }
}

MemoryPool::~MemoryPool() {
    cleanup();
}

MemoryPool::MemoryPool(MemoryPool&& other) noexcept
    : blockSize_(other.blockSize_),
      poolBlocks_(std::move(other.poolBlocks_)),
      freeList_(other.freeList_),
      allocationSize_(std::move(other.allocationSize_)) {
    other.freeList_ = nullptr;
}

MemoryPool& MemoryPool::operator=(MemoryPool&& other) noexcept {
    if (this != &other) {
        cleanup();

        blockSize_ = other.blockSize_;
        poolBlocks_ = std::move(other.poolBlocks_);
        freeList_ = other.freeList_;
        allocationSize_ = std::move(other.allocationSize_);

        other.freeList_ = nullptr;
    }
    return *this;
}

void* MemoryPool::allocateRaw(size_t size) {
    if (size == 0) return nullptr;

    std::lock_guard<std::mutex> lock(mutex_);

    // Find suitable free block
    Block* block = findFreeBlock(size);
    if (!block) {
        // Try to expand pool
        expandPool();
        block = findFreeBlock(size);
        if (!block) {
            return nullptr; // Out of memory
        }
    }

    // Check if we need to split the block
    if (block->size > size + sizeof(Block) + 16) { // +16 for alignment padding
        // Split block
        Block* newBlock = reinterpret_cast<Block*>(
            reinterpret_cast<std::byte*>(block->start) + size);

        newBlock->start = block->start + size;
        newBlock->size = block->size - size;
        newBlock->free = true;
        newBlock->next = block->next;
        newBlock->prev = block;
        newBlock->allocationId = 0;

        if (block->next) {
            block->next->prev = newBlock;
        }
        block->next = newBlock;
        block->size = size;
    }

    block->free = false;
    block->allocationId = nextAllocationId_.fetch_add(1);

    void* userPtr = block->start;
    allocationSize_[userPtr] = size;
    totalAllocated_.fetch_add(size);

    return userPtr;
}

void MemoryPool::deallocateRaw(void* ptr) {
    if (!ptr) return;

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = allocationSize_.find(ptr);
    if (it == allocationSize_.end()) {
        // Invalid pointer or already freed
        return;
    }

    size_t size = it->second;
    totalAllocated_.fetch_sub(size);
    allocationSize_.erase(it);

    // Find the block and mark as free
    Block* block = freeList_;
    while (block) {
        if (block->start == ptr && !block->free) {
            block->free = true;
            mergeFreeBlocks();
            return;
        }
        block = block->next;
    }
}

void MemoryPool::defragment() {
    std::lock_guard<std::mutex> lock(mutex_);

    // Simple defragmentation: collect all free blocks and merge them
    mergeFreeBlocks();

    // If we still have significant fragmentation, consider more advanced strategies
    // For now, we'll just ensure free blocks are properly merged
}

size_t MemoryPool::totalAllocated() const {
    return totalAllocated_.load();
}

size_t MemoryPool::totalFree() const {
    std::lock_guard<std::mutex> lock(mutex_);

    size_t freeBytes = 0;
    Block* block = freeList_;
    while (block) {
        if (block->free) {
            freeBytes += block->size;
        }
        block = block->next;
    }
    return freeBytes;
}

float MemoryPool::utilization() const {
    size_t total = totalAllocated() + totalFree();
    if (total == 0) return 0.0f;
    return (static_cast<float>(totalAllocated()) / static_cast<float>(total)) * 100.0f;
}

float MemoryPool::fragmentationRatio() const {
    std::lock_guard<std::mutex> lock(mutex_);

    size_t totalFreeBytes = 0;
    size_t largestFreeBlock = 0;
    int freeBlockCount = 0;

    Block* block = freeList_;
    while (block) {
        if (block->free) {
            totalFreeBytes += block->size;
            largestFreeBlock = std::max(largestFreeBlock, block->size);
            freeBlockCount++;
        }
        block = block->next;
    }

    if (totalFreeBytes == 0 || freeBlockCount <= 1) {
        return 0.0f; // No fragmentation if no free memory or only one free block
    }

    // Fragmentation ratio based on largest free block vs total free memory
    return 1.0f - (static_cast<float>(largestFreeBlock) / static_cast<float>(totalFreeBytes));
}

void MemoryPool::expandPool() {
    // Allocate new block
    auto newBlockMemory = std::make_unique<std::byte[]>(blockSize_);

    // Create block header at the beginning
    Block* newBlock = new (newBlockMemory.get()) Block{
        newBlockMemory.get(),
        blockSize_,
        true, // free
        nullptr,
        nullptr,
        0 // allocationId
    };

    // Add to pool blocks list (without the Block header size)
    poolBlocks_.push_back(std::unique_ptr<std::byte[]>(
        newBlockMemory.release() + sizeof(Block)));

    // Insert into free list (sorted by address for better cache locality)
    if (!freeList_ || newBlock < freeList_) {
        newBlock->next = freeList_;
        if (freeList_) {
            freeList_->prev = newBlock;
        }
        freeList_ = newBlock;
    } else {
        Block* current = freeList_;
        while (current->next && current->next < newBlock) {
            current = current->next;
        }
        newBlock->next = current->next;
        newBlock->prev = current;
        if (current->next) {
            current->next->prev = newBlock;
        }
        current->next = newBlock;
    }
}

MemoryPool::Block* MemoryPool::findFreeBlock(size_t size) {
    Block* block = freeList_;
    while (block) {
        if (block->free && block->size >= size) {
            return block;
        }
        block = block->next;
    }
    return nullptr;
}

void MemoryPool::mergeFreeBlocks() {
    Block* block = freeList_;
    while (block && block->next) {
        if (block->free && block->next->free) {
            // Merge adjacent free blocks
            block->size += block->next->size;

            Block* nextNext = block->next->next;
            if (nextNext) {
                nextNext->prev = block;
            }
            block->next = nextNext;
        } else {
            block = block->next;
        }
    }
}

void MemoryPool::cleanup() {
    // Clear allocation tracking
    allocationSize_.clear();
    totalAllocated_.store(0);

    // Note: poolBlocks_ will be automatically cleaned up by unique_ptr
    // Block structures are allocated within the pool memory, so they'll be freed too
}

} // namespace FoundryEngine
