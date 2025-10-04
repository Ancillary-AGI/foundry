#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <future>

namespace FoundryEngine {

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

class Asset {
public:
    virtual ~Asset() = default;
    virtual bool load(const std::string& path) = 0;
    virtual void unload() = 0;
    virtual bool isLoaded() const = 0;
    virtual AssetType getType() const = 0;
    virtual const std::string& getPath() const = 0;
    virtual size_t getMemoryUsage() const = 0;
    virtual uint64_t getLastModified() const = 0;
};

template<typename T>
class TypedAsset : public Asset {
public:
    T* getData() { return data_.get(); }
    const T* getData() const { return data_.get(); }
    
protected:
    std::unique_ptr<T> data_;
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