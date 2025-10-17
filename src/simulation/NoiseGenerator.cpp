/**
 * @file NoiseGenerator.cpp
 * @brief Implementation of high-performance procedural noise generation
 */

#include "../../include/GameEngine/simulation/NoiseGenerator.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <iostream>

namespace Foundry {

// Perlin Noise Implementation
PerlinNoise::PerlinNoise(int seed) : seed_(seed) {
    std::mt19937 rng(seed);
    permutation_.resize(256);

    // Fill permutation array
    for (int i = 0; i < 256; ++i) {
        permutation_[i] = i;
    }

    // Shuffle permutation array
    std::shuffle(permutation_.begin(), permutation_.end(), rng);

    // Duplicate for overflow handling
    permutation_.insert(permutation_.end(), permutation_.begin(), permutation_.end());
}

PerlinNoise::~PerlinNoise() {
    cleanupGPU();
}

float PerlinNoise::noise(float x, float y) const {
    // Determine grid cell coordinates
    int x0 = static_cast<int>(floorf(x));
    int x1 = x0 + 1;
    int y0 = static_cast<int>(floorf(y));
    int y1 = y0 + 1;

    // Determine interpolation weights
    float sx = x - static_cast<float>(x0);
    float sy = y - static_cast<float>(y0);

    // Interpolate between grid point gradients
    float n0, n1, ix0, ix1, value;

    n0 = dotGridGradient(x0, y0, x, y);
    n1 = dotGridGradient(x1, y0, x, y);
    ix0 = lerp(n0, n1, sx);

    n0 = dotGridGradient(x0, y1, x, y);
    n1 = dotGridGradient(x1, y1, x, y);
    ix1 = lerp(n0, n1, sx);

    value = lerp(ix0, ix1, sy);
    return value;
}

float PerlinNoise::noise(float x, float y, float z) const {
    // Determine grid cell coordinates
    int x0 = static_cast<int>(floorf(x));
    int x1 = x0 + 1;
    int y0 = static_cast<int>(floorf(y));
    int y1 = y0 + 1;
    int z0 = static_cast<int>(floorf(z));
    int z1 = z0 + 1;

    // Determine interpolation weights
    float sx = x - static_cast<float>(x0);
    float sy = y - static_cast<float>(y0);
    float sz = z - static_cast<float>(z0);

    // Interpolate between grid point gradients
    float n000, n001, n010, n011, n100, n101, n110, n111;
    float ix00, ix01, ix10, ix11, iy0, iy1, value;

    n000 = dotGridGradient(x0, y0, z0, x, y, z);
    n001 = dotGridGradient(x0, y0, z1, x, y, z);
    n010 = dotGridGradient(x0, y1, z0, x, y, z);
    n011 = dotGridGradient(x0, y1, z1, x, y, z);
    n100 = dotGridGradient(x1, y0, z0, x, y, z);
    n101 = dotGridGradient(x1, y0, z1, x, y, z);
    n110 = dotGridGradient(x1, y1, z0, x, y, z);
    n111 = dotGridGradient(x1, y1, z1, x, y, z);

    ix00 = lerp(n000, n100, sx);
    ix01 = lerp(n001, n101, sx);
    ix10 = lerp(n010, n110, sx);
    ix11 = lerp(n011, n111, sx);

    iy0 = lerp(ix00, ix10, sy);
    iy1 = lerp(ix01, ix11, sy);

    value = lerp(iy0, iy1, sz);
    return value;
}

float PerlinNoise::fractal(float x, float y, int octaves, float persistence, float lacunarity) const {
    float value = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxValue = 0.0f;

    for (int i = 0; i < octaves; ++i) {
        value += noise(x * frequency, y * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }

    return value / maxValue;
}

float PerlinNoise::fractal(float x, float y, float z, int octaves, float persistence, float lacunarity) const {
    float value = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxValue = 0.0f;

    for (int i = 0; i < octaves; ++i) {
        value += noise(x * frequency, y * frequency, z * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }

    return value / maxValue;
}

std::vector<unsigned char> PerlinNoise::generateTexture(int width, int height, int octaves) {
    std::vector<unsigned char> texture(width * height * 4); // RGBA

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float u = static_cast<float>(x) / static_cast<float>(width);
            float v = static_cast<float>(y) / static_cast<float>(height);

            // Generate noise value
            float noiseValue = fractal(u * 8.0f, v * 8.0f, octaves);

            // Convert to 0-255 range
            unsigned char pixelValue = static_cast<unsigned char>((noiseValue * 0.5f + 0.5f) * 255.0f);

            int index = (y * width + x) * 4;
            texture[index] = pixelValue;     // R
            texture[index + 1] = pixelValue; // G
            texture[index + 2] = pixelValue; // B
            texture[index + 3] = 255;        // A
        }
    }

    return texture;
}

std::vector<unsigned char> PerlinNoise::generateTextureGPU(int width, int height, int octaves) {
    // GPU implementation would go here
    return generateTexture(width, height, octaves); // Fall back to CPU
}

float PerlinNoise::fade(float t) const {
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

float PerlinNoise::lerp(float a, float b, float t) const {
    return a + t * (b - a);
}

float PerlinNoise::grad(int hash, float x, float y) const {
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : (h == 12 || h == 14 ? x : 0.0f);
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

float PerlinNoise::grad(int hash, float x, float y, float z) const {
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
    float w = h < 2 ? z : (h == 12 || h == 14 ? y : x);
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v) + ((h & 4) == 0 ? w : -w);
}

float PerlinNoise::dotGridGradient(int ix, int iy, float x, float y) const {
    int hash = permutation_[permutation_[ix & 255] + (iy & 255)];
    float dx = x - static_cast<float>(ix);
    float dy = y - static_cast<float>(iy);
    return grad(hash, dx, dy);
}

float PerlinNoise::dotGridGradient(int ix, int iy, int iz, float x, float y, float z) const {
    int hash = permutation_[permutation_[permutation_[ix & 255] + (iy & 255)] + (iz & 255)];
    float dx = x - static_cast<float>(ix);
    float dy = y - static_cast<float>(iy);
    float dz = z - static_cast<float>(iz);
    return grad(hash, dx, dy, dz);
}

bool PerlinNoise::initializeGPU() {
    return false; // Not implemented yet
}

void PerlinNoise::cleanupGPU() {
    // GPU cleanup
}

// Simplex Noise Implementation
SimplexNoise::SimplexNoise(int seed) : seed_(seed) {
    std::mt19937 rng(seed);

    // Initialize permutation arrays
    for (int i = 0; i < 256; ++i) {
        perm_[i] = static_cast<unsigned char>(i);
    }

    // Shuffle permutation array
    for (int i = 255; i > 0; --i) {
        int j = rng() % (i + 1);
        std::swap(perm_[i], perm_[j]);
    }

    // Extend permutation array
    for (int i = 0; i < 256; ++i) {
        perm_[i + 256] = perm_[i];
        permMod12_[i] = static_cast<unsigned char>(perm_[i] % 12);
        permMod12_[i + 256] = permMod12_[i];
    }
}

SimplexNoise::~SimplexNoise() {
    cleanupGPU();
}

float SimplexNoise::noise(float x, float y) const {
    // Simplex noise in 2D
    float n0, n1, n2; // Noise contributions from the three corners

    // Skew the input space to determine which simplex cell we're in
    const float F2 = 0.5f * (sqrtf(3.0f) - 1.0f);
    const float s = (x + y) * F2; // Hairy factor for 2D
    const int i = static_cast<int>(floorf(x + s));
    const int j = static_cast<int>(floorf(y + s));

    const float G2 = (3.0f - sqrtf(3.0f)) / 6.0f;
    const float t = static_cast<float>(i + j) * G2;
    const float X0 = static_cast<float>(i) - t; // Unskew the cell origin back to (x,y) space
    const float Y0 = static_cast<float>(j) - t;
    const float x0 = x - X0; // The x,y distances from the cell origin
    const float y0 = y - Y0;

    // For the 2D case, the simplex shape is an equilateral triangle.
    // Determine which simplex we are in.
    int i1, j1; // Offsets for second (middle) corner of simplex in (i,j) coords
    if (x0 > y0) { i1 = 1; j1 = 0; } // lower triangle, XY order: (0,0)->(1,0)->(1,1)
    else { i1 = 0; j1 = 1; } // upper triangle, YX order: (0,0)->(0,1)->(1,1)

    // A step of (1,0) in (i,j) means a step of (1-c,-c) in (x,y), and
    // a step of (0,1) in (i,j) means a step of (-c,1-c) in (x,y), where
    // c = (3-sqrt(3))/6

    const float x1 = x0 - static_cast<float>(i1) + G2; // Offsets for middle corner in (x,y) unskewed coords
    const float y1 = y0 - static_cast<float>(j1) + G2;
    const float x2 = x0 - 1.0f + 2.0f * G2; // Offsets for last corner in (x,y) unskewed coords
    const float y2 = y0 - 1.0f + 2.0f * G2;

    // Work out the hashed gradient indices of the three simplex corners
    const int ii = i & 255;
    const int jj = j & 255;
    const int gi0 = permMod12_[ii + perm_[jj]];
    const int gi1 = permMod12_[ii + i1 + perm_[jj + j1]];
    const int gi2 = permMod12_[ii + 1 + perm_[jj + 1]];

    // Calculate the contribution from the three corners
    float t0 = 0.5f - x0 * x0 - y0 * y0;
    if (t0 < 0) n0 = 0.0f;
    else {
        t0 *= t0;
        n0 = t0 * t0 * dot(grad3_[gi0], x0, y0); // (x,y) of grad3 used for 2D gradient
    }

    float t1 = 0.5f - x1 * x1 - y1 * y1;
    if (t1 < 0) n1 = 0.0f;
    else {
        t1 *= t1;
        n1 = t1 * t1 * dot(grad3_[gi1], x1, y1);
    }

    float t2 = 0.5f - x2 * x2 - y2 * y2;
    if (t2 < 0) n2 = 0.0f;
    else {
        t2 *= t2;
        n2 = t2 * t2 * dot(grad3_[gi2], x2, y2);
    }

    // Add contributions from each corner to get the final noise value.
    // The result is scaled to return values in the interval [-1,1].
    return 70.0f * (n0 + n1 + n2);
}

float SimplexNoise::noise(float x, float y, float z) const {
    // 3D Simplex noise implementation
    float n0, n1, n2, n3; // Noise contributions from the four corners

    // Skew the input space to determine which simplex cell we're in
    const float F3 = 1.0f / 3.0f;
    const float s = (x + y + z) * F3; // Very nice and simple skew factor for 3D
    const int i = static_cast<int>(floorf(x + s));
    const int j = static_cast<int>(floorf(y + s));
    const int k = static_cast<int>(floorf(z + s));

    const float G3 = 1.0f / 6.0f; // Very nice and simple unskew factor, too
    const float t = static_cast<float>(i + j + k) * G3;
    const float X0 = static_cast<float>(i) - t; // Unskew the cell origin back to (x,y,z) space
    const float Y0 = static_cast<float>(j) - t;
    const float Z0 = static_cast<float>(k) - t;
    const float x0 = x - X0; // The x,y,z distances from the cell origin
    const float y0 = y - Y0;
    const float z0 = z - Z0;

    // For the 3D case, the simplex shape is a slightly irregular tetrahedron.
    // Determine which simplex we are in.
    int i1, j1, k1; // Offsets for second corner of simplex in (i,j,k) coords
    int i2, j2, k2; // Offsets for third corner of simplex in (i,j,k) coords

    if (x0 >= y0) {
        if (y0 >= z0) { i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 1; k2 = 0; } // X Y Z order
        else if (x0 >= z0) { i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 0; k2 = 1; } // X Z Y order
        else { i1 = 0; j1 = 0; k1 = 1; i2 = 1; j2 = 0; k2 = 1; } // Z X Y order
    } else { // x0<y0
        if (y0 < z0) { i1 = 0; j1 = 0; k1 = 1; i2 = 0; j2 = 1; k2 = 1; } // Z Y X order
        else if (x0 < z0) { i1 = 0; j1 = 1; k1 = 0; i2 = 0; j2 = 1; k2 = 1; } // Y Z X order
        else { i1 = 0; j1 = 1; k1 = 0; i2 = 1; j2 = 1; k2 = 0; } // Y X Z order
    }

    // A step of (1,0,0) in (i,j,k) means a step of (1-c,-c,-c) in (x,y,z),
    // a step of (0,1,0) in (i,j,k) means a step of (-c,1-c,-c) in (x,y,z), and
    // a step of (0,0,1) in (i,j,k) means a step of (-c,-c,1-c) in (x,y,z), where
    // c = 1/6.

    const float x1 = x0 - static_cast<float>(i1) + G3; // Offsets for second corner in (x,y,z) coords
    const float y1 = y0 - static_cast<float>(j1) + G3;
    const float z1 = z0 - static_cast<float>(k1) + G3;
    const float x2 = x0 - static_cast<float>(i2) + 2.0f * G3; // Offsets for third corner in (x,y,z) coords
    const float y2 = y0 - static_cast<float>(j2) + 2.0f * G3;
    const float z2 = z0 - static_cast<float>(k2) + 2.0f * G3;
    const float x3 = x0 - 1.0f + 3.0f * G3; // Offsets for last corner in (x,y,z) coords
    const float y3 = y0 - 1.0f + 3.0f * G3;
    const float z3 = z0 - 1.0f + 3.0f * G3;

    // Work out the hashed gradient indices of the four simplex corners
    const int ii = i & 255;
    const int jj = j & 255;
    const int kk = k & 255;
    const int gi0 = permMod12_[ii + perm_[jj + perm_[kk]]];
    const int gi1 = permMod12_[ii + i1 + perm_[jj + j1 + perm_[kk + k1]]];
    const int gi2 = permMod12_[ii + i2 + perm_[jj + j2 + perm_[kk + k2]]];
    const int gi3 = permMod12_[ii + 1 + perm_[jj + 1 + perm_[kk + 1]]];

    // Calculate the contribution from the four corners
    float t0 = 0.6f - x0 * x0 - y0 * y0 - z0 * z0;
    if (t0 < 0) n0 = 0.0f;
    else {
        t0 *= t0;
        n0 = t0 * t0 * dot(grad3_[gi0], x0, y0, z0);
    }

    float t1 = 0.6f - x1 * x1 - y1 * y1 - z1 * z1;
    if (t1 < 0) n1 = 0.0f;
    else {
        t1 *= t1;
        n1 = t1 * t1 * dot(grad3_[gi1], x1, y1, z1);
    }

    float t2 = 0.6f - x2 * x2 - y2 * y2 - z2 * z2;
    if (t2 < 0) n2 = 0.0f;
    else {
        t2 *= t2;
        n2 = t2 * t2 * dot(grad3_[gi2], x2, y2, z2);
    }

    float t3 = 0.6f - x3 * x3 - y3 * y3 - z3 * z3;
    if (t3 < 0) n3 = 0.0f;
    else {
        t3 *= t3;
        n3 = t3 * t3 * dot(grad3_[gi3], x3, y3, z3);
    }

    // Add contributions from each corner to get the final noise value.
    // The result is scaled to stay just inside [-1,1]
    return 32.0f * (n0 + n1 + n2 + n3);
}

float SimplexNoise::noise(float x, float y, float z, float w) const {
    // 4D Simplex noise - simplified implementation
    // In practice, this would use a full 4D simplex noise algorithm
    return (noise(x, y, z) + noise(x + 100.0f, y + 100.0f, z + 100.0f, w)) * 0.5f;
}

float SimplexNoise::fractal(float x, float y, int octaves, float persistence, float lacunarity) const {
    float value = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxValue = 0.0f;

    for (int i = 0; i < octaves; ++i) {
        value += noise(x * frequency, y * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }

    return value / maxValue;
}

float SimplexNoise::fractal(float x, float y, float z, int octaves, float persistence, float lacunarity) const {
    float value = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxValue = 0.0f;

    for (int i = 0; i < octaves; ++i) {
        value += noise(x * frequency, y * frequency, z * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }

    return value / maxValue;
}

std::vector<unsigned char> SimplexNoise::generateTextureGPU(int width, int height, int octaves) {
    // GPU implementation would go here
    std::vector<unsigned char> texture(width * height * 4);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float u = static_cast<float>(x) / static_cast<float>(width);
            float v = static_cast<float>(y) / static_cast<float>(height);

            float noiseValue = fractal(u * 8.0f, v * 8.0f, octaves);
            unsigned char pixelValue = static_cast<unsigned char>((noiseValue * 0.5f + 0.5f) * 255.0f);

            int index = (y * width + x) * 4;
            texture[index] = pixelValue;
            texture[index + 1] = pixelValue;
            texture[index + 2] = pixelValue;
            texture[index + 3] = 255;
        }
    }

    return texture;
}

float SimplexNoise::dot(const std::vector<int>& g, float x, float y) const {
    return g[0] * x + g[1] * y;
}

float SimplexNoise::dot(const std::vector<int>& g, float x, float y, float z) const {
    return g[0] * x + g[1] * y + g[2] * z;
}

float SimplexNoise::dot(const std::vector<int>& g, float x, float y, float z, float w) const {
    return g[0] * x + g[1] * y + g[2] * z + g[3] * w;
}

// Gradient tables for Simplex noise
const std::vector<int> SimplexNoise::grad3_[12][3] = {
    {1,1,0}, {-1,1,0}, {1,-1,0}, {-1,-1,0},
    {1,0,1}, {-1,0,1}, {1,0,-1}, {-1,0,-1},
    {0,1,1}, {0,-1,1}, {0,1,-1}, {0,-1,-1}
};

bool SimplexNoise::initializeGPU() {
    return false; // Not implemented yet
}

void SimplexNoise::cleanupGPU() {
    // GPU cleanup
}

// Value Noise Implementation
ValueNoise::ValueNoise(int size, int seed) : size_(size), seed_(seed) {
    std::mt19937 rng(seed);
    values_.resize(size * size * size);

    for (size_t i = 0; i < values_.size(); ++i) {
        values_[i] = static_cast<float>(rng()) / static_cast<float>(rng.max());
    }
}

ValueNoise::~ValueNoise() {
    cleanupGPU();
}

float ValueNoise::noise(float x, float y) const {
    int ix = static_cast<int>(floorf(x));
    int iy = static_cast<int>(floorf(y));

    float fx = x - static_cast<float>(ix);
    float fy = y - static_cast<float>(iy);

    // Get values at grid points
    float v00 = getValue(ix, iy);
    float v10 = getValue(ix + 1, iy);
    float v01 = getValue(ix, iy + 1);
    float v11 = getValue(ix + 1, iy + 1);

    // Smooth interpolation
    float sx = smoothStep(fx);
    float sy = smoothStep(fy);

    // Bilinear interpolation
    float v0 = lerp(v00, v10, sx);
    float v1 = lerp(v01, v11, sx);

    return lerp(v0, v1, sy);
}

float ValueNoise::noise(float x, float y, float z) const {
    int ix = static_cast<int>(floorf(x));
    int iy = static_cast<int>(floorf(y));
    int iz = static_cast<int>(floorf(z));

    float fx = x - static_cast<float>(ix);
    float fy = y - static_cast<float>(iy);
    float fz = z - static_cast<float>(iz);

    // Trilinear interpolation
    float v000 = getValue(ix, iy, iz);
    float v100 = getValue(ix + 1, iy, iz);
    float v010 = getValue(ix, iy + 1, iz);
    float v110 = getValue(ix + 1, iy + 1, iz);
    float v001 = getValue(ix, iy, iz + 1);
    float v101 = getValue(ix + 1, iy, iz + 1);
    float v011 = getValue(ix, iy + 1, iz + 1);
    float v111 = getValue(ix + 1, iy + 1, iz + 1);

    float sx = smoothStep(fx);
    float sy = smoothStep(fy);
    float sz = smoothStep(fz);

    // Trilinear interpolation
    float v00 = lerp(v000, v100, sx);
    float v10 = lerp(v010, v110, sx);
    float v01 = lerp(v001, v101, sx);
    float v11 = lerp(v011, v111, sx);

    float v0 = lerp(v00, v10, sy);
    float v1 = lerp(v01, v11, sy);

    return lerp(v0, v1, sz);
}

float ValueNoise::fractal(float x, float y, int octaves, float persistence, float lacunarity) const {
    float value = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxValue = 0.0f;

    for (int i = 0; i < octaves; ++i) {
        value += noise(x * frequency, y * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }

    return value / maxValue;
}

float ValueNoise::fractal(float x, float y, float z, int octaves, float persistence, float lacunarity) const {
    float value = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxValue = 0.0f;

    for (int i = 0; i < octaves; ++i) {
        value += noise(x * frequency, y * frequency, z * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }

    return value / maxValue;
}

std::vector<unsigned char> ValueNoise::generateTextureGPU(int width, int height, int octaves) {
    std::vector<unsigned char> texture(width * height * 4);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float u = static_cast<float>(x) / static_cast<float>(width);
            float v = static_cast<float>(y) / static_cast<float>(height);

            float noiseValue = fractal(u * 8.0f, v * 8.0f, octaves);
            unsigned char pixelValue = static_cast<unsigned char>((noiseValue * 0.5f + 0.5f) * 255.0f);

            int index = (y * width + x) * 4;
            texture[index] = pixelValue;
            texture[index + 1] = pixelValue;
            texture[index + 2] = pixelValue;
            texture[index + 3] = 255;
        }
    }

    return texture;
}

float ValueNoise::smoothStep(float t) const {
    return t * t * (3.0f - 2.0f * t);
}

float ValueNoise::lerp(float a, float b, float t) const {
    return a + t * (b - a);
}

float ValueNoise::getValue(int x, int y) const {
    int hash = hash(x, y);
    return values_[hash % values_.size()];
}

float ValueNoise::getValue(int x, int y, int z) const {
    int hash = hash(x, y, z);
    return values_[hash % values_.size()];
}

int ValueNoise::hash(int x, int y) const {
    // Simple hash function
    int h = seed_;
    h ^= x;
    h ^= y;
    h = (h << 13) ^ h;
    return abs(h) % (size_ * size_);
}

int ValueNoise::hash(int x, int y, int z) const {
    int h = seed_;
    h ^= x;
    h ^= y;
    h ^= z;
    h = (h << 13) ^ h;
    return abs(h) % (size_ * size_ * size_);
}

bool ValueNoise::initializeGPU() {
    return false; // Not implemented yet
}

void ValueNoise::cleanupGPU() {
    // GPU cleanup
}

// Worley Noise Implementation
WorleyNoise::WorleyNoise(int seed, int pointsPerCell) : seed_(seed), pointsPerCell_(pointsPerCell) {
    // Generate points for Worley noise
    // This is a simplified implementation
}

WorleyNoise::~WorleyNoise() {
    cleanupGPU();
}

float WorleyNoise::noise(float x, float y) const {
    int cellX = static_cast<int>(floorf(x));
    int cellY = static_cast<int>(floorf(y));

    float minDist = std::numeric_limits<float>::max();

    // Check neighboring cells
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            Point point = getPoint(cellX + dx, cellY + dy);
            float dist = distance(point, Point(x, y, 0.0f));
            if (dist < minDist) {
                minDist = dist;
            }
        }
    }

    return minDist;
}

float WorleyNoise::noise(float x, float y, float z) const {
    int cellX = static_cast<int>(floorf(x));
    int cellY = static_cast<int>(floorf(y));
    int cellZ = static_cast<int>(floorf(z));

    float minDist = std::numeric_limits<float>::max();

    for (int dz = -1; dz <= 1; ++dz) {
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                Point point = getPoint(cellX + dx, cellY + dy, cellZ + dz);
                float dist = distance(point, Point(x, y, z));
                if (dist < minDist) {
                    minDist = dist;
                }
            }
        }
    }

    return minDist;
}

void WorleyNoise::noise(float x, float y, float* distances, int numDistances) const {
    // Implementation for multiple distances would go here
    // For now, just return the single distance
    distances[0] = noise(x, y);
    for (int i = 1; i < numDistances; ++i) {
        distances[i] = 1.0f; // Placeholder
    }
}

std::vector<unsigned char> WorleyNoise::generateTextureGPU(int width, int height) {
    std::vector<unsigned char> texture(width * height * 4);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float u = static_cast<float>(x) / static_cast<float>(width);
            float v = static_cast<float>(y) / static_cast<float>(height);

            float noiseValue = noise(u * 8.0f, v * 8.0f);
            unsigned char pixelValue = static_cast<unsigned char>(noiseValue * 255.0f);

            int index = (y * width + x) * 4;
            texture[index] = pixelValue;
            texture[index + 1] = pixelValue;
            texture[index + 2] = pixelValue;
            texture[index + 3] = 255;
        }
    }

    return texture;
}

float WorleyNoise::distance(const Point& a, const Point& b) const {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

WorleyNoise::Point WorleyNoise::getPoint(int cellX, int cellY) const {
    // Simple point generation based on cell coordinates
    std::mt19937 rng(seed_ + cellX * 73856093 + cellY * 19349663);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    return Point(
        static_cast<float>(cellX) + dist(rng),
        static_cast<float>(cellY) + dist(rng),
        0.0f
    );
}

WorleyNoise::Point WorleyNoise::getPoint(int cellX, int cellY, int cellZ) const {
    std::mt19937 rng(seed_ + cellX * 73856093 + cellY * 19349663 + cellZ * 83492791);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    return Point(
        static_cast<float>(cellX) + dist(rng),
        static_cast<float>(cellY) + dist(rng),
        static_cast<float>(cellZ) + dist(rng)
    );
}

bool WorleyNoise::initializeGPU() {
    return false; // Not implemented yet
}

void WorleyNoise::cleanupGPU() {
    // GPU cleanup
}

// Noise Utilities Implementation
float NoiseUtils::combine(const std::vector<float>& noises, const std::vector<float>& weights) {
    if (noises.empty() || weights.empty()) return 0.0f;

    float result = 0.0f;
    float totalWeight = 0.0f;

    size_t count = std::min(noises.size(), weights.size());
    for (size_t i = 0; i < count; ++i) {
        result += noises[i] * weights[i];
        totalWeight += weights[i];
    }

    return totalWeight > 0.0f ? result / totalWeight : 0.0f;
}

float NoiseUtils::turbulence(float x, float y, int octaves, float strength) {
    float value = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;

    for (int i = 0; i < octaves; ++i) {
        value += fabsf(PerlinNoise().noise(x * frequency, y * frequency)) * amplitude;
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }

    return value * strength;
}

float NoiseUtils::ridged(float x, float y, int octaves, float offset) {
    float value = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxValue = 0.0f;

    for (int i = 0; i < octaves; ++i) {
        float noise = PerlinNoise().noise(x * frequency, y * frequency);
        value += (1.0f - fabsf(noise)) * amplitude;
        maxValue += amplitude;
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }

    return (value / maxValue) + offset;
}

Vector2 NoiseUtils::domainWarp(float x, float y, float strength) {
    PerlinNoise noise;
    float offsetX = noise.noise(x, y) * strength;
    float offsetY = noise.noise(x + 100.0f, y + 100.0f) * strength;

    return Vector2(x + offsetX, y + offsetY);
}

std::vector<unsigned char> NoiseUtils::heightToNormalMap(const std::vector<unsigned char>& heightMap,
                                                        int width, int height, float strength) {
    std::vector<unsigned char> normalMap(width * height * 4);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Get neighboring heights
            float h = getHeight(heightMap, width, height, x, y) / 255.0f;
            float hR = getHeight(heightMap, width, height, x + 1, y) / 255.0f;
            float hL = getHeight(heightMap, width, height, x - 1, y) / 255.0f;
            float hD = getHeight(heightMap, width, height, x, y + 1) / 255.0f;
            float hU = getHeight(heightMap, width, height, x, y - 1) / 255.0f;

            // Calculate normal
            Vector3 normal(
                (hL - hR) * strength,
                (hU - hD) * strength,
                1.0f
            );
            normal = normal.normalized();

            // Convert to 0-255 range
            int index = (y * width + x) * 4;
            normalMap[index] = static_cast<unsigned char>((normal.x * 0.5f + 0.5f) * 255.0f);
            normalMap[index + 1] = static_cast<unsigned char>((normal.y * 0.5f + 0.5f) * 255.0f);
            normalMap[index + 2] = static_cast<unsigned char>((normal.z * 0.5f + 0.5f) * 255.0f);
            normalMap[index + 3] = 255;
        }
    }

    return normalMap;
}

std::vector<unsigned char> NoiseUtils::makeSeamless(const std::vector<unsigned char>& noiseMap,
                                                   int width, int height) {
    std::vector<unsigned char> seamlessMap = noiseMap;

    // Apply seamless blending
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Calculate blend factors for seamless tiling
            float u = static_cast<float>(x) / static_cast<float>(width);
            float v = static_cast<float>(y) / static_cast<float>(height);

            // Apply smoothstep blending at edges
            float blendU = smoothstepBlend(u);
            float blendV = smoothstepBlend(v);
            float blendFactor = std::max(blendU, blendV);

            int index = (y * width + x) * 4;
            for (int c = 0; c < 3; ++c) {
                float original = seamlessMap[index + c] / 255.0f;
                float blended = original * (1.0f - blendFactor) + 0.5f * blendFactor;
                seamlessMap[index + c] = static_cast<unsigned char>(blended * 255.0f);
            }
        }
    }

    return seamlessMap;
}

float NoiseUtils::getHeight(const std::vector<unsigned char>& heightMap, int width, int height, int x, int y) {
    // Clamp coordinates
    x = std::max(0, std::min(width - 1, x));
    y = std::max(0, std::min(height - 1, y));

    return static_cast<float>(heightMap[(y * width + x) * 4]); // Use red channel
}

float NoiseUtils::smoothstepBlend(float t) {
    if (t < 0.1f) {
        return smoothstep(0.0f, 0.1f, t);
    } else if (t > 0.9f) {
        return smoothstep(1.0f, 0.9f, t);
    }
    return 0.0f;
}

float NoiseUtils::smoothstep(float edge0, float edge1, float x) {
    x = std::max(0.0f, std::min(1.0f, (x - edge0) / (edge1 - edge0)));
    return x * x * (3.0f - 2.0f * x);
}

} // namespace Foundry