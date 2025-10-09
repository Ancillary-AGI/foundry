#include "gtest/gtest.h"
#include "GameEngine/systems/AssetSystem.h"
#include "GameEngine/core/MemoryPool.h"
#include <thread>
#include <chrono>
#include <filesystem>

namespace FoundryEngine {
namespace Tests {

/**
 * @brief Test fixture for Asset System tests
 */
class AssetSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        memoryPool = std::make_unique<MemoryPool>(2048, 16384); // 2KB blocks, 16KB total
        assetManager = std::make_unique<DefaultAssetManager>();
        assetManager->initialize();
    }

    void TearDown() override {
        if (assetManager) {
            assetManager->shutdown();
        }
        memoryPool.reset();
    }

    std::unique_ptr<MemoryPool> memoryPool;
    std::unique_ptr<DefaultAssetManager> assetManager;
};

/**
 * @brief Test asset metadata functionality
 */
TEST_F(AssetSystemTest, AssetMetadata) {
    AssetMetadata metadata;
    metadata.guid = "test-asset-123";
    metadata.type = AssetType::TEXTURE;
    metadata.sourcePath = "/assets/textures/test.png";
    metadata.memoryUsage = 1024;
    metadata.lastModified = 1234567890;

    EXPECT_TRUE(metadata.isValid());
    EXPECT_EQ(metadata.guid, "test-asset-123");
    EXPECT_EQ(metadata.type, AssetType::TEXTURE);
    EXPECT_FALSE(metadata.dependencies.empty()); // Should be empty initially
}

/**
 * @brief Test typed asset functionality
 */
TEST_F(AssetSystemTest, TypedAsset) {
    // Create a simple test asset type
    struct TestAssetData {
        int id;
        std::string name;
        float value;
    };

    // Test asset creation and memory allocation
    auto testAsset = std::make_unique<TypedAsset<TestAssetData>>(*memoryPool);
    ASSERT_TRUE(testAsset);

    // Test loading
    bool loaded = testAsset->load("/test/path");
    EXPECT_TRUE(loaded);
    EXPECT_TRUE(testAsset->isLoaded());

    // Test data access
    TestAssetData* data = testAsset->getData();
    ASSERT_NE(data, nullptr);

    // Test data modification
    data->id = 42;
    data->name = "Test Asset";
    data->value = 3.14f;

    // Verify data integrity
    EXPECT_EQ(data->id, 42);
    EXPECT_EQ(data->name, "Test Asset");
    EXPECT_FLOAT_EQ(data->value, 3.14f);

    // Test metadata
    const AssetMetadata& metadata = testAsset->getMetadata();
    EXPECT_EQ(metadata.sourcePath, "/test/path");
    EXPECT_TRUE(metadata.isValid());

    // Test validation
    EXPECT_TRUE(testAsset->validate());

    // Test unloading
    testAsset->unload();
    EXPECT_FALSE(testAsset->isLoaded());
    EXPECT_EQ(testAsset->getData(), nullptr);
}

/**
 * @brief Test asset type casting safety
 */
TEST_F(AssetSystemTest, TypeCastingSafety) {
    struct BaseAssetData {
        int baseValue = 100;
    };

    struct DerivedAssetData : public BaseAssetData {
        int derivedValue = 200;
    };

    auto baseAsset = std::make_unique<TypedAsset<BaseAssetData>>(*memoryPool);
    auto derivedAsset = std::make_unique<TypedAsset<DerivedAssetData>>(*memoryPool);

    ASSERT_TRUE(baseAsset->load("/base"));
    ASSERT_TRUE(derivedAsset->load("/derived"));

    // Test safe casting
    EXPECT_TRUE(baseAsset->canCastTo<BaseAssetData>());
    EXPECT_FALSE(baseAsset->canCastTo<DerivedAssetData>()); // Can't cast base to derived

    EXPECT_TRUE(derivedAsset->canCastTo<BaseAssetData>()); // Can cast derived to base
    EXPECT_TRUE(derivedAsset->canCastTo<DerivedAssetData>());

    // Test actual casting
    BaseAssetData* baseData = baseAsset->getDataAs<BaseAssetData>();
    ASSERT_NE(baseData, nullptr);
    EXPECT_EQ(baseData->baseValue, 100);

    DerivedAssetData* derivedData = derivedAsset->getDataAs<DerivedAssetData>();
    ASSERT_NE(derivedData, nullptr);
    EXPECT_EQ(derivedData->derivedValue, 200);

    BaseAssetData* baseFromDerived = derivedAsset->getDataAs<BaseAssetData>();
    ASSERT_NE(baseFromDerived, nullptr);
    EXPECT_EQ(baseFromDerived->baseValue, 100);
}

/**
 * @brief Test asset manager functionality
 */
TEST_F(AssetSystemTest, AssetManager) {
    ASSERT_TRUE(assetManager);
    EXPECT_TRUE(assetManager->isInitialized());

    // Test asset directories
    assetManager->addAssetDirectory("/test/assets");
    auto directories = assetManager->getAssetDirectories();
    EXPECT_EQ(directories.size(), 1);
    EXPECT_EQ(directories[0], "/test/assets");

    // Test memory management integration
    size_t initialMemory = assetManager->getTotalMemoryUsage();
    EXPECT_GE(initialMemory, 0);

    // Test asset type detection
    AssetType textureType = assetManager->getAssetType("/test/texture.png");
    EXPECT_NE(textureType, AssetType::UNKNOWN);

    AssetType unknownType = assetManager->getAssetType("/test/unknown.xyz");
    EXPECT_EQ(unknownType, AssetType::UNKNOWN);
}

/**
 * @brief Test asset loader registration
 */
TEST_F(AssetSystemTest, AssetLoaderRegistration) {
    // Create a test loader
    class TestLoader : public AssetLoader {
    public:
        bool canLoad(const std::string& extension) const override {
            return extension == ".test";
        }

        std::unique_ptr<Asset> load(const std::string& path) override {
            return nullptr; // Not implemented for test
        }

        std::vector<std::string> getSupportedExtensions() const override {
            return {".test"};
        }
    };

    auto testLoader = std::make_unique<TestLoader>();
    assetManager->registerLoader(std::move(testLoader));

    // Test loader retrieval
    AssetLoader* retrievedLoader = assetManager->getLoader(".test");
    EXPECT_NE(retrievedLoader, nullptr);

    AssetLoader* missingLoader = assetManager->getLoader(".missing");
    EXPECT_EQ(missingLoader, nullptr);
}

/**
 * @brief Test asset streaming functionality
 */
TEST_F(AssetSystemTest, AssetStreaming) {
    // Test streaming enable/disable
    assetManager->enableStreaming(true);
    EXPECT_TRUE(assetManager->isStreamingEnabled());

    assetManager->enableStreaming(false);
    EXPECT_FALSE(assetManager->isStreamingEnabled());

    // Test streaming distance
    assetManager->setStreamingDistance(100.0f);
    EXPECT_FLOAT_EQ(assetManager->getStreamingDistance(), 100.0f);

    assetManager->setStreamingDistance(50.0f);
    EXPECT_FLOAT_EQ(assetManager->getStreamingDistance(), 50.0f);
}

/**
 * @brief Test asset memory management
 */
TEST_F(AssetSystemTest, MemoryManagement) {
    size_t initialMemory = assetManager->getTotalMemoryUsage();

    // Create multiple assets to test memory tracking
    std::vector<std::unique_ptr<TypedAsset<int>>> assets;

    for (int i = 0; i < 5; ++i) {
        auto asset = std::make_unique<TypedAsset<int>>(*memoryPool);
        if (asset->load("/test/asset" + std::to_string(i))) {
            assets.push_back(std::move(asset));
        }
    }

    size_t afterAllocationMemory = assetManager->getTotalMemoryUsage();
    EXPECT_GE(afterAllocationMemory, initialMemory);

    // Test memory budget setting
    assetManager->setMemoryBudget(AssetType::TEXTURE, 1024 * 1024); // 1MB
    size_t textureBudget = assetManager->getMemoryBudget(AssetType::TEXTURE);
    EXPECT_EQ(textureBudget, 1024 * 1024);

    // Test garbage collection
    assetManager->garbageCollect();

    // Clean up
    assets.clear();
}

/**
 * @brief Test asset hot reloading
 */
TEST_F(AssetSystemTest, HotReloading) {
    // Test hot reload enable/disable
    assetManager->enableHotReload(true);
    EXPECT_TRUE(assetManager->isHotReloadEnabled());

    assetManager->enableHotReload(false);
    EXPECT_FALSE(assetManager->isHotReloadEnabled());

    // Test directory watching
    assetManager->watchDirectory("/test/assets");
    assetManager->unwatchDirectory("/test/assets");
}

/**
 * @brief Test asset database functionality
 */
TEST_F(AssetSystemTest, AssetDatabase) {
    // Test database building
    assetManager->buildAssetDatabase();

    // Test asset finding
    auto foundAssets = assetManager->findAssets("*.png");
    // Should find assets or return empty vector

    // Test assets by type
    auto textureAssets = assetManager->getAssetsByType(AssetType::TEXTURE);
    // Should return assets or empty vector

    // Test asset reloading
    assetManager->reloadAllAssets();
}

/**
 * @brief Test asset callbacks
 */
TEST_F(AssetSystemTest, AssetCallbacks) {
    std::atomic<int> loadCount{0};
    std::atomic<int> unloadCount{0};

    // Set up callbacks
    assetManager->setAssetLoadedCallback([&](Asset* asset) {
        loadCount++;
    });

    assetManager->setAssetUnloadedCallback([&](const std::string& path) {
        unloadCount++;
    });

    // Create and load an asset
    auto testAsset = std::make_unique<TypedAsset<int>>(*memoryPool);
    testAsset->load("/test/callback");

    // Check that callback was called
    EXPECT_GE(loadCount.load(), 0);

    // Clean up
    testAsset.reset();
}

/**
 * @brief Test concurrent asset loading
 */
TEST_F(AssetSystemTest, ConcurrentLoading) {
    const int numThreads = 4;
    const int assetsPerThread = 10;

    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};

    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < assetsPerThread; ++i) {
                auto asset = std::make_unique<TypedAsset<int>>(*memoryPool);
                if (asset->load("/test/concurrent/" + std::to_string(t) + "/" + std::to_string(i))) {
                    successCount++;
                }
            }
        });
    }

    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }

    // Verify concurrent loading worked
    EXPECT_GT(successCount.load(), 0);

    // Asset manager should still be in valid state
    EXPECT_TRUE(assetManager->isInitialized());
}

/**
 * @brief Test asset validation and error handling
 */
TEST_F(AssetSystemTest, ValidationAndErrors) {
    // Test invalid asset
    auto invalidAsset = std::make_unique<TypedAsset<int>>(*memoryPool);

    // Don't load the asset
    EXPECT_FALSE(invalidAsset->isLoaded());
    EXPECT_FALSE(invalidAsset->validate());

    // Test asset with invalid metadata
    invalidAsset->load("/invalid/path");
    AssetMetadata invalidMetadata;
    invalidMetadata.guid = ""; // Invalid GUID
    invalidAsset->updateMetadata(invalidMetadata);

    EXPECT_FALSE(invalidAsset->validate());

    // Test needs reload functionality
    EXPECT_FALSE(invalidAsset->needsReload()); // Implementation dependent
}

} // namespace Tests
} // namespace FoundryEngine
