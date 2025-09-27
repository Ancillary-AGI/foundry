#ifndef FOUNDRYENGINE_GPU_DEBUGGER_H
#define FOUNDRYENGINE_GPU_DEBUGGER_H

#include "../../core/System.h"
#include <jni.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>
#include <chrono>
#include <fstream>

namespace FoundryEngine {

// Forward declarations
class GPUDebugger;
class RenderDocManager;
class PerfettoManager;
class MaliGraphicsDebugger;
class AdrenoProfiler;
class FrameCaptureManager;
class PerformanceProfiler;
class MemoryTracker;
class ShaderDebugger;

// Debug capture types
enum class CaptureType {
    FRAME_CAPTURE,      // Single frame capture
    PERFORMANCE_TRACE,  // Performance profiling trace
    MEMORY_SNAPSHOT,    // Memory usage snapshot
    SHADER_DEBUG,       // Shader debugging session
    RENDER_PASS,        // Render pass capture
    COMPUTE_DISPATCH    // Compute shader dispatch capture
};

// Debug output formats
enum class DebugOutputFormat {
    RENDERDOC_RDC,      // RenderDoc native format
    JSON,               // JSON format for analysis
    HTML,               // HTML report format
    CSV,                // CSV data format
    BINARY              // Binary capture data
};

// GPU vendor types
enum class GPUVendor {
    QUALCOMM_ADRENO,    // Qualcomm Adreno GPU
    ARM_MALI,          // ARM Mali GPU
    NVIDIA_TEGRA,      // NVIDIA Tegra GPU
    IMAGINATION,       // Imagination PowerVR GPU
    APPLE,             // Apple GPU
    INTEL,             // Intel GPU
    UNKNOWN            // Unknown GPU vendor
};

// Performance counter types
enum class PerformanceCounter {
    GPU_CYCLES,         // GPU clock cycles
    VERTEX_COUNT,       // Vertices processed
    PRIMITIVE_COUNT,    // Primitives processed
    FRAGMENT_COUNT,     // Fragments processed
    TEXTURE_FETCHES,    // Texture fetch operations
    MEMORY_READS,       // Memory read operations
    MEMORY_WRITES,      // Memory write operations
    CACHE_HITS,         // Cache hit count
    CACHE_MISSES,       // Cache miss count
    BANDWIDTH_USAGE,    // Memory bandwidth usage
    POWER_CONSUMPTION,  // GPU power consumption
    TEMPERATURE,        // GPU temperature
    FREQUENCY,          // GPU frequency
    UTILIZATION         // GPU utilization percentage
};

// Frame capture settings
struct FrameCaptureSettings {
    bool captureCallstack;
    bool captureShaders;
    bool captureTextures;
    bool captureBuffers;
    bool capturePipelineState;
    bool captureRenderTargets;
    int maxTextureDimension;
    int maxBufferSize;
    std::string outputDirectory;
    DebugOutputFormat outputFormat;
    bool compressOutput;
    bool includePerformanceCounters;
    std::vector<PerformanceCounter> enabledCounters;
};

// Performance profiling settings
struct ProfilingSettings {
    bool enableGPUCounters;
    bool enableCPUCounters;
    bool enableMemoryCounters;
    bool enablePowerCounters;
    bool enableThermalCounters;
    int samplingIntervalMs;
    int bufferSize;
    std::string outputFile;
    bool realTimeDisplay;
    bool trackFrameTime;
    bool trackDrawCalls;
    bool trackMemoryAllocations;
    bool trackResourceBindings;
};

// Memory tracking settings
struct MemoryTrackingSettings {
    bool trackAllocations;
    bool trackDeallocations;
    bool trackLeaks;
    bool trackFragmentation;
    size_t allocationThreshold;
    std::string logFile;
    bool generateReport;
    int reportIntervalSeconds;
    bool trackGPUMemory;
    bool trackSystemMemory;
    bool trackTextureMemory;
    bool trackBufferMemory;
};

// Shader debugging settings
struct ShaderDebugSettings {
    bool enableSourceDebug;
    bool enableBinaryDebug;
    bool enableOptimizationAnalysis;
    bool enablePerformanceHints;
    std::string shaderSourcePath;
    std::string binaryOutputPath;
    bool generateDisassembly;
    bool validateSPIRV;
    bool enableWatchVariables;
    std::vector<std::string> watchVariables;
};

// Debug event types
enum class DebugEventType {
    FRAME_START,
    FRAME_END,
    DRAW_CALL,
    COMPUTE_DISPATCH,
    RESOURCE_BIND,
    SHADER_COMPILE,
    TEXTURE_LOAD,
    BUFFER_UPDATE,
    RENDER_PASS_START,
    RENDER_PASS_END,
    MEMORY_ALLOCATION,
    MEMORY_DEALLOCATION,
    PERFORMANCE_MARKER,
    ERROR,
    WARNING,
    INFO
};

// Debug event data
struct DebugEvent {
    DebugEventType type;
    std::string name;
    std::string description;
    uint64_t timestamp;
    std::unordered_map<std::string, std::string> metadata;
    std::vector<uint8_t> binaryData;
};

// Performance metrics
struct PerformanceMetrics {
    float frameTimeMs;
    float gpuTimeMs;
    float cpuTimeMs;
    int drawCallCount;
    int triangleCount;
    int vertexCount;
    size_t memoryUsageBytes;
    float gpuUtilizationPercent;
    float memoryBandwidthMBps;
    float powerConsumptionWatts;
    float temperatureCelsius;
    int shaderSwitches;
    int textureBinds;
    int bufferBinds;
    std::unordered_map<PerformanceCounter, float> customCounters;
};

// Memory allocation info
struct MemoryAllocation {
    void* address;
    size_t size;
    std::string type;
    std::string sourceFile;
    int sourceLine;
    uint64_t timestamp;
    bool isGPUAllocation;
    std::string resourceName;
};

// Shader debug info
struct ShaderDebugInfo {
    std::string shaderName;
    std::string sourceCode;
    std::string disassembly;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    std::vector<std::string> performanceHints;
    std::unordered_map<std::string, std::string> variables;
    std::vector<std::string> callStack;
    bool isOptimized;
    size_t binarySize;
    std::string compileTime;
};

// Callback types
using DebugEventCallback = std::function<void(const DebugEvent&)>;
using PerformanceUpdateCallback = std::function<void(const PerformanceMetrics&)>;
using MemoryUpdateCallback = std::function<void(const std::vector<MemoryAllocation>&)>;
using ShaderDebugCallback = std::function<void(const ShaderDebugInfo&)>;
using CaptureCompleteCallback = std::function<void(const std::string&, CaptureType)>;

// ========== GPU DEBUGGER ==========
class GPUDebugger : public System {
private:
    static GPUDebugger* instance_;

    RenderDocManager* renderDocManager_;
    PerfettoManager* perfettoManager_;
    MaliGraphicsDebugger* maliDebugger_;
    AdrenoProfiler* adrenoProfiler_;
    FrameCaptureManager* frameCaptureManager_;
    PerformanceProfiler* performanceProfiler_;
    MemoryTracker* memoryTracker_;
    ShaderDebugger* shaderDebugger_;

    // JNI environment
    JNIEnv* env_;
    jobject context_;

    // Debug state
    std::atomic<bool> initialized_;
    std::atomic<bool> captureActive_;
    std::atomic<bool> profilingActive_;
    std::atomic<bool> memoryTrackingActive_;
    std::atomic<bool> shaderDebugActive_;
    GPUVendor gpuVendor_;
    std::string gpuName_;
    std::string gpuDriverVersion_;

    // Settings
    FrameCaptureSettings frameCaptureSettings_;
    ProfilingSettings profilingSettings_;
    MemoryTrackingSettings memoryTrackingSettings_;
    ShaderDebugSettings shaderDebugSettings_;

    // Event system
    std::unordered_map<std::string, DebugEventCallback> eventCallbacks_;
    std::unordered_map<std::string, PerformanceUpdateCallback> performanceCallbacks_;
    std::unordered_map<std::string, MemoryUpdateCallback> memoryCallbacks_;
    std::unordered_map<std::string, ShaderDebugCallback> shaderCallbacks_;
    std::unordered_map<std::string, CaptureCompleteCallback> captureCallbacks_;

    // Debug data storage
    std::vector<DebugEvent> eventHistory_;
    std::vector<PerformanceMetrics> performanceHistory_;
    std::vector<MemoryAllocation> memoryHistory_;
    std::mutex dataMutex_;

    // Service management
    std::atomic<bool> serviceRunning_;
    std::thread captureThread_;
    std::thread profilingThread_;
    std::thread memoryThread_;

    // Output management
    std::string outputDirectory_;
    bool autoSaveCaptures_;
    int maxStoredCaptures_;

public:
    GPUDebugger();
    ~GPUDebugger();

    static GPUDebugger* getInstance();

    // System interface
    void initialize() override;
    void update(float dt) override;
    void shutdown() override;

    // JNI setup
    void setJNIEnvironment(JNIEnv* env, jobject context);

    // GPU information
    GPUVendor getGPUVendor() const { return gpuVendor_; }
    std::string getGPUName() const { return gpuName_; }
    std::string getGPUDriverVersion() const { return gpuDriverVersion_; }
    bool isGPUVendor(GPUVendor vendor) const { return gpuVendor_ == vendor; }

    // RenderDoc integration
    bool initializeRenderDoc();
    void enableRenderDoc(bool enable);
    bool isRenderDocAvailable() const;
    bool isRenderDocEnabled() const;

    // Perfetto tracing
    bool initializePerfetto();
    void enablePerfettoTracing(bool enable);
    bool isPerfettoAvailable() const;
    bool isPerfettoEnabled() const;

    // Vendor-specific debugging
    bool initializeVendorDebugger();
    void enableVendorDebugging(bool enable);
    bool isVendorDebuggingAvailable() const;

    // Frame capture
    void startFrameCapture(const FrameCaptureSettings& settings = FrameCaptureSettings());
    void stopFrameCapture();
    bool isFrameCaptureActive() const { return captureActive_; }
    void saveFrameCapture(const std::string& filename);
    std::vector<std::string> getAvailableCaptures() const;
    bool loadCapture(const std::string& filename);

    // Performance profiling
    void startPerformanceProfiling(const ProfilingSettings& settings = ProfilingSettings());
    void stopPerformanceProfiling();
    bool isPerformanceProfilingActive() const { return profilingActive_; }
    PerformanceMetrics getCurrentPerformanceMetrics() const;
    std::vector<PerformanceMetrics> getPerformanceHistory() const;
    void clearPerformanceHistory();

    // Memory tracking
    void startMemoryTracking(const MemoryTrackingSettings& settings = MemoryTrackingSettings());
    void stopMemoryTracking();
    bool isMemoryTrackingActive() const { return memoryTrackingActive_; }
    std::vector<MemoryAllocation> getMemoryAllocations() const;
    size_t getTotalMemoryUsage() const;
    std::vector<MemoryAllocation> getMemoryLeaks() const;
    void clearMemoryHistory();

    // Shader debugging
    void startShaderDebugging(const ShaderDebugSettings& settings = ShaderDebugSettings());
    void stopShaderDebugging();
    bool isShaderDebuggingActive() const { return shaderDebugActive_; }
    void debugShader(const std::string& shaderName, const std::string& sourceCode);
    std::vector<ShaderDebugInfo> getShaderDebugInfo() const;
    void clearShaderDebugHistory();

    // Debug events
    void logDebugEvent(const DebugEvent& event);
    void logDebugEvent(DebugEventType type, const std::string& name, const std::string& description = "");
    std::vector<DebugEvent> getDebugEvents() const;
    std::vector<DebugEvent> getDebugEventsByType(DebugEventType type) const;
    void clearDebugHistory();

    // Settings management
    void setFrameCaptureSettings(const FrameCaptureSettings& settings);
    void setProfilingSettings(const ProfilingSettings& settings);
    void setMemoryTrackingSettings(const MemoryTrackingSettings& settings);
    void setShaderDebugSettings(const ShaderDebugSettings& settings);

    FrameCaptureSettings getFrameCaptureSettings() const { return frameCaptureSettings_; }
    ProfilingSettings getProfilingSettings() const { return profilingSettings_; }
    MemoryTrackingSettings getMemoryTrackingSettings() const { return memoryTrackingSettings_; }
    ShaderDebugSettings getShaderDebugSettings() const { return shaderDebugSettings_; }

    // Output management
    void setOutputDirectory(const std::string& directory);
    std::string getOutputDirectory() const { return outputDirectory_; }
    void setAutoSaveCaptures(bool autoSave);
    void setMaxStoredCaptures(int maxCaptures);
    bool exportDebugData(const std::string& filename, DebugOutputFormat format);

    // Callback management
    void registerDebugEventCallback(const std::string& id, DebugEventCallback callback);
    void unregisterDebugEventCallback(const std::string& id);
    void registerPerformanceUpdateCallback(const std::string& id, PerformanceUpdateCallback callback);
    void unregisterPerformanceUpdateCallback(const std::string& id);
    void registerMemoryUpdateCallback(const std::string& id, MemoryUpdateCallback callback);
    void unregisterMemoryUpdateCallback(const std::string& id);
    void registerShaderDebugCallback(const std::string& id, ShaderDebugCallback callback);
    void unregisterShaderDebugCallback(const std::string& id);
    void registerCaptureCompleteCallback(const std::string& id, CaptureCompleteCallback callback);
    void unregisterCaptureCompleteCallback(const std::string& id);

    // Utility functions
    bool isDebuggingAvailable() const;
    std::string getDebugStatus() const;
    void generateDebugReport(const std::string& filename);
    void clearAllData();

    // Advanced debugging
    void setPerformanceMarker(const std::string& name);
    void beginDebugRegion(const std::string& name);
    void endDebugRegion();
    void insertDebugMessage(const std::string& message, DebugEventType type = DebugEventType::INFO);

    // Testing and validation
    void testGPUDebugger();
    bool validateDebugSetup();
    void benchmarkGPUPerformance();

private:
    void initializeDefaults();
    void detectGPUVendor();
    void startServiceThreads();
    void stopServiceThreads();
    void captureThreadLoop();
    void profilingThreadLoop();
    void memoryThreadLoop();

    // JNI helper methods
    void initializeRenderDocJNI();
    void initializePerfettoJNI();
    void initializeVendorDebuggerJNI();
    void startFrameCaptureJNI();
    void stopFrameCaptureJNI();
    void startPerformanceProfilingJNI();
    void stopPerformanceProfilingJNI();
    void startMemoryTrackingJNI();
    void stopMemoryTrackingJNI();

    // Event processing
    void onDebugEvent(const DebugEvent& event);
    void onPerformanceUpdate(const PerformanceMetrics& metrics);
    void onMemoryUpdate(const std::vector<MemoryAllocation>& allocations);
    void onShaderDebug(const ShaderDebugInfo& info);
    void onCaptureComplete(const std::string& filename, CaptureType type);

    // Data management
    void saveDebugData();
    void loadDebugData();
    void cleanupOldCaptures();
    void limitHistorySize();

    // GPU-specific operations
    void initializeAdrenoTools();
    void initializeMaliTools();
    void initializeNvidiaTools();
    void initializeGenericTools();

    // Performance counter management
    void initializePerformanceCounters();
    void updatePerformanceCounters();
    void shutdownPerformanceCounters();

    // Memory tracking
    void trackMemoryAllocation(void* address, size_t size, const std::string& type,
                             const std::string& sourceFile, int sourceLine);
    void trackMemoryDeallocation(void* address);
    void analyzeMemoryUsage();
    void detectMemoryLeaks();

    // Shader debugging
    void compileShaderWithDebug(const std::string& shaderName, const std::string& sourceCode);
    void analyzeShaderPerformance(const std::string& shaderName);
    void validateShaderOptimizations(const std::string& shaderName);
};

// ========== RENDERDOC MANAGER ==========
class RenderDocManager {
private:
    GPUDebugger* debugger_;

    // RenderDoc state
    std::atomic<bool> initialized_;
    std::atomic<bool> enabled_;
    void* renderDocAPI_;
    int captureCount_;

    // Capture settings
    bool captureCallstack_;
    bool captureShaders_;
    bool captureTextures_;
    bool captureBuffers_;
    bool capturePipelineState_;
    bool captureRenderTargets_;
    int maxTextureDimension_;
    int maxBufferSize_;

public:
    RenderDocManager(GPUDebugger* debugger);
    ~RenderDocManager();

    bool initialize();
    void shutdown();

    // RenderDoc API
    bool isAvailable() const;
    bool isEnabled() const { return enabled_; }
    void setEnabled(bool enable);

    // Frame capture
    void startFrameCapture();
    void endFrameCapture();
    bool isCapturing() const;
    void saveCapture(const std::string& filename);

    // Settings
    void setCaptureCallstack(bool enable);
    void setCaptureShaders(bool enable);
    void setCaptureTextures(bool enable);
    void setCaptureBuffers(bool enable);
    void setCapturePipelineState(bool enable);
    void setCaptureRenderTargets(bool enable);
    void setMaxTextureDimension(int dimension);
    void setMaxBufferSize(int size);

    bool getCaptureCallstack() const { return captureCallstack_; }
    bool getCaptureShaders() const { return captureShaders_; }
    bool getCaptureTextures() const { return captureTextures_; }
    bool getCaptureBuffers() const { return captureBuffers_; }
    bool getCapturePipelineState() const { return capturePipelineState_; }
    bool getCaptureRenderTargets() const { return captureRenderTargets_; }
    int getMaxTextureDimension() const { return maxTextureDimension_; }
    int getMaxBufferSize() const { return maxBufferSize_; }

    // Advanced features
    void setActiveWindow(void* windowHandle);
    void triggerMultiFrameCapture(int frameCount);
    void setCaptureFilePathTemplate(const std::string& pathTemplate);
    void setCaptureOptions(const std::string& options);

private:
    void loadRenderDocAPI();
    void unloadRenderDocAPI();
    bool validateRenderDocInstallation();
    std::string getRenderDocPath();
};

// ========== PERFETTO MANAGER ==========
class PerfettoManager {
private:
    GPUDebugger* debugger_;

    // Perfetto state
    std::atomic<bool> initialized_;
    std::atomic<bool> tracingActive_;
    void* tracingSession_;

    // Trace settings
    bool enableGPUCounters_;
    bool enableCPUCounters_;
    bool enableMemoryCounters_;
    int bufferSizeKB_;
    int durationSeconds_;

public:
    PerfettoManager(GPUDebugger* debugger);
    ~PerfettoManager();

    bool initialize();
    void shutdown();

    // Tracing control
    bool isAvailable() const;
    bool isTracingActive() const { return tracingActive_; }
    void startTracing(const std::string& categories, int durationSeconds = 30);
    void stopTracing();
    void saveTrace(const std::string& filename);

    // Settings
    void setEnableGPUCounters(bool enable);
    void setEnableCPUCounters(bool enable);
    void setEnableMemoryCounters(bool enable);
    void setBufferSizeKB(int size);
    void setDurationSeconds(int duration);

    bool getEnableGPUCounters() const { return enableGPUCounters_; }
    bool getEnableCPUCounters() const { return enableCPUCounters_; }
    bool getEnableMemoryCounters() const { return enableMemoryCounters_; }
    int getBufferSizeKB() const { return bufferSizeKB_; }
    int getDurationSeconds() const { return durationSeconds_; }

    // Advanced tracing
    void addTraceEvent(const std::string& name, const std::string& category);
    void beginTraceEvent(const std::string& name, const std::string& category);
    void endTraceEvent(const std::string& name, const std::string& category);
    void addTraceCounter(const std::string& name, double value);

private:
    bool initializePerfettoSDK();
    void configureDataSource();
    void startTracingSession();
    void stopTracingSession();
    void processTraceData();
};

// ========== MALI GRAPHICS DEBUGGER ==========
class MaliGraphicsDebugger {
private:
    GPUDebugger* debugger_;

    // Mali tools state
    std::atomic<bool> initialized_;
    std::atomic<bool> enabled_;
    void* maliContext_;

    // Mali-specific features
    bool offlineCompilerAvailable_;
    bool performanceCountersAvailable_;
    bool frameBufferCaptureAvailable_;

public:
    MaliGraphicsDebugger(GPUDebugger* debugger);
    ~MaliGraphicsDebugger();

    bool initialize();
    void shutdown();

    // Mali tools availability
    bool isAvailable() const;
    bool isEnabled() const { return enabled_; }
    void setEnabled(bool enable);

    // Shader debugging
    void analyzeShader(const std::string& shaderSource, const std::string& outputFile);
    void compileShaderOffline(const std::string& shaderSource, const std::string& outputFile);
    std::vector<std::string> getShaderAnalysis();

    // Performance analysis
    void startPerformanceCapture();
    void stopPerformanceCapture();
    void collectPerformanceCounters();
    std::unordered_map<std::string, float> getPerformanceMetrics();

    // Frame capture
    void captureFrameBuffer(const std::string& outputFile);
    void captureShaderBinaries(const std::string& outputDirectory);
    void capturePipelineState(const std::string& outputFile);

    // Advanced features
    void setShaderDebugCallback(void (*callback)(const char*, const char*));
    void enablePerformanceWarnings(bool enable);
    void setOptimizationLevel(int level);

private:
    bool detectMaliGPU();
    bool loadMaliLibraries();
    void initializeOfflineCompiler();
    void initializePerformanceCounters();
    void initializeFrameCapture();
};

// ========== ADRENO PROFILER ==========
class AdrenoProfiler {
private:
    GPUDebugger* debugger_;

    // Adreno tools state
    std::atomic<bool> initialized_;
    std::atomic<bool> enabled_;
    void* profilerContext_;

    // Adreno-specific features
    bool snapdragonProfilerAvailable_;
    bool adrenoGPUProfilerAvailable_;
    bool frameCaptureAvailable_;

public:
    AdrenoProfiler(GPUDebugger* debugger);
    ~AdrenoProfiler();

    bool initialize();
    void shutdown();

    // Adreno tools availability
    bool isAvailable() const;
    bool isEnabled() const { return enabled_; }
    void setEnabled(bool enable);

    // Performance profiling
    void startProfilingSession();
    void stopProfilingSession();
    void collectProfilingData();
    std::unordered_map<std::string, float> getProfilingMetrics();

    // Frame capture
    void captureFrame(const std::string& outputFile);
    void captureShaders(const std::string& outputDirectory);
    void captureTextures(const std::string& outputDirectory);

    // GPU debugging
    void debugShader(const std::string& shaderName, const std::string& sourceCode);
    void analyzePerformanceBottlenecks();
    void generateOptimizationHints();

    // Advanced features
    void setProfilingRate(int frames);
    void enableDetailedMetrics(bool enable);
    void setOutputFormat(const std::string& format);

private:
    bool detectAdrenoGPU();
    bool loadAdrenoLibraries();
    void initializeSnapdragonProfiler();
    void initializeAdrenoGPUProfiler();
    void initializeFrameCapture();
};

// ========== FRAME CAPTURE MANAGER ==========
class FrameCaptureManager {
private:
    GPUDebugger* debugger_;

    // Capture state
    std::atomic<bool> captureActive_;
    std::atomic<int> frameCount_;
    std::string currentCaptureFile_;
    std::vector<std::string> captureHistory_;

    // Capture settings
    bool captureCallstack_;
    bool captureShaders_;
    bool captureTextures_;
    bool captureBuffers_;
    bool capturePipelineState_;
    bool captureRenderTargets_;
    int maxTextureDimension_;
    int maxBufferSize_;

public:
    FrameCaptureManager(GPUDebugger* debugger);
    ~FrameCaptureManager();

    bool initialize();
    void shutdown();

    // Frame capture control
    void startCapture();
    void stopCapture();
    bool isCaptureActive() const { return captureActive_; }
    void saveCapture(const std::string& filename);

    // Settings
    void setCaptureCallstack(bool enable);
    void setCaptureShaders(bool enable);
    void setCaptureTextures(bool enable);
    void setCaptureBuffers(bool enable);
    void setCapturePipelineState(bool enable);
    void setCaptureRenderTargets(bool enable);
    void setMaxTextureDimension(int dimension);
    void setMaxBufferSize(int size);

    // Capture management
    std::vector<std::string> getCaptureHistory() const { return captureHistory_; }
    void clearCaptureHistory();
    bool loadCapture(const std::string& filename);
    void deleteCapture(const std::string& filename);

    // Advanced capture
    void captureSingleFrame();
    void captureMultipleFrames(int frameCount);
    void captureRenderPass(const std::string& passName);

private:
    void generateCaptureFilename();
    void validateCaptureSettings();
    void processCaptureData();
    void saveCaptureMetadata();
};

// ========== PERFORMANCE PROFILER ==========
class PerformanceProfiler {
private:
    GPUDebugger* debugger_;

    // Profiling state
    std::atomic<bool> profilingActive_;
    std::atomic<int> frameCount_;
    std::chrono::steady_clock::time_point profilingStartTime_;

    // Performance data
    std::vector<PerformanceMetrics> frameMetrics_;
    std::unordered_map<PerformanceCounter, std::vector<float>> counterHistory_;
    std::mutex dataMutex_;

    // Settings
    bool enableGPUCounters_;
    bool enableCPUCounters_;
    bool enableMemoryCounters_;
    bool enablePowerCounters_;
    bool enableThermalCounters_;
    int samplingIntervalMs_;

public:
    PerformanceProfiler(GPUDebugger* debugger);
    ~PerformanceProfiler();

    bool initialize();
    void shutdown();

    // Profiling control
    void startProfiling();
    void stopProfiling();
    bool isProfilingActive() const { return profilingActive_; }
    void updateMetrics();

    // Performance data
    PerformanceMetrics getCurrentMetrics() const;
    std::vector<PerformanceMetrics> getFrameMetrics() const;
    std::unordered_map<PerformanceCounter, std::vector<float>> getCounterHistory() const;
    float getAverageFrameTime() const;
    float getAverageGPUUtilization() const;

    // Settings
    void setEnableGPUCounters(bool enable);
    void setEnableCPUCounters(bool enable);
    void setEnableMemoryCounters(bool enable);
    void setEnablePowerCounters(bool enable);
    void setEnableThermalCounters(bool enable);
    void setSamplingIntervalMs(int interval);

    // Advanced profiling
    void markFrameStart();
    void markFrameEnd();
    void addCustomCounter(const std::string& name, float value);
    void beginPerformanceMarker(const std::string& name);
    void endPerformanceMarker(const std::string& name);

private:
    void collectGPUCounters();
    void collectCPUCounters();
    void collectMemoryCounters();
    void collectPowerCounters();
    void collectThermalCounters();
    void calculateAverages();
    void detectPerformanceIssues();
};

// ========== MEMORY TRACKER ==========
class MemoryTracker {
private:
    GPUDebugger* debugger_;

    // Tracking state
    std::atomic<bool> trackingActive_;
    std::unordered_map<void*, MemoryAllocation> activeAllocations_;
    std::vector<MemoryAllocation> allocationHistory_;
    std::vector<MemoryAllocation> leakedAllocations_;
    std::mutex trackingMutex_;

    // Settings
    bool trackAllocations_;
    bool trackDeallocations_;
    bool trackLeaks_;
    bool trackFragmentation_;
    size_t allocationThreshold_;
    bool trackGPUMemory_;
    bool trackSystemMemory_;
    bool trackTextureMemory_;
    bool trackBufferMemory_;

public:
    MemoryTracker(GPUDebugger* debugger);
    ~MemoryTracker();

    bool initialize();
    void shutdown();

    // Memory tracking control
    void startTracking();
    void stopTracking();
    bool isTrackingActive() const { return trackingActive_; }
    void updateTracking();

    // Memory data
    std::vector<MemoryAllocation> getActiveAllocations() const;
    std::vector<MemoryAllocation> getAllocationHistory() const;
    std::vector<MemoryAllocation> getMemoryLeaks() const;
    size_t getTotalAllocatedMemory() const;
    size_t getTotalActiveMemory() const;
    size_t getPeakMemoryUsage() const;

    // Settings
    void setTrackAllocations(bool enable);
    void setTrackDeallocations(bool enable);
    void setTrackLeaks(bool enable);
    void setTrackFragmentation(bool enable);
    void setAllocationThreshold(size_t threshold);
    void setTrackGPUMemory(bool enable);
    void setTrackSystemMemory(bool enable);
    void setTrackTextureMemory(bool enable);
    void setTrackBufferMemory(bool enable);

    // Memory analysis
    void analyzeMemoryUsage();
    void detectMemoryLeaks();
    void generateMemoryReport(const std::string& filename);
    void clearTrackingData();

private:
    void trackAllocation(void* address, size_t size, const std::string& type,
                        const std::string& sourceFile, int sourceLine);
    void trackDeallocation(void* address);
    void checkForLeaks();
    void analyzeFragmentation();
    void saveMemorySnapshot();
};

// ========== SHADER DEBUGGER ==========
class ShaderDebugger {
private:
    GPUDebugger* debugger_;

    // Debug state
    std::atomic<bool> debugActive_;
    std::unordered_map<std::string, ShaderDebugInfo> shaderDebugInfo_;
    std::vector<std::string> debugHistory_;
    std::mutex debugMutex_;

    // Settings
    bool enableSourceDebug_;
    bool enableBinaryDebug_;
    bool enableOptimizationAnalysis_;
    bool enablePerformanceHints_;
    bool generateDisassembly_;
    bool validateSPIRV_;
    bool enableWatchVariables_;

public:
    ShaderDebugger(GPUDebugger* debugger);
    ~ShaderDebugger();

    bool initialize();
    void shutdown();

    // Shader debugging control
    void startDebugging();
    void stopDebugging();
    bool isDebugActive() const { return debugActive_; }
    void debugShader(const std::string& shaderName, const std::string& sourceCode);

    // Debug data
    std::vector<ShaderDebugInfo> getShaderDebugInfo() const;
    ShaderDebugInfo getShaderDebugInfo(const std::string& shaderName) const;
    std::vector<std::string> getDebugHistory() const;

    // Settings
    void setEnableSourceDebug(bool enable);
    void setEnableBinaryDebug(bool enable);
    void setEnableOptimizationAnalysis(bool enable);
    void setEnablePerformanceHints(bool enable);
    void setGenerateDisassembly(bool enable);
    void setValidateSPIRV(bool enable);
    void setEnableWatchVariables(bool enable);

    // Advanced debugging
    void addWatchVariable(const std::string& variable);
    void removeWatchVariable(const std::string& variable);
    void setBreakpoint(const std::string& shaderName, int lineNumber);
    void clearBreakpoints(const std::string& shaderName);
    void stepThroughShader(const std::string& shaderName);

private:
    void compileWithDebugInfo(const std::string& shaderName, const std::string& sourceCode);
    void analyzeShaderOptimizations(const std::string& shaderName);
    void generatePerformanceHints(const std::string& shaderName);
    void validateShaderCorrectness(const std::string& shaderName);
    void extractDebugInformation(const std::string& shaderName);
};

// ========== JNI BRIDGE FUNCTIONS ==========
extern "C" {

    // RenderDoc callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_GPUDebugger_onRenderDocInitialized(
        JNIEnv* env, jobject thiz, jboolean success);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_GPUDebugger_onFrameCaptureStarted(
        JNIEnv* env, jobject thiz);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_GPUDebugger_onFrameCaptureEnded(
        JNIEnv* env, jobject thiz, jstring filename);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_GPUDebugger_onRenderDocError(
        JNIEnv* env, jobject thiz, jstring error);

    // Perfetto callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_GPUDebugger_onPerfettoInitialized(
        JNIEnv* env, jobject thiz, jboolean success);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_GPUDebugger_onTracingStarted(
        JNIEnv* env, jobject thiz);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_GPUDebugger_onTracingEnded(
        JNIEnv* env, jobject thiz, jstring filename);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_GPUDebugger_onTraceEvent(
        JNIEnv* env, jobject thiz, jstring name, jstring category, jlong timestamp);

    // Vendor-specific callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_GPUDebugger_onVendorDebuggerInitialized(
        JNIEnv* env, jobject thiz, jstring vendor, jboolean success);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_GPUDebugger_onPerformanceDataAvailable(
        JNIEnv* env, jobject thiz, jstring dataJson);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_GPUDebugger_onMemoryDataAvailable(
        JNIEnv* env, jobject thiz, jstring dataJson);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_GPUDebugger_onShaderDebugDataAvailable(
        JNIEnv* env, jobject thiz, jstring shaderName, jstring dataJson);

    // Debug event callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_GPUDebugger_onDebugEvent(
        JNIEnv* env, jobject thiz, jstring eventType, jstring name, jstring description, jlong timestamp);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_GPUDebugger_onPerformanceMetrics(
        JNIEnv* env, jobject thiz, jstring metricsJson);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_GPUDebugger_onMemoryAllocation(
        JNIEnv* env, jobject thiz, jstring address, jlong size, jstring type);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_GPUDebugger_onMemoryDeallocation(
        JNIEnv* env, jobject thiz, jstring address);

}

} // namespace FoundryEngine

#endif // FOUNDRYENGINE_GPU_DEBUGGER_H
