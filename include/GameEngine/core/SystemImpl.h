#ifndef FOUNDRY_GAMEENGINE_SYSTEM_IMPL_H
#define FOUNDRY_GAMEENGINE_SYSTEM_IMPL_H

#include <memory>
#include <string>
#include <chrono>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <sstream>
#include <iomanip>

namespace FoundryEngine {

/**
 * @brief Performance metrics for system monitoring
 */
struct SystemMetrics {
    std::atomic<uint64_t> updateCount{0};
    std::atomic<uint64_t> totalUpdateTimeNs{0};
    std::atomic<uint64_t> maxUpdateTimeNs{0};
    std::atomic<uint64_t> minUpdateTimeNs{UINT64_MAX};
    std::chrono::steady_clock::time_point lastUpdateTime;

    void recordUpdate(uint64_t durationNs) {
        updateCount++;
        totalUpdateTimeNs += durationNs;
        if (durationNs > maxUpdateTimeNs) maxUpdateTimeNs = durationNs;
        if (durationNs < minUpdateTimeNs) minUpdateTimeNs = durationNs;
        lastUpdateTime = std::chrono::steady_clock::now();
    }

    double getAverageUpdateTimeMs() const {
        return updateCount > 0 ? (totalUpdateTimeNs / static_cast<double>(updateCount)) / 1e6 : 0.0;
    }

    double getMaxUpdateTimeMs() const {
        return maxUpdateTimeNs / 1e6;
    }

    double getMinUpdateTimeMs() const {
        return minUpdateTimeNs < UINT64_MAX ? minUpdateTimeNs / 1e6 : 0.0;
    }
};

/**
 * @brief Thread-safe configuration storage
 */
class SystemConfig {
private:
    std::unordered_map<std::string, std::string> config_;
    mutable std::mutex mutex_;

public:
    void set(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_[key] = value;
    }

    std::string get(const std::string& key, const std::string& defaultValue = "") const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = config_.find(key);
        return it != config_.end() ? it->second : defaultValue;
    }

    bool has(const std::string& key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return config_.find(key) != config_.end();
    }

    void remove(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_.erase(key);
    }

    std::unordered_map<std::string, std::string> getAll() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return config_;
    }
};

/**
 * @brief Base class for system implementations using PIMPL pattern
 *
 * This class provides a common interface for all system implementations,
 * reducing code duplication and ensuring consistent behavior across systems.
 * Includes performance monitoring, configuration management, and error handling.
 */
class SystemImpl {
public:
    SystemImpl() = default;
    virtual ~SystemImpl() = default;

    // Prevent copying
    SystemImpl(const SystemImpl&) = delete;
    SystemImpl& operator=(const SystemImpl&) = delete;

    // Allow moving
    SystemImpl(SystemImpl&&) = default;
    SystemImpl& operator=(SystemImpl&&) = default;

    /**
     * @brief Initialize the system implementation
     * @return true if initialization succeeded, false otherwise
     */
    virtual bool initialize() = 0;

    /**
     * @brief Shutdown the system implementation
     */
    virtual void shutdown() = 0;

    /**
     * @brief Update the system implementation
     * @param deltaTime Time elapsed since last update in seconds
     */
    virtual void update(float deltaTime) = 0;

    /**
     * @brief Get the name of this system implementation
     * @return System name as string
     */
    virtual std::string getName() const = 0;

    /**
     * @brief Check if the system is initialized
     * @return true if initialized, false otherwise
     */
    virtual bool isInitialized() const = 0;

    /**
     * @brief Get system-specific statistics
     * @return String containing system statistics
     */
    virtual std::string getStatistics() const { return "No statistics available"; }

    /**
     * @brief Get performance metrics
     * @return Current performance metrics
     */
    const SystemMetrics& getMetrics() const { return metrics_; }

    /**
     * @brief Reset performance metrics
     */
    void resetMetrics() {
        metrics_ = SystemMetrics{};
    }

    /**
     * @brief Get system configuration
     * @return Reference to system configuration
     */
    SystemConfig& getConfig() { return config_; }
    const SystemConfig& getConfig() const { return config_; }

    /**
     * @brief Check if system is healthy
     * @return true if system is operating normally
     */
    virtual bool isHealthy() const { return isInitialized(); }

    /**
     * @brief Get last error message
     * @return Last error message, empty if no error
     */
    std::string getLastError() const {
        std::lock_guard<std::mutex> lock(errorMutex_);
        return lastError_;
    }

    /**
     * @brief Clear last error
     */
    void clearError() {
        std::lock_guard<std::mutex> lock(errorMutex_);
        lastError_.clear();
    }

protected:
    SystemMetrics metrics_;
    SystemConfig config_;
    mutable std::mutex errorMutex_;
    std::string lastError_;

    /**
     * @brief Set error message
     * @param error Error message
     */
    void setError(const std::string& error) {
        std::lock_guard<std::mutex> lock(errorMutex_);
        lastError_ = error;
    }

    /**
     * @brief Measure execution time of a function
     * @param func Function to measure
     * @return Result of the function
     */
    template<typename Func>
    auto measureExecution(Func&& func) {
        auto start = std::chrono::high_resolution_clock::now();
        auto result = func();
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        metrics_.recordUpdate(duration.count());
        return result;
    }
};

/**
 * @brief Template base class for system implementations
 *
 * Provides common functionality for system implementations using CRTP pattern.
 * Includes error handling, performance monitoring, and configuration management.
 */
template<typename Derived>
class SystemImplBase : public SystemImpl {
protected:
    bool initialized_ = false;
    std::string name_;

public:
    SystemImplBase(std::string name) : name_(std::move(name)) {}

    std::string getName() const override { return name_; }
    bool isInitialized() const override { return initialized_; }

    bool initialize() override {
        if (initialized_) {
            return true;
        }

        try {
            auto result = measureExecution([this]() {
                return static_cast<Derived*>(this)->onInitialize();
            });

            if (result) {
                initialized_ = true;
                clearError();
                return true;
            } else {
                setError("Initialization failed in derived class");
                return false;
            }
        } catch (const std::exception& e) {
            setError(std::string("Initialization exception: ") + e.what());
            return false;
        } catch (...) {
            setError("Unknown initialization exception");
            return false;
        }
    }

    void shutdown() override {
        if (initialized_) {
            try {
                static_cast<Derived*>(this)->onShutdown();
                initialized_ = false;
                clearError();
            } catch (const std::exception& e) {
                setError(std::string("Shutdown exception: ") + e.what());
            } catch (...) {
                setError("Unknown shutdown exception");
            }
        }
    }

    void update(float deltaTime) override {
        if (initialized_) {
            try {
                measureExecution([this, deltaTime]() {
                    static_cast<Derived*>(this)->onUpdate(deltaTime);
                });
                clearError();
            } catch (const std::exception& e) {
                setError(std::string("Update exception: ") + e.what());
            } catch (...) {
                setError("Unknown update exception");
            }
        }
    }

    std::string getStatistics() const override {
        std::stringstream ss;
        ss << "System: " << name_ << "\n";
        ss << "Initialized: " << (initialized_ ? "Yes" : "No") << "\n";
        ss << "Healthy: " << (isHealthy() ? "Yes" : "No") << "\n";
        ss << "Update Count: " << metrics_.updateCount << "\n";
        ss << "Avg Update Time: " << std::fixed << std::setprecision(3) << metrics_.getAverageUpdateTimeMs() << " ms\n";
        ss << "Max Update Time: " << std::fixed << std::setprecision(3) << metrics_.getMaxUpdateTimeMs() << " ms\n";
        ss << "Min Update Time: " << std::fixed << std::setprecision(3) << metrics_.getMinUpdateTimeMs() << " ms\n";

        if (!getLastError().empty()) {
            ss << "Last Error: " << getLastError() << "\n";
        }

        // Add derived class specific statistics
        ss << static_cast<const Derived*>(this)->getDerivedStatistics();

        return ss.str();
    }

    bool isHealthy() const override {
        return initialized_ && getLastError().empty();
    }

protected:
    /**
     * @brief Called during initialization
     * @return true if initialization succeeded
     */
    virtual bool onInitialize() = 0;

    /**
     * @brief Called during shutdown
     */
    virtual void onShutdown() = 0;

    /**
     * @brief Called during update
     * @param deltaTime Time elapsed since last update
     */
    virtual void onUpdate(float deltaTime) = 0;

    /**
     * @brief Get derived class specific statistics
     * @return Statistics string from derived class
     */
    virtual std::string getDerivedStatistics() const { return ""; }
};

} // namespace FoundryEngine

#endif // FOUNDRY_GAMEENGINE_SYSTEM_IMPL_H