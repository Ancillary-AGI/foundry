#include "../../include/GameEngine/typescript/NativeTypeScriptRuntime.h"
#include "../../include/GameEngine/core/Engine.h"
#include "../../include/GameEngine/platform/PlatformInterface.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <future>
#include <filesystem>
#include <unordered_map>
#include <memory>

namespace FoundryEngine {

class NativeTypeScriptRuntimeImpl {
private:
    std::unique_ptr<TypeScriptCompiler> compiler_;
    std::unique_ptr<TypeScriptVM> vm_;
    std::unordered_map<std::string, std::shared_ptr<TypeScriptModule>> loadedModules_;
    std::string projectRoot_;
    std::string buildOutput_;
    PlatformType targetPlatform_;
    bool isInitialized_ = false;

    // TypeScript to native code generation
    struct CodeGenerator {
        std::string generateNativeCode(const std::string& typescriptCode, PlatformType platform);
        std::string generateCppBindings(const std::string& typescriptCode);
        std::string generatePlatformSpecificCode(const std::string& typescriptCode, PlatformType platform);
    } codeGenerator_;

public:
    NativeTypeScriptRuntimeImpl() = default;
    ~NativeTypeScriptRuntimeImpl() = default;

    bool initialize(const std::string& projectRoot, PlatformType targetPlatform) {
        projectRoot_ = projectRoot;
        targetPlatform_ = targetPlatform;
        buildOutput_ = projectRoot_ + "/build/" + getPlatformName(targetPlatform_);

        // Create build directory
        std::filesystem::create_directories(buildOutput_);

        // Initialize TypeScript compiler
        compiler_ = std::make_unique<TypeScriptCompiler>();
        if (!compiler_->initialize()) {
            std::cerr << "Failed to initialize TypeScript compiler" << std::endl;
            return false;
        }

        // Initialize TypeScript VM for runtime execution
        vm_ = std::make_unique<TypeScriptVM>();
        if (!vm_->initialize()) {
            std::cerr << "Failed to initialize TypeScript VM" << std::endl;
            return false;
        }

        // Set up platform-specific bindings
        setupPlatformBindings();

        isInitialized_ = true;
        std::cout << "Native TypeScript Runtime initialized for " << getPlatformName(targetPlatform_) << std::endl;
        return true;
    }

    void shutdown() {
        if (vm_) {
            vm_->shutdown();
        }
        if (compiler_) {
            compiler_->shutdown();
        }
        loadedModules_.clear();
        isInitialized_ = false;
    }

    bool compileProject() {
        if (!isInitialized_) {
            return false;
        }

        std::cout << "Compiling TypeScript project to native code..." << std::endl;

        // Find all TypeScript files
        std::vector<std::string> tsFiles = findTypeScriptFiles(projectRoot_ + "/src");
        
        // Compile each file
        for (const auto& tsFile : tsFiles) {
            if (!compileFile(tsFile)) {
                std::cerr << "Failed to compile: " << tsFile << std::endl;
                return false;
            }
        }

        // Generate platform-specific entry point
        generatePlatformEntryPoint();

        // Link and build native executable
        return buildNativeExecutable();
    }

    bool runProject() {
        if (!isInitialized_) {
            return false;
        }

        // Load and execute the compiled native code
        std::string executablePath = buildOutput_ + "/game";
        
        #ifdef _WIN32
        executablePath += ".exe";
        #endif

        if (!std::filesystem::exists(executablePath)) {
            std::cerr << "Executable not found: " << executablePath << std::endl;
            return false;
        }

        // Execute the native game
        std::string command = executablePath;
        int result = std::system(command.c_str());
        
        return result == 0;
    }

    bool hotReload(const std::string& changedFile) {
        if (!isInitialized_) {
            return false;
        }

        std::cout << "Hot reloading: " << changedFile << std::endl;

        // Recompile the changed file
        if (!compileFile(changedFile)) {
            std::cerr << "Failed to recompile: " << changedFile << std::endl;
            return false;
        }

        // Reload the module in the VM
        std::string moduleName = getModuleName(changedFile);
        auto it = loadedModules_.find(moduleName);
        if (it != loadedModules_.end()) {
            vm_->reloadModule(it->second);
        }

        return true;
    }

private:
    void setupPlatformBindings() {
        // Set up platform-specific API bindings
        vm_->registerNativeFunction("console.log", [](const std::vector<TypeScriptValue>& args) -> TypeScriptValue {
            for (const auto& arg : args) {
                std::cout << arg.toString() << " ";
            }
            std::cout << std::endl;
            return TypeScriptValue::undefined();
        });

        vm_->registerNativeFunction("foundry.engine.initialize", [](const std::vector<TypeScriptValue>& args) -> TypeScriptValue {
            Engine& engine = Engine::getInstance();
            bool success = engine.initialize();
            return TypeScriptValue::boolean(success);
        });

        vm_->registerNativeFunction("foundry.engine.shutdown", [](const std::vector<TypeScriptValue>& args) -> TypeScriptValue {
            Engine& engine = Engine::getInstance();
            engine.shutdown();
            return TypeScriptValue::undefined();
        });

        vm_->registerNativeFunction("foundry.engine.update", [](const std::vector<TypeScriptValue>& args) -> TypeScriptValue {
            Engine& engine = Engine::getInstance();
            float deltaTime = args.size() > 0 ? args[0].toNumber() : 0.016f;
            engine.update(deltaTime);
            return TypeScriptValue::undefined();
        });

        vm_->registerNativeFunction("foundry.engine.render", [](const std::vector<TypeScriptValue>& args) -> TypeScriptValue {
            Engine& engine = Engine::getInstance();
            engine.render();
            return TypeScriptValue::undefined();
        });

        vm_->registerNativeFunction("foundry.world.createEntity", [](const std::vector<TypeScriptValue>& args) -> TypeScriptValue {
            Engine& engine = Engine::getInstance();
            auto world = engine.getWorld();
            uint32_t entityId = world->createEntity();
            return TypeScriptValue::number(static_cast<double>(entityId));
        });

        vm_->registerNativeFunction("foundry.world.destroyEntity", [](const std::vector<TypeScriptValue>& args) -> TypeScriptValue {
            if (args.size() > 0) {
                Engine& engine = Engine::getInstance();
                auto world = engine.getWorld();
                uint32_t entityId = static_cast<uint32_t>(args[0].toNumber());
                world->destroyEntity(entityId);
            }
            return TypeScriptValue::undefined();
        });

        // Add more platform-specific bindings based on target platform
        setupPlatformSpecificBindings();
    }

    void setupPlatformSpecificBindings() {
        switch (targetPlatform_) {
            case PlatformType::WINDOWS:
                setupWindowsBindings();
                break;
            case PlatformType::MACOS:
                setupMacOSBindings();
                break;
            case PlatformType::LINUX:
                setupLinuxBindings();
                break;
            case PlatformType::ANDROID:
                setupAndroidBindings();
                break;
            case PlatformType::IOS:
                setupiOSBindings();
                break;
            case PlatformType::WEB:
                setupWebBindings();
                break;
        }
    }

    void setupWindowsBindings() {
        vm_->registerNativeFunction("foundry.platform.windows.showMessageBox", [](const std::vector<TypeScriptValue>& args) -> TypeScriptValue {
            // Windows-specific message box implementation
            std::string message = args.size() > 0 ? args[0].toString() : "Hello from Windows!";
            MessageBoxA(nullptr, message.c_str(), "FoundryEngine", MB_OK);
            return TypeScriptValue::undefined();
        });
    }

    void setupMacOSBindings() {
        vm_->registerNativeFunction("foundry.platform.macos.showAlert", [](const std::vector<TypeScriptValue>& args) -> TypeScriptValue {
            // macOS-specific alert implementation
            std::string message = args.size() > 0 ? args[0].toString() : "Hello from macOS!";
            // Would use NSAlert in real implementation
            std::cout << "macOS Alert: " << message << std::endl;
            return TypeScriptValue::undefined();
        });
    }

    void setupLinuxBindings() {
        vm_->registerNativeFunction("foundry.platform.linux.showDialog", [](const std::vector<TypeScriptValue>& args) -> TypeScriptValue {
            // Linux-specific dialog implementation
            std::string message = args.size() > 0 ? args[0].toString() : "Hello from Linux!";
            // Would use GTK or Qt in real implementation
            std::cout << "Linux Dialog: " << message << std::endl;
            return TypeScriptValue::undefined();
        });
    }

    void setupAndroidBindings() {
        vm_->registerNativeFunction("foundry.platform.android.showToast", [](const std::vector<TypeScriptValue>& args) -> TypeScriptValue {
            // Android-specific toast implementation
            std::string message = args.size() > 0 ? args[0].toString() : "Hello from Android!";
            // Would use Android Toast in real implementation
            std::cout << "Android Toast: " << message << std::endl;
            return TypeScriptValue::undefined();
        });
    }

    void setupiOSBindings() {
        vm_->registerNativeFunction("foundry.platform.ios.showAlert", [](const std::vector<TypeScriptValue>& args) -> TypeScriptValue {
            // iOS-specific alert implementation
            std::string message = args.size() > 0 ? args[0].toString() : "Hello from iOS!";
            // Would use UIAlertController in real implementation
            std::cout << "iOS Alert: " << message << std::endl;
            return TypeScriptValue::undefined();
        });
    }

    void setupWebBindings() {
        vm_->registerNativeFunction("foundry.platform.web.showAlert", [](const std::vector<TypeScriptValue>& args) -> TypeScriptValue {
            // Web-specific alert implementation
            std::string message = args.size() > 0 ? args[0].toString() : "Hello from Web!";
            // Would use JavaScript alert in real implementation
            std::cout << "Web Alert: " << message << std::endl;
            return TypeScriptValue::undefined();
        });
    }

    std::vector<std::string> findTypeScriptFiles(const std::string& directory) {
        std::vector<std::string> files;
        
        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
                if (entry.is_regular_file() && entry.path().extension() == ".ts") {
                    files.push_back(entry.path().string());
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error scanning directory: " << e.what() << std::endl;
        }
        
        return files;
    }

    bool compileFile(const std::string& tsFile) {
        // Read TypeScript file
        std::ifstream file(tsFile);
        if (!file.is_open()) {
            return false;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string typescriptCode = buffer.str();
        file.close();

        // Compile TypeScript to native code
        std::string nativeCode = codeGenerator_.generateNativeCode(typescriptCode, targetPlatform_);
        
        // Generate output file path
        std::string relativePath = std::filesystem::relative(tsFile, projectRoot_ + "/src").string();
        std::string outputPath = buildOutput_ + "/" + relativePath;
        
        // Change extension to .cpp
        size_t lastDot = outputPath.find_last_of('.');
        if (lastDot != std::string::npos) {
            outputPath = outputPath.substr(0, lastDot) + ".cpp";
        }

        // Create output directory
        std::filesystem::create_directories(std::filesystem::path(outputPath).parent_path());

        // Write compiled code
        std::ofstream outputFile(outputPath);
        if (!outputFile.is_open()) {
            return false;
        }

        outputFile << nativeCode;
        outputFile.close();

        return true;
    }

    void generatePlatformEntryPoint() {
        std::string entryPoint = generateMainFunction();
        
        std::string outputPath = buildOutput_ + "/main.cpp";
        std::ofstream file(outputPath);
        if (file.is_open()) {
            file << entryPoint;
            file.close();
        }
    }

    std::string generateMainFunction() {
        std::stringstream ss;
        
        ss << "#include \"GameEngine/core/Engine.h\"\n";
        ss << "#include \"GameEngine/typescript/NativeTypeScriptRuntime.h\"\n";
        ss << "#include <iostream>\n\n";
        
        ss << "using namespace FoundryEngine;\n\n";
        
        ss << "int main() {\n";
        ss << "    std::cout << \"Starting FoundryEngine TypeScript Game...\" << std::endl;\n\n";
        
        ss << "    // Initialize engine\n";
        ss << "    Engine& engine = Engine::getInstance();\n";
        ss << "    if (!engine.initialize()) {\n";
        ss << "        std::cerr << \"Failed to initialize engine\" << std::endl;\n";
        ss << "        return -1;\n";
        ss << "    }\n\n";
        
        ss << "    // Initialize TypeScript runtime\n";
        ss << "    NativeTypeScriptRuntime runtime;\n";
        ss << "    if (!runtime.initialize(\".\", PlatformType::" << getPlatformName(targetPlatform_) << ")) {\n";
        ss << "        std::cerr << \"Failed to initialize TypeScript runtime\" << std::endl;\n";
        ss << "        engine.shutdown();\n";
        ss << "        return -1;\n";
        ss << "    }\n\n";
        
        ss << "    // Execute TypeScript game code\n";
        ss << "    runtime.executeGameCode();\n\n";
        
        ss << "    // Main game loop\n";
        ss << "    while (engine.isRunning()) {\n";
        ss << "        float deltaTime = engine.getDeltaTime();\n";
        ss << "        engine.update(deltaTime);\n";
        ss << "        runtime.update(deltaTime);\n";
        ss << "        engine.render();\n";
        ss << "    }\n\n";
        
        ss << "    // Cleanup\n";
        ss << "    runtime.shutdown();\n";
        ss << "    engine.shutdown();\n\n";
        
        ss << "    return 0;\n";
        ss << "}\n";
        
        return ss.str();
    }

    bool buildNativeExecutable() {
        std::string cmakeLists = generateCMakeLists();
        
        // Write CMakeLists.txt
        std::string cmakePath = buildOutput_ + "/CMakeLists.txt";
        std::ofstream cmakeFile(cmakePath);
        if (!cmakeFile.is_open()) {
            return false;
        }
        cmakeFile << cmakeLists;
        cmakeFile.close();

        // Build with CMake
        std::string buildCommand = "cd " + buildOutput_ + " && cmake . && cmake --build .";
        int result = std::system(buildCommand.c_str());
        
        return result == 0;
    }

    std::string generateCMakeLists() {
        std::stringstream ss;
        
        ss << "cmake_minimum_required(VERSION 3.16)\n";
        ss << "project(FoundryTypeScriptGame)\n\n";
        
        ss << "set(CMAKE_CXX_STANDARD 20)\n";
        ss << "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n\n";
        
        ss << "# Find FoundryEngine\n";
        ss << "find_package(FoundryEngine REQUIRED)\n\n";
        
        ss << "# Source files\n";
        ss << "file(GLOB_RECURSE SOURCES \"*.cpp\")\n\n";
        
        ss << "# Create executable\n";
        ss << "add_executable(game ${SOURCES})\n\n";
        
        ss << "# Link libraries\n";
        ss << "target_link_libraries(game FoundryEngine)\n\n";
        
        // Platform-specific settings
        switch (targetPlatform_) {
            case PlatformType::WINDOWS:
                ss << "target_link_libraries(game d3d11 dxgi xaudio2 xinput)\n";
                break;
            case PlatformType::MACOS:
                ss << "target_link_libraries(game \"-framework OpenGL\" \"-framework Cocoa\")\n";
                break;
            case PlatformType::LINUX:
                ss << "target_link_libraries(game GL X11 pthread)\n";
                break;
            case PlatformType::ANDROID:
                ss << "target_link_libraries(game log android EGL GLESv2)\n";
                break;
            case PlatformType::IOS:
                ss << "target_link_libraries(game \"-framework OpenGLES\" \"-framework UIKit\")\n";
                break;
            case PlatformType::WEB:
                ss << "# Web platform uses Emscripten\n";
                break;
        }
        
        return ss.str();
    }

    std::string getModuleName(const std::string& filePath) {
        std::filesystem::path path(filePath);
        return path.stem().string();
    }

    std::string getPlatformName(PlatformType platform) {
        switch (platform) {
            case PlatformType::WINDOWS: return "WINDOWS";
            case PlatformType::MACOS: return "MACOS";
            case PlatformType::LINUX: return "LINUX";
            case PlatformType::ANDROID: return "ANDROID";
            case PlatformType::IOS: return "IOS";
            case PlatformType::WEB: return "WEB";
            default: return "UNKNOWN";
        }
    }
};

// CodeGenerator implementation
std::string NativeTypeScriptRuntimeImpl::CodeGenerator::generateNativeCode(const std::string& typescriptCode, PlatformType platform) {
    std::stringstream ss;
    
    ss << "// Auto-generated native code from TypeScript\n";
    ss << "// Platform: " << getPlatformName(platform) << "\n\n";
    
    ss << "#include \"GameEngine/core/Engine.h\"\n";
    ss << "#include \"GameEngine/typescript/NativeTypeScriptRuntime.h\"\n";
    ss << "#include <iostream>\n";
    ss << "#include <memory>\n\n";
    
    ss << "using namespace FoundryEngine;\n\n";
    
    // Parse TypeScript code and generate equivalent C++
    // This is a simplified implementation - in reality, you'd use a proper TypeScript parser
    
    // Convert TypeScript classes to C++ classes
    std::string cppCode = convertTypeScriptToCpp(typescriptCode);
    ss << cppCode;
    
    return ss.str();
}

std::string NativeTypeScriptRuntimeImpl::CodeGenerator::convertTypeScriptToCpp(const std::string& typescriptCode) {
    // Simplified TypeScript to C++ conversion
    // In a real implementation, you'd use a proper AST parser
    
    std::string cppCode = typescriptCode;
    
    // Replace TypeScript syntax with C++ syntax
    // This is a basic example - real implementation would be much more sophisticated
    
    // Replace 'class' with 'class'
    // Replace 'extends' with ': public'
    // Replace 'constructor' with constructor
    // Replace TypeScript types with C++ types
    // etc.
    
    return cppCode;
}

// Public interface implementation
NativeTypeScriptRuntime::NativeTypeScriptRuntime() : impl_(std::make_unique<NativeTypeScriptRuntimeImpl>()) {}
NativeTypeScriptRuntime::~NativeTypeScriptRuntime() = default;

bool NativeTypeScriptRuntime::initialize(const std::string& projectRoot, PlatformType targetPlatform) {
    return impl_->initialize(projectRoot, targetPlatform);
}

void NativeTypeScriptRuntime::shutdown() {
    impl_->shutdown();
}

bool NativeTypeScriptRuntime::compileProject() {
    return impl_->compileProject();
}

bool NativeTypeScriptRuntime::runProject() {
    return impl_->runProject();
}

bool NativeTypeScriptRuntime::hotReload(const std::string& changedFile) {
    return impl_->hotReload(changedFile);
}

void NativeTypeScriptRuntime::executeGameCode() {
    // Execute the main game TypeScript code
    // This would load and execute the compiled native code
}

void NativeTypeScriptRuntime::update(float deltaTime) {
    // Update TypeScript game logic
    // This would call the update methods in the compiled code
}

} // namespace FoundryEngine
