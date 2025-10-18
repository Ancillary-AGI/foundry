#pragma once

#include "../core/System.h"
#include <string>
#include <functional>
#include <memory>
#include <atomic>

namespace FoundryEngine {

/**
 * Update System for FoundryEngine
 * Handles automatic updates, version checking, and installation
 */
class UpdateSystem : public System {
public:
    UpdateSystem();
    ~UpdateSystem();
    
    bool initialize() override;
    void shutdown() override;
    void update(float deltaTime) override;
    
    // Update checking
    bool isUpdateAvailable() const;
    bool isUpdateInProgress() const;
    std::string getCurrentVersion() const;
    std::string getLatestVersion() const;
    
    // Update configuration
    void setUpdateChannel(const std::string& channel); // "stable", "beta", "alpha"
    
    // Update callbacks
    void setProgressCallback(std::function<void(float)> callback); // progress 0.0-1.0
    void setCompletionCallback(std::function<void(bool, const std::string&)> callback); // success, message
    
    // Update operations
    bool startUpdate(); // Start downloading update
    bool installDownloadedUpdate(); // Install downloaded update
    void checkForUpdatesAsync(); // Check for updates in background
    
    // Statistics
    std::string getStatistics() const override;

private:
    class UpdateSystemImpl;
    std::unique_ptr<UpdateSystemImpl> impl_;
};

} // namespace FoundryEngine
