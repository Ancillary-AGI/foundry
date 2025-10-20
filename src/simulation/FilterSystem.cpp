/**
 * @file FilterSystem.cpp
 * @brief Implementation of advanced image filtering and post-processing system
 */

#include "../../include/GameEngine/simulation/FilterSystem.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace Foundry {

// Gaussian Blur Filter Implementation
GaussianBlurFilter::GaussianBlurFilter(float sigma) : sigma_(sigma), kernelSize_(0), kernelDirty_(true) {
    updateKernel();
}

GaussianBlurFilter::~GaussianBlurFilter() {
    cleanupGPU();
}

void GaussianBlurFilter::apply(const std::vector<unsigned char>& input,
                              std::vector<unsigned char>& output,
                              int width, int height) {
    if (kernelDirty_) {
        updateKernel();
    }

    if (initializeGPU()) {
        applyGPU(input, output, width, height);
    } else {
        applyCPU(input, output, width, height);
    }
}

const char* GaussianBlurFilter::getName() const {
    return "GaussianBlur";
}

void GaussianBlurFilter::setParameter(const std::string& name, float value) {
    if (name == "sigma") {
        sigma_ = std::max(0.1f, value);
        kernelDirty_ = true;
    }
}

float GaussianBlurFilter::getParameter(const std::string& name) const {
    if (name == "sigma") {
        return sigma_;
    }
    return 0.0f;
}

void GaussianBlurFilter::updateKernel() {
    // Calculate kernel size (should cover 3*sigma)
    kernelSize_ = static_cast<int>(ceilf(3.0f * sigma_));
    if (kernelSize_ % 2 == 0) kernelSize_++; // Make sure it's odd

    kernel_.resize(kernelSize_ * kernelSize_);
    float sum = 0.0f;

    int center = kernelSize_ / 2;
    for (int y = 0; y < kernelSize_; ++y) {
        for (int x = 0; x < kernelSize_; ++x) {
            float dx = static_cast<float>(x - center);
            float dy = static_cast<float>(y - center);
            float value = expf(-(dx * dx + dy * dy) / (2.0f * sigma_ * sigma_));
            kernel_[y * kernelSize_ + x] = value;
            sum += value;
        }
    }

    // Normalize kernel
    for (float& value : kernel_) {
        value /= sum;
    }

    kernelDirty_ = false;
}

void GaussianBlurFilter::applyCPU(const std::vector<unsigned char>& input,
                                 std::vector<unsigned char>& output,
                                 int width, int height) {
    output.resize(input.size());

    int halfKernel = kernelSize_ / 2;

    // Apply separable Gaussian blur (horizontal then vertical for efficiency)
    std::vector<unsigned char> temp(input.size());

    // Horizontal pass
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            for (int c = 0; c < 4; ++c) { // RGBA channels
                float sum = 0.0f;
                float weightSum = 0.0f;

                for (int kx = -halfKernel; kx <= halfKernel; ++kx) {
                    int px = std::min(width - 1, std::max(0, x + kx));
                    int kernelIdx = (kx + halfKernel) + halfKernel * kernelSize_; // 1D kernel for horizontal

                    float weight = expf(-(kx * kx) / (2.0f * sigma_ * sigma_));
                    sum += input[(y * width + px) * 4 + c] * weight;
                    weightSum += weight;
                }

                temp[(y * width + x) * 4 + c] = static_cast<unsigned char>(sum / weightSum);
            }
        }
    }

    // Vertical pass
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            for (int c = 0; c < 4; ++c) {
                float sum = 0.0f;
                float weightSum = 0.0f;

                for (int ky = -halfKernel; ky <= halfKernel; ++ky) {
                    int py = std::min(height - 1, std::max(0, y + ky));

                    float weight = expf(-(ky * ky) / (2.0f * sigma_ * sigma_));
                    sum += temp[(py * width + x) * 4 + c] * weight;
                    weightSum += weight;
                }

                output[(y * width + x) * 4 + c] = static_cast<unsigned char>(sum / weightSum);
            }
        }
    }
}

void GaussianBlurFilter::applyGPU(const std::vector<unsigned char>& input,
                                 std::vector<unsigned char>& output,
                                 int width, int height) {
    // GPU implementation would go here
    applyCPU(input, output, width, height); // Fall back to CPU
}

bool GaussianBlurFilter::initializeGPU() {
    return false; // Not implemented yet
}

void GaussianBlurFilter::cleanupGPU() {
    // GPU cleanup
}

// Bilateral Filter Implementation
BilateralFilter::BilateralFilter(float sigmaSpatial, float sigmaRange) :
    sigmaSpatial_(sigmaSpatial), sigmaRange_(sigmaRange), kernelSize_(5) {
}

BilateralFilter::~BilateralFilter() {
    cleanupGPU();
}

void BilateralFilter::apply(const std::vector<unsigned char>& input,
                           std::vector<unsigned char>& output,
                           int width, int height) {
    if (initializeGPU()) {
        applyGPU(input, output, width, height);
    } else {
        applyCPU(input, output, width, height);
    }
}

const char* BilateralFilter::getName() const {
    return "Bilateral";
}

void BilateralFilter::setParameter(const std::string& name, float value) {
    if (name == "sigmaSpatial") {
        sigmaSpatial_ = std::max(0.1f, value);
    } else if (name == "sigmaRange") {
        sigmaRange_ = std::max(0.1f, value);
    }
}

float BilateralFilter::getParameter(const std::string& name) const {
    if (name == "sigmaSpatial") {
        return sigmaSpatial_;
    } else if (name == "sigmaRange") {
        return sigmaRange_;
    }
    return 0.0f;
}

void BilateralFilter::applyCPU(const std::vector<unsigned char>& input,
                              std::vector<unsigned char>& output,
                              int width, int height) {
    output.resize(input.size());

    int halfKernel = kernelSize_ / 2;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            for (int c = 0; c < 4; ++c) {
                float sum = 0.0f;
                float weightSum = 0.0f;

                float centerValue = input[(y * width + x) * 4 + c];

                for (int ky = -halfKernel; ky <= halfKernel; ++ky) {
                    for (int kx = -halfKernel; kx <= halfKernel; ++kx) {
                        int px = std::min(width - 1, std::max(0, x + kx));
                        int py = std::min(height - 1, std::max(0, y + ky));

                        float neighborValue = input[(py * width + px) * 4 + c];

                        // Spatial weight
                        float spatialDist = sqrtf(kx * kx + ky * ky);
                        float spatialWeight = expf(-(spatialDist * spatialDist) /
                                                 (2.0f * sigmaSpatial_ * sigmaSpatial_));

                        // Range weight
                        float rangeDiff = centerValue - neighborValue;
                        float rangeWeight = expf(-(rangeDiff * rangeDiff) /
                                                (2.0f * sigmaRange_ * sigmaRange_));

                        float weight = spatialWeight * rangeWeight;
                        sum += neighborValue * weight;
                        weightSum += weight;
                    }
                }

                output[(y * width + x) * 4 + c] = static_cast<unsigned char>(sum / weightSum);
            }
        }
    }
}

void BilateralFilter::applyGPU(const std::vector<unsigned char>& input,
                              std::vector<unsigned char>& output,
                              int width, int height) {
    // GPU implementation would go here
    applyCPU(input, output, width, height);
}

bool BilateralFilter::initializeGPU() {
    return false;
}

void BilateralFilter::cleanupGPU() {
    // GPU cleanup
}

// Median Filter Implementation
MedianFilter::MedianFilter(int kernelSize) : kernelSize_(kernelSize) {
}

MedianFilter::~MedianFilter() {
    cleanupGPU();
}

void MedianFilter::apply(const std::vector<unsigned char>& input,
                        std::vector<unsigned char>& output,
                        int width, int height) {
    if (initializeGPU()) {
        applyGPU(input, output, width, height);
    } else {
        applyCPU(input, output, width, height);
    }
}

const char* MedianFilter::getName() const {
    return "Median";
}

void MedianFilter::setParameter(const std::string& name, float value) {
    if (name == "kernelSize") {
        kernelSize_ = std::max(3, static_cast<int>(value) | 1); // Ensure odd size
    }
}

float MedianFilter::getParameter(const std::string& name) const {
    if (name == "kernelSize") {
        return static_cast<float>(kernelSize_);
    }
    return 0.0f;
}

void MedianFilter::applyCPU(const std::vector<unsigned char>& input,
                           std::vector<unsigned char>& output,
                           int width, int height) {
    output.resize(input.size());

    int halfKernel = kernelSize_ / 2;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            for (int c = 0; c < 4; ++c) {
                std::vector<unsigned char> values;

                // Collect neighborhood values
                for (int ky = -halfKernel; ky <= halfKernel; ++ky) {
                    for (int kx = -halfKernel; kx <= halfKernel; ++kx) {
                        int px = std::min(width - 1, std::max(0, x + kx));
                        int py = std::min(height - 1, std::max(0, y + ky));

                        values.push_back(input[(py * width + px) * 4 + c]);
                    }
                }

                // Find median
                std::sort(values.begin(), values.end());
                size_t medianIdx = values.size() / 2;
                output[(y * width + x) * 4 + c] = values[medianIdx];
            }
        }
    }
}

void MedianFilter::applyGPU(const std::vector<unsigned char>& input,
                           std::vector<unsigned char>& output,
                           int width, int height) {
    // GPU implementation would go here
    applyCPU(input, output, width, height);
}

bool MedianFilter::initializeGPU() {
    return false;
}

void MedianFilter::cleanupGPU() {
    // GPU cleanup
}

// Sobel Filter Implementation
SobelFilter::SobelFilter(float threshold, bool grayscale) :
    threshold_(threshold), grayscale_(grayscale) {
}

SobelFilter::~SobelFilter() {
    cleanupGPU();
}

void SobelFilter::apply(const std::vector<unsigned char>& input,
                       std::vector<unsigned char>& output,
                       int width, int height) {
    if (initializeGPU()) {
        applyGPU(input, output, width, height);
    } else {
        applyCPU(input, output, width, height);
    }
}

const char* SobelFilter::getName() const {
    return "Sobel";
}

void SobelFilter::setParameter(const std::string& name, float value) {
    if (name == "threshold") {
        threshold_ = std::max(0.0f, std::min(1.0f, value));
    }
}

float SobelFilter::getParameter(const std::string& name) const {
    if (name == "threshold") {
        return threshold_;
    }
    return 0.0f;
}

void SobelFilter::applyCPU(const std::vector<unsigned char>& input,
                          std::vector<unsigned char>& output,
                          int width, int height) {
    output.resize(input.size());

    // Sobel kernels
    const int Gx[3][3] = {
        {-1, 0, 1},
        {-2, 0, 2},
        {-1, 0, 1}
    };

    const int Gy[3][3] = {
        {-1, -2, -1},
        {0, 0, 0},
        {1, 2, 1}
    };

    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            // Convert to grayscale if needed
            float gray = grayscale_ ?
                (input[(y * width + x) * 4] * 0.299f +
                 input[(y * width + x) * 4 + 1] * 0.587f +
                 input[(y * width + x) * 4 + 2] * 0.114f) / 255.0f :
                (input[(y * width + x) * 4] / 255.0f); // Use red channel

            // Apply Sobel operator
            float sumX = 0.0f;
            float sumY = 0.0f;

            for (int ky = -1; ky <= 1; ++ky) {
                for (int kx = -1; kx <= 1; ++kx) {
                    int px = x + kx;
                    int py = y + ky;

                    float pixel = grayscale_ ?
                        (input[(py * width + px) * 4] * 0.299f +
                         input[(py * width + px) * 4 + 1] * 0.587f +
                         input[(py * width + px) * 4 + 2] * 0.114f) / 255.0f :
                        (input[(py * width + px) * 4] / 255.0f);

                    sumX += pixel * Gx[ky + 1][kx + 1];
                    sumY += pixel * Gy[ky + 1][kx + 1];
                }
            }

            float magnitude = sqrtf(sumX * sumX + sumY * sumY);
            unsigned char edge = (magnitude > threshold_) ? 255 : 0;

            int idx = (y * width + x) * 4;
            output[idx] = edge;     // R
            output[idx + 1] = edge; // G
            output[idx + 2] = edge; // B
            output[idx + 3] = 255;  // A
        }
    }

    // Handle borders (copy original)
    for (int x = 0; x < width; ++x) {
        int idx = x * 4;
        output[idx] = input[idx];
        output[idx + 1] = input[idx + 1];
        output[idx + 2] = input[idx + 2];
        output[idx + 3] = input[idx + 3];

        idx = ((height - 1) * width + x) * 4;
        output[idx] = input[idx];
        output[idx + 1] = input[idx + 1];
        output[idx + 2] = input[idx + 2];
        output[idx + 3] = input[idx + 3];
    }

    for (int y = 1; y < height - 1; ++y) {
        int idx = y * width * 4;
        output[idx] = input[idx];
        output[idx + 1] = input[idx + 1];
        output[idx + 2] = input[idx + 2];
        output[idx + 3] = input[idx + 3];

        idx = (y * width + width - 1) * 4;
        output[idx] = input[idx];
        output[idx + 1] = input[idx + 1];
        output[idx + 2] = input[idx + 2];
        output[idx + 3] = input[idx + 3];
    }
}

void SobelFilter::applyGPU(const std::vector<unsigned char>& input,
                          std::vector<unsigned char>& output,
                          int width, int height) {
    // GPU implementation would go here
    applyCPU(input, output, width, height);
}

bool SobelFilter::initializeGPU() {
    return false;
}

void SobelFilter::cleanupGPU() {
    // GPU cleanup
}

// Bloom Filter Implementation
BloomFilter::BloomFilter(float threshold, float intensity, float radius) :
    threshold_(threshold), intensity_(intensity), radius_(radius), passes_(4) {
}

BloomFilter::~BloomFilter() {
    cleanupGPU();
}

void BloomFilter::apply(const std::vector<unsigned char>& input,
                       std::vector<unsigned char>& output,
                       int width, int height) {
    if (initializeGPU()) {
        applyGPU(input, output, width, height);
    } else {
        applyCPU(input, output, width, height);
    }
}

const char* BloomFilter::getName() const {
    return "Bloom";
}

void BloomFilter::setParameter(const std::string& name, float value) {
    if (name == "threshold") {
        threshold_ = std::max(0.0f, std::min(1.0f, value));
    } else if (name == "intensity") {
        intensity_ = std::max(0.0f, value);
    } else if (name == "radius") {
        radius_ = std::max(0.1f, value);
    }
}

float BloomFilter::getParameter(const std::string& name) const {
    if (name == "threshold") return threshold_;
    if (name == "intensity") return intensity_;
    if (name == "radius") return radius_;
    return 0.0f;
}

void BloomFilter::applyCPU(const std::vector<unsigned char>& input,
                          std::vector<unsigned char>& output,
                          int width, int height) {
    output.resize(input.size());

    // Extract bright areas
    std::vector<unsigned char> brightAreas(input.size(), 0);
    for (size_t i = 0; i < input.size(); i += 4) {
        float r = input[i] / 255.0f;
        float g = input[i + 1] / 255.0f;
        float b = input[i + 2] / 255.0f;

        float luminance = r * 0.299f + g * 0.587f + b * 0.114f;
        if (luminance > threshold_) {
            brightAreas[i] = input[i];
            brightAreas[i + 1] = input[i + 1];
            brightAreas[i + 2] = input[i + 2];
            brightAreas[i + 3] = 255;
        }
    }

    // Apply Gaussian blur to bright areas
    GaussianBlurFilter blur(radius_ / 4.0f);
    std::vector<unsigned char> blurred(width * height * 4);
    blur.apply(brightAreas, blurred, width, height);

    // Apply multiple blur passes for better quality
    for (int pass = 1; pass < passes_; ++pass) {
        blur.apply(blurred, blurred, width, height);
    }

    // Combine original with bloom
    for (size_t i = 0; i < input.size(); i += 4) {
        float origR = input[i] / 255.0f;
        float origG = input[i + 1] / 255.0f;
        float origB = input[i + 2] / 255.0f;

        float bloomR = blurred[i] / 255.0f * intensity_;
        float bloomG = blurred[i + 1] / 255.0f * intensity_;
        float bloomB = blurred[i + 2] / 255.0f * intensity_;

        float finalR = std::min(1.0f, origR + bloomR);
        float finalG = std::min(1.0f, origG + bloomG);
        float finalB = std::min(1.0f, origB + bloomB);

        output[i] = static_cast<unsigned char>(finalR * 255.0f);
        output[i + 1] = static_cast<unsigned char>(finalG * 255.0f);
        output[i + 2] = static_cast<unsigned char>(finalB * 255.0f);
        output[i + 3] = input[i + 3];
    }
}

void BloomFilter::applyGPU(const std::vector<unsigned char>& input,
                          std::vector<unsigned char>& output,
                          int width, int height) {
    // GPU implementation would go here
    applyCPU(input, output, width, height);
}

bool BloomFilter::initializeGPU() {
    return false;
}

void BloomFilter::cleanupGPU() {
    // GPU cleanup
}

// Tone Mapping Filter Implementation
ToneMappingFilter::ToneMappingFilter(float exposure, float gamma, bool useACES) :
    exposure_(exposure), gamma_(gamma), useACES_(useACES) {
}

ToneMappingFilter::~ToneMappingFilter() {
    cleanupGPU();
}

void ToneMappingFilter::apply(const std::vector<unsigned char>& input,
                             std::vector<unsigned char>& output,
                             int width, int height) {
    if (initializeGPU()) {
        applyGPU(input, output, width, height);
    } else {
        applyCPU(input, output, width, height);
    }
}

const char* ToneMappingFilter::getName() const {
    return "ToneMapping";
}

void ToneMappingFilter::setParameter(const std::string& name, float value) {
    if (name == "exposure") {
        exposure_ = value;
    } else if (name == "gamma") {
        gamma_ = std::max(0.1f, value);
    }
}

float ToneMappingFilter::getParameter(const std::string& name) const {
    if (name == "exposure") return exposure_;
    if (name == "gamma") return gamma_;
    return 0.0f;
}

void ToneMappingFilter::applyCPU(const std::vector<unsigned char>& input,
                                std::vector<unsigned char>& output,
                                int width, int height) {
    output.resize(input.size());

    for (size_t i = 0; i < input.size(); i += 4) {
        // Convert to linear RGB (assuming input is sRGB)
        float r = powf(input[i] / 255.0f, 2.2f);
        float g = powf(input[i + 1] / 255.0f, 2.2f);
        float b = powf(input[i + 2] / 255.0f, 2.2f);

        // Apply exposure
        r *= powf(2.0f, exposure_);
        g *= powf(2.0f, exposure_);
        b *= powf(2.0f, exposure_);

        // Apply tone mapping
        if (useACES_) {
            r = acesToneMapping(r);
            g = acesToneMapping(g);
            b = acesToneMapping(b);
        } else {
            // Reinhard tone mapping
            r = r / (1.0f + r);
            g = g / (1.0f + g);
            b = b / (1.0f + b);
        }

        // Apply gamma correction
        r = powf(r, 1.0f / gamma_);
        g = powf(g, 1.0f / gamma_);
        b = powf(b, 1.0f / gamma_);

        // Convert back to sRGB
        output[i] = static_cast<unsigned char>(std::min(255.0f, powf(r, 1.0f / 2.2f) * 255.0f));
        output[i + 1] = static_cast<unsigned char>(std::min(255.0f, powf(g, 1.0f / 2.2f) * 255.0f));
        output[i + 2] = static_cast<unsigned char>(std::min(255.0f, powf(b, 1.0f / 2.2f) * 255.0f));
        output[i + 3] = input[i + 3];
    }
}

float ToneMappingFilter::acesToneMapping(float x) const {
    // ACES filmic tone mapping curve
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;

    return (x * (a * x + b)) / (x * (c * x + d) + e);
}

void ToneMappingFilter::applyGPU(const std::vector<unsigned char>& input,
                                std::vector<unsigned char>& output,
                                int width, int height) {
    // GPU implementation would go here
    applyCPU(input, output, width, height);
}

bool ToneMappingFilter::initializeGPU() {
    return false;
}

void ToneMappingFilter::cleanupGPU() {
    // GPU cleanup
}

// Chromatic Aberration Filter Implementation
ChromaticAberrationFilter::ChromaticAberrationFilter(float intensity, const Vector2& center) :
    intensity_(intensity), center_(center) {
}

ChromaticAberrationFilter::~ChromaticAberrationFilter() {
    cleanupGPU();
}

void ChromaticAberrationFilter::apply(const std::vector<unsigned char>& input,
                                    std::vector<unsigned char>& output,
                                    int width, int height) {
    if (initializeGPU()) {
        applyGPU(input, output, width, height);
    } else {
        applyCPU(input, output, width, height);
    }
}

const char* ChromaticAberrationFilter::getName() const {
    return "ChromaticAberration";
}

void ChromaticAberrationFilter::setParameter(const std::string& name, float value) {
    if (name == "intensity") {
        intensity_ = std::max(0.0f, value);
    }
}

float ChromaticAberrationFilter::getParameter(const std::string& name) const {
    if (name == "intensity") return intensity_;
    return 0.0f;
}

void ChromaticAberrationFilter::applyCPU(const std::vector<unsigned char>& input,
                                       std::vector<unsigned char>& output,
                                       int width, int height) {
    output.resize(input.size());

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Calculate direction from center
            Vector2 pos(static_cast<float>(x) / width, static_cast<float>(y) / height);
            Vector2 dir = pos - center_;
            float distance = dir.length();

            // Calculate aberration amount
            float aberration = distance * intensity_;

            // Sample red channel (shift towards center)
            Vector2 redPos = pos - dir * aberration * 0.01f;
            int redX = static_cast<int>(redPos.x * width);
            int redY = static_cast<int>(redPos.y * height);
            redX = std::max(0, std::min(width - 1, redX));
            redY = std::max(0, std::min(height - 1, redY));

            // Sample blue channel (shift away from center)
            Vector2 bluePos = pos + dir * aberration * 0.01f;
            int blueX = static_cast<int>(bluePos.x * width);
            int blueY = static_cast<int>(bluePos.y * height);
            blueX = std::max(0, std::min(width - 1, blueX));
            blueY = std::max(0, std::min(height - 1, blueY));

            int idx = (y * width + x) * 4;
            int redIdx = (redY * width + redX) * 4;
            int blueIdx = (blueY * width + blueX) * 4;

            output[idx] = input[redIdx];       // R
            output[idx + 1] = input[idx + 1];  // G (no shift)
            output[idx + 2] = input[blueIdx + 2]; // B
            output[idx + 3] = input[idx + 3];  // A
        }
    }
}

void ChromaticAberrationFilter::applyGPU(const std::vector<unsigned char>& input,
                                       std::vector<unsigned char>& output,
                                       int width, int height) {
    // GPU implementation would go here
    applyCPU(input, output, width, height);
}

bool ChromaticAberrationFilter::initializeGPU() {
    return false;
}

void ChromaticAberrationFilter::cleanupGPU() {
    // GPU cleanup
}

// Filter Chain Implementation
FilterChain::FilterChain() {
}

FilterChain::~FilterChain() {
    clear();
}

void FilterChain::addFilter(std::unique_ptr<ImageFilter> filter) {
    filters_.push_back(std::move(filter));
}

void FilterChain::removeFilter(size_t index) {
    if (index < filters_.size()) {
        filters_.erase(filters_.begin() + index);
    }
}

void FilterChain::apply(const std::vector<unsigned char>& input,
                       std::vector<unsigned char>& output,
                       int width, int height) {
    if (filters_.empty()) {
        output = input;
        return;
    }

    // Resize temp buffers if needed
    size_t bufferSize = width * height * 4;
    if (tempBuffer1_.size() != bufferSize) {
        tempBuffer1_.resize(bufferSize);
        tempBuffer2_.resize(bufferSize);
    }

    // Apply first filter
    filters_[0]->apply(input, tempBuffer1_, width, height);

    // Apply remaining filters
    for (size_t i = 1; i < filters_.size(); ++i) {
        bool useTemp1AsOutput = (i % 2 == 0);
        const std::vector<unsigned char>& currentInput = useTemp1AsOutput ? tempBuffer2_ : tempBuffer1_;
        std::vector<unsigned char>& currentOutput = useTemp1AsOutput ? tempBuffer1_ : tempBuffer2_;

        filters_[i]->apply(currentInput, currentOutput, width, height);
    }

    // Copy final result to output
    bool finalInTemp1 = ((filters_.size() - 1) % 2 == 0);
    output = finalInTemp1 ? tempBuffer1_ : tempBuffer2_;
}

void FilterChain::clear() {
    filters_.clear();
    tempBuffer1_.clear();
    tempBuffer2_.clear();
}

ImageFilter* FilterChain::getFilter(size_t index) const {
    if (index < filters_.size()) {
        return filters_[index].get();
    }
    return nullptr;
}

// FXAA Filter Implementation
FXAAFilter::FXAAFilter(float quality, float threshold) :
    quality_(quality), threshold_(threshold) {
}

FXAAFilter::~FXAAFilter() {
    cleanupGPU();
}

void FXAAFilter::apply(const std::vector<unsigned char>& input,
                      std::vector<unsigned char>& output,
                      int width, int height) {
    if (initializeGPU()) {
        applyGPU(input, output, width, height);
    } else {
        applyCPU(input, output, width, height);
    }
}

const char* FXAAFilter::getName() const {
    return "FXAA";
}

void FXAAFilter::setParameter(const std::string& name, float value) {
    if (name == "quality") {
        quality_ = std::max(0.0f, std::min(1.0f, value));
    } else if (name == "threshold") {
        threshold_ = std::max(0.0f, std::min(1.0f, value));
    }
}

float FXAAFilter::getParameter(const std::string& name) const {
    if (name == "quality") return quality_;
    if (name == "threshold") return threshold_;
    return 0.0f;
}

void FXAAFilter::applyCPU(const std::vector<unsigned char>& input,
                         std::vector<unsigned char>& output,
                         int width, int height) {
    output.resize(input.size());

    // FXAA implementation - Fast Approximate Anti-Aliasing
    // This is a simplified version focusing on edge detection and smoothing

    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            int idx = (y * width + x) * 4;

            // Convert to luminance for edge detection
            float lumCenter = (input[idx] * 0.299f + input[idx + 1] * 0.587f + input[idx + 2] * 0.114f) / 255.0f;

            // Sample neighboring pixels
            float lumN = (input[((y-1) * width + x) * 4] * 0.299f + input[((y-1) * width + x) * 4 + 1] * 0.587f + input[((y-1) * width + x) * 4 + 2] * 0.114f) / 255.0f;
            float lumS = (input[((y+1) * width + x) * 4] * 0.299f + input[((y+1) * width + x) * 4 + 1] * 0.587f + input[((y+1) * width + x) * 4 + 2] * 0.114f) / 255.0f;
            float lumE = (input[(y * width + x + 1) * 4] * 0.299f + input[(y * width + x + 1) * 4 + 1] * 0.587f + input[(y * width + x + 1) * 4 + 2] * 0.114f) / 255.0f;
            float lumW = (input[(y * width + x - 1) * 4] * 0.299f + input[(y * width + x - 1) * 4 + 1] * 0.587f + input[(y * width + x - 1) * 4 + 2] * 0.114f) / 255.0f;

            // Calculate gradients
            float gradN = fabs(lumCenter - lumN);
            float gradS = fabs(lumCenter - lumS);
            float gradE = fabs(lumCenter - lumE);
            float gradW = fabs(lumCenter - lumW);

            // Find maximum gradient direction
            float maxGrad = std::max({gradN, gradS, gradE, gradW});

            if (maxGrad > threshold_) {
                // This is an edge pixel - apply smoothing
                float blendFactor = std::min(quality_, maxGrad / threshold_);

                // Simple edge-directed smoothing
                float smoothedLum = lumCenter * (1.0f - blendFactor) +
                                  (lumN + lumS + lumE + lumW) * 0.25f * blendFactor;

                // Apply smoothing to RGB channels proportionally
                float ratio = smoothedLum / lumCenter;
                if (lumCenter > 0.001f) { // Avoid division by zero
                    output[idx] = static_cast<unsigned char>(std::min(255.0f, input[idx] * ratio));
                    output[idx + 1] = static_cast<unsigned char>(std::min(255.0f, input[idx + 1] * ratio));
                    output[idx + 2] = static_cast<unsigned char>(std::min(255.0f, input[idx + 2] * ratio));
                } else {
                    output[idx] = input[idx];
                    output[idx + 1] = input[idx + 1];
                    output[idx + 2] = input[idx + 2];
                }
            } else {
                // Not an edge - copy original
                output[idx] = input[idx];
                output[idx + 1] = input[idx + 1];
                output[idx + 2] = input[idx + 2];
            }

            output[idx + 3] = input[idx + 3]; // Alpha channel unchanged
        }
    }

    // Handle borders - copy original pixels
    for (int x = 0; x < width; ++x) {
        // Top row
        int idx = x * 4;
        output[idx] = input[idx];
        output[idx + 1] = input[idx + 1];
        output[idx + 2] = input[idx + 2];
        output[idx + 3] = input[idx + 3];

        // Bottom row
        idx = ((height - 1) * width + x) * 4;
        output[idx] = input[idx];
        output[idx + 1] = input[idx + 1];
        output[idx + 2] = input[idx + 2];
        output[idx + 3] = input[idx + 3];
    }

    for (int y = 1; y < height - 1; ++y) {
        // Left column
        int idx = (y * width) * 4;
        output[idx] = input[idx];
        output[idx + 1] = input[idx + 1];
        output[idx + 2] = input[idx + 2];
        output[idx + 3] = input[idx + 3];

        // Right column
        idx = (y * width + width - 1) * 4;
        output[idx] = input[idx];
        output[idx + 1] = input[idx + 1];
        output[idx + 2] = input[idx + 2];
        output[idx + 3] = input[idx + 3];
    }
}

void FXAAFilter::applyGPU(const std::vector<unsigned char>& input,
                         std::vector<unsigned char>& output,
                         int width, int height) {
    // GPU implementation would go here
    applyCPU(input, output, width, height);
}

bool FXAAFilter::initializeGPU() {
    return false;
}

void FXAAFilter::cleanupGPU() {
    // GPU cleanup
}

} // namespace Foundry