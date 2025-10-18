#pragma once

#include "../core/System.h"
#include "../platform/PlatformInterface.h"
#include <string>
#include <memory>
#include <vector>

namespace FoundryEngine {

// Forward declarations
class TypeScriptCompiler;
class TypeScriptVM;
class TypeScriptModule;
class TypeScriptValue;

/**
 * Native TypeScript Runtime for FoundryEngine
 * Compiles TypeScript code to native C++ for each platform
 * Similar to Flutter's approach with Dart
 */
class NativeTypeScriptRuntime {
public:
    NativeTypeScriptRuntime();
    ~NativeTypeScriptRuntime();
    
    // Initialization
    bool initialize(const std::string& projectRoot, PlatformType targetPlatform);
    void shutdown();
    
    // Compilation and execution
    bool compileProject();
    bool runProject();
    bool hotReload(const std::string& changedFile);
    
    // Runtime execution
    void executeGameCode();
    void update(float deltaTime);

private:
    class NativeTypeScriptRuntimeImpl;
    std::unique_ptr<NativeTypeScriptRuntimeImpl> impl_;
};

/**
 * TypeScript Compiler
 * Compiles TypeScript code to native C++ code
 */
class TypeScriptCompiler {
public:
    TypeScriptCompiler() = default;
    ~TypeScriptCompiler() = default;
    
    bool initialize();
    void shutdown();
    
    // Compile TypeScript to native code
    std::string compileToNative(const std::string& typescriptCode, PlatformType platform);
    std::string compileToCpp(const std::string& typescriptCode);
    
    // Project compilation
    bool compileProject(const std::string& projectRoot, const std::string& outputDir, PlatformType platform);
    
    // Error handling
    struct CompilationError {
        std::string file;
        int line;
        int column;
        std::string message;
        std::string severity; // "error", "warning", "info"
    };
    
    std::vector<CompilationError> getErrors() const { return errors_; }
    std::vector<CompilationError> getWarnings() const { return warnings_; }

private:
    std::vector<CompilationError> errors_;
    std::vector<CompilationError> warnings_;
    
    // TypeScript AST parsing
    class TypeScriptAST;
    std::unique_ptr<TypeScriptAST> parseTypeScript(const std::string& code);
    std::string generateCppFromAST(const TypeScriptAST& ast, PlatformType platform);
};

/**
 * TypeScript Virtual Machine
 * Executes compiled TypeScript code at runtime
 */
class TypeScriptVM {
public:
    TypeScriptVM() = default;
    ~TypeScriptVM() = default;
    
    bool initialize();
    void shutdown();
    
    // Module management
    std::shared_ptr<TypeScriptModule> loadModule(const std::string& name, const std::string& code);
    void unloadModule(const std::string& name);
    void reloadModule(std::shared_ptr<TypeScriptModule> module);
    
    // Function execution
    TypeScriptValue callFunction(const std::string& moduleName, const std::string& functionName, 
                                const std::vector<TypeScriptValue>& args);
    
    // Native function registration
    using NativeFunction = std::function<TypeScriptValue(const std::vector<TypeScriptValue>&)>;
    void registerNativeFunction(const std::string& name, NativeFunction func);
    
    // Global state
    void setGlobalVariable(const std::string& name, const TypeScriptValue& value);
    TypeScriptValue getGlobalVariable(const std::string& name);

private:
    std::unordered_map<std::string, std::shared_ptr<TypeScriptModule>> modules_;
    std::unordered_map<std::string, NativeFunction> nativeFunctions_;
    std::unordered_map<std::string, TypeScriptValue> globalVariables_;
};

/**
 * TypeScript Module
 * Represents a compiled TypeScript module
 */
class TypeScriptModule {
public:
    TypeScriptModule(const std::string& name, const std::string& code);
    ~TypeScriptModule() = default;
    
    const std::string& getName() const { return name_; }
    const std::string& getCode() const { return code_; }
    
    // Function execution
    TypeScriptValue callFunction(const std::string& functionName, const std::vector<TypeScriptValue>& args);
    
    // Hot reload
    void updateCode(const std::string& newCode);

private:
    std::string name_;
    std::string code_;
    std::unordered_map<std::string, std::function<TypeScriptValue(const std::vector<TypeScriptValue>&)>> functions_;
};

/**
 * TypeScript Value
 * Represents a value in the TypeScript runtime
 */
class TypeScriptValue {
public:
    enum Type {
        UNDEFINED,
        NULL,
        BOOLEAN,
        NUMBER,
        STRING,
        OBJECT,
        FUNCTION,
        ARRAY
    };
    
    TypeScriptValue() : type_(UNDEFINED) {}
    TypeScriptValue(bool value) : type_(BOOLEAN), boolValue_(value) {}
    TypeScriptValue(double value) : type_(NUMBER), numberValue_(value) {}
    TypeScriptValue(const std::string& value) : type_(STRING), stringValue_(value) {}
    TypeScriptValue(const char* value) : type_(STRING), stringValue_(value) {}
    
    // Static constructors
    static TypeScriptValue undefined() { return TypeScriptValue(); }
    static TypeScriptValue null() { TypeScriptValue v; v.type_ = NULL; return v; }
    static TypeScriptValue boolean(bool value) { return TypeScriptValue(value); }
    static TypeScriptValue number(double value) { return TypeScriptValue(value); }
    static TypeScriptValue string(const std::string& value) { return TypeScriptValue(value); }
    
    // Type checking
    Type getType() const { return type_; }
    bool isUndefined() const { return type_ == UNDEFINED; }
    bool isNull() const { return type_ == NULL; }
    bool isBoolean() const { return type_ == BOOLEAN; }
    bool isNumber() const { return type_ == NUMBER; }
    bool isString() const { return type_ == STRING; }
    bool isObject() const { return type_ == OBJECT; }
    bool isFunction() const { return type_ == FUNCTION; }
    bool isArray() const { return type_ == ARRAY; }
    
    // Value access
    bool toBoolean() const;
    double toNumber() const;
    std::string toString() const;
    
    // Object access
    TypeScriptValue getProperty(const std::string& name) const;
    void setProperty(const std::string& name, const TypeScriptValue& value);
    
    // Array access
    size_t getArrayLength() const;
    TypeScriptValue getArrayElement(size_t index) const;
    void setArrayElement(size_t index, const TypeScriptValue& value);
    
    // Function call
    TypeScriptValue call(const std::vector<TypeScriptValue>& args) const;

private:
    Type type_;
    bool boolValue_;
    double numberValue_;
    std::string stringValue_;
    std::unordered_map<std::string, TypeScriptValue> objectValue_;
    std::vector<TypeScriptValue> arrayValue_;
    std::function<TypeScriptValue(const std::vector<TypeScriptValue>&)> functionValue_;
};

/**
 * TypeScript Project Template
 * Generates project structure for TypeScript games
 */
class TypeScriptProjectTemplate {
public:
    struct ProjectConfig {
        std::string name;
        std::string description;
        std::string author;
        std::string version;
        std::vector<PlatformType> targetPlatforms;
        bool includeServer;
        bool includeNetworking;
        bool includePhysics;
        bool includeAudio;
        std::string templateType; // "2d", "3d", "vr", "mobile", "web"
    };
    
    static bool createProject(const std::string& projectPath, const ProjectConfig& config);
    static bool createFromTemplate(const std::string& projectPath, const std::string& templateName);
    static bool createFromGitHub(const std::string& projectPath, const std::string& repoUrl);
    
    // Template management
    static std::vector<std::string> getAvailableTemplates();
    static bool installTemplate(const std::string& templateName, const std::string& source);
    static bool uninstallTemplate(const std::string& templateName);

private:
    static std::string generateProjectStructure(const ProjectConfig& config);
    static std::string generateMainTypeScriptFile(const ProjectConfig& config);
    static std::string generatePackageJson(const ProjectConfig& config);
    static std::string generateTsConfig(const ProjectConfig& config);
    static std::string generateCMakeLists(const ProjectConfig& config);
    static std::string generatePlatformSpecificFiles(const ProjectConfig& config, PlatformType platform);
};

/**
 * TypeScript Server Integration
 * Integrates TypeScript with Go server backend
 */
class TypeScriptServerIntegration {
public:
    struct ServerConfig {
        std::string host;
        int port;
        std::string protocol; // "http", "https", "websocket"
        bool enableSSL;
        std::string certFile;
        std::string keyFile;
    };
    
    static bool initializeServer(const ServerConfig& config);
    static void shutdownServer();
    
    // API generation
    static std::string generateTypeScriptClient(const std::string& goServerPath);
    static std::string generateGoServerStub(const std::string& typescriptInterface);
    
    // Runtime integration
    static bool connectToServer(const std::string& url);
    static void disconnectFromServer();
    static TypeScriptValue callServerFunction(const std::string& functionName, const std::vector<TypeScriptValue>& args);
    
    // Real-time communication
    static void onServerEvent(const std::string& eventName, std::function<void(const TypeScriptValue&)> callback);
    static void emitServerEvent(const std::string& eventName, const TypeScriptValue& data);

private:
    static std::string parseGoServerAPI(const std::string& goCode);
    static std::string generateTypeScriptInterface(const std::string& goAPI);
};

} // namespace FoundryEngine
