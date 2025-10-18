#pragma once

#include "../core/System.h"
#include "../math/Vector3.h"
#include "../math/Vector2.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <ctime>

namespace FoundryEngine {

// Forward declarations
struct ImageData;
struct MeshData;
struct AudioData;
struct Vertex;
struct BoundingBox;

// Asset processing settings
struct AssetProcessingSettings {
    std::string format;
    bool generateMipmaps = true;
    bool compressTextures = true;
    bool optimizeMeshes = true;
    bool convertAudio = true;
    bool generateLODs = true;
    int maxLODLevels = 4;
    int compressionQuality = 85;
    int maxTextureSize = 2048;
};

// Asset processing job
struct AssetProcessingJob {
    std::string assetId;
    AssetProcessingSettings settings;
    std::function<void(bool)> callback;
};

// Asset structure
struct Asset {
    std::string guid;
    std::string type; // texture, mesh, audio, script, etc.
    std::string sourcePath;
    std::vector<std::string> dependencies;
    std::unordered_map<std::string, std::string> metadata;
    void* runtimeData = nullptr;
    bool processed = false;
    std::time_t lastProcessed = 0;
    size_t memoryUsage = 0;
};

// Image data structure
struct ImageData {
    int width = 0;
    int height = 0;
    int channels = 0;
    std::string format;
    std::vector<uint8_t> data;
    std::vector<ImageData> mipmaps;
    bool compressed = false;
    float compressionRatio = 1.0f;
};

// Mesh data structure
struct Vertex {
    Vector3 position;
    Vector3 normal;
    Vector2 texCoord;
    
    bool operator==(const Vertex& other) const {
        return position == other.position && normal == other.normal && texCoord == other.texCoord;
    }
};

struct BoundingBox {
    Vector3 min;
    Vector3 max;
    Vector3 center;
    Vector3 size;
};

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<MeshData> lods;
    BoundingBox boundingBox;
    std::string material;
};

// Audio data structure
struct AudioData {
    int sampleRate = 0;
    int channels = 0;
    float duration = 0.0f;
    std::string format;
    std::vector<float> data;
    bool compressed = false;
    float compressionRatio = 1.0f;
};

// Texture atlas structure
struct TextureAtlas {
    int width = 0;
    int height = 0;
    std::vector<std::pair<Vector2, Vector2>> regions; // uv_min, uv_max
    std::vector<std::string> packedTextures;
    
    void packTextures(const std::vector<std::pair<int, int>>& textureSizes,
                     std::function<bool(int, Vector2, Vector2)> placementCallback);
};

// Asset Pipeline System
class AssetPipeline : public System {
public:
    AssetPipeline();
    ~AssetPipeline();
    
    bool initialize() override;
    void shutdown() override;
    void update(float deltaTime) override;
    
    // Asset optimization
    void optimizeTexture(const std::string& assetId, const std::string& format);
    void generateLODs(const std::string& meshAssetId, int maxLOD = 4);
    
    // Texture atlas packing
    void packTextures(const std::vector<std::pair<int, int>>& textureSizes,
                     std::function<bool(int, Vector2, Vector2)> placementCallback);
    
    // Asset management
    void addAsset(const Asset& asset);
    void removeAsset(const std::string& assetId);
    Asset* getAsset(const std::string& assetId);
    
    // Processing
    void processAsset(const std::string& assetId, const AssetProcessingSettings& settings);
    bool isAssetProcessed(const std::string& assetId);
    
    // Statistics
    std::string getStatistics() const override;
    
    // Texture atlas access
    const TextureAtlas& getTextureAtlas() const { return textureAtlas_; }

private:
    class AssetPipelineImpl;
    std::unique_ptr<AssetPipelineImpl> impl_;
    
    TextureAtlas textureAtlas_;
};

} // namespace FoundryEngine
