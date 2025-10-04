#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <memory>

namespace FoundryEngine {

struct ProfilerSample {
    std::string name;
    std::chrono::high_resolution_clock::time_point startTime;
    std::chrono::high_resolution_clock::time_point endTime;
    double duration;
    uint32_t threadId;
    uint32_t frameNumber;
    std::string category;
    std::unordered_map<std::string, std::string> metadata;
};

struct MemorySnapshot {
    size_t totalAllocated;
    size_t totalFreed;
    size_t currentUsage;
    size_t peakUsage;
    uint32_t allocationCount;
    uint32_t freeCount;
    std::chrono::high_resolution_clock::time_point timestamp;
};

struct GPUProfilerSample {
    std::string name;
    double gpuTime;
    uint32_t drawCalls;
    uint32_t triangles;
    uint32_t vertices;
    uint32_t frameNumber;
    std::string category;
};

class ProfilerScope {
public:
    ProfilerScope(const std::string& name, const std::string& category = "General");
    ~ProfilerScope();
    
private:
    std::string name_;
    std::string category_;
    std::chrono::high_resolution_clock::time_point startTime_;
};

class ProfileManager {
public:
    virtual ~ProfileManager() = default;
    
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update() = 0;
    
    // Profiling control
    virtual void startProfiling() = 0;
    virtual void stopProfiling() = 0;
    virtual bool isProfiling() const = 0;
    virtual void pauseProfiling() = 0;
    virtual void resumeProfiling() = 0;
    virtual bool isPaused() const = 0;
    
    // Frame profiling
    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;
    virtual uint32_t getCurrentFrame() const = 0;
    virtual void setMaxFrames(uint32_t maxFrames) = 0;
    virtual uint32_t getMaxFrames() const = 0;
    
    // CPU profiling
    virtual void beginSample(const std::string& name, const std::string& category = "General") = 0;
    virtual void endSample(const std::string& name) = 0;
    virtual void addSample(const ProfilerSample& sample) = 0;
    virtual std::vector<ProfilerSample> getSamples(uint32_t frameNumber = 0) const = 0;
    virtual std::vector<ProfilerSample> getSamplesByCategory(const std::string& category, uint32_t frameNumber = 0) const = 0;
    
    // Memory profiling
    virtual void recordAllocation(size_t size, const std::string& category = "General") = 0;
    virtual void recordDeallocation(size_t size, const std::string& category = "General") = 0;
    virtual MemorySnapshot getMemorySnapshot() const = 0;
    virtual std::vector<MemorySnapshot> getMemoryHistory() const = 0;
    virtual size_t getCurrentMemoryUsage() const = 0;
    virtual size_t getPeakMemoryUsage() const = 0;
    
    // GPU profiling
    virtual void beginGPUSample(const std::string& name, const std::string& category = "Rendering") = 0;
    virtual void endGPUSample(const std::string& name) = 0;
    virtual void addGPUSample(const GPUProfilerSample& sample) = 0;
    virtual std::vector<GPUProfilerSample> getGPUSamples(uint32_t frameNumber = 0) const = 0;
    
    // Statistics
    virtual double getAverageFrameTime() const = 0;
    virtual double getMinFrameTime() const = 0;
    virtual double getMaxFrameTime() const = 0;
    virtual double getCurrentFPS() const = 0;
    virtual double getAverageFPS() const = 0;
    virtual double getMinFPS() const = 0;
    virtual double getMaxFPS() const = 0;
    
    // Sample analysis
    virtual double getTotalTime(const std::string& sampleName, uint32_t frameCount = 1) const = 0;
    virtual double getAverageTime(const std::string& sampleName, uint32_t frameCount = 60) const = 0;
    virtual double getMinTime(const std::string& sampleName, uint32_t frameCount = 60) const = 0;
    virtual double getMaxTime(const std::string& sampleName, uint32_t frameCount = 60) const = 0;
    virtual uint32_t getSampleCount(const std::string& sampleName, uint32_t frameCount = 1) const = 0;
    
    // Category analysis
    virtual std::vector<std::string> getCategories() const = 0;
    virtual double getCategoryTime(const std::string& category, uint32_t frameNumber = 0) const = 0;
    virtual double getCategoryPercentage(const std::string& category, uint32_t frameNumber = 0) const = 0;
    
    // Thread profiling
    virtual void setThreadName(uint32_t threadId, const std::string& name) = 0;
    virtual std::string getThreadName(uint32_t threadId) const = 0;
    virtual std::vector<uint32_t> getActiveThreads() const = 0;
    virtual std::vector<ProfilerSample> getThreadSamples(uint32_t threadId, uint32_t frameNumber = 0) const = 0;
    
    // Data export
    virtual void exportToFile(const std::string& filename, uint32_t frameCount = 0) const = 0;
    virtual void exportToJSON(const std::string& filename, uint32_t frameCount = 0) const = 0;
    virtual void exportToCSV(const std::string& filename, uint32_t frameCount = 0) const = 0;
    virtual std::string exportToString(uint32_t frameCount = 0) const = 0;
    
    // Real-time monitoring
    virtual void enableRealTimeMonitoring(bool enable) = 0;
    virtual bool isRealTimeMonitoringEnabled() const = 0;
    virtual void setMonitoringInterval(double intervalSeconds) = 0;
    virtual double getMonitoringInterval() const = 0;
    
    // Alerts and thresholds
    virtual void setFrameTimeThreshold(double thresholdMs) = 0;
    virtual double getFrameTimeThreshold() const = 0;
    virtual void setMemoryThreshold(size_t thresholdBytes) = 0;
    virtual size_t getMemoryThreshold() const = 0;
    virtual void setSampleTimeThreshold(const std::string& sampleName, double thresholdMs) = 0;
    virtual double getSampleTimeThreshold(const std::string& sampleName) const = 0;
    
    // Callbacks
    virtual void setFrameTimeExceededCallback(std::function<void(double)> callback) = 0;
    virtual void setMemoryThresholdExceededCallback(std::function<void(size_t)> callback) = 0;
    virtual void setSampleTimeExceededCallback(std::function<void(const std::string&, double)> callback) = 0;
    
    // Configuration
    virtual void setMaxSamplesPerFrame(uint32_t maxSamples) = 0;
    virtual uint32_t getMaxSamplesPerFrame() const = 0;
    virtual void setMaxMemorySnapshots(uint32_t maxSnapshots) = 0;
    virtual uint32_t getMaxMemorySnapshots() const = 0;
    virtual void enableGPUProfiling(bool enable) = 0;
    virtual bool isGPUProfilingEnabled() const = 0;
    
    // Cleanup
    virtual void clearSamples() = 0;
    virtual void clearMemoryHistory() = 0;
    virtual void clearGPUSamples() = 0;
    virtual void clearAll() = 0;
};

class DefaultProfileManager : public ProfileManager {
public:
    bool initialize() override;
    void shutdown() override;
    void update() override;
    void startProfiling() override;
    void stopProfiling() override;
    bool isProfiling() const override;
    void pauseProfiling() override;
    void resumeProfiling() override;
    bool isPaused() const override;
    void beginFrame() override;
    void endFrame() override;
    uint32_t getCurrentFrame() const override;
    void setMaxFrames(uint32_t maxFrames) override;
    uint32_t getMaxFrames() const override;
    void beginSample(const std::string& name, const std::string& category) override;
    void endSample(const std::string& name) override;
    void addSample(const ProfilerSample& sample) override;
    std::vector<ProfilerSample> getSamples(uint32_t frameNumber) const override;
    std::vector<ProfilerSample> getSamplesByCategory(const std::string& category, uint32_t frameNumber) const override;
    void recordAllocation(size_t size, const std::string& category) override;
    void recordDeallocation(size_t size, const std::string& category) override;
    MemorySnapshot getMemorySnapshot() const override;
    std::vector<MemorySnapshot> getMemoryHistory() const override;
    size_t getCurrentMemoryUsage() const override;
    size_t getPeakMemoryUsage() const override;
    void beginGPUSample(const std::string& name, const std::string& category) override;
    void endGPUSample(const std::string& name) override;
    void addGPUSample(const GPUProfilerSample& sample) override;
    std::vector<GPUProfilerSample> getGPUSamples(uint32_t frameNumber) const override;
    double getAverageFrameTime() const override;
    double getMinFrameTime() const override;
    double getMaxFrameTime() const override;
    double getCurrentFPS() const override;
    double getAverageFPS() const override;
    double getMinFPS() const override;
    double getMaxFPS() const override;
    double getTotalTime(const std::string& sampleName, uint32_t frameCount) const override;
    double getAverageTime(const std::string& sampleName, uint32_t frameCount) const override;
    double getMinTime(const std::string& sampleName, uint32_t frameCount) const override;
    double getMaxTime(const std::string& sampleName, uint32_t frameCount) const override;
    uint32_t getSampleCount(const std::string& sampleName, uint32_t frameCount) const override;
    std::vector<std::string> getCategories() const override;
    double getCategoryTime(const std::string& category, uint32_t frameNumber) const override;
    double getCategoryPercentage(const std::string& category, uint32_t frameNumber) const override;
    void setThreadName(uint32_t threadId, const std::string& name) override;
    std::string getThreadName(uint32_t threadId) const override;
    std::vector<uint32_t> getActiveThreads() const override;
    std::vector<ProfilerSample> getThreadSamples(uint32_t threadId, uint32_t frameNumber) const override;
    void exportToFile(const std::string& filename, uint32_t frameCount) const override;
    void exportToJSON(const std::string& filename, uint32_t frameCount) const override;
    void exportToCSV(const std::string& filename, uint32_t frameCount) const override;
    std::string exportToString(uint32_t frameCount) const override;
    void enableRealTimeMonitoring(bool enable) override;
    bool isRealTimeMonitoringEnabled() const override;
    void setMonitoringInterval(double intervalSeconds) override;
    double getMonitoringInterval() const override;
    void setFrameTimeThreshold(double thresholdMs) override;
    double getFrameTimeThreshold() const override;
    void setMemoryThreshold(size_t thresholdBytes) override;
    size_t getMemoryThreshold() const override;
    void setSampleTimeThreshold(const std::string& sampleName, double thresholdMs) override;
    double getSampleTimeThreshold(const std::string& sampleName) const override;
    void setFrameTimeExceededCallback(std::function<void(double)> callback) override;
    void setMemoryThresholdExceededCallback(std::function<void(size_t)> callback) override;
    void setSampleTimeExceededCallback(std::function<void(const std::string&, double)> callback) override;
    void setMaxSamplesPerFrame(uint32_t maxSamples) override;
    uint32_t getMaxSamplesPerFrame() const override;
    void setMaxMemorySnapshots(uint32_t maxSnapshots) override;
    uint32_t getMaxMemorySnapshots() const override;
    void enableGPUProfiling(bool enable) override;
    bool isGPUProfilingEnabled() const override;
    void clearSamples() override;
    void clearMemoryHistory() override;
    void clearGPUSamples() override;
    void clearAll() override;
};

// Profiling macros for easy use
#define PROFILE_SCOPE(name) ProfilerScope _prof_scope(name)
#define PROFILE_SCOPE_CATEGORY(name, category) ProfilerScope _prof_scope(name, category)
#define PROFILE_FUNCTION() PROFILE_SCOPE(__FUNCTION__)
#define PROFILE_BEGIN(name) Engine::getInstance().getProfiler()->beginSample(name)
#define PROFILE_END(name) Engine::getInstance().getProfiler()->endSample(name)

} // namespace FoundryEngine