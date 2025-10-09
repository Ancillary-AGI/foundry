#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <future>
#include <type_traits>
#include <chrono>
#include <atomic>
#include "../core/MemoryPool.h"

namespace FoundryEngine {

/**
 * @brief Asset types supported by the engine
 */
enum class AssetType {
    TEXTURE,
    MESH,
    MATERIAL,
    SHADER,
    AUDIO,
    SCRIPT,
    SCENE,
    PREFAB,
    ANIMATION,
    FONT,
    VIDEO,
    UNKNOWN
};

/**
 * @brief Asset metadata for tracking and management
 */
struct AssetMetadata {
    std::string guid;
    AssetType type;
    std::string sourcePath;
    size_t memoryUsage;
    uint64_t lastModified;
    std::chrono::steady_clock::time_point loadTime;
    std::vector<std::string> dependencies;
    std::unordered_map<std::string, std::string> customProperties;

    bool isValid() const { return !guid.empty() && !sourcePath.empty(); }
};

/**
 * @brief Base asset interface with type-safe data access
 */
class Asset {
public:
    virtual ~Asset() = default;

    /**
     * @brief Load asset from file path
     * @param path File path to load from
     * @return true if loading succeeded
     */
    virtual bool load(const std::string& path) = 0;

    /**
     * @brief Unload asset and free resources
     */
    virtual void unload() = 0;

    /**
     * @brief Check if asset is currently loaded
     * @return true if asset is loaded and ready for use
     */
    virtual bool isLoaded() const = 0;

    /**
     * @brief Get the asset type
     * @return Asset type enumeration
     */
    virtual AssetType getType() const = 0;

    /**
     * @brief Get the source file path
     * @return Source file path
     */
    virtual const std::string& getPath() const = 0;

    /**
     * @brief Get memory usage in bytes
     * @return Memory usage in bytes
     */
    virtual size_t getMemoryUsage() const = 0;

    /**
     * @brief Get last modification time
     * @return Last modification timestamp
     */
    virtual uint64_t getLastModified() const = 0;

    /**
     * @brief Get asset metadata
     * @return Reference to asset metadata
     */
    virtual const AssetMetadata& getMetadata() const = 0;

    /**
     * @brief Update asset metadata
     * @param metadata New metadata to set
     */
    virtual void updateMetadata(const AssetMetadata& metadata) = 0;

    /**
     * @brief Check if asset needs reloading (file changed)
     * @return true if asset should be reloaded
     */
    virtual bool needsReload() const = 0;

    /**
     * @brief Get asset dependencies
     * @return Vector of dependent asset GUIDs
     */
    virtual std::vector<std::string> getDependencies() const = 0;

    /**
     * @brief Validate asset integrity
     * @return true if asset is valid and usable
     */
    virtual bool validate() const = 0;
};

/**
 * @brief Type-safe asset wrapper with RAII support
 */
template<typename T>
class TypedAsset : public Asset {
public:
    /**
     * @brief Construct typed asset with memory pool
     * @param memoryPool Memory pool for allocations
     */
    explicit TypedAsset(MemoryPool& memoryPool) : memoryPool_(memoryPool) {}

    /**
     * @brief Get typed data pointer
     * @return Pointer to asset data
     */
    T* getData() {
        return isLoaded() ? data_.get() : nullptr;
    }

    /**
     * @brief Get typed data pointer (const)
     * @return Const pointer to asset data
     */
    const T* getData() const {
        return isLoaded() ? data_.get() : nullptr;
    }

    /**
     * @brief Get data with type safety check
     * @tparam U Target type to cast to
     * @return Pointer to cast data or nullptr if incompatible
     */
    template<typename U>
    U* getDataAs() {
        static_assert(std::is_base_of_v<T, U> || std::is_same_v<T, U>,
                     "Target type must be same as or base class of asset type");
        return static_cast<U*>(getData());
    }

    /**
     * @brief Check if data can be cast to target type
     * @tparam U Target type to check
     * @return true if cast is safe
     */
    template<typename U>
    bool canCastTo() const {
        return std::is_base_of_v<T, U> || std::is_same_v<T, U>;
    }

    // Asset interface implementation
    bool load(const std::string& path) override {
        // Implementation would load specific asset type
        // This is a base implementation - derived classes should override
        path_ = path;
        metadata_.sourcePath = path;
        metadata_.loadTime = std::chrono::steady_clock::now();

        // Allocate memory for asset data
        auto allocation = memoryPool_.allocateType<T>();
        if (!allocation) {
            return false;
        }

        data_ = std::unique_ptr<T>(allocation.release());
        loaded_ = true;

        return true;
    }

    void unload() override {
        data_.reset();
        loaded_ = false;
    }

    bool isLoaded() const override {
        return loaded_;
    }

    AssetType getType() const override {
        return metadata_.type;
    }

    const std::string& getPath() const override {
        return path_;
    }

    size_t getMemoryUsage() const override {
        return metadata_.memoryUsage;
    }

    uint64_t getLastModified() const override {
        return metadata_.lastModified;
    }

    const AssetMetadata& getMetadata() const override {
        return metadata_;
    }

    void updateMetadata(const AssetMetadata& metadata) override {
        metadata_ = metadata;
    }

    bool needsReload() const override {
        // Check if file modification time is newer than load time
        return false; // Implementation would check file system
    }

    std::vector<std::string> getDependencies() const override {
        return metadata_.dependencies;
    }

    bool validate() const override {
        return isLoaded() && data_ != nullptr && metadata_.isValid();
    }

protected:
    MemoryPool& memoryPool_;
    std::unique_ptr<T> data_;
    std::string path_;
    AssetMetadata metadata_;
    bool loaded_ = false;
};

class AssetLoader {
public:
    virtual ~AssetLoader() = default;
    virtual bool canLoad(const std::string& extension) const = 0;
    virtual std::unique_ptr<Asset> load(const std::string& path) = 0;
    virtual std::vector<std::string> getSupportedExtensions() const = 0;
};

class AssetManager {
public:
    virtual ~AssetManager() = default;
    
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update() = 0;
    
    // Asset loading
    virtual Asset* loadAsset(const std::string& path) = 0;
    virtual std::future<Asset*> loadAssetAsync(const std::string& path) = 0;
    virtual void unloadAsset(Asset* asset) = 0;
    virtual void unloadAsset(const std::string& path) = 0;
    
    // Asset retrieval
    virtual Asset* getAsset(const std::string& path) = 0;
    template<typename T>
    T* getAsset(const std::string& path) {
        Asset* asset = getAsset(path);
        return asset ? static_cast<TypedAsset<T>*>(asset)->getData() : nullptr;
    }
    
    // Asset management
    virtual bool isAssetLoaded(const std::string& path) const = 0;
    virtual std::vector<std::string> getLoadedAssets() const = 0;
    virtual void reloadAsset(const std::string& path) = 0;
    virtual void reloadAllAssets() = 0;
    
    // Asset directories
    virtual void addAssetDirectory(const std::string& directory) = 0;
    virtual void removeAssetDirectory(const std::string& directory) = 0;
    virtual std::vector<std::string> getAssetDirectories() const = 0;
    
    // Asset loaders
    virtual void registerLoader(std::unique_ptr<AssetLoader> loader) = 0;
    virtual AssetLoader* getLoader(const std::string& extension) = 0;
    
    // Asset streaming
    virtual void enableStreaming(bool enable) = 0;
    virtual bool isStreamingEnabled() const = 0;
    virtual void setStreamingDistance(float distance) = 0;
    virtual float getStreamingDistance() const = 0;
    
    // Memory management
    virtual size_t getTotalMemoryUsage() const = 0;
    virtual size_t getMemoryUsage(AssetType type) const = 0;
    virtual void setMemoryBudget(AssetType type, size_t budget) = 0;
    virtual size_t getMemoryBudget(AssetType type) const = 0;
    virtual void garbageCollect() = 0;
    
    // Hot reloading
    virtual void enableHotReload(bool enable) = 0;
    virtual bool isHotReloadEnabled() const = 0;
    virtual void watchDirectory(const std::string& directory) = 0;
    virtual void unwatchDirectory(const std::string& directory) = 0;
    
    // Asset database
    virtual void buildAssetDatabase() = 0;
    virtual std::vector<std::string> findAssets(const std::string& pattern) const = 0;
    virtual std::vector<std::string> getAssetsByType(AssetType type) const = 0;
    virtual AssetType getAssetType(const std::string& path) const = 0;
    
    // Callbacks
    virtual void setAssetLoadedCallback(std::function<void(Asset*)> callback) = 0;
    virtual void setAssetUnloadedCallback(std::function<void(const std::string&)> callback) = 0;
    virtual void setAssetReloadedCallback(std::function<void(Asset*)> callback) = 0;
};

// Specific asset loaders
class TextureLoader : public AssetLoader {
public:
    bool canLoad(const std::string& extension) const override;
    std::unique_ptr<Asset> load(const std::string& path) override;
    std::vector<std::string> getSupportedExtensions() const override;
};

class MeshLoader : public AssetLoader {
public:
    bool canLoad(const std::string& extension) const override;
    std::unique_ptr<Asset> load(const std::string& path) override;
    std::vector<std::string> getSupportedExtensions() const override;
};

class AudioLoader : public AssetLoader {
public:
    bool canLoad(const std::string& extension) const override;
    std::unique_ptr<Asset> load(const std::string& path) override;
    std::vector<std::string> getSupportedExtensions() const override;
};

class ShaderLoader : public AssetLoader {
public:
    bool canLoad(const std::string& extension) const override;
    std::unique_ptr<Asset> load(const std::string& path) override;
    std::vector<std::string> getSupportedExtensions() const override;
};

class ScriptLoader : public AssetLoader {
public:
    bool canLoad(const std::string& extension) const override;
    std::unique_ptr<Asset> load(const std::string& path) override;
    std::vector<std::string> getSupportedExtensions() const override;
};

class DefaultAssetManager : public AssetManager {
public:
    bool initialize() override;
    void shutdown() override;
    void update() override;
    Asset* loadAsset(const std::string& path) override;
    std::future<Asset*> loadAssetAsync(const std::string& path) override;
    void unloadAsset(Asset* asset) override;
    void unloadAsset(const std::string& path) override;
    Asset* getAsset(const std::string& path) override;
    bool isAssetLoaded(const std::string& path) const override;
    std::vector<std::string> getLoadedAssets() const override;
    void reloadAsset(const std::string& path) override;
    void reloadAllAssets() override;
    void addAssetDirectory(const std::string& directory) override;
    void removeAssetDirectory(const std::string& directory) override;
    std::vector<std::string> getAssetDirectories() const override;
    void registerLoader(std::unique_ptr<AssetLoader> loader) override;
    AssetLoader* getLoader(const std::string& extension) override;
    void enableStreaming(bool enable) override;
    bool isStreamingEnabled() const override;
    void setStreamingDistance(float distance) override;
    float getStreamingDistance() const override;
    size_t getTotalMemoryUsage() const override;
    size_t getMemoryUsage(AssetType type) const override;
    void setMemoryBudget(AssetType type, size_t budget) override;
    size_t getMemoryBudget(AssetType type) const override;
    void garbageCollect() override;
    void enableHotReload(bool enable) override;
    bool isHotReloadEnabled() const override;
    void watchDirectory(const std::string& directory) override;
    void unwatchDirectory(const std::string& directory) override;
    void buildAssetDatabase() override;
    std::vector<std::string> findAssets(const std::string& pattern) const override;
    std::vector<std::string> getAssetsByType(AssetType type) const override;
    AssetType getAssetType(const std::string& path) const override;
    void setAssetLoadedCallback(std::function<void(Asset*)> callback) override;
    void setAssetUnloadedCallback(std::function<void(const std::string&)> callback) override;
    void setAssetReloadedCallback(std::function<void(Asset*)> callback) override;
};

} // namespace FoundryEngine
