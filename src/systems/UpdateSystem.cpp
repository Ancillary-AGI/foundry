#include "../../include/GameEngine/systems/UpdateSystem.h"
#include "../../include/GameEngine/core/SystemImpl.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <future>
#include <curl/curl.h>
#include <json/json.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <filesystem>
#include <algorithm>
#include <chrono>

namespace FoundryEngine {

class UpdateSystemImpl : public SystemImplBase<UpdateSystemImpl> {
private:
    std::string updateServerUrl_;
    std::string currentVersion_;
    std::string updateChannel_;
    std::atomic<bool> updateAvailable_{false};
    std::atomic<bool> updateInProgress_{false};
    std::string latestVersion_;
    std::string downloadUrl_;
    size_t downloadSize_;
    std::atomic<size_t> downloadedBytes_{0};
    std::function<void(float)> progressCallback_;
    std::function<void(bool, const std::string&)> completionCallback_;
    std::thread downloadThread_;
    std::atomic<bool> shouldStop_{false};

    friend class SystemImplBase<UpdateSystemImpl>;

    bool onInitialize() override {
        std::cout << "Update System initialized" << std::endl;
        
        // Initialize libcurl
        curl_global_init(CURL_GLOBAL_DEFAULT);
        
        // Load configuration
        loadConfiguration();
        
        // Check for updates in background
        std::thread([this]() {
            checkForUpdates();
        }).detach();
        
        return true;
    }

    void onShutdown() override {
        shouldStop_ = true;
        
        if (downloadThread_.joinable()) {
            downloadThread_.join();
        }
        
        curl_global_cleanup();
        std::cout << "Update System shutdown" << std::endl;
    }

    void onUpdate(float deltaTime) override {
        // Update system doesn't need per-frame updates
    }

    void loadConfiguration() {
        std::ifstream configFile("update_config.json");
        if (configFile.is_open()) {
            Json::Value config;
            configFile >> config;
            
            updateServerUrl_ = config.get("updateServerUrl", "https://updates.foundryengine.dev").asString();
            currentVersion_ = config.get("currentVersion", "1.0.0").asString();
            updateChannel_ = config.get("updateChannel", "stable").asString();
            
            configFile.close();
        } else {
            // Default configuration
            updateServerUrl_ = "https://updates.foundryengine.dev";
            currentVersion_ = "1.0.0";
            updateChannel_ = "stable";
        }
    }

    void saveConfiguration() {
        Json::Value config;
        config["updateServerUrl"] = updateServerUrl_;
        config["currentVersion"] = currentVersion_;
        config["updateChannel"] = updateChannel_;
        
        std::ofstream configFile("update_config.json");
        if (configFile.is_open()) {
            configFile << config;
            configFile.close();
        }
    }

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        size_t totalSize = size * nmemb;
        std::string* response = static_cast<std::string*>(userp);
        response->append(static_cast<char*>(contents), totalSize);
        return totalSize;
    }

    static size_t DownloadCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        size_t totalSize = size * nmemb;
        UpdateSystemImpl* updateSystem = static_cast<UpdateSystemImpl*>(userp);
        
        updateSystem->downloadedBytes_ += totalSize;
        
        if (updateSystem->progressCallback_) {
            float progress = static_cast<float>(updateSystem->downloadedBytes_) / updateSystem->downloadSize_;
            updateSystem->progressCallback_(progress);
        }
        
        return totalSize;
    }

    void checkForUpdates() {
        try {
            CURL* curl = curl_easy_init();
            if (!curl) {
                std::cerr << "Failed to initialize CURL" << std::endl;
                return;
            }
            
            // Build update check URL
            std::string url = updateServerUrl_ + "/api/check-update?version=" + currentVersion_ + "&channel=" + updateChannel_;
            
            std::string response;
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "FoundryEngine/1.0");
            
            CURLcode res = curl_easy_perform(curl);
            
            if (res == CURLE_OK) {
                parseUpdateResponse(response);
            } else {
                std::cerr << "Failed to check for updates: " << curl_easy_strerror(res) << std::endl;
            }
            
            curl_easy_cleanup(curl);
            
        } catch (const std::exception& e) {
            std::cerr << "Error checking for updates: " << e.what() << std::endl;
        }
    }

    void parseUpdateResponse(const std::string& response) {
        try {
            Json::Value root;
            Json::Reader reader;
            
            if (reader.parse(response, root)) {
                bool hasUpdate = root.get("hasUpdate", false).asBool();
                if (hasUpdate) {
                    latestVersion_ = root.get("latestVersion", "").asString();
                    downloadUrl_ = root.get("downloadUrl", "").asString();
                    downloadSize_ = root.get("downloadSize", 0).asUInt64();
                    
                    updateAvailable_ = true;
                    
                    std::cout << "Update available: " << latestVersion_ << std::endl;
                } else {
                    updateAvailable_ = false;
                    std::cout << "No updates available" << std::endl;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error parsing update response: " << e.what() << std::endl;
        }
    }

    bool downloadUpdate(const std::string& outputPath) {
        if (downloadUrl_.empty()) {
            return false;
        }
        
        updateInProgress_ = true;
        downloadedBytes_ = 0;
        
        downloadThread_ = std::thread([this, outputPath]() {
            try {
                CURL* curl = curl_easy_init();
                if (!curl) {
                    if (completionCallback_) {
                        completionCallback_(false, "Failed to initialize CURL");
                    }
                    updateInProgress_ = false;
                    return;
                }
                
                FILE* file = fopen(outputPath.c_str(), "wb");
                if (!file) {
                    if (completionCallback_) {
                        completionCallback_(false, "Failed to create output file");
                    }
                    curl_easy_cleanup(curl);
                    updateInProgress_ = false;
                    return;
                }
                
                curl_easy_setopt(curl, CURLOPT_URL, downloadUrl_.c_str());
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, DownloadCallback);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
                curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L); // 5 minutes
                curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
                curl_easy_setopt(curl, CURLOPT_USERAGENT, "FoundryEngine/1.0");
                
                CURLcode res = curl_easy_perform(curl);
                
                fclose(file);
                curl_easy_cleanup(curl);
                
                if (res == CURLE_OK) {
                    // Verify download
                    if (verifyDownload(outputPath)) {
                        if (completionCallback_) {
                            completionCallback_(true, "Download completed successfully");
                        }
                    } else {
                        if (completionCallback_) {
                            completionCallback_(false, "Download verification failed");
                        }
                    }
                } else {
                    if (completionCallback_) {
                        completionCallback_(false, "Download failed: " + std::string(curl_easy_strerror(res)));
                    }
                }
                
                updateInProgress_ = false;
                
            } catch (const std::exception& e) {
                if (completionCallback_) {
                    completionCallback_(false, "Download error: " + std::string(e.what()));
                }
                updateInProgress_ = false;
            }
        });
        
        return true;
    }

    bool verifyDownload(const std::string& filePath) {
        try {
            // Get file size
            std::ifstream file(filePath, std::ios::binary | std::ios::ate);
            if (!file.is_open()) {
                return false;
            }
            
            size_t fileSize = file.tellg();
            if (fileSize != downloadSize_) {
                std::cerr << "File size mismatch: expected " << downloadSize_ << ", got " << fileSize << std::endl;
                return false;
            }
            
            // Calculate SHA256 hash
            file.seekg(0, std::ios::beg);
            std::vector<char> buffer(8192);
            EVP_MD_CTX* ctx = EVP_MD_CTX_new();
            EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
            
            while (file.read(buffer.data(), buffer.size()) || file.gcount() > 0) {
                EVP_DigestUpdate(ctx, buffer.data(), file.gcount());
            }
            
            unsigned char hash[SHA256_DIGEST_LENGTH];
            EVP_DigestFinal_ex(ctx, hash, nullptr);
            EVP_MD_CTX_free(ctx);
            
            // Convert hash to hex string
            std::stringstream ss;
            for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
            }
            std::string fileHash = ss.str();
            
            // TODO: Compare with expected hash from server
            std::cout << "Downloaded file hash: " << fileHash << std::endl;
            
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "Error verifying download: " << e.what() << std::endl;
            return false;
        }
    }

    bool installUpdate(const std::string& updatePath) {
        try {
            // Create backup of current installation
            std::string backupPath = "backup_" + std::to_string(std::time(nullptr));
            if (!createBackup(backupPath)) {
                std::cerr << "Failed to create backup" << std::endl;
                return false;
            }
            
            // Extract update
            if (!extractUpdate(updatePath)) {
                std::cerr << "Failed to extract update" << std::endl;
                restoreBackup(backupPath);
                return false;
            }
            
            // Update version info
            currentVersion_ = latestVersion_;
            saveConfiguration();
            
            // Clean up
            std::filesystem::remove(updatePath);
            std::filesystem::remove_all(backupPath);
            
            std::cout << "Update installed successfully" << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "Error installing update: " << e.what() << std::endl;
            return false;
        }
    }

    bool createBackup(const std::string& backupPath) {
        try {
            std::filesystem::create_directories(backupPath);
            
            // Copy important files
            std::vector<std::string> filesToBackup = {
                "FoundryEngine.exe",
                "FoundryEngine.dll",
                "config.json",
                "update_config.json"
            };
            
            for (const auto& file : filesToBackup) {
                if (std::filesystem::exists(file)) {
                    std::filesystem::copy_file(file, backupPath + "/" + file);
                }
            }
            
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "Error creating backup: " << e.what() << std::endl;
            return false;
        }
    }

    bool extractUpdate(const std::string& updatePath) {
        try {
            // Extract update archive (simplified - would use actual archive library)
            std::ifstream updateFile(updatePath, std::ios::binary);
            if (!updateFile.is_open()) {
                return false;
            }
            
            // TODO: Implement actual archive extraction
            // For now, just copy the file
            std::filesystem::copy_file(updatePath, "FoundryEngine_new.exe");
            
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "Error extracting update: " << e.what() << std::endl;
            return false;
        }
    }

    void restoreBackup(const std::string& backupPath) {
        try {
            // Restore files from backup
            for (const auto& entry : std::filesystem::directory_iterator(backupPath)) {
                if (entry.is_regular_file()) {
                    std::filesystem::copy_file(entry.path(), entry.path().filename());
                }
            }
            
            std::cout << "Backup restored successfully" << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "Error restoring backup: " << e.what() << std::endl;
        }
    }

public:
    UpdateSystemImpl() : SystemImplBase("UpdateSystem") {}

    bool isUpdateAvailable() const {
        return updateAvailable_;
    }

    bool isUpdateInProgress() const {
        return updateInProgress_;
    }

    std::string getCurrentVersion() const {
        return currentVersion_;
    }

    std::string getLatestVersion() const {
        return latestVersion_;
    }

    void setUpdateChannel(const std::string& channel) {
        updateChannel_ = channel;
        saveConfiguration();
    }

    void setProgressCallback(std::function<void(float)> callback) {
        progressCallback_ = callback;
    }

    void setCompletionCallback(std::function<void(bool, const std::string&)> callback) {
        completionCallback_ = callback;
    }

    bool startUpdate() {
        if (!updateAvailable_ || updateInProgress_) {
            return false;
        }
        
        std::string updatePath = "update_" + latestVersion_ + ".zip";
        return downloadUpdate(updatePath);
    }

    bool installDownloadedUpdate() {
        if (updateInProgress_) {
            return false;
        }
        
        std::string updatePath = "update_" + latestVersion_ + ".zip";
        return installUpdate(updatePath);
    }

    void checkForUpdatesAsync() {
        std::thread([this]() {
            checkForUpdates();
        }).detach();
    }

    std::string getStatistics() const override {
        char buffer[256];
        snprintf(buffer, sizeof(buffer),
                 "Current: %s, Latest: %s, Available: %s, InProgress: %s",
                 currentVersion_.c_str(),
                 latestVersion_.c_str(),
                 updateAvailable_ ? "Yes" : "No",
                 updateInProgress_ ? "Yes" : "No");
        return std::string(buffer);
    }
};

UpdateSystem::UpdateSystem() : impl_(std::make_unique<UpdateSystemImpl>()) {}
UpdateSystem::~UpdateSystem() = default;

bool UpdateSystem::initialize() { return impl_->initialize(); }
void UpdateSystem::shutdown() { impl_->shutdown(); }
void UpdateSystem::update(float deltaTime) { impl_->update(deltaTime); }

bool UpdateSystem::isUpdateAvailable() const {
    return impl_->isUpdateAvailable();
}

bool UpdateSystem::isUpdateInProgress() const {
    return impl_->isUpdateInProgress();
}

std::string UpdateSystem::getCurrentVersion() const {
    return impl_->getCurrentVersion();
}

std::string UpdateSystem::getLatestVersion() const {
    return impl_->getLatestVersion();
}

void UpdateSystem::setUpdateChannel(const std::string& channel) {
    impl_->setUpdateChannel(channel);
}

void UpdateSystem::setProgressCallback(std::function<void(float)> callback) {
    impl_->setProgressCallback(callback);
}

void UpdateSystem::setCompletionCallback(std::function<void(bool, const std::string&)> callback) {
    impl_->setCompletionCallback(callback);
}

bool UpdateSystem::startUpdate() {
    return impl_->startUpdate();
}

bool UpdateSystem::installDownloadedUpdate() {
    return impl_->installDownloadedUpdate();
}

void UpdateSystem::checkForUpdatesAsync() {
    impl_->checkForUpdatesAsync();
}

std::string UpdateSystem::getStatistics() const {
    return impl_->getStatistics();
}

} // namespace FoundryEngine
