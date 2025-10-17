/**
 * @file NoiseGenerator.h
 * @brief High-performance procedural noise generation with GPU acceleration
 */

#pragma once

#include "../math/Vector2.h"
#include "../math/Vector3.h"
#include "../graphics/Renderer.h"
#include <vector>
#include <memory>
#include <random>

namespace Foundry {

/**
 * @brief Perlin noise generator with multiple octaves
 * Time Complexity: O(1) per sample, Space Complexity: O(1)
 * GPU-accelerated for texture generation
 */
class PerlinNoise {
private:
    std::vector<int> permutation_;
    int seed_;

    // GPU compute resources
    void* gpuPermutationBuffer_ = nullptr;
    void* gpuNoiseBuffer_ = nullptr;
    void* gpuComputePipeline_ = nullptr;

public:
    PerlinNoise(int seed = 0);
    ~PerlinNoise();

    /**
     * Generate 2D Perlin noise
     * @param x X coordinate
     * @param y Y coordinate
     * @return Noise value in range [-1, 1]
     */
    float noise(float x, float y) const;

    /**
     * Generate 3D Perlin noise
     * @param x X coordinate
     * @param y Y coordinate
     * @param z Z coordinate
     * @return Noise value in range [-1, 1]
     */
    float noise(float x, float y, float z) const;

    /**
     * Generate fractal noise with multiple octaves
     * @param x X coordinate
     * @param y Y coordinate
     * @param octaves Number of octaves
     * @param persistence Amplitude multiplier per octave
     * @param lacunarity Frequency multiplier per octave
     * @return Fractal noise value
     */
    float fractal(float x, float y, int octaves = 4, float persistence = 0.5f, float lacunarity = 2.0f) const;

    /**
     * Generate 3D fractal noise
     */
    float fractal(float x, float y, float z, int octaves = 4, float persistence = 0.5f, float lacunarity = 2.0f) const;

    /**
     * Generate noise texture
     * @param width Texture width
     * @param height Texture height
     * @param octaves Number of octaves
     * @return Vector of RGBA values
     */
    std::vector<unsigned char> generateTexture(int width, int height, int octaves = 4);

    /**
     * GPU-accelerated texture generation
     */
    std::vector<unsigned char> generateTextureGPU(int width, int height, int octaves = 4);

private:
    float fade(float t) const;
    float lerp(float a, float b, float t) const;
    float grad(int hash, float x, float y) const;
    float grad(int hash, float x, float y, float z) const;

    bool initializeGPU();
    void cleanupGPU();
};

/**
 * @brief Simplex noise generator (improved Perlin noise)
 * Time Complexity: O(1) per sample, Space Complexity: O(1)
 * Better performance and visual quality than Perlin noise
 */
class SimplexNoise {
private:
    std::vector<unsigned char> perm_;
    std::vector<unsigned char> permMod12_;
    int seed_;

    // GPU compute resources
    void* gpuPermutationBuffer_ = nullptr;
    void* gpuNoiseBuffer_ = nullptr;
    void* gpuComputePipeline_ = nullptr;

public:
    SimplexNoise(int seed = 0);
    ~SimplexNoise();

    /**
     * Generate 2D Simplex noise
     * @param x X coordinate
     * @param y Y coordinate
     * @return Noise value in range [-1, 1]
     */
    float noise(float x, float y) const;

    /**
     * Generate 3D Simplex noise
     * @param x X coordinate
     * @param y Y coordinate
     * @param z Z coordinate
     * @return Noise value in range [-1, 1]
     */
    float noise(float x, float y, float z) const;

    /**
     * Generate 4D Simplex noise
     * @param x X coordinate
     * @param y Y coordinate
     * @param z Z coordinate
     * @param w W coordinate
     * @return Noise value in range [-1, 1]
     */
    float noise(float x, float y, float z, float w) const;

    /**
     * Generate fractal noise
     */
    float fractal(float x, float y, int octaves = 4, float persistence = 0.5f, float lacunarity = 2.0f) const;
    float fractal(float x, float y, float z, int octaves = 4, float persistence = 0.5f, float lacunarity = 2.0f) const;

    /**
     * GPU-accelerated texture generation
     */
    std::vector<unsigned char> generateTextureGPU(int width, int height, int octaves = 4);

private:
    float dot(const std::vector<int>& g, float x, float y) const;
    float dot(const std::vector<int>& g, float x, float y, float z) const;
    float dot(const std::vector<int>& g, float x, float y, float z, float w) const;

    bool initializeGPU();
    void cleanupGPU();
};

/**
 * @brief Value noise generator (fastest noise type)
 * Time Complexity: O(1) per sample, Space Complexity: O(1)
 * Excellent for performance-critical applications
 */
class ValueNoise {
private:
    std::vector<float> values_;
    int size_;
    int seed_;

    // GPU compute resources
    void* gpuValuesBuffer_ = nullptr;
    void* gpuNoiseBuffer_ = nullptr;
    void* gpuComputePipeline_ = nullptr;

public:
    ValueNoise(int size = 256, int seed = 0);
    ~ValueNoise();

    /**
     * Generate 2D value noise
     */
    float noise(float x, float y) const;

    /**
     * Generate 3D value noise
     */
    float noise(float x, float y, float z) const;

    /**
     * Generate fractal noise
     */
    float fractal(float x, float y, int octaves = 4, float persistence = 0.5f, float lacunarity = 2.0f) const;
    float fractal(float x, float y, float z, int octaves = 4, float persistence = 0.5f, float lacunarity = 2.0f) const;

    /**
     * GPU-accelerated texture generation
     */
    std::vector<unsigned char> generateTextureGPU(int width, int height, int octaves = 4);

private:
    float smoothStep(float t) const;
    float lerp(float a, float b, float t) const;
    int hash(int x, int y) const;
    int hash(int x, int y, int z) const;

    bool initializeGPU();
    void cleanupGPU();
};

/**
 * @brief Worley noise (cellular noise) generator
 * Time Complexity: O(k) per sample where k is points per cell
 * Space Complexity: O(1) with precomputed point sets
 */
class WorleyNoise {
private:
    struct Point {
        float x, y, z;
        Point(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
    };

    std::vector<Point> points_;
    int seed_;
    int pointsPerCell_;

    // GPU compute resources
    void* gpuPointsBuffer_ = nullptr;
    void* gpuNoiseBuffer_ = nullptr;
    void* gpuComputePipeline_ = nullptr;

public:
    WorleyNoise(int seed = 0, int pointsPerCell = 1);
    ~WorleyNoise();

    /**
     * Generate 2D Worley noise
     * @param x X coordinate
     * @param y Y coordinate
     * @return Distance to nearest point
     */
    float noise(float x, float y) const;

    /**
     * Generate 3D Worley noise
     */
    float noise(float x, float y, float z) const;

    /**
     * Generate Worley noise with multiple distances
     * @param x X coordinate
     * @param y Y coordinate
     * @param distances Output array for multiple distances
     * @param numDistances Number of distances to compute
     */
    void noise(float x, float y, float* distances, int numDistances) const;

    /**
     * GPU-accelerated texture generation
     */
    std::vector<unsigned char> generateTextureGPU(int width, int height);

private:
    float distance(const Point& a, const Point& b) const;
    Point getPoint(int cellX, int cellY) const;
    Point getPoint(int cellX, int cellY, int cellZ) const;

    bool initializeGPU();
    void cleanupGPU();
};

/**
 * @brief Advanced noise utilities and combinations
 */
class NoiseUtils {
public:
    /**
     * Combine multiple noise functions
     */
    static float combine(const std::vector<float>& noises, const std::vector<float>& weights);

    /**
     * Apply turbulence to noise
     */
    static float turbulence(float x, float y, int octaves, float strength = 1.0f);

    /**
     * Generate ridged noise (mountain-like)
     */
    static float ridged(float x, float y, int octaves, float offset = 1.0f);

    /**
     * Domain warping
     */
    static Vector2 domainWarp(float x, float y, float strength);

    /**
     * Generate normal map from height map
     */
    static std::vector<unsigned char> heightToNormalMap(const std::vector<unsigned char>& heightMap, int width, int height, float strength = 1.0f);

    /**
     * Generate seamless noise texture
     */
    static std::vector<unsigned char> makeSeamless(const std::vector<unsigned char>& noiseMap, int width, int height);
};

} // namespace Foundry