/**
 * @file ImmersiveWorldCreator.h
 * @brief Advanced world generation and immersive environment creation
 * @author FoundryEngine Team
 * @date 2024
 * @version 2.0.0
 */

#pragma once

#include "../core/System.h"
#include "../math/Vector3.h"
#include <memory>
#include <vector>
#include <functional>

namespace FoundryEngine {

/**
 * @class ImmersiveWorldCreator
 * @brief Comprehensive world generation with procedural content and AI-driven environments
 */
class ImmersiveWorldCreator : public System {
public:
    enum class BiomeType {
        Forest, Desert, Mountain, Ocean, Arctic, Jungle, Swamp, Volcanic, Alien
    };

    struct WorldConfig {
        uint32_t seed = 12345;
        float worldSize = 10000.0f;
        uint32_t chunkSize = 256;
        BiomeType primaryBiome = BiomeType::Forest;
        bool enableWeatherSystem = true;
        bool enableDayNightCycle = true;
        bool enableSeasons = true;
        float detailLevel = 1.0f;
    };

    ImmersiveWorldCreator();
    ~ImmersiveWorldCreator();

    bool initialize(const WorldConfig& config = WorldConfig{}) override;
    void shutdown() override;
    void update(float deltaTime) override;

    // World generation
    void generateWorld(const WorldConfig& config);
    void generateChunk(int32_t chunkX, int32_t chunkZ);
    void generateBiome(BiomeType biome, const Vector3& center, float radius);

private:
    class ImmersiveWorldCreatorImpl;
    std::unique_ptr<ImmersiveWorldCreatorImpl> impl_;
};

} // namespace FoundryEngine