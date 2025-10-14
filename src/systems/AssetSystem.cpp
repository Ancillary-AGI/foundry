#include "../../include/GameEngine/systems/AssetSystem.h"
#include "../../include/GameEngine/core/SystemImpl.h"
#include <iostream>
#include <unordered_map>
#include <memory>
#include <filesystem>

namespace FoundryEngine {

class DefaultAssetManagerImpl : public SystemImplBase<DefaultAssetManagerImpl> {
private:
    std::unordered_map<std::string, std::unique_ptr<Asset>> loadedAssets_;
    std::filesystem::path assetRootPath_;
    size_t totalMemoryUsage_ = 0;

    friend class SystemImplBase<DefaultAssetManagerImpl>;

    bool onInitialize() override {
        assetRootPath_ = std::filesystem::current_path() / "assets";
        std::cout << "Default Asset Manager initialized with root path: " << assetRootPath_ << std::endl;
        return true;
    }

    void onShutdown() override {
        loadedAssets_.clear();
        totalMemoryUsage_ = 0;
        std::cout << "Default Asset Manager shutdown" << std::endl;
    }

    void onUpdate(float deltaTime) override {
        // Handle async asset loading/unloading
        // This would be implemented with proper async loading in a real system
    }

public:
    DefaultAssetManagerImpl() : SystemImplBase("DefaultAssetManager") {}

    Asset* loadAsset(const std::string& path, AssetType type) {
        auto it = loadedAssets_.find(path);
        if (it != loadedAssets_.end()) {
            return it->second.get();
        }

        // Create asset based on type
        std::unique_ptr<Asset> asset;
        switch (type) {
            case AssetType::Texture:
                asset = std::make_unique<TextureAsset>();
                break;
            case AssetType::Mesh:
                asset = std::make_unique<MeshAsset>();
                break;
            case AssetType::Audio:
                asset = std::make_unique<AudioAsset>();
                break;
            case AssetType::Script:
                asset = std::make_unique<ScriptAsset>();
                break;
            default:
                return nullptr;
        }

        if (asset->load((assetRootPath_ / path).string())) {
            Asset* assetPtr = asset.get();
            loadedAssets_[path] = std::move(asset);
            totalMemoryUsage_ += assetPtr->getMemoryUsage();
            return assetPtr;
        }

        return nullptr;
    }

    void unloadAsset(const std::string& path) {
        auto it = loadedAssets_.find(path);
        if (it != loadedAssets_.end()) {
            totalMemoryUsage_ -= it->second->getMemoryUsage();
            loadedAssets_.erase(it);
        }
    }

    Asset* getAsset(const std::string& path) const {
        auto it = loadedAssets_.find(path);
        return (it != loadedAssets_.end()) ? it->second.get() : nullptr;
    }

    std::string getStatistics() const override {
        char buffer[256];
        snprintf(buffer, sizeof(buffer),
                 "Assets: %zu loaded, Memory: %zu bytes",
                 loadedAssets_.size(), totalMemoryUsage_);
        return std::string(buffer);
    }
};

DefaultAssetManager::DefaultAssetManager() : impl_(std::make_unique<DefaultAssetManagerImpl>()) {}
DefaultAssetManager::~DefaultAssetManager() = default;

bool DefaultAssetManager::initialize() { return impl_->initialize(); }
void DefaultAssetManager::shutdown() { impl_->shutdown(); }
void DefaultAssetManager::update(float deltaTime) { impl_->update(deltaTime); }

Asset* DefaultAssetManager::loadAsset(const std::string& path, AssetType type) {
    return static_cast<DefaultAssetManagerImpl*>(impl_.get())->loadAsset(path, type);
}

void DefaultAssetManager::unloadAsset(const std::string& path) {
    static_cast<DefaultAssetManagerImpl*>(impl_.get())->unloadAsset(path);
}

Asset* DefaultAssetManager::getAsset(const std::string& path) const {
    return static_cast<DefaultAssetManagerImpl*>(impl_.get())->getAsset(path);
}

std::string DefaultAssetManager::getStatistics() const {
    return impl_->getStatistics();
}

} // namespace FoundryEngine
