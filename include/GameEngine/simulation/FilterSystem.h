/**
 * @file FilterSystem.h
 * @brief Advanced image filtering and post-processing system
 */

#pragma once

#include "../math/Vector2.h"
#include "../graphics/Renderer.h"
#include <vector>
#include <memory>
#include <functional>

namespace Foundry {

/**
 * @brief Image filter base class
 * Time Complexity: O(width * height) per frame
 * Space Complexity: O(width * height) for intermediate buffers
 */
class ImageFilter {
public:
    virtual ~ImageFilter() = default;

    /**
     * Apply filter to image data
     * @param input RGBA image data
     * @param output Filtered RGBA image data
     * @param width Image width
     * @param height Image height
     */
    virtual void apply(const std::vector<unsigned char>& input,
                      std::vector<unsigned char>& output,
                      int width, int height) = 0;

    /**
     * Get filter name
     */
    virtual const char* getName() const = 0;

    /**
     * Set filter parameter
     */
    virtual void setParameter(const std::string& name, float value) {}

    /**
     * Get filter parameter
     */
    virtual float getParameter(const std::string& name) const { return 0.0f; }
};

/**
 * @brief Gaussian blur filter
 * Time Complexity: O(width * height * kernel_size)
 * Space Complexity: O(width * height)
 */
class GaussianBlurFilter : public ImageFilter {
private:
    float sigma_;
    int kernelSize_;

    // Precomputed kernel
    std::vector<float> kernel_;
    bool kernelDirty_;

    // GPU compute resources
    void* gpuKernelBuffer_ = nullptr;
    void* gpuInputBuffer_ = nullptr;
    void* gpuOutputBuffer_ = nullptr;
    void* gpuComputePipeline_ = nullptr;

public:
    GaussianBlurFilter(float sigma = 1.0f);
    ~GaussianBlurFilter();

    void apply(const std::vector<unsigned char>& input,
              std::vector<unsigned char>& output,
              int width, int height) override;

    const char* getName() const override { return "GaussianBlur"; }

    void setParameter(const std::string& name, float value) override;
    float getParameter(const std::string& name) const override;

private:
    void updateKernel();
    void applyCPU(const std::vector<unsigned char>& input,
                 std::vector<unsigned char>& output,
                 int width, int height);
    void applyGPU(const std::vector<unsigned char>& input,
                 std::vector<unsigned char>& output,
                 int width, int height);

    bool initializeGPU();
    void cleanupGPU();
};

/**
 * @brief Bilateral filter for edge-preserving smoothing
 * Time Complexity: O(width * height * kernel_size)
 * Space Complexity: O(width * height)
 */
class BilateralFilter : public ImageFilter {
private:
    float sigmaSpatial_;
    float sigmaRange_;
    int kernelSize_;

    // GPU compute resources
    void* gpuInputBuffer_ = nullptr;
    void* gpuOutputBuffer_ = nullptr;
    void* gpuComputePipeline_ = nullptr;

public:
    BilateralFilter(float sigmaSpatial = 1.0f, float sigmaRange = 0.1f);
    ~BilateralFilter();

    void apply(const std::vector<unsigned char>& input,
              std::vector<unsigned char>& output,
              int width, int height) override;

    const char* getName() const override { return "Bilateral"; }

    void setParameter(const std::string& name, float value) override;
    float getParameter(const std::string& name) const override;

private:
    void applyCPU(const std::vector<unsigned char>& input,
                 std::vector<unsigned char>& output,
                 int width, int height);
    void applyGPU(const std::vector<unsigned char>& input,
                 std::vector<unsigned char>& output,
                 int width, int height);

    bool initializeGPU();
    void cleanupGPU();
};

/**
 * @brief Median filter for noise reduction
 * Time Complexity: O(width * height * kernel_size * log(kernel_size))
 * Space Complexity: O(width * height)
 */
class MedianFilter : public ImageFilter {
private:
    int kernelSize_;

    // GPU compute resources
    void* gpuInputBuffer_ = nullptr;
    void* gpuOutputBuffer_ = nullptr;
    void* gpuComputePipeline_ = nullptr;

public:
    MedianFilter(int kernelSize = 3);
    ~MedianFilter();

    void apply(const std::vector<unsigned char>& input,
              std::vector<unsigned char>& output,
              int width, int height) override;

    const char* getName() const override { return "Median"; }

    void setParameter(const std::string& name, float value) override;
    float getParameter(const std::string& name) const override;

private:
    void applyCPU(const std::vector<unsigned char>& input,
                 std::vector<unsigned char>& output,
                 int width, int height);
    void applyGPU(const std::vector<unsigned char>& input,
                 std::vector<unsigned char>& output,
                 int width, int height);

    bool initializeGPU();
    void cleanupGPU();
};

/**
 * @brief Sobel edge detection filter
 * Time Complexity: O(width * height)
 * Space Complexity: O(width * height)
 */
class SobelFilter : public ImageFilter {
private:
    float threshold_;
    bool grayscale_;

    // GPU compute resources
    void* gpuInputBuffer_ = nullptr;
    void* gpuOutputBuffer_ = nullptr;
    void* gpuComputePipeline_ = nullptr;

public:
    SobelFilter(float threshold = 0.1f, bool grayscale = true);
    ~SobelFilter();

    void apply(const std::vector<unsigned char>& input,
              std::vector<unsigned char>& output,
              int width, int height) override;

    const char* getName() const override { return "Sobel"; }

    void setParameter(const std::string& name, float value) override;
    float getParameter(const std::string& name) const override;

private:
    void applyCPU(const std::vector<unsigned char>& input,
                 std::vector<unsigned char>& output,
                 int width, int height);
    void applyGPU(const std::vector<unsigned char>& input,
                 std::vector<unsigned char>& output,
                 int width, int height);

    bool initializeGPU();
    void cleanupGPU();
};

/**
 * @brief Bloom effect filter
 * Time Complexity: O(width * height * log(width * height))
 * Space Complexity: O(width * height)
 */
class BloomFilter : public ImageFilter {
private:
    float threshold_;
    float intensity_;
    float radius_;
    int passes_;

    // GPU compute resources
    void* gpuInputBuffer_ = nullptr;
    void* gpuTempBuffers_[4] = {nullptr, nullptr, nullptr, nullptr};
    void* gpuOutputBuffer_ = nullptr;
    void* gpuComputePipeline_ = nullptr;

public:
    BloomFilter(float threshold = 0.8f, float intensity = 1.0f, float radius = 5.0f);
    ~BloomFilter();

    void apply(const std::vector<unsigned char>& input,
              std::vector<unsigned char>& output,
              int width, int height) override;

    const char* getName() const override { return "Bloom"; }

    void setParameter(const std::string& name, float value) override;
    float getParameter(const std::string& name) const override;

private:
    void applyCPU(const std::vector<unsigned char>& input,
                 std::vector<unsigned char>& output,
                 int width, int height);
    void applyGPU(const std::vector<unsigned char>& input,
                 std::vector<unsigned char>& output,
                 int width, int height);

    bool initializeGPU();
    void cleanupGPU();
};

/**
 * @brief Tone mapping filter for HDR
 * Time Complexity: O(width * height)
 * Space Complexity: O(width * height)
 */
class ToneMappingFilter : public ImageFilter {
private:
    float exposure_;
    float gamma_;
    bool useACES_;

    // GPU compute resources
    void* gpuInputBuffer_ = nullptr;
    void* gpuOutputBuffer_ = nullptr;
    void* gpuComputePipeline_ = nullptr;

public:
    ToneMappingFilter(float exposure = 1.0f, float gamma = 2.2f, bool useACES = true);
    ~ToneMappingFilter();

    void apply(const std::vector<unsigned char>& input,
              std::vector<unsigned char>& output,
              int width, int height) override;

    const char* getName() const override { return "ToneMapping"; }

    void setParameter(const std::string& name, float value) override;
    float getParameter(const std::string& name) const override;

private:
    void applyCPU(const std::vector<unsigned char>& input,
                 std::vector<unsigned char>& output,
                 int width, int height);
    void applyGPU(const std::vector<unsigned char>& input,
                 std::vector<unsigned char>& output,
                 int width, int height);

    float acesToneMapping(float x) const;

    bool initializeGPU();
    void cleanupGPU();
};

/**
 * @brief Chromatic aberration filter
 * Time Complexity: O(width * height)
 * Space Complexity: O(width * height)
 */
class ChromaticAberrationFilter : public ImageFilter {
private:
    float intensity_;
    Vector2 center_;

    // GPU compute resources
    void* gpuInputBuffer_ = nullptr;
    void* gpuOutputBuffer_ = nullptr;
    void* gpuComputePipeline_ = nullptr;

public:
    ChromaticAberrationFilter(float intensity = 0.01f, const Vector2& center = Vector2(0.5f, 0.5f));
    ~ChromaticAberrationFilter();

    void apply(const std::vector<unsigned char>& input,
              std::vector<unsigned char>& output,
              int width, int height) override;

    const char* getName() const override { return "ChromaticAberration"; }

    void setParameter(const std::string& name, float value) override;
    float getParameter(const std::string& name) const override;

private:
    void applyCPU(const std::vector<unsigned char>& input,
                 std::vector<unsigned char>& output,
                 int width, int height);
    void applyGPU(const std::vector<unsigned char>& input,
                 std::vector<unsigned char>& output,
                 int width, int height);

    bool initializeGPU();
    void cleanupGPU();
};

/**
 * @brief Filter chain for combining multiple effects
 * Time Complexity: Sum of all filter complexities
 * Space Complexity: O(width * height) intermediate buffers
 */
class FilterChain {
private:
    std::vector<std::unique_ptr<ImageFilter>> filters_;
    std::vector<unsigned char> tempBuffer1_;
    std::vector<unsigned char> tempBuffer2_;

public:
    FilterChain();
    ~FilterChain() = default;

    /**
     * Add filter to chain
     */
    void addFilter(std::unique_ptr<ImageFilter> filter);

    /**
     * Remove filter from chain
     */
    void removeFilter(size_t index);

    /**
     * Apply entire filter chain
     */
    void apply(const std::vector<unsigned char>& input,
              std::vector<unsigned char>& output,
              int width, int height);

    /**
     * Clear all filters
     */
    void clear();

    /**
     * Get number of filters in chain
     */
    size_t size() const { return filters_.size(); }

    /**
     * Get filter at index
     */
    ImageFilter* getFilter(size_t index) const;
};

/**
 * @brief Fast approximate anti-aliasing (FXAA)
 * Time Complexity: O(width * height)
 * Space Complexity: O(width * height)
 */
class FXAAFilter : public ImageFilter {
private:
    float quality_;
    float threshold_;

    // GPU compute resources
    void* gpuInputBuffer_ = nullptr;
    void* gpuOutputBuffer_ = nullptr;
    void* gpuComputePipeline_ = nullptr;

public:
    FXAAFilter(float quality = 1.0f, float threshold = 0.125f);
    ~FXAAFilter();

    void apply(const std::vector<unsigned char>& input,
              std::vector<unsigned char>& output,
              int width, int height) override;

    const char* getName() const override { return "FXAA"; }

    void setParameter(const std::string& name, float value) override;
    float getParameter(const std::string& name) const override;

private:
    void applyCPU(const std::vector<unsigned char>& input,
                 std::vector<unsigned char>& output,
                 int width, int height);
    void applyGPU(const std::vector<unsigned char>& input,
                 std::vector<unsigned char>& output,
                 int width, int height);

    bool initializeGPU();
    void cleanupGPU();
};

} // namespace Foundry