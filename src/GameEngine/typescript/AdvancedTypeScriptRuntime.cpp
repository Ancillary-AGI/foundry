/**
 * @file AdvancedTypeScriptRuntime.cpp
 * @brief Implementation of advanced TypeScript runtime with JIT compilation
 */

#include "GameEngine/typescript/AdvancedTypeScriptRuntime.h"
#include <thread>
#include <fstream>
#include <sstream>

namespace FoundryEngine {

class AdvancedTypeScriptRuntime::AdvancedTypeScriptRuntimeImpl {
public:
    RuntimeConfig config_;
    std::unique_ptr<TypeScriptJITCompiler> compiler_;
    std::unique_ptr<TypeScriptDebugger> debugger_;
    std::unique_ptr<TypeScriptProfiler> profiler_;
    std::unique_ptr<HotModuleReplacer> hmr_;
    
    std::unordered_map<std::string, std::shared_ptr<TypeScriptModule>> modules_;
    std::unordered_map<std::string, NativeFunction> nativeFunctions_;
    std::unordered_map<std::string, TypeScriptValue> globalVariables_;
    
    mutable std::mutex modulesMutex_;
    mutable std::mutex globalsMutex_;
    std::atomic<bool> isInitialized_{false};
    
    PerformanceMetrics metrics_;
    std::function<void(const std::string&)> errorHandler_;
    std::function<void(const std::string&)> warningHandler_;
};

AdvancedTypeScriptRuntime::AdvancedTypeScriptRuntime() 
    : impl_(std::make_unique<AdvancedTypeScriptRuntimeImpl>()) {}

AdvancedTypeScriptRuntime::~AdvancedTypeScriptRuntime() = default;

bool AdvancedTypeScriptRuntime::initialize(const RuntimeConfig& config) {
    impl_->config_ = config;
    
    // Initialize JIT compiler
    impl_->compiler_ = std::make_unique<TypeScriptJITCompiler>();
    if (!impl_->compiler_->initialize()) {
        return false;
    }
    
    // Initialize debugger
    if (config.enableDebugging) {
        impl_->debugger_ = std::make_unique<TypeScriptDebugger>();
        impl_->debugger_->initialize();
    }
    
    // Initialize profiler
    if (config.enableProfiling) {
        impl_->profiler_ = std::make_unique<TypeScriptProfiler>();
        impl_->profiler_->initialize();
    }
    
    // Initialize hot module replacement
    if (config.enableHMR) {
        impl_->hmr_ = std::make_unique<HotModuleReplacer>(this);
        impl_->hmr_->initialize(config.projectRoot);
    }
    
    // Set up engine bindings
    setupEngineBindings();
    
    impl_->isInitialized_ = true;
    return true;
}

void AdvancedTypeScriptRuntime::shutdown() {
    impl_->isInitialized_ = false;
    
    if (impl_->hmr_) {
        impl_->hmr_->shutdown();
    }
    
    if (impl_->profiler_) {
        impl_->profiler_->shutdown();
    }
    
    if (impl_->debugger_) {
        impl_->debugger_->shutdown();
    }
    
    if (impl_->compiler_) {
        impl_->compiler_->shutdown();
    }
    
    // Clear all modules
    std::lock_guard<std::mutex> lock(impl_->modulesMutex_);
    impl_->modules_.clear();
}

void AdvancedTypeScriptRuntime::update(float deltaTime) {
    if (!impl_->isInitialized_) return;
    
    // Update profiler
    if (impl_->profiler_) {
        impl_->profiler_->update(deltaTime);
    }
    
    // Update HMR
    if (impl_->hmr_) {
        impl_->hmr_->update(deltaTime);
    }
    
    // Update performance metrics
    updateMetrics();
}

AdvancedTypeScriptRuntime::CompilationResult AdvancedTypeScriptRuntime::compileModule(
    const std::string& moduleName, const std::string& sourceCode) {
    
    if (!impl_->compiler_) {
        CompilationResult result;
        result.success = false;
        result.errors.push_back("Compiler not initialized");
        return result;
    }
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Compile TypeScript to native code
    auto result = impl_->compiler_->compile(moduleName, sourceCode);
    
    if (result.success) {
        // Create module
        auto module = std::make_shared<TypeScriptModule>(moduleName, sourceCode, result.nativeCode);
        
        if (module->load()) {
            std::lock_guard<std::mutex> lock(impl_->modulesMutex_);
            impl_->modules_[moduleName] = module;
            impl_->metrics_.modulesCompiled++;
        } else {
            result.success = false;
            result.errors.push_back("Failed to load compiled module");
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    result.compilationTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    impl_->metrics_.totalCompilationTime += result.compilationTime;
    
    return result;
}

AdvancedTypeScriptRuntime::CompilationResult AdvancedTypeScriptRuntime::compileFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        CompilationResult result;
        result.success = false;
        result.errors.push_back("Failed to open file: " + filePath);
        return result;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string sourceCode = buffer.str();
    
    // Extract module name from file path
    std::string moduleName = filePath;
    size_t lastSlash = moduleName.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        moduleName = moduleName.substr(lastSlash + 1);
    }
    size_t lastDot = moduleName.find_last_of('.');
    if (lastDot != std::string::npos) {
        moduleName = moduleName.substr(0, lastDot);
    }
    
    return compileModule(moduleName, sourceCode);
}

bool AdvancedTypeScriptRuntime::loadModule(const std::string& moduleName) {
    std::lock_guard<std::mutex> lock(impl_->modulesMutex_);
    
    auto it = impl_->modules_.find(moduleName);
    if (it != impl_->modules_.end()) {
        return it->second->load();
    }
    
    return false;
}

bool AdvancedTypeScriptRuntime::unloadModule(const std::string& moduleName) {
    std::lock_guard<std::mutex> lock(impl_->modulesMutex_);
    
    auto it = impl_->modules_.find(moduleName);
    if (it != impl_->modules_.end()) {
        it->second->unload();
        impl_->modules_.erase(it);
        return true;
    }
    
    return false;
}

bool AdvancedTypeScriptRuntime::reloadModule(const std::string& moduleName) {
    std::lock_guard<std::mutex> lock(impl_->modulesMutex_);
    
    auto it = impl_->modules_.find(moduleName);
    if (it != impl_->modules_.end()) {
        // Reload with existing source code
        return it->second->reload(it->second->getSourceCode(), it->second->getNativeCode());
    }
    
    return false;
}

std::vector<std::string> AdvancedTypeScriptRuntime::getLoadedModules() const {
    std::lock_guard<std::mutex> lock(impl_->modulesMutex_);
    
    std::vector<std::string> modules;
    for (const auto& [name, module] : impl_->modules_) {
        modules.push_back(name);
    }
    
    return modules;
}

TypeScriptValue AdvancedTypeScriptRuntime::callFunction(const std::string& moduleName, 
                                                       const std::string& functionName,
                                                       const std::vector<TypeScriptValue>& args) {
    std::lock_guard<std::mutex> lock(impl_->modulesMutex_);
    
    auto it = impl_->modules_.find(moduleName);
    if (it != impl_->modules_.end()) {
        auto startTime = std::chrono::high_resolution_clock::now();
        
        TypeScriptValue result = it->second->callFunction(functionName, args);
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        
        impl_->metrics_.functionsExecuted++;
        impl_->metrics_.totalExecutionTime += std::chrono::milliseconds(duration.count() / 1000);
        
        return result;
    }
    
    return TypeScriptValue::undefined();
}

TypeScriptValue AdvancedTypeScriptRuntime::callGlobalFunction(const std::string& functionName,
                                                             const std::vector<TypeScriptValue>& args) {
    // Look for function in native functions first
    auto it = impl_->nativeFunctions_.find(functionName);
    if (it != impl_->nativeFunctions_.end()) {
        return it->second(args);
    }
    
    // Look for function in loaded modules
    std::lock_guard<std::mutex> lock(impl_->modulesMutex_);
    for (const auto& [moduleName, module] : impl_->modules_) {
        auto exportedFunctions = module->getExportedFunctions();
        if (std::find(exportedFunctions.begin(), exportedFunctions.end(), functionName) != exportedFunctions.end()) {
            return module->callFunction(functionName, args);
        }
    }
    
    return TypeScriptValue::undefined();
}

void AdvancedTypeScriptRuntime::registerNativeFunction(const std::string& name, NativeFunction function) {
    impl_->nativeFunctions_[name] = function;
}

void AdvancedTypeScriptRuntime::registerEngineBindings() {
    setupEngineBindings();
    setupPlatformBindings();
    setupMathBindings();
    setupGraphicsBindings();
    setupPhysicsBindings();
    setupAudioBindings();
    setupNetworkingBindings();
    setupUIBindings();
    setupFileSystemBindings();
    setupUtilityBindings();
}

void AdvancedTypeScriptRuntime::setGlobalVariable(const std::string& name, const TypeScriptValue& value) {
    std::lock_guard<std::mutex> lock(impl_->globalsMutex_);
    impl_->globalVariables_[name] = value;
}

TypeScriptValue AdvancedTypeScriptRuntime::getGlobalVariable(const std::string& name) {
    std::lock_guard<std::mutex> lock(impl_->globalsMutex_);
    
    auto it = impl_->globalVariables_.find(name);
    if (it != impl_->globalVariables_.end()) {
        return it->second;
    }
    
    return TypeScriptValue::undefined();
}

void AdvancedTypeScriptRuntime::clearGlobalVariables() {
    std::lock_guard<std::mutex> lock(impl_->globalsMutex_);
    impl_->globalVariables_.clear();
}

bool AdvancedTypeScriptRuntime::enableHMR(const std::string& watchDirectory) {
    if (!impl_->hmr_) {
        impl_->hmr_ = std::make_unique<HotModuleReplacer>(this);
        impl_->hmr_->initialize(watchDirectory);
    }
    
    impl_->hmr_->addWatchPath(watchDirectory);
    impl_->hmr_->startWatching();
    
    return true;
}

void AdvancedTypeScriptRuntime::disableHMR() {
    if (impl_->hmr_) {
        impl_->hmr_->stopWatching();
    }
}

bool AdvancedTypeScriptRuntime::isHMREnabled() const {
    return impl_->hmr_ && impl_->hmr_->isWatching();
}

void AdvancedTypeScriptRuntime::onFileChanged(const std::string& filePath) {
    // Recompile and reload the changed module
    auto result = compileFile(filePath);
    if (result.success) {
        impl_->metrics_.hotReloads++;
        
        if (impl_->hmr_) {
            // Notify HMR system
        }
    } else {
        handleError("Hot reload failed for " + filePath);
    }
}

AdvancedTypeScriptRuntime::PerformanceMetrics AdvancedTypeScriptRuntime::getPerformanceMetrics() const {
    return impl_->metrics_;
}

void AdvancedTypeScriptRuntime::resetMetrics() {
    impl_->metrics_ = PerformanceMetrics{};
}

void AdvancedTypeScriptRuntime::setErrorHandler(std::function<void(const std::string&)> handler) {
    impl_->errorHandler_ = handler;
}

void AdvancedTypeScriptRuntime::setWarningHandler(std::function<void(const std::string&)> handler) {
    impl_->warningHandler_ = handler;
}

const AdvancedTypeScriptRuntime::RuntimeConfig& AdvancedTypeScriptRuntime::getConfig() const {
    return impl_->config_;
}

void AdvancedTypeScriptRuntime::updateConfig(const RuntimeConfig& config) {
    impl_->config_ = config;
}

void AdvancedTypeScriptRuntime::setupEngineBindings() {
    // Register core engine functions
    registerNativeFunction("log", [](const std::vector<TypeScriptValue>& args) -> TypeScriptValue {
        if (!args.empty()) {
            std::cout << args[0].toString() << std::endl;
        }
        return TypeScriptValue::undefined();
    });
    
    registerNativeFunction("error", [](const std::vector<TypeScriptValue>& args) -> TypeScriptValue {
        if (!args.empty()) {
            std::cerr << "Error: " << args[0].toString() << std::endl;
        }
        return TypeScriptValue::undefined();
    });
    
    registerNativeFunction("warn", [](const std::vector<TypeScriptValue>& args) -> TypeScriptValue {
        if (!args.empty()) {
            std::cout << "Warning: " << args[0].toString() << std::endl;
        }
        return TypeScriptValue::undefined();
    });
}

void AdvancedTypeScriptRuntime::setupPlatformBindings() {
    // Platform-specific bindings
    registerNativeFunction("getPlatform", [](const std::vector<TypeScriptValue>& args) -> TypeScriptValue {
#ifdef _WIN32
        return TypeScriptValue::string("windows");
#elif defined(__APPLE__)
        return TypeScriptValue::string("macos");
#elif defined(__linux__)
        return TypeScriptValue::string("linux");
#else
        return TypeScriptValue::string("unknown");
#endif
    });
}

void AdvancedTypeScriptRuntime::setupMathBindings() {
    // Math utility functions
    registerNativeFunction("sqrt", [](const std::vector<TypeScriptValue>& args) -> TypeScriptValue {
        if (!args.empty() && args[0].isNumber()) {
            return TypeScriptValue::number(std::sqrt(args[0].toDouble()));
        }
        return TypeScriptValue::number(0.0);
    });
    
    registerNativeFunction("sin", [](const std::vector<TypeScriptValue>& args) -> TypeScriptValue {
        if (!args.empty() && args[0].isNumber()) {
            return TypeScriptValue::number(std::sin(args[0].toDouble()));
        }
        return TypeScriptValue::number(0.0);
    });
    
    registerNativeFunction("cos", [](const std::vector<TypeScriptValue>& args) -> TypeScriptValue {
        if (!args.empty() && args[0].isNumber()) {
            return TypeScriptValue::number(std::cos(args[0].toDouble()));
        }
        return TypeScriptValue::number(0.0);
    });
}

void AdvancedTypeScriptRuntime::setupGraphicsBindings() {
    // Graphics system bindings would go here
}

void AdvancedTypeScriptRuntime::setupPhysicsBindings() {
    // Physics system bindings would go here
}

void AdvancedTypeScriptRuntime::setupAudioBindings() {
    // Audio system bindings would go here
}

void AdvancedTypeScriptRuntime::setupNetworkingBindings() {
    // Networking system bindings would go here
}

void AdvancedTypeScriptRuntime::setupUIBindings() {
    // UI system bindings would go here
}

void AdvancedTypeScriptRuntime::setupFileSystemBindings() {
    // File system bindings would go here
}

void AdvancedTypeScriptRuntime::setupUtilityBindings() {
    // Utility function bindings would go here
}

void AdvancedTypeScriptRuntime::updateMetrics() {
    // Update performance metrics
    impl_->metrics_.memoryUsage = getCurrentMemoryUsage();
    impl_->metrics_.peakMemoryUsage = std::max(impl_->metrics_.peakMemoryUsage, impl_->metrics_.memoryUsage);
    
    // Calculate average frame time
    static float frameTimeAccumulator = 0.0f;
    static int frameCount = 0;
    
    frameTimeAccumulator += getCurrentFrameTime();
    frameCount++;
    
    if (frameCount >= 60) { // Update every 60 frames
        impl_->metrics_.averageFrameTime = frameTimeAccumulator / frameCount;
        frameTimeAccumulator = 0.0f;
        frameCount = 0;
    }
}

void AdvancedTypeScriptRuntime::handleError(const std::string& error) {
    if (impl_->errorHandler_) {
        impl_->errorHandler_(error);
    } else {
        std::cerr << "TypeScript Runtime Error: " << error << std::endl;
    }
}

void AdvancedTypeScriptRuntime::handleWarning(const std::string& warning) {
    if (impl_->warningHandler_) {
        impl_->warningHandler_(warning);
    } else {
        std::cout << "TypeScript Runtime Warning: " << warning << std::endl;
    }
}

size_t AdvancedTypeScriptRuntime::getCurrentMemoryUsage() const {
    // Get current memory usage
    // This would use platform-specific APIs
    return 0; // Placeholder
}

float AdvancedTypeScriptRuntime::getCurrentFrameTime() const {
    // Get current frame time
    // This would be provided by the engine
    return 16.67f; // Placeholder for 60 FPS
}

} // namespace FoundryEngine