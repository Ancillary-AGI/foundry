/**
 * @file AdvancedTypeScriptRuntime.h
 * @brief Advanced TypeScript Runtime with JIT compilation and native performance
 * @author FoundryEngine Team
 * @date 2024
 * @version 2.0.0
 *
 * This file contains the next-generation TypeScript runtime for FoundryEngine.
 * Features include JIT compilation to native code, hot module replacement,
 * advanced type system support, and zero-copy data exchange with C++.
 *
 * Key Features:
 * - Native TypeScript JIT compilation using LLVM
 * - Hot Module Replacement (HMR) with instant updates
 * - Full TypeScript 5.0+ support including decorators
 * - Memory-safe bindings with zero-copy data exchange
 * - Advanced debugging and profiling capabilities
 * - Seamless integration with all engine systems
 */

#pragma once

#include "../core/System.h"
#include "../platform/PlatformInterface.h"
#include "../world/World.h"
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>

namespace FoundryEngine {

// Forward declarations
class TypeScriptJITCompiler;
class TypeScriptModule;
class TypeScriptValue;
class TypeScriptDebugger;
class TypeScriptProfiler;
class HotModuleReplacer;

/**
 * @class AdvancedTypeScriptRuntime
 * @brief Next-generation TypeScript runtime with native performance
 *
 * This runtime provides enterprise-grade TypeScript execution with:
 * - JIT compilation to native machine code
 * - Hot module replacement for instant development feedback
 * - Advanced type system with full TypeScript 5.0+ support
 * - Memory-safe bindings with zero-copy data exchange
 * - Comprehensive debugging and profiling tools
 * - Seamless integration with all engine systems
 */
class AdvancedTypeScriptRuntime : public System {
public:
    /**
     * @brief Runtime configuration options
     */
    struct RuntimeConfig {
        bool enableJIT = true;                    ///< Enable JIT compilation
        bool enableHMR = true;                    ///< Enable Hot Module Replacement
        bool enableDebugging = true;              ///< Enable debugging support
        bool enableProfiling = true;              ///< Enable performance profiling
        bool enableTypeChecking = true;          ///< Enable runtime type checking
        bool enableOptimizations = true;         ///< Enable code optimizations
        size_t maxMemoryUsage = 1024 * 1024 * 1024; ///< Max memory usage (1GB)
        size_t compilationThreads = 4;           ///< Number of compilation threads
        std::string projectRoot = ".";           ///< Project root directory
        std::string cacheDirectory = ".foundry/cache"; ///< Compilation cache directory
        std::vector<std::string> includePaths;   ///< Additional include paths
        std::vector<std::string> libraryPaths;   ///< Library search paths
    };

    /**
     * @brief Module compilation result
     */
    struct CompilationResult {
        bool success = false;                     ///< Compilation success
        std::string moduleName;                   ///< Module name
        std::string nativeCode;                   ///< Generated native code
        std::vector<std::string> errors;         ///< Compilation errors
        std::vector<std::string> warnings;       ///< Compilation warnings
        std::chrono::milliseconds compilationTime; ///< Time taken to compile
        size_t codeSize = 0;                     ///< Size of generated code
        size_t optimizationLevel = 0;           ///< Applied optimization level
    };

    /**
     * @brief Performance metrics
     */
    struct PerformanceMetrics {
        std::chrono::milliseconds totalCompilationTime{0}; ///< Total compilation time
        std::chrono::milliseconds totalExecutionTime{0};   ///< Total execution time
        size_t modulesCompiled = 0;               ///< Number of modules compiled
        size_t functionsExecuted = 0;            ///< Number of functions executed
        size_t memoryUsage = 0;                  ///< Current memory usage
        size_t peakMemoryUsage = 0;              ///< Peak memory usage
        double averageFrameTime = 0.0;           ///< Average frame execution time
        size_t hotReloads = 0;                   ///< Number of hot reloads performed
    };

    AdvancedTypeScriptRuntime();
    ~AdvancedTypeScriptRuntime();

    // System interface
    bool initialize(const RuntimeConfig& config = RuntimeConfig{}) override;
    void shutdown() override;
    void update(float deltaTime) override;

    // Module management
    CompilationResult compileModule(const std::string& moduleName, const std::string& sourceCode);
    CompilationResult compileFile(const std::string& filePath);
    CompilationResult compileProject(const std::string& projectPath);
    bool loadModule(const std::string& moduleName);
    bool unloadModule(const std::string& moduleName);
    bool reloadModule(const std::string& moduleName);
    std::vector<std::string> getLoadedModules() const;

    // Hot Module Replacement
    bool enableHMR(const std::string& watchDirectory);
    void disableHMR();
    bool isHMREnabled() const;
    void onFileChanged(const std::string& filePath);

    // Function execution
    TypeScriptValue callFunction(const std::string& moduleName, const std::string& functionName, 
                                const std::vector<TypeScriptValue>& args = {});
    TypeScriptValue callGlobalFunction(const std::string& functionName, 
                                      const std::vector<TypeScriptValue>& args = {});

    // Native function registration
    using NativeFunction = std::function<TypeScriptValue(const std::vector<TypeScriptValue>&)>;
    void registerNativeFunction(const std::string& name, NativeFunction function);
    void registerNativeClass(const std::string& className, const std::unordered_map<std::string, NativeFunction>& methods);
    void registerEngineBindings();

    // Global state management
    void setGlobalVariable(const std::string& name, const TypeScriptValue& value);
    TypeScriptValue getGlobalVariable(const std::string& name);
    void clearGlobalVariables();

    // Debugging support
    TypeScriptDebugger* getDebugger() const { return debugger_.get(); }
    bool setBreakpoint(const std::string& moduleName, int line);
    bool removeBreakpoint(const std::string& moduleName, int line);
    void stepInto();
    void stepOver();
    void stepOut();
    void continue_();
    std::vector<std::string> getCallStack() const;
    std::unordered_map<std::string, TypeScriptValue> getLocalVariables() const;

    // Profiling support
    TypeScriptProfiler* getProfiler() const { return profiler_.get(); }
    void startProfiling();
    void stopProfiling();
    PerformanceMetrics getPerformanceMetrics() const;
    void resetMetrics();

    // Error handling
    void setErrorHandler(std::function<void(const std::string&)> handler);
    void setWarningHandler(std::function<void(const std::string&)> handler);

    // Configuration
    const RuntimeConfig& getConfig() const { return config_; }
    void updateConfig(const RuntimeConfig& config);

private:
    class AdvancedTypeScriptRuntimeImpl;
    std::unique_ptr<AdvancedTypeScriptRuntimeImpl> impl_;

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

    // Internal methods
    void setupEngineBindings();
    void setupPlatformBindings();
    void setupMathBindings();
    void setupGraphicsBindings();
    void setupPhysicsBindings();
    void setupAudioBindings();
    void setupNetworkingBindings();
    void setupUIBindings();
    void setupFileSystemBindings();
    void setupUtilityBindings();

    bool validateModule(const std::string& moduleName, const std::string& sourceCode);
    void updateMetrics();
    void handleError(const std::string& error);
    void handleWarning(const std::string& warning);
};

/**
 * @class TypeScriptJITCompiler
 * @brief Just-In-Time compiler for TypeScript to native code
 *
 * This compiler uses LLVM to generate optimized native machine code
 * from TypeScript source, providing near-native performance.
 */
class TypeScriptJITCompiler {
public:
    /**
     * @brief Compilation optimization levels
     */
    enum class OptimizationLevel {
        None = 0,        ///< No optimizations
        Basic = 1,       ///< Basic optimizations
        Aggressive = 2,  ///< Aggressive optimizations
        Maximum = 3      ///< Maximum optimizations
    };

    /**
     * @brief Target architecture for compilation
     */
    enum class TargetArch {
        X86_64,          ///< x86-64 architecture
        ARM64,           ///< ARM64 architecture
        WASM32,          ///< WebAssembly 32-bit
        Auto             ///< Auto-detect current architecture
    };

    TypeScriptJITCompiler();
    ~TypeScriptJITCompiler();

    bool initialize(TargetArch arch = TargetArch::Auto);
    void shutdown();

    // Compilation
    AdvancedTypeScriptRuntime::CompilationResult compile(const std::string& moduleName, 
                                                        const std::string& sourceCode,
                                                        OptimizationLevel optimization = OptimizationLevel::Aggressive);
    
    // Code generation
    std::string generateNativeCode(const std::string& typescriptAST, TargetArch arch);
    std::string generateBindings(const std::string& typescriptInterface);
    
    // Optimization
    void setOptimizationLevel(OptimizationLevel level) { optimizationLevel_ = level; }
    OptimizationLevel getOptimizationLevel() const { return optimizationLevel_; }

    // Cache management
    bool isCached(const std::string& moduleName, const std::string& sourceHash) const;
    void cacheCompiledModule(const std::string& moduleName, const std::string& sourceHash, 
                           const std::string& nativeCode);
    std::string getCachedModule(const std::string& moduleName, const std::string& sourceHash) const;
    void clearCache();

private:
    class TypeScriptJITCompilerImpl;
    std::unique_ptr<TypeScriptJITCompilerImpl> impl_;

    TargetArch targetArch_;
    OptimizationLevel optimizationLevel_;
    std::string cacheDirectory_;
    
    // Internal compilation methods
    std::string parseTypeScript(const std::string& sourceCode);
    std::string optimizeAST(const std::string& ast);
    std::string generateLLVMIR(const std::string& optimizedAST);
    std::string compileLLVMIR(const std::string& llvmIR, TargetArch arch);
    
    std::string calculateSourceHash(const std::string& sourceCode) const;
    bool validateGeneratedCode(const std::string& nativeCode) const;
};

/**
 * @class TypeScriptModule
 * @brief Represents a compiled TypeScript module with native execution
 */
class TypeScriptModule {
public:
    /**
     * @brief Module execution state
     */
    enum class ExecutionState {
        NotLoaded,       ///< Module not loaded
        Loading,         ///< Module is loading
        Loaded,          ///< Module loaded successfully
        Error,           ///< Module failed to load
        Executing        ///< Module is currently executing
    };

    TypeScriptModule(const std::string& name, const std::string& sourceCode, 
                    const std::string& nativeCode);
    ~TypeScriptModule();

    // Module information
    const std::string& getName() const { return name_; }
    const std::string& getSourceCode() const { return sourceCode_; }
    const std::string& getNativeCode() const { return nativeCode_; }
    ExecutionState getState() const { return state_; }

    // Execution
    bool load();
    void unload();
    TypeScriptValue callFunction(const std::string& functionName, 
                               const std::vector<TypeScriptValue>& args);
    
    // Hot reload
    bool reload(const std::string& newSourceCode, const std::string& newNativeCode);
    bool canHotReload() const;

    // Exports
    std::vector<std::string> getExportedFunctions() const;
    std::vector<std::string> getExportedClasses() const;
    std::vector<std::string> getExportedVariables() const;

    // Dependencies
    void addDependency(std::shared_ptr<TypeScriptModule> dependency);
    void removeDependency(const std::string& dependencyName);
    std::vector<std::string> getDependencies() const;

private:
    std::string name_;
    std::string sourceCode_;
    std::string nativeCode_;
    ExecutionState state_;
    
    void* nativeHandle_;  // Platform-specific handle to loaded native code
    std::unordered_map<std::string, void*> exportedFunctions_;
    std::vector<std::shared_ptr<TypeScriptModule>> dependencies_;
    
    mutable std::mutex stateMutex_;
    
    bool loadNativeCode();
    void unloadNativeCode();
    void* getFunctionPointer(const std::string& functionName);
};

/**
 * @class HotModuleReplacer
 * @brief Handles hot module replacement for instant development feedback
 */
class HotModuleReplacer {
public:
    HotModuleReplacer(AdvancedTypeScriptRuntime* runtime);
    ~HotModuleReplacer();

    bool initialize(const std::string& watchDirectory);
    void shutdown();

    void startWatching();
    void stopWatching();
    bool isWatching() const { return isWatching_; }

    // File watching
    void addWatchPath(const std::string& path);
    void removeWatchPath(const std::string& path);
    std::vector<std::string> getWatchPaths() const;

    // Hot reload callbacks
    void setPreReloadCallback(std::function<void(const std::string&)> callback);
    void setPostReloadCallback(std::function<void(const std::string&, bool)> callback);

private:
    AdvancedTypeScriptRuntime* runtime_;
    std::atomic<bool> isWatching_{false};
    std::vector<std::string> watchPaths_;
    std::thread watchThread_;
    
    std::function<void(const std::string&)> preReloadCallback_;
    std::function<void(const std::string&, bool)> postReloadCallback_;
    
    void watchThreadFunction();
    void handleFileChange(const std::string& filePath);
    bool shouldReload(const std::string& filePath) const;
};

/**
 * @class TypeScriptValue
 * @brief Enhanced TypeScript value with native performance and type safety
 */
class TypeScriptValue {
public:
    enum class Type {
        Undefined,
        Null,
        Boolean,
        Number,
        BigInt,
        String,
        Symbol,
        Object,
        Function,
        Array,
        Promise,
        NativePointer  // For zero-copy native data
    };

    // Constructors
    TypeScriptValue();
    TypeScriptValue(bool value);
    TypeScriptValue(int32_t value);
    TypeScriptValue(uint32_t value);
    TypeScriptValue(int64_t value);
    TypeScriptValue(uint64_t value);
    TypeScriptValue(float value);
    TypeScriptValue(double value);
    TypeScriptValue(const std::string& value);
    TypeScriptValue(const char* value);
    TypeScriptValue(void* nativePointer, const std::string& typeName);

    // Copy and move semantics
    TypeScriptValue(const TypeScriptValue& other);
    TypeScriptValue(TypeScriptValue&& other) noexcept;
    TypeScriptValue& operator=(const TypeScriptValue& other);
    TypeScriptValue& operator=(TypeScriptValue&& other) noexcept;

    ~TypeScriptValue();

    // Type checking
    Type getType() const { return type_; }
    bool isUndefined() const { return type_ == Type::Undefined; }
    bool isNull() const { return type_ == Type::Null; }
    bool isBoolean() const { return type_ == Type::Boolean; }
    bool isNumber() const { return type_ == Type::Number; }
    bool isBigInt() const { return type_ == Type::BigInt; }
    bool isString() const { return type_ == Type::String; }
    bool isSymbol() const { return type_ == Type::Symbol; }
    bool isObject() const { return type_ == Type::Object; }
    bool isFunction() const { return type_ == Type::Function; }
    bool isArray() const { return type_ == Type::Array; }
    bool isPromise() const { return type_ == Type::Promise; }
    bool isNativePointer() const { return type_ == Type::NativePointer; }

    // Value conversion
    bool toBoolean() const;
    int32_t toInt32() const;
    uint32_t toUint32() const;
    int64_t toInt64() const;
    uint64_t toUint64() const;
    float toFloat() const;
    double toDouble() const;
    std::string toString() const;
    void* toNativePointer() const;

    // Object operations
    TypeScriptValue getProperty(const std::string& name) const;
    void setProperty(const std::string& name, const TypeScriptValue& value);
    bool hasProperty(const std::string& name) const;
    std::vector<std::string> getPropertyNames() const;

    // Array operations
    size_t getArrayLength() const;
    TypeScriptValue getArrayElement(size_t index) const;
    void setArrayElement(size_t index, const TypeScriptValue& value);
    void pushArrayElement(const TypeScriptValue& value);
    TypeScriptValue popArrayElement();

    // Function operations
    TypeScriptValue call(const std::vector<TypeScriptValue>& args) const;
    TypeScriptValue callAsMethod(const TypeScriptValue& thisValue, 
                                const std::vector<TypeScriptValue>& args) const;

    // Promise operations
    bool isPromiseResolved() const;
    bool isPromiseRejected() const;
    TypeScriptValue getPromiseResult() const;
    void resolvePromise(const TypeScriptValue& value);
    void rejectPromise(const TypeScriptValue& reason);

    // Static factory methods
    static TypeScriptValue undefined();
    static TypeScriptValue null();
    static TypeScriptValue boolean(bool value);
    static TypeScriptValue number(double value);
    static TypeScriptValue string(const std::string& value);
    static TypeScriptValue object();
    static TypeScriptValue array();
    static TypeScriptValue promise();
    static TypeScriptValue nativePointer(void* ptr, const std::string& typeName);

    // Utility
    std::string getTypeName() const;
    size_t getMemoryUsage() const;
    TypeScriptValue clone() const;

private:
    Type type_;
    
    union {
        bool boolValue_;
        double numberValue_;
        int64_t bigIntValue_;
        void* pointerValue_;
    };
    
    std::string stringValue_;
    std::string nativeTypeName_;
    std::unordered_map<std::string, TypeScriptValue> objectProperties_;
    std::vector<TypeScriptValue> arrayElements_;
    std::function<TypeScriptValue(const std::vector<TypeScriptValue>&)> functionValue_;
    
    // Promise state
    enum class PromiseState { Pending, Resolved, Rejected };
    PromiseState promiseState_ = PromiseState::Pending;
    TypeScriptValue promiseResult_;
    
    void cleanup();
    void copyFrom(const TypeScriptValue& other);
    void moveFrom(TypeScriptValue&& other);
};

} // namespace FoundryEngine