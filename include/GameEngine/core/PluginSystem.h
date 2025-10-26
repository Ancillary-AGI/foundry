/**
 * @file PluginSystem.h
 * @brief Advanced plugin system for extensible engine architecture
 * @author FoundryEngine Team
 * @date 2024
 * @version 2.0.0
 */

#pragma once

#include "System.h"
#include <memory>
#include <vector>
#include <functional>
#include <unordered_map>

namespace FoundryEngine {

/**
 * @class PluginSystem
 * @brief Dynamic plugin loading and management system
 */
class PluginSystem : public System {
public:
    enum class PluginType {
        Renderer,
        Physics,
        Audio,
        Input,
        Networking,
        AI,
        Scripting,
        Tool,
        Custom
    };
    
    struct PluginInfo {
        std::string name;
        std::string version;
        std::string author;
        std::string description;
        PluginType type;
        std::vector<std::string> dependencies;
        std::string apiVersion;
        bool isLoaded;
        void* handle; // Platform-specific handle
    };
    
    struct PluginAPI {
        // Plugin lifecycle
        bool (*initialize)(void* engineAPI);
        void (*shutdown)();
        void (*update)(float deltaTime);
        
        // Plugin info
        const char* (*getName)();
        const char* (*getVersion)();
        const char* (*getAuthor)();
        const char* (*getDescription)();
        int (*getType)();
        
        // Plugin functionality
        void* (*getInterface)(const char* interfaceName);
        bool (*hasFeature)(const char* featureName);
        void (*registerCallback)(const char* eventName, void* callback);
    };
    
    PluginSystem();
    ~PluginSystem();
    
    bool initialize() override;
    void shutdown() override;
    void update(float deltaTime) override;
    
    // Plugin management
    bool loadPlugin(const std::string& pluginPath);
    bool unloadPlugin(const std::string& pluginName);
    bool reloadPlugin(const std::string& pluginName);
    bool isPluginLoaded(const std::string& pluginName) const;
    
    // Plugin discovery
    std::vector<std::string> scanForPlugins(const std::string& directory) const;
    std::vector<PluginInfo> getAvailablePlugins() const;
    std::vector<PluginInfo> getLoadedPlugins() const;
    PluginInfo getPluginInfo(const std::string& pluginName) const;
    
    // Plugin interfaces
    template<typename T>
    T* getPluginInterface(const std::string& pluginName, const std::string& interfaceName);
    
    void registerPluginInterface(const std::string& interfaceName, void* interface);
    void unregisterPluginInterface(const std::string& interfaceName);
    
    // Plugin communication
    void registerPluginEvent(const std::string& eventName);
    void triggerPluginEvent(const std::string& eventName, void* eventData = nullptr);
    void subscribeToEvent(const std::string& eventName, 
                         std::function<void(void*)> callback);
    
    // Plugin dependencies
    bool checkDependencies(const std::string& pluginName) const;
    std::vector<std::string> resolveDependencyOrder(const std::vector<std::string>& plugins) const;
    
    // Hot reloading
    void enableHotReloading(bool enable);
    void watchPluginDirectory(const std::string& directory);
    void onPluginFileChanged(const std::string& filePath);
    
    // Plugin validation
    bool validatePlugin(const std::string& pluginPath) const;
    bool checkAPICompatibility(const std::string& pluginPath) const;
    std::vector<std::string> getPluginErrors(const std::string& pluginName) const;
    
    // Plugin marketplace integration
    void setMarketplaceURL(const std::string& url);
    std::vector<PluginInfo> searchMarketplace(const std::string& query) const;
    bool downloadPlugin(const std::string& pluginId, const std::string& version);
    bool updatePlugin(const std::string& pluginName);
    
    // Security
    void enablePluginSandboxing(bool enable);
    void setPluginPermissions(const std::string& pluginName, 
                             const std::vector<std::string>& permissions);
    bool hasPluginPermission(const std::string& pluginName, 
                            const std::string& permission) const;

private:
    class PluginSystemImpl;
    std::unique_ptr<PluginSystemImpl> impl_;
    
    // Platform-specific plugin loading
    void* loadPluginLibrary(const std::string& path);
    void unloadPluginLibrary(void* handle);
    void* getPluginSymbol(void* handle, const std::string& symbolName);
    
    // Plugin validation helpers
    bool validatePluginSignature(const std::string& pluginPath) const;
    bool checkPluginVersion(const PluginAPI* api) const;
};

/**
 * @class IPlugin
 * @brief Base interface that all plugins must implement
 */
class IPlugin {
public:
    virtual ~IPlugin() = default;
    
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update(float deltaTime) = 0;
    
    virtual const char* getName() const = 0;
    virtual const char* getVersion() const = 0;
    virtual const char* getAuthor() const = 0;
    virtual const char* getDescription() const = 0;
    virtual PluginType getType() const = 0;
    
    virtual void* getInterface(const std::string& interfaceName) { return nullptr; }
    virtual bool hasFeature(const std::string& featureName) const { return false; }
};

/**
 * @class PluginManager
 * @brief High-level plugin management utilities
 */
class PluginManager {
public:
    static PluginManager& getInstance();
    
    // Convenience methods
    template<typename T>
    T* getPlugin(const std::string& name);
    
    template<typename T>
    std::vector<T*> getPluginsOfType();
    
    void loadPluginsFromDirectory(const std::string& directory);
    void loadPluginsFromConfig(const std::string& configFile);
    
    // Plugin categories
    void registerPluginCategory(const std::string& category, PluginType type);
    std::vector<PluginInfo> getPluginsByCategory(const std::string& category) const;
    
    // Plugin ratings and reviews
    void setPluginRating(const std::string& pluginName, float rating);
    float getPluginRating(const std::string& pluginName) const;
    void addPluginReview(const std::string& pluginName, const std::string& review);

private:
    PluginSystem* pluginSystem_;
    std::unordered_map<std::string, PluginType> categories_;
    std::unordered_map<std::string, float> ratings_;
    std::unordered_map<std::string, std::vector<std::string>> reviews_;
};

} // namespace FoundryEngine

// Plugin export macros for easy plugin creation
#define FOUNDRY_PLUGIN_EXPORT extern "C" __declspec(dllexport)

#define FOUNDRY_PLUGIN_MAIN(PluginClass) \
    FOUNDRY_PLUGIN_EXPORT FoundryEngine::IPlugin* createPlugin() { \
        return new PluginClass(); \
    } \
    FOUNDRY_PLUGIN_EXPORT void destroyPlugin(FoundryEngine::IPlugin* plugin) { \
        delete plugin; \
    } \
    FOUNDRY_PLUGIN_EXPORT const char* getPluginAPIVersion() { \
        return "2.0.0"; \
    }