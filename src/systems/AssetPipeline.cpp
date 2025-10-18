#include "../../include/GameEngine/systems/AssetPipeline.h"
#include "../../include/GameEngine/core/SystemImpl.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <thread>
#include <future>
#include <unordered_set>

namespace FoundryEngine {

class AssetPipelineImpl : public SystemImplBase<AssetPipelineImpl> {
private:
    std::unordered_map<std::string, Asset> assetDatabase_;
    std::unordered_map<std::string, std::vector<std::string>> referenceGraph_;
    std::unordered_map<std::string, std::string> assetHashes_;
    std::vector<std::thread> processingThreads_;
    std::atomic<bool> processingActive_{false};
    std::queue<AssetProcessingJob> processingQueue_;
    std::mutex queueMutex_;
    std::condition_variable queueCondition_;

    // Asset optimization settings
    struct OptimizationSettings {
        bool generateMipmaps = true;
        bool compressTextures = true;
        bool optimizeMeshes = true;
        bool convertAudio = true;
        int maxTextureSize = 2048;
        int compressionQuality = 85;
        bool generateLODs = true;
        int maxLODLevels = 4;
    } optimizationSettings_;

    friend class SystemImplBase<AssetPipelineImpl>;

    bool onInitialize() override {
        std::cout << "Asset Pipeline initialized" << std::endl;
        
        // Start processing threads
        processingActive_ = true;
        for (int i = 0; i < std::thread::hardware_concurrency(); ++i) {
            processingThreads_.emplace_back(&AssetPipelineImpl::processingWorker, this);
        }
        
        return true;
    }

    void onShutdown() override {
        // Stop processing threads
        processingActive_ = false;
        queueCondition_.notify_all();
        
        for (auto& thread : processingThreads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        processingThreads_.clear();
        
        assetDatabase_.clear();
        referenceGraph_.clear();
        assetHashes_.clear();
        
        std::cout << "Asset Pipeline shutdown" << std::endl;
    }

    void onUpdate(float deltaTime) override {
        // Process completed assets and update reference tracking
        processCompletedAssets();
    }

    void processingWorker() {
        while (processingActive_) {
            std::unique_lock<std::mutex> lock(queueMutex_);
            queueCondition_.wait(lock, [this] { return !processingQueue_.empty() || !processingActive_; });
            
            if (!processingActive_) break;
            
            if (!processingQueue_.empty()) {
                AssetProcessingJob job = processingQueue_.front();
                processingQueue_.pop();
                lock.unlock();
                
                processAsset(job);
            }
        }
    }

    void processAsset(const AssetProcessingJob& job) {
        try {
            Asset& asset = assetDatabase_[job.assetId];
            
            switch (asset.type) {
                case "texture":
                    processTexture(asset, job.settings);
                    break;
                case "mesh":
                    processMesh(asset, job.settings);
                    break;
                case "audio":
                    processAudio(asset, job.settings);
                    break;
                case "script":
                    processScript(asset, job.settings);
                    break;
                default:
                    processGeneric(asset, job.settings);
                    break;
            }
            
            asset.processed = true;
            asset.lastProcessed = std::time(nullptr);
            
        } catch (const std::exception& e) {
            std::cerr << "Error processing asset " << job.assetId << ": " << e.what() << std::endl;
        }
    }

    void processTexture(Asset& asset, const AssetProcessingSettings& settings) {
        std::cout << "Processing texture: " << asset.guid << std::endl;
        
        // Load texture data
        std::ifstream file(asset.sourcePath, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open texture file: " + asset.sourcePath);
        }
        
        // Get file size
        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);
        
        // Read file data
        std::vector<uint8_t> fileData(fileSize);
        file.read(reinterpret_cast<char*>(fileData.data()), fileSize);
        file.close();
        
        // Parse image format (simplified - would use actual image library)
        ImageData imageData = parseImageData(fileData);
        
        // Generate mipmaps if requested
        if (settings.generateMipmaps && optimizationSettings_.generateMipmaps) {
            generateMipmaps(imageData);
        }
        
        // Compress texture if requested
        if (settings.compressTextures && optimizationSettings_.compressTextures) {
            compressTexture(imageData, optimizationSettings_.compressionQuality);
        }
        
        // Resize if too large
        if (imageData.width > optimizationSettings_.maxTextureSize || 
            imageData.height > optimizationSettings_.maxTextureSize) {
            resizeTexture(imageData, optimizationSettings_.maxTextureSize);
        }
        
        // Store processed data
        asset.runtimeData = new ImageData(std::move(imageData));
        asset.metadata["width"] = std::to_string(imageData.width);
        asset.metadata["height"] = std::to_string(imageData.height);
        asset.metadata["channels"] = std::to_string(imageData.channels);
        asset.metadata["format"] = imageData.format;
    }

    void processMesh(Asset& asset, const AssetProcessingSettings& settings) {
        std::cout << "Processing mesh: " << asset.guid << std::endl;
        
        // Load mesh data
        std::ifstream file(asset.sourcePath, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open mesh file: " + asset.sourcePath);
        }
        
        // Parse mesh format (simplified - would use actual mesh library)
        MeshData meshData = parseMeshData(file);
        file.close();
        
        // Optimize mesh if requested
        if (settings.optimizeMeshes && optimizationSettings_.optimizeMeshes) {
            optimizeMesh(meshData);
        }
        
        // Generate LODs if requested
        if (settings.generateLODs && optimizationSettings_.generateLODs) {
            generateLODs(meshData, optimizationSettings_.maxLODLevels);
        }
        
        // Calculate bounding box
        calculateBoundingBox(meshData);
        
        // Store processed data
        asset.runtimeData = new MeshData(std::move(meshData));
        asset.metadata["vertexCount"] = std::to_string(meshData.vertices.size());
        asset.metadata["indexCount"] = std::to_string(meshData.indices.size());
        asset.metadata["lodCount"] = std::to_string(meshData.lods.size());
    }

    void processAudio(Asset& asset, const AssetProcessingSettings& settings) {
        std::cout << "Processing audio: " << asset.guid << std::endl;
        
        // Load audio data
        std::ifstream file(asset.sourcePath, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open audio file: " + asset.sourcePath);
        }
        
        // Parse audio format (simplified - would use actual audio library)
        AudioData audioData = parseAudioData(file);
        file.close();
        
        // Convert audio if requested
        if (settings.convertAudio && optimizationSettings_.convertAudio) {
            convertAudio(audioData);
        }
        
        // Compress audio
        compressAudio(audioData);
        
        // Store processed data
        asset.runtimeData = new AudioData(std::move(audioData));
        asset.metadata["duration"] = std::to_string(audioData.duration);
        asset.metadata["sampleRate"] = std::to_string(audioData.sampleRate);
        asset.metadata["channels"] = std::to_string(audioData.channels);
        asset.metadata["format"] = audioData.format;
    }

    void processScript(Asset& asset, const AssetProcessingSettings& settings) {
        std::cout << "Processing script: " << asset.guid << std::endl;
        
        // Load script data
        std::ifstream file(asset.sourcePath);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open script file: " + asset.sourcePath);
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string scriptContent = buffer.str();
        file.close();
        
        // Validate script syntax
        validateScript(scriptContent);
        
        // Store processed data
        asset.runtimeData = new std::string(std::move(scriptContent));
        asset.metadata["lineCount"] = std::to_string(std::count(scriptContent.begin(), scriptContent.end(), '\n') + 1);
        asset.metadata["size"] = std::to_string(scriptContent.size());
    }

    void processGeneric(Asset& asset, const AssetProcessingSettings& settings) {
        std::cout << "Processing generic asset: " << asset.guid << std::endl;
        
        // Load file data
        std::ifstream file(asset.sourcePath, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + asset.sourcePath);
        }
        
        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);
        
        std::vector<uint8_t> fileData(fileSize);
        file.read(reinterpret_cast<char*>(fileData.data()), fileSize);
        file.close();
        
        // Store raw data
        asset.runtimeData = new std::vector<uint8_t>(std::move(fileData));
        asset.metadata["size"] = std::to_string(fileSize);
    }

    void generateMipmaps(ImageData& imageData) {
        // Generate mipmap levels
        int currentWidth = imageData.width;
        int currentHeight = imageData.height;
        
        while (currentWidth > 1 || currentHeight > 1) {
            currentWidth = std::max(1, currentWidth / 2);
            currentHeight = std::max(1, currentHeight / 2);
            
            // Create downsampled version (simplified)
            ImageData mipLevel;
            mipLevel.width = currentWidth;
            mipLevel.height = currentHeight;
            mipLevel.channels = imageData.channels;
            mipLevel.format = imageData.format;
            mipLevel.data.resize(currentWidth * currentHeight * imageData.channels);
            
            // Downsample (simplified - would use proper filtering)
            for (int y = 0; y < currentHeight; ++y) {
                for (int x = 0; x < currentWidth; ++x) {
                    for (int c = 0; c < imageData.channels; ++c) {
                        int srcX = x * 2;
                        int srcY = y * 2;
                        int srcIndex = (srcY * imageData.width + srcX) * imageData.channels + c;
                        int dstIndex = (y * currentWidth + x) * imageData.channels + c;
                        
                        if (srcIndex < imageData.data.size()) {
                            mipLevel.data[dstIndex] = imageData.data[srcIndex];
                        }
                    }
                }
            }
            
            imageData.mipmaps.push_back(std::move(mipLevel));
        }
    }

    void compressTexture(ImageData& imageData, int quality) {
        // Compress texture data (simplified - would use actual compression)
        std::vector<uint8_t> compressedData;
        
        // Simple compression simulation
        compressedData.reserve(imageData.data.size() / 2);
        
        for (size_t i = 0; i < imageData.data.size(); i += 2) {
            uint8_t avg = (imageData.data[i] + imageData.data[i + 1]) / 2;
            compressedData.push_back(avg);
        }
        
        imageData.data = std::move(compressedData);
        imageData.compressed = true;
        imageData.compressionRatio = 2.0f;
    }

    void resizeTexture(ImageData& imageData, int maxSize) {
        // Resize texture to fit within maxSize (simplified)
        if (imageData.width > maxSize || imageData.height > maxSize) {
            float scale = std::min(
                static_cast<float>(maxSize) / imageData.width,
                static_cast<float>(maxSize) / imageData.height
            );
            
            int newWidth = static_cast<int>(imageData.width * scale);
            int newHeight = static_cast<int>(imageData.height * scale);
            
            std::vector<uint8_t> resizedData(newWidth * newHeight * imageData.channels);
            
            // Simple resize (would use proper filtering in real implementation)
            for (int y = 0; y < newHeight; ++y) {
                for (int x = 0; x < newWidth; ++x) {
                    int srcX = static_cast<int>(x / scale);
                    int srcY = static_cast<int>(y / scale);
                    
                    for (int c = 0; c < imageData.channels; ++c) {
                        int srcIndex = (srcY * imageData.width + srcX) * imageData.channels + c;
                        int dstIndex = (y * newWidth + x) * imageData.channels + c;
                        
                        if (srcIndex < imageData.data.size()) {
                            resizedData[dstIndex] = imageData.data[srcIndex];
                        }
                    }
                }
            }
            
            imageData.width = newWidth;
            imageData.height = newHeight;
            imageData.data = std::move(resizedData);
        }
    }

    void optimizeMesh(MeshData& meshData) {
        // Optimize mesh (simplified - would use actual mesh optimization)
        
        // Remove duplicate vertices
        std::unordered_map<Vertex, uint32_t> vertexMap;
        std::vector<Vertex> uniqueVertices;
        std::vector<uint32_t> newIndices;
        
        for (uint32_t index : meshData.indices) {
            Vertex vertex = meshData.vertices[index];
            
            auto it = vertexMap.find(vertex);
            if (it == vertexMap.end()) {
                uint32_t newIndex = static_cast<uint32_t>(uniqueVertices.size());
                vertexMap[vertex] = newIndex;
                uniqueVertices.push_back(vertex);
                newIndices.push_back(newIndex);
            } else {
                newIndices.push_back(it->second);
            }
        }
        
        meshData.vertices = std::move(uniqueVertices);
        meshData.indices = std::move(newIndices);
    }

    void generateLODs(MeshData& meshData, int maxLODs) {
        // Generate LOD levels (simplified - would use actual LOD generation)
        for (int lod = 1; lod < maxLODs; ++lod) {
            MeshData lodMesh = meshData;
            
            // Reduce triangle count (simplified)
            int reductionFactor = 1 << lod; // 2, 4, 8, etc.
            size_t newIndexCount = meshData.indices.size() / reductionFactor;
            
            lodMesh.indices.resize(newIndexCount);
            for (size_t i = 0; i < newIndexCount; ++i) {
                lodMesh.indices[i] = meshData.indices[i * reductionFactor];
            }
            
            meshData.lods.push_back(std::move(lodMesh));
        }
    }

    void calculateBoundingBox(MeshData& meshData) {
        if (meshData.vertices.empty()) return;
        
        Vector3 min = meshData.vertices[0].position;
        Vector3 max = meshData.vertices[0].position;
        
        for (const auto& vertex : meshData.vertices) {
            min.x = std::min(min.x, vertex.position.x);
            min.y = std::min(min.y, vertex.position.y);
            min.z = std::min(min.z, vertex.position.z);
            
            max.x = std::max(max.x, vertex.position.x);
            max.y = std::max(max.y, vertex.position.y);
            max.z = std::max(max.z, vertex.position.z);
        }
        
        meshData.boundingBox.min = min;
        meshData.boundingBox.max = max;
        meshData.boundingBox.center = (min + max) * 0.5f;
        meshData.boundingBox.size = max - min;
    }

    void convertAudio(AudioData& audioData) {
        // Convert audio to target format (simplified)
        if (audioData.sampleRate > 44100) {
            // Downsample to 44.1kHz
            float ratio = 44100.0f / audioData.sampleRate;
            size_t newSize = static_cast<size_t>(audioData.data.size() * ratio);
            
            std::vector<float> newData(newSize);
            for (size_t i = 0; i < newSize; ++i) {
                size_t srcIndex = static_cast<size_t>(i / ratio);
                if (srcIndex < audioData.data.size()) {
                    newData[i] = audioData.data[srcIndex];
                }
            }
            
            audioData.data = std::move(newData);
            audioData.sampleRate = 44100;
        }
    }

    void compressAudio(AudioData& audioData) {
        // Compress audio data (simplified - would use actual audio compression)
        std::vector<uint8_t> compressedData;
        compressedData.reserve(audioData.data.size() / 4);
        
        // Simple compression simulation
        for (size_t i = 0; i < audioData.data.size(); i += 4) {
            float avg = 0.0f;
            for (int j = 0; j < 4 && i + j < audioData.data.size(); ++j) {
                avg += audioData.data[i + j];
            }
            avg /= 4.0f;
            
            int16_t sample = static_cast<int16_t>(avg * 32767.0f);
            compressedData.push_back(sample & 0xFF);
            compressedData.push_back((sample >> 8) & 0xFF);
        }
        
        audioData.data.clear();
        audioData.compressed = true;
        audioData.compressionRatio = 4.0f;
    }

    void validateScript(const std::string& scriptContent) {
        // Basic script validation (simplified)
        if (scriptContent.empty()) {
            throw std::runtime_error("Script is empty");
        }
        
        // Check for basic syntax (would use actual parser in real implementation)
        int braceCount = 0;
        int parenCount = 0;
        
        for (char c : scriptContent) {
            if (c == '{') braceCount++;
            else if (c == '}') braceCount--;
            else if (c == '(') parenCount++;
            else if (c == ')') parenCount--;
        }
        
        if (braceCount != 0) {
            throw std::runtime_error("Unmatched braces in script");
        }
        
        if (parenCount != 0) {
            throw std::runtime_error("Unmatched parentheses in script");
        }
    }

    void processCompletedAssets() {
        // Process any completed asset processing jobs
        // This would handle callbacks and update asset states
    }

    // Helper functions for parsing different asset formats
    ImageData parseImageData(const std::vector<uint8_t>& data) {
        ImageData imageData;
        
        // Use stb_image to parse image data
        int width, height, channels;
        unsigned char* image = stbi_load_from_memory(data.data(), data.size(), &width, &height, &channels, 0);
        
        if (image) {
            imageData.width = width;
            imageData.height = height;
            imageData.channels = channels;
            imageData.format = channels == 3 ? "RGB8" : "RGBA8";
            imageData.data.assign(image, image + width * height * channels);
            stbi_image_free(image);
        } else {
            // Fallback for invalid image data
            imageData.width = 0;
            imageData.height = 0;
            imageData.channels = 0;
            imageData.format = "UNKNOWN";
        }
        
        return imageData;
    }

    MeshData parseMeshData(std::ifstream& file) {
        MeshData meshData;
        
        // Use tinyobjloader to parse OBJ files
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;
        
        // Read file content
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, 
                                   content.c_str(), nullptr, true);
        
        if (!warn.empty()) {
            std::cout << "TinyObjLoader Warning: " << warn << std::endl;
        }
        if (!err.empty()) {
            std::cerr << "TinyObjLoader Error: " << err << std::endl;
        }
        
        if (ret) {
            // Process vertices
            for (size_t i = 0; i < attrib.vertices.size(); i += 3) {
                meshData.vertices.push_back({
                    attrib.vertices[i],
                    attrib.vertices[i + 1],
                    attrib.vertices[i + 2]
                });
            }
            
            // Process normals
            for (size_t i = 0; i < attrib.normals.size(); i += 3) {
                meshData.normals.push_back({
                    attrib.normals[i],
                    attrib.normals[i + 1],
                    attrib.normals[i + 2]
                });
            }
            
            // Process texture coordinates
            for (size_t i = 0; i < attrib.texcoords.size(); i += 2) {
                meshData.texCoords.push_back({
                    attrib.texcoords[i],
                    attrib.texcoords[i + 1]
                });
            }
            
            // Process indices
            for (const auto& shape : shapes) {
                for (const auto& index : shape.mesh.indices) {
                    meshData.indices.push_back(index.vertex_index);
                }
            }
        }
        
        return meshData;
    }

    AudioData parseAudioData(std::ifstream& file) {
        AudioData audioData;
        
        // Read file content
        std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)),
                                 std::istreambuf_iterator<char>());
        
        // Basic audio format detection and parsing
        if (data.size() >= 4) {
            // Check for WAV format
            if (data[0] == 'R' && data[1] == 'I' && data[2] == 'F' && data[3] == 'F') {
                audioData.format = "WAV";
                // Parse WAV header (simplified)
                if (data.size() >= 44) {
                    audioData.sampleRate = *reinterpret_cast<uint32_t*>(&data[24]);
                    audioData.channels = *reinterpret_cast<uint16_t*>(&data[22]);
                    audioData.bitsPerSample = *reinterpret_cast<uint16_t*>(&data[34]);
                    audioData.data = std::vector<uint8_t>(data.begin() + 44, data.end());
                }
            }
            // Check for OGG format
            else if (data[0] == 'O' && data[1] == 'g' && data[2] == 'g' && data[3] == 'S') {
                audioData.format = "OGG";
                audioData.data = data;
                // OGG parsing would require libvorbis
            }
            // Default to raw audio data
            else {
                audioData.format = "RAW";
                audioData.data = data;
                audioData.sampleRate = 44100; // Default
                audioData.channels = 2; // Default stereo
                audioData.bitsPerSample = 16; // Default
            }
        }
        
        return audioData;
    }

public:
    AssetPipelineImpl() : SystemImplBase("AssetPipeline") {}

    void optimizeTexture(const std::string& assetId, const std::string& format) {
        auto it = assetDatabase_.find(assetId);
        if (it != assetDatabase_.end()) {
            AssetProcessingJob job;
            job.assetId = assetId;
            job.settings.format = format;
            job.settings.compressTextures = true;
            job.settings.generateMipmaps = true;
            
            std::lock_guard<std::mutex> lock(queueMutex_);
            processingQueue_.push(job);
            queueCondition_.notify_one();
        }
    }

    void generateLODs(const std::string& meshAssetId, int maxLOD = 4) {
        auto it = assetDatabase_.find(meshAssetId);
        if (it != assetDatabase_.end()) {
            AssetProcessingJob job;
            job.assetId = meshAssetId;
            job.settings.generateLODs = true;
            job.settings.maxLODLevels = maxLOD;
            
            std::lock_guard<std::mutex> lock(queueMutex_);
            processingQueue_.push(job);
            queueCondition_.notify_one();
        }
    }

    void packTextures(const std::vector<std::pair<int, int>>& textureSizes,
                     std::function<bool(int, Vector2, Vector2)> placementCallback) {
        // Implement texture atlas packing using bin packing algorithm
        textureAtlas_.width = 2048;
        textureAtlas_.height = 2048;
        
        // Simple bin packing (would use more sophisticated algorithm)
        Vector2 currentPos(0, 0);
        int currentRowHeight = 0;
        
        for (size_t i = 0; i < textureSizes.size(); ++i) {
            int width = textureSizes[i].first;
            int height = textureSizes[i].second;
            
            // Check if texture fits in current row
            if (currentPos.x + width > textureAtlas_.width) {
                // Move to next row
                currentPos.x = 0;
                currentPos.y += currentRowHeight;
                currentRowHeight = 0;
            }
            
            // Check if texture fits in atlas
            if (currentPos.y + height > textureAtlas_.height) {
                break; // Atlas is full
            }
            
            // Place texture
            Vector2 uvMin = Vector2(currentPos.x / textureAtlas_.width, currentPos.y / textureAtlas_.height);
            Vector2 uvMax = Vector2((currentPos.x + width) / textureAtlas_.width, (currentPos.y + height) / textureAtlas_.height);
            
            textureAtlas_.regions.push_back({uvMin, uvMax});
            textureAtlas_.packedTextures.push_back("texture_" + std::to_string(i));
            
            if (placementCallback) {
                placementCallback(i, uvMin, uvMax);
            }
            
            currentPos.x += width;
            currentRowHeight = std::max(currentRowHeight, height);
        }
    }

    std::string getStatistics() const override {
        char buffer[256];
        snprintf(buffer, sizeof(buffer),
                 "Assets: %zu total, %zu processed, Queue: %zu",
                 assetDatabase_.size(),
                 std::count_if(assetDatabase_.begin(), assetDatabase_.end(),
                              [](const auto& pair) { return pair.second.processed; }),
                 processingQueue_.size());
        return std::string(buffer);
    }

    TextureAtlas textureAtlas_;
};

AssetPipeline::AssetPipeline() : impl_(std::make_unique<AssetPipelineImpl>()) {}
AssetPipeline::~AssetPipeline() = default;

bool AssetPipeline::initialize() { return impl_->initialize(); }
void AssetPipeline::shutdown() { impl_->shutdown(); }
void AssetPipeline::update(float deltaTime) { impl_->update(deltaTime); }

void AssetPipeline::optimizeTexture(const std::string& assetId, const std::string& format) {
    static_cast<AssetPipelineImpl*>(impl_.get())->optimizeTexture(assetId, format);
}

void AssetPipeline::generateLODs(const std::string& meshAssetId, int maxLOD) {
    static_cast<AssetPipelineImpl*>(impl_.get())->generateLODs(meshAssetId, maxLOD);
}

void AssetPipeline::packTextures(const std::vector<std::pair<int, int>>& textureSizes,
                                std::function<bool(int, Vector2, Vector2)> placementCallback) {
    static_cast<AssetPipelineImpl*>(impl_.get())->packTextures(textureSizes, placementCallback);
}

std::string AssetPipeline::getStatistics() const {
    return impl_->getStatistics();
}

} // namespace FoundryEngine
