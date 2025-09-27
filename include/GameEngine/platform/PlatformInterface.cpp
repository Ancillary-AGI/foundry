#include "PlatformInterface.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <mutex>

namespace FoundryEngine {

// Static registry for platform creators
static std::unordered_map<PlatformType, std::function<std::unique_ptr<PlatformInterface>()>> platformCreators;
static std::mutex platformRegistryMutex;

// ========== PLATFORM INTERFACE BASE IMPLEMENTATION ==========
PlatformInterface::PlatformInterface()
    : platformType_(PlatformType::UNKNOWN), graphicsContext_(nullptr),
      audioContext_(nullptr), inputContext_(nullptr), networkContext_(nullptr),
      storageContext_(nullptr), platformServices_(nullptr), windowManager_(nullptr),
      eventSystem_(nullptr) {
}

PlatformInterface::~PlatformInterface() {
    // Clean up contexts and services
    if (graphicsContext_) {
        delete graphicsContext_;
        graphicsContext_ = nullptr;
    }

    if (audioContext_) {
        delete audioContext_;
        audioContext_ = nullptr;
    }

    if (inputContext_) {
        delete inputContext_;
        inputContext_ = nullptr;
    }

    if (networkContext_) {
        delete networkContext_;
        networkContext_ = nullptr;
    }

    if (storageContext_) {
        delete storageContext_;
        storageContext_ = nullptr;
    }

    if (platformServices_) {
        delete platformServices_;
        platformServices_ = nullptr;
    }

    if (windowManager_) {
        delete windowManager_;
        windowManager_ = nullptr;
    }

    if (eventSystem_) {
        delete eventSystem_;
        eventSystem_ = nullptr;
    }
}

// ========== PLATFORM FACTORY IMPLEMENTATION ==========
std::unique_ptr<PlatformInterface> PlatformFactory::createPlatform(PlatformType type) {
    std::lock_guard<std::mutex> lock(platformRegistryMutex);

    auto it = platformCreators.find(type);
    if (it != platformCreators.end()) {
        return it->second();
    }

    // Return null if platform type is not registered
    return nullptr;
}

PlatformType PlatformFactory::detectPlatform() {
    // Platform detection logic
#if defined(__ANDROID__)
    return PlatformType::ANDROID;
#elif defined(_WIN32) || defined(_WIN64)
    return PlatformType::WINDOWS;
#elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE
        return PlatformType::IOS;
    #else
        return PlatformType::MACOS;
    #endif
#elif defined(__linux__)
    return PlatformType::LINUX;
#elif defined(__EMSCRIPTEN__)
    return PlatformType::WEB;
#else
    return PlatformType::UNKNOWN;
#endif
}

std::vector<PlatformType> PlatformFactory::getSupportedPlatforms() {
    std::lock_guard<std::mutex> lock(platformRegistryMutex);

    std::vector<PlatformType> supported;
    supported.reserve(platformCreators.size());

    for (const auto& pair : platformCreators) {
        supported.push_back(pair.first);
    }

    return supported;
}

std::string PlatformFactory::getPlatformName(PlatformType type) {
    switch (type) {
        case PlatformType::ANDROID: return "Android";
        case PlatformType::WINDOWS: return "Windows";
        case PlatformType::MACOS: return "macOS";
        case PlatformType::IOS: return "iOS";
        case PlatformType::LINUX: return "Linux";
        case PlatformType::WEB: return "Web";
        case PlatformType::CONSOLE: return "Console";
        default: return "Unknown";
    }
}

bool PlatformFactory::isPlatformSupported(PlatformType type) {
    std::lock_guard<std::mutex> lock(platformRegistryMutex);
    return platformCreators.find(type) != platformCreators.end();
}

// ========== PLATFORM REGISTRY IMPLEMENTATION ==========
void PlatformRegistry::registerPlatform(PlatformType type, std::function<std::unique_ptr<PlatformInterface>()> creator) {
    std::lock_guard<std::mutex> lock(platformRegistryMutex);
    platformCreators[type] = creator;
}

void PlatformRegistry::unregisterPlatform(PlatformType type) {
    std::lock_guard<std::mutex> lock(platformRegistryMutex);
    platformCreators.erase(type);
}

bool PlatformRegistry::isPlatformRegistered(PlatformType type) {
    std::lock_guard<std::mutex> lock(platformRegistryMutex);
    return platformCreators.find(type) != platformCreators.end();
}

std::unique_ptr<PlatformInterface> PlatformRegistry::createPlatform(PlatformType type) {
    return PlatformFactory::createPlatform(type);
}

std::unordered_map<PlatformType, std::function<std::unique_ptr<PlatformInterface>()>>& PlatformRegistry::getRegistry() {
    return platformCreators;
}

// ========== UTILITY FUNCTIONS ==========
std::string getCurrentTimeString() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

long long getCurrentTimeMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

std::string generateUUID() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static std::uniform_int_distribution<> dis2(8, 11);

    std::stringstream ss;
    ss << std::hex;

    for (int i = 0; i < 8; i++) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 4; i++) ss << dis(gen);
    ss << "-4"; // UUID version 4
    for (int i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    ss << dis2(gen); // UUID variant
    for (int i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 12; i++) ss << dis(gen);

    return ss.str();
}

bool isValidFilePath(const std::string& path) {
    if (path.empty()) return false;

    // Check for invalid characters
    const std::string invalidChars = "<>:\"|?*";
    for (char c : path) {
        if (invalidChars.find(c) != std::string::npos) {
            return false;
        }
    }

    return true;
}

size_t getStringHash(const std::string& str) {
    std::hash<std::string> hasher;
    return hasher(str);
}

void sleepMs(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

std::vector<std::string> splitString(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;

    while (std::getline(ss, token, delimiter)) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }

    return tokens;
}

std::string joinStrings(const std::vector<std::string>& strings, const std::string& delimiter) {
    if (strings.empty()) return "";

    std::stringstream ss;
    for (size_t i = 0; i < strings.size(); ++i) {
        if (i > 0) ss << delimiter;
        ss << strings[i];
    }

    return ss.str();
}

std::string toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::string toUpper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

bool startsWith(const std::string& str, const std::string& prefix) {
    return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
}

bool endsWith(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";

    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, last - first + 1);
}

void replaceAll(std::string& str, const std::string& from, const std::string& to) {
    size_t startPos = 0;
    while ((startPos = str.find(from, startPos)) != std::string::npos) {
        str.replace(startPos, from.length(), to);
        startPos += to.length();
    }
}

std::string formatString(const char* format, ...) {
    va_list args;
    va_start(args, format);

    va_list argsCopy;
    va_copy(argsCopy, args);
    int size = vsnprintf(nullptr, 0, format, argsCopy);
    va_end(argsCopy);

    if (size <= 0) {
        va_end(args);
        return "";
    }

    std::vector<char> buffer(size + 1);
    vsnprintf(buffer.data(), buffer.size(), format, args);
    va_end(args);

    return std::string(buffer.data());
}

// ========== PLATFORM CAPABILITIES UTILITY ==========
PlatformCapabilities createDefaultCapabilities(PlatformType type) {
    PlatformCapabilities caps;
    caps.type = type;
    caps.name = PlatformFactory::getPlatformName(type);
    caps.version = "1.0.0";
    caps.architecture = "Unknown";

    // Set default capabilities based on platform type
    switch (type) {
        case PlatformType::ANDROID:
            caps.supportsVulkan = true;
            caps.supportsOpenGL = true;
            caps.supportsOpenGL_ES = true;
            caps.supportsSpatialAudio = true;
            caps.supportsLowLatencyAudio = true;
            caps.supportsTouch = true;
            caps.supportsStylus = true;
            caps.supportsGamepad = true;
            caps.supportsWebRTC = true;
            caps.supportsWebSocket = true;
            caps.supportsCloudSave = true;
            caps.supportsIAP = true;
            caps.supportsAchievements = true;
            caps.supportsLeaderboards = true;
            caps.supportsPushNotifications = true;
            caps.supportsThermalManagement = true;
            caps.supportsBackgroundTasks = true;
            caps.supportsGestureRecognition = true;
            caps.supportsAccessibility = true;
            caps.maxTextureSize = 4096;
            caps.maxRenderTargets = 8;
            caps.maxComputeUnits = 256;
            caps.maxMemoryMB = 4096;
            caps.maxThreadCount = 8;
            caps.maxDisplayWidth = 2560;
            caps.maxDisplayHeight = 1440;
            caps.maxRefreshRate = 120;
            caps.supportsHDR = true;
            caps.supportsMultipleDisplays = false;
            break;

        case PlatformType::WINDOWS:
            caps.supportsVulkan = true;
            caps.supportsDirectX = true;
            caps.supportsOpenGL = true;
            caps.supportsSpatialAudio = true;
            caps.supportsLowLatencyAudio = true;
            caps.supportsKeyboard = true;
            caps.supportsMouse = true;
            caps.supportsGamepad = true;
            caps.supportsWebRTC = true;
            caps.supportsWebSocket = true;
            caps.supportsCloudSave = true;
            caps.supportsIAP = false;
            caps.supportsAchievements = false;
            caps.supportsLeaderboards = false;
            caps.supportsPushNotifications = false;
            caps.supportsThermalManagement = false;
            caps.supportsBackgroundTasks = true;
            caps.supportsGestureRecognition = false;
            caps.supportsAccessibility = true;
            caps.maxTextureSize = 16384;
            caps.maxRenderTargets = 8;
            caps.maxComputeUnits = 1024;
            caps.maxMemoryMB = 32768;
            caps.maxThreadCount = 16;
            caps.maxDisplayWidth = 7680;
            caps.maxDisplayHeight = 4320;
            caps.maxRefreshRate = 240;
            caps.supportsHDR = true;
            caps.supportsMultipleDisplays = true;
            break;

        case PlatformType::MACOS:
            caps.supportsVulkan = true;
            caps.supportsMetal = true;
            caps.supportsOpenGL = true;
            caps.supportsSpatialAudio = true;
            caps.supportsLowLatencyAudio = true;
            caps.supportsKeyboard = true;
            caps.supportsMouse = true;
            caps.supportsGamepad = true;
            caps.supportsWebRTC = true;
            caps.supportsWebSocket = true;
            caps.supportsCloudSave = true;
            caps.supportsIAP = false;
            caps.supportsAchievements = false;
            caps.supportsLeaderboards = false;
            caps.supportsPushNotifications = false;
            caps.supportsThermalManagement = true;
            caps.supportsBackgroundTasks = true;
            caps.supportsGestureRecognition = true;
            caps.supportsAccessibility = true;
            caps.maxTextureSize = 16384;
            caps.maxRenderTargets = 8;
            caps.maxComputeUnits = 1024;
            caps.maxMemoryMB = 65536;
            caps.maxThreadCount = 16;
            caps.maxDisplayWidth = 7680;
            caps.maxDisplayHeight = 4320;
            caps.maxRefreshRate = 240;
            caps.supportsHDR = true;
            caps.supportsMultipleDisplays = true;
            break;

        case PlatformType::IOS:
            caps.supportsVulkan = true;
            caps.supportsMetal = true;
            caps.supportsOpenGL_ES = true;
            caps.supportsSpatialAudio = true;
            caps.supportsLowLatencyAudio = true;
            caps.supportsTouch = true;
            caps.supportsGamepad = true;
            caps.supportsWebRTC = true;
            caps.supportsWebSocket = true;
            caps.supportsCloudSave = true;
            caps.supportsIAP = true;
            caps.supportsAchievements = true;
            caps.supportsLeaderboards = true;
            caps.supportsPushNotifications = true;
            caps.supportsThermalManagement = true;
            caps.supportsBackgroundTasks = true;
            caps.supportsGestureRecognition = true;
            caps.supportsAccessibility = true;
            caps.maxTextureSize = 8192;
            caps.maxRenderTargets = 8;
            caps.maxComputeUnits = 512;
            caps.maxMemoryMB = 6144;
            caps.maxThreadCount = 6;
            caps.maxDisplayWidth = 2778;
            caps.maxDisplayHeight = 1284;
            caps.maxRefreshRate = 120;
            caps.supportsHDR = true;
            caps.supportsMultipleDisplays = false;
            break;

        case PlatformType::WEB:
            caps.supportsWebGL = true;
            caps.supportsSpatialAudio = true;
            caps.supportsLowLatencyAudio = false;
            caps.supportsTouch = true;
            caps.supportsKeyboard = true;
            caps.supportsMouse = true;
            caps.supportsGamepad = true;
            caps.supportsWebRTC = true;
            caps.supportsWebSocket = true;
            caps.supportsCloudSave = true;
            caps.supportsIAP = false;
            caps.supportsAchievements = false;
            caps.supportsLeaderboards = false;
            caps.supportsPushNotifications = true;
            caps.supportsThermalManagement = false;
            caps.supportsBackgroundTasks = false;
            caps.supportsGestureRecognition = true;
            caps.supportsAccessibility = true;
            caps.maxTextureSize = 4096;
            caps.maxRenderTargets = 8;
            caps.maxComputeUnits = 256;
            caps.maxMemoryMB = 4096;
            caps.maxThreadCount = 4;
            caps.maxDisplayWidth = 3840;
            caps.maxDisplayHeight = 2160;
            caps.maxRefreshRate = 60;
            caps.supportsHDR = false;
            caps.supportsMultipleDisplays = false;
            break;

        default:
            // Minimal capabilities for unknown platforms
            caps.maxTextureSize = 2048;
            caps.maxRenderTargets = 4;
            caps.maxComputeUnits = 64;
            caps.maxMemoryMB = 1024;
            caps.maxThreadCount = 2;
            caps.maxDisplayWidth = 1920;
            caps.maxDisplayHeight = 1080;
            caps.maxRefreshRate = 60;
            caps.supportsHDR = false;
            caps.supportsMultipleDisplays = false;
            break;
    }

    return caps;
}

// ========== PLATFORM CONFIG UTILITY ==========
PlatformConfig createDefaultConfig(const std::string& appName, PlatformType platformType) {
    PlatformConfig config;

    config.appName = appName;
    config.appVersion = "1.0.0";
    config.bundleId = "com.foundryengine." + toLower(appName);

    // Set platform-specific paths
    switch (platformType) {
        case PlatformType::ANDROID:
            config.dataPath = "/data/data/" + config.bundleId + "/files";
            config.cachePath = "/data/data/" + config.bundleId + "/cache";
            config.tempPath = "/data/data/" + config.bundleId + "/cache/temp";
            break;
        case PlatformType::WINDOWS:
            config.dataPath = "%APPDATA%\\" + appName;
            config.cachePath = "%LOCALAPPDATA%\\" + appName + "\\cache";
            config.tempPath = "%TEMP%\\" + appName;
            break;
        case PlatformType::MACOS:
            config.dataPath = "~/Library/Application Support/" + appName;
            config.cachePath = "~/Library/Caches/" + appName;
            config.tempPath = "/tmp/" + appName;
            break;
        case PlatformType::IOS:
            config.dataPath = "~/Documents";
            config.cachePath = "~/Library/Caches";
            config.tempPath = "/tmp";
            break;
        case PlatformType::WEB:
            config.dataPath = "/persistent";
            config.cachePath = "/session";
            config.tempPath = "/temp";
            break;
        default:
            config.dataPath = "./data";
            config.cachePath = "./cache";
            config.tempPath = "./temp";
            break;
    }

    // Window configuration
    config.windowWidth = 1280;
    config.windowHeight = 720;
    config.fullscreen = false;
    config.resizable = true;
    config.vsync = true;

    // Graphics configuration
    config.graphicsAPI = 0; // Default to Vulkan
    config.msaaSamples = 4;
    config.enableHDR = false;
    config.enableRayTracing = false;

    // Audio configuration
    config.audioSampleRate = 44100;
    config.audioChannels = 2;
    config.audioBufferSize = 512;
    config.enableSpatialAudio = true;

    // Performance configuration
    config.targetFPS = 60;
    config.maxFrameTime = 16;
    config.enableOptimizations = true;
    config.enableMultithreading = true;

    return config;
}

} // namespace FoundryEngine
