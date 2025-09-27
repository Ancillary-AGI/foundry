#ifndef NEUTRAL_GAMEENGINE_POST_PROCESSING_H
#define NEUTRAL_GAMEENGINE_POST_PROCESSING_H

#include <vector>
#include <memory>
#include "../../math/Vector3.h"
#include "../../math/Matrix4.h"
#include "../System.h"

namespace NeutralGameEngine {

// Tone mapping with filmic curves (ACES)
class ToneMapper {
public:
    enum class ToneMappingOperator {
        LINEAR,
        REINHARD,
        ACES_FILMIC,
        UNCHARTED2,
        CUSTOM_LUT
    };

    ToneMappingOperator operatorType = ToneMappingOperator::ACES_FILMIC;
    float exposure = 1.0f;
    float whitePoint = 11.2f; // For Reinhard

    Vector3 apply(const Vector3& hdrColor) const;

private:
    // ACES tone mapping curve
    Vector3 acesFilmic(const Vector3& x) const;

    // Uncharted 2 curve
    Vector3 uncharted2(const Vector3& x) const;
};

// 3D Color Grading LUT
class ColorGradingLUT {
public:
    ColorGradingLUT(int size = 32);

    void setLUTValue(int r, int g, int b, const Vector3& color);
    Vector3 sampleLUT(float r, float g, float b) const;

    // Load from file (simulated)
    bool loadFromFile(const std::string& filename);

    // Apply color grading
    Vector3 applyGrading(const Vector3& color) const;

private:
    int size_;
    std::vector<Vector3> lutData_;
    int getLUTIndex(int r, int g, int b) const;
    Vector3 trilinearSample(float r, float g, float b) const;
};

// Motion blur with per-object vectors
class MotionBlur {
public:
    enum class MotionBlurType {
        CAMERA_MOTION,
        OBJECT_MOTION,
        BOTH
    };

    MotionBlurType blurType = MotionBlurType::BOTH;
    int numSamples = 16;
    float blurStrength = 1.0f;

    struct MotionVector {
        Vector2 velocity;
        float depth;
    };

    void generateMotionVectors(const std::vector<Vector3>& currentPositions,
                              const std::vector<Vector3>& previousPositions,
                              std::vector<MotionVector>& motionVectors);

    Vector3 applyMotionBlur(const Vector3& originalColor,
                           const MotionVector& motion,
                           const std::vector<std::vector<Vector3>>& frameBuffer) const;

    // Camera motion blur using velocity buffer
    Vector3 cameraMotionBlur(const Vector3& color,
                            const Vector2& velocity,
                            const std::vector<std::vector<Vector3>>& frameBuffer,
                            const Vector2& texCoord) const;
};

// Depth of Field with bokeh simulation
class DepthOfField {
public:
    float focalDistance = 10.0f;
    float focalLength = 50.0f;    // in mm
    float fStop = 2.8f;           // Aperture
    int apertureBlades = 6;       // For bokeh shape

    struct CoC { // Circle of Confusion
        float radius;
        Vector2 center;
    };

    // Compute circle of confusion
    CoC computeCoC(float depth, float focalDepth) const;

    // Bokeh simulation with hexagonal aperture
    Vector3 simulateBokeh(const Vector3& color, float cocRadius, const Vector2& position) const;

    // Apply depth of field blur
    Vector3 applyDOF(const Vector3& originalColor,
                    const std::vector<std::vector<Vector3>>& frameBuffer,
                    const std::vector<std::vector<float>>& depthBuffer,
                    const Vector2& texCoord) const;

    // Physically based bokeh using Fourier optics
    Vector3 physicalBokeh(const Vector3& color,
                         const Vector2& offset,
                         float cocRadius,
                         const std::vector<float>& apertureWeights) const;

private:
    float sensorSize = 36.0f; // 35mm equivalent

    // Generate aperture samples for physically based bokeh
    std::vector<Vector2> generateApertureSamples(int blades, int samples) const;
};

// Screen Space Ambient Occlusion (SSAO/HBAO/GTAO)
class SSAO {
public:
    enum class SSAOType {
        BASIC_SSAO,
        HBAO,          // Horizon-Based Ambient Occlusion
        GTAO           // Ground Truth Ambient Occlusion
    };

    SSAOType aoType = SSAOType::HBAO;
    int kernelSize = 64;
    float radius = 0.5f;
    float bias = 0.025f;
    float intensity = 1.0f;

    // Generate noise texture for randomization
    std::vector<Vector2> generateNoiseTexture(int width, int height);

    // Compute ambient occlusion factor
    float computeAO(const Vector2& texCoord,
                   const std::vector<std::vector<float>>& depthBuffer,
                   const std::vector<std::vector<Vector3>>& normalBuffer,
                   const Matrix4& projectionMatrix) const;

    // Horizon-Based AO (screen space)
    float HBAO(const Vector2& texCoord,
              const std::vector<std::vector<float>>& depthBuffer,
              const Vector3& viewNormal,
              const Matrix4& projectionMatrix) const;

    // Ground Truth AO approximation
    float GTAO(const Vector2& texCoord,
              const Vector2& screenSpaceNormal,
              const std::vector<std::vector<float>>& depthBuffer,
              const Matrix4& projectionMatrix) const;

    // Bilateral blur for denoising
    std::vector<std::vector<float>> bilateralBlur(const std::vector<std::vector<float>>& aoBuffer,
                                                 const std::vector<std::vector<float>>& depthBuffer,
                                                 float sigmaSpatial = 1.0f,
                                                 float sigmaRange = 0.1f) const;

private:
    // Hemisphere sampling
    std::vector<Vector3> generateHemisphereSamples(int samples) const;

    // Screen space to view space conversion
    Vector3 screenToViewSpace(const Vector2& screenPos, float depth, const Matrix4& projectionInverse) const;
};

// FidelityFX Super Resolution (FSR)/AMD FidelityFX Super Resolution
class FSR {
public:
    struct FSRConstants {
        Vector2 inputSize;
        Vector2 outputSize;
        float sharpness = 0.0f;
        float exposure = 1.0f;
    };

    Vector3 upscale(const std::vector<std::vector<Vector3>>& inputFrame,
                   const FSRConstants& constants) const;

    // Edge adaptive sharpening
    Vector3 EAS(const Vector3& color,
               const std::vector<std::vector<Vector3>>& frameBuffer,
               const Vector2& texCoord,
               float sharpness) const;

    // Robust contrast adaptive sharpening (RCAS)
    Vector3 RCAS(const Vector3& color,
                const std::vector<std::vector<Vector3>>& frameBuffer,
                const Vector2& texCoord,
                float sharpness) const;

private:
    // Luminance extraction
    float luminance(const Vector3& color) const;
    Vector3 contrastAdaptiveSharpening(const Vector3& color,
                                      const Vector3& neighbor1,
                                      const Vector3& neighbor2,
                                      float sharpness) const;
};

// NVIDIA DLSS (Deep Learning Super Sampling) simulation
class DLSS {
public:
    struct DLSSContext {
        Vector2 inputSize;
        Vector2 outputSize;
        float quality = 1.0f; // 0.5 = ultra quality, 2.0 = ultra performance
    };

    Vector3 applyDLSS(const std::vector<std::vector<Vector3>>& inputFrame,
                     const std::vector<std::vector<Vector3>>& motionVectors,
                     const std::vector<std::vector<float>>& depthBuffer,
                     const DLSSContext& context) const;

private:
    // Neural network approximation (simplified)
    Vector3 neuralUpsample(const Vector3& center,
                          const std::vector<Vector3>& neighbors,
                          const Vector2& motion,
                          float depth) const;
};

// Post-processing pipeline orchestrator
class PostProcessingPipeline {
public:
    // Pipeline stages
    struct PipelineStage {
        std::string name;
        std::function<Vector3(const Vector3&, const Vector2&)> process;
        bool enabled = true;
    };

    std::vector<PipelineStage> stages;

    // Add stage to pipeline
    void addStage(const std::string& name,
                 std::function<Vector3(const Vector3&, const Vector2&)> processor,
                 bool enabled = true);

    // Process frame through pipeline
    std::vector<std::vector<Vector3>> processFrame(const std::vector<std::vector<Vector3>>& input,
                                                  const std::vector<std::vector<float>>& depthBuffer = {});

    // Built-in pipeline presets
    void createUnrealStylePipeline();
    void createPhotographicPipeline();
    void createGameStylePipeline();
};

} // namespace NeutralGameEngine

#endif // NEUTRAL_GAMEENGINE_POST_PROCESSING_H
