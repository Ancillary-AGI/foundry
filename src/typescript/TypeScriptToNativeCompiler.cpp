#include "../../include/GameEngine/typescript/TypeScriptToNativeCompiler.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <regex>
#include <unordered_map>
#include <vector>
#include <memory>

namespace FoundryEngine {

class TypeScriptToNativeCompilerImpl {
private:
    struct TypeScriptAST {
        enum NodeType {
            PROGRAM,
            CLASS_DECLARATION,
            FUNCTION_DECLARATION,
            VARIABLE_DECLARATION,
            EXPRESSION_STATEMENT,
            CALL_EXPRESSION,
            MEMBER_EXPRESSION,
            IDENTIFIER,
            LITERAL,
            BINARY_EXPRESSION,
            ASSIGNMENT_EXPRESSION,
            IF_STATEMENT,
            FOR_STATEMENT,
            WHILE_STATEMENT,
            RETURN_STATEMENT,
            BLOCK_STATEMENT,
            IMPORT_DECLARATION,
            EXPORT_DECLARATION
        };
        
        NodeType type;
        std::string value;
        std::vector<std::shared_ptr<TypeScriptAST>> children;
        std::unordered_map<std::string, std::string> properties;
        
        TypeScriptAST(NodeType t) : type(t) {}
    };
    
    std::unordered_map<std::string, std::string> typeMapping_;
    std::unordered_map<std::string, std::string> functionMapping_;
    std::vector<std::string> includes_;
    std::string currentNamespace_;
    PlatformType targetPlatform_;

public:
    TypeScriptToNativeCompilerImpl() {
        initializeTypeMapping();
        initializeFunctionMapping();
    }
    
    std::string compileToNative(const std::string& typescriptCode, PlatformType platform) {
        targetPlatform_ = platform;
        includes_.clear();
        currentNamespace_ = "FoundryEngine";
        
        // Parse TypeScript code to AST
        auto ast = parseTypeScript(typescriptCode);
        if (!ast) {
            return "";
        }
        
        // Generate C++ code from AST
        return generateCppFromAST(ast);
    }
    
    bool compileProject(const std::string& projectRoot, const std::string& outputDir, PlatformType platform) {
        targetPlatform_ = platform;
        
        // Find all TypeScript files
        std::vector<std::string> tsFiles = findTypeScriptFiles(projectRoot);
        
        // Compile each file
        for (const auto& tsFile : tsFiles) {
            if (!compileFile(tsFile, outputDir)) {
                std::cerr << "Failed to compile: " << tsFile << std::endl;
                return false;
            }
        }
        
        // Generate main.cpp
        generateMainFile(outputDir);
        
        // Generate CMakeLists.txt
        generateCMakeLists(outputDir);
        
        return true;
    }

private:
    void initializeTypeMapping() {
        // TypeScript to C++ type mapping
        typeMapping_["number"] = "double";
        typeMapping_["string"] = "std::string";
        typeMapping_["boolean"] = "bool";
        typeMapping_["void"] = "void";
        typeMapping_["any"] = "auto";
        typeMapping_["object"] = "std::unordered_map<std::string, TypeScriptValue>";
        typeMapping_["Array"] = "std::vector<TypeScriptValue>";
        
        // FoundryEngine specific types
        typeMapping_["Vector3"] = "Vector3";
        typeMapping_["Vector2"] = "Vector2";
        typeMapping_["Matrix4"] = "Matrix4";
        typeMapping_["Quaternion"] = "Quaternion";
        typeMapping_["Transform"] = "Transform";
        typeMapping_["Entity"] = "uint32_t";
        typeMapping_["World"] = "World*";
        typeMapping_["Scene"] = "Scene*";
        typeMapping_["Camera"] = "Camera*";
        typeMapping_["Renderer"] = "Renderer*";
        typeMapping_["Engine"] = "Engine&";
        typeMapping_["RigidBody"] = "RigidBody*";
        typeMapping_["AudioClip"] = "AudioClip*";
        typeMapping_["Texture"] = "Texture*";
        typeMapping_["Mesh"] = "Mesh*";
        typeMapping_["Material"] = "Material*";
        typeMapping_["Shader"] = "Shader*";
    }
    
    void initializeFunctionMapping() {
        // TypeScript to C++ function mapping
        functionMapping_["console.log"] = "std::cout";
        functionMapping_["Math.random"] = "static_cast<double>(rand()) / RAND_MAX";
        functionMapping_["Math.floor"] = "std::floor";
        functionMapping_["Math.ceil"] = "std::ceil";
        functionMapping_["Math.round"] = "std::round";
        functionMapping_["Math.abs"] = "std::abs";
        functionMapping_["Math.sqrt"] = "std::sqrt";
        functionMapping_["Math.sin"] = "std::sin";
        functionMapping_["Math.cos"] = "std::cos";
        functionMapping_["Math.tan"] = "std::tan";
        functionMapping_["Math.PI"] = "M_PI";
        functionMapping_["Math.E"] = "M_E";
        
        // FoundryEngine specific functions
        functionMapping_["foundry.engine.initialize"] = "Engine::getInstance().initialize()";
        functionMapping_["foundry.engine.shutdown"] = "Engine::getInstance().shutdown()";
        functionMapping_["foundry.engine.update"] = "Engine::getInstance().update";
        functionMapping_["foundry.engine.render"] = "Engine::getInstance().render()";
        functionMapping_["foundry.world.createEntity"] = "Engine::getInstance().getWorld()->createEntity()";
        functionMapping_["foundry.world.destroyEntity"] = "Engine::getInstance().getWorld()->destroyEntity";
        functionMapping_["foundry.scene.addObject"] = "Engine::getInstance().getScene()->addObject";
        functionMapping_["foundry.scene.removeObject"] = "Engine::getInstance().getScene()->removeObject";
        functionMapping_["foundry.physics.setGravity"] = "Engine::getInstance().getPhysics()->setGravity";
        functionMapping_["foundry.audio.playSound"] = "Engine::getInstance().getAudio()->playSound";
    }
    
    std::shared_ptr<TypeScriptAST> parseTypeScript(const std::string& code) {
        // Simplified TypeScript parser
        // In a real implementation, you'd use a proper TypeScript parser like TypeScript's own parser
        
        auto program = std::make_shared<TypeScriptAST>(TypeScriptAST::PROGRAM);
        
        // Split code into lines and parse
        std::istringstream stream(code);
        std::string line;
        std::vector<std::string> lines;
        
        while (std::getline(stream, line)) {
            lines.push_back(line);
        }
        
        // Parse each line
        for (size_t i = 0; i < lines.size(); ++i) {
            auto node = parseLine(lines[i], i);
            if (node) {
                program->children.push_back(node);
            }
        }
        
        return program;
    }
    
    std::shared_ptr<TypeScriptAST> parseLine(const std::string& line, size_t lineNumber) {
        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed.starts_with("//")) {
            return nullptr;
        }
        
        // Parse different types of statements
        if (trimmed.starts_with("import ")) {
            return parseImportStatement(trimmed);
        } else if (trimmed.starts_with("export ")) {
            return parseExportStatement(trimmed);
        } else if (trimmed.starts_with("class ")) {
            return parseClassDeclaration(trimmed);
        } else if (trimmed.starts_with("function ") || trimmed.contains("(") && trimmed.contains(")")) {
            return parseFunctionDeclaration(trimmed);
        } else if (trimmed.starts_with("let ") || trimmed.starts_with("const ") || trimmed.starts_with("var ")) {
            return parseVariableDeclaration(trimmed);
        } else if (trimmed.starts_with("if ")) {
            return parseIfStatement(trimmed);
        } else if (trimmed.starts_with("for ")) {
            return parseForStatement(trimmed);
        } else if (trimmed.starts_with("while ")) {
            return parseWhileStatement(trimmed);
        } else if (trimmed.starts_with("return ")) {
            return parseReturnStatement(trimmed);
        } else {
            return parseExpressionStatement(trimmed);
        }
    }
    
    std::shared_ptr<TypeScriptAST> parseImportStatement(const std::string& line) {
        auto node = std::make_shared<TypeScriptAST>(TypeScriptAST::IMPORT_DECLARATION);
        
        // Extract import path
        std::regex importRegex(R"(import\s+.*\s+from\s+['"]([^'"]+)['"])");
        std::smatch match;
        if (std::regex_search(line, match, importRegex)) {
            std::string importPath = match[1].str();
            
            // Convert import to include
            if (importPath.starts_with("@foundry/")) {
                std::string includePath = "#include \"GameEngine/" + importPath.substr(9) + ".h\"";
                includes_.push_back(includePath);
            }
        }
        
        return node;
    }
    
    std::shared_ptr<TypeScriptAST> parseExportStatement(const std::string& line) {
        auto node = std::make_shared<TypeScriptAST>(TypeScriptAST::EXPORT_DECLARATION);
        // Export statements are handled during code generation
        return node;
    }
    
    std::shared_ptr<TypeScriptAST> parseClassDeclaration(const std::string& line) {
        auto node = std::make_shared<TypeScriptAST>(TypeScriptAST::CLASS_DECLARATION);
        
        // Extract class name
        std::regex classRegex(R"(class\s+(\w+))");
        std::smatch match;
        if (std::regex_search(line, match, classRegex)) {
            node->value = match[1].str();
        }
        
        return node;
    }
    
    std::shared_ptr<TypeScriptAST> parseFunctionDeclaration(const std::string& line) {
        auto node = std::make_shared<TypeScriptAST>(TypeScriptAST::FUNCTION_DECLARATION);
        
        // Extract function name and parameters
        std::regex funcRegex(R"((\w+)\s*\(([^)]*)\)\s*(?::\s*(\w+))?\s*{)");
        std::smatch match;
        if (std::regex_search(line, match, funcRegex)) {
            node->value = match[1].str();
            if (match.size() > 2) {
                node->properties["parameters"] = match[2].str();
            }
            if (match.size() > 3) {
                node->properties["returnType"] = match[3].str();
            }
        }
        
        return node;
    }
    
    std::shared_ptr<TypeScriptAST> parseVariableDeclaration(const std::string& line) {
        auto node = std::make_shared<TypeScriptAST>(TypeScriptAST::VARIABLE_DECLARATION);
        
        // Extract variable name and type
        std::regex varRegex(R"((let|const|var)\s+(\w+)(?::\s*(\w+))?\s*=\s*(.+))");
        std::smatch match;
        if (std::regex_search(line, match, varRegex)) {
            node->properties["keyword"] = match[1].str();
            node->value = match[2].str();
            if (match.size() > 3 && !match[3].str().empty()) {
                node->properties["type"] = match[3].str();
            }
            if (match.size() > 4) {
                node->properties["value"] = match[4].str();
            }
        }
        
        return node;
    }
    
    std::shared_ptr<TypeScriptAST> parseIfStatement(const std::string& line) {
        auto node = std::make_shared<TypeScriptAST>(TypeScriptAST::IF_STATEMENT);
        
        // Extract condition
        std::regex ifRegex(R"(if\s*\(([^)]+)\))");
        std::smatch match;
        if (std::regex_search(line, match, ifRegex)) {
            node->properties["condition"] = match[1].str();
        }
        
        return node;
    }
    
    std::shared_ptr<TypeScriptAST> parseForStatement(const std::string& line) {
        auto node = std::make_shared<TypeScriptAST>(TypeScriptAST::FOR_STATEMENT);
        
        // Extract for loop components
        std::regex forRegex(R"(for\s*\(([^;]+);\s*([^;]+);\s*([^)]+)\))");
        std::smatch match;
        if (std::regex_search(line, match, forRegex)) {
            node->properties["initializer"] = match[1].str();
            node->properties["condition"] = match[2].str();
            node->properties["increment"] = match[3].str();
        }
        
        return node;
    }
    
    std::shared_ptr<TypeScriptAST> parseWhileStatement(const std::string& line) {
        auto node = std::make_shared<TypeScriptAST>(TypeScriptAST::WHILE_STATEMENT);
        
        // Extract condition
        std::regex whileRegex(R"(while\s*\(([^)]+)\))");
        std::smatch match;
        if (std::regex_search(line, match, whileRegex)) {
            node->properties["condition"] = match[1].str();
        }
        
        return node;
    }
    
    std::shared_ptr<TypeScriptAST> parseReturnStatement(const std::string& line) {
        auto node = std::make_shared<TypeScriptAST>(TypeScriptAST::RETURN_STATEMENT);
        
        // Extract return value
        std::regex returnRegex(R"(return\s+(.+))");
        std::smatch match;
        if (std::regex_search(line, match, returnRegex)) {
            node->properties["value"] = match[1].str();
        }
        
        return node;
    }
    
    std::shared_ptr<TypeScriptAST> parseExpressionStatement(const std::string& line) {
        auto node = std::make_shared<TypeScriptAST>(TypeScriptAST::EXPRESSION_STATEMENT);
        node->value = line;
        return node;
    }
    
    std::string generateCppFromAST(std::shared_ptr<TypeScriptAST> ast) {
        std::stringstream ss;
        
        // Generate includes
        generateIncludes(ss);
        
        // Generate namespace
        ss << "using namespace " << currentNamespace_ << ";\n\n";
        
        // Generate code from AST nodes
        for (const auto& child : ast->children) {
            ss << generateNode(child) << "\n";
        }
        
        return ss.str();
    }
    
    void generateIncludes(std::stringstream& ss) {
        // Add standard includes
        ss << "#include <iostream>\n";
        ss << "#include <memory>\n";
        ss << "#include <vector>\n";
        ss << "#include <unordered_map>\n";
        ss << "#include <string>\n";
        ss << "#include <cmath>\n";
        ss << "#include <cstdlib>\n";
        ss << "#include <ctime>\n\n";
        
        // Add FoundryEngine includes
        ss << "#include \"GameEngine/core/Engine.h\"\n";
        ss << "#include \"GameEngine/core/World.h\"\n";
        ss << "#include \"GameEngine/core/Scene.h\"\n";
        ss << "#include \"GameEngine/math/Vector3.h\"\n";
        ss << "#include \"GameEngine/math/Matrix4.h\"\n";
        ss << "#include \"GameEngine/math/Quaternion.h\"\n";
        ss << "#include \"GameEngine/graphics/Renderer.h\"\n";
        ss << "#include \"GameEngine/graphics/Camera.h\"\n";
        ss << "#include \"GameEngine/physics/PhysicsWorld.h\"\n";
        ss << "#include \"GameEngine/audio/AudioSystem.h\"\n\n";
        
        // Add custom includes
        for (const auto& include : includes_) {
            ss << include << "\n";
        }
        ss << "\n";
    }
    
    std::string generateNode(std::shared_ptr<TypeScriptAST> node) {
        switch (node->type) {
            case TypeScriptAST::IMPORT_DECLARATION:
                return ""; // Handled in includes
                
            case TypeScriptAST::EXPORT_DECLARATION:
                return ""; // Not needed in C++
                
            case TypeScriptAST::CLASS_DECLARATION:
                return generateClass(node);
                
            case TypeScriptAST::FUNCTION_DECLARATION:
                return generateFunction(node);
                
            case TypeScriptAST::VARIABLE_DECLARATION:
                return generateVariable(node);
                
            case TypeScriptAST::IF_STATEMENT:
                return generateIfStatement(node);
                
            case TypeScriptAST::FOR_STATEMENT:
                return generateForStatement(node);
                
            case TypeScriptAST::WHILE_STATEMENT:
                return generateWhileStatement(node);
                
            case TypeScriptAST::RETURN_STATEMENT:
                return generateReturnStatement(node);
                
            case TypeScriptAST::EXPRESSION_STATEMENT:
                return generateExpression(node);
                
            default:
                return "";
        }
    }
    
    std::string generateClass(std::shared_ptr<TypeScriptAST> node) {
        std::stringstream ss;
        ss << "class " << node->value << " {\n";
        ss << "public:\n";
        ss << "    " << node->value << "() = default;\n";
        ss << "    ~" << node->value << "() = default;\n";
        ss << "};\n";
        return ss.str();
    }
    
    std::string generateFunction(std::shared_ptr<TypeScriptAST> node) {
        std::stringstream ss;
        
        // Get return type
        std::string returnType = "void";
        if (node->properties.find("returnType") != node->properties.end()) {
            returnType = mapType(node->properties["returnType"]);
        }
        
        ss << returnType << " " << node->value << "(";
        
        // Get parameters
        if (node->properties.find("parameters") != node->properties.end()) {
            ss << node->properties["parameters"];
        }
        
        ss << ") {\n";
        
        // Generate function body based on function name
        if (node->value == "initialize") {
            ss << "    Engine& engine = Engine::getInstance();\n";
            ss << "    if (!engine.initialize()) {\n";
            ss << "        return false;\n";
            ss << "    }\n";
            ss << "    return true;\n";
        } else if (node->value == "update") {
            ss << "    Engine& engine = Engine::getInstance();\n";
            ss << "    engine.update(deltaTime);\n";
        } else if (node->value == "render") {
            ss << "    Engine& engine = Engine::getInstance();\n";
            ss << "    engine.render();\n";
        } else if (node->value == "shutdown") {
            ss << "    Engine& engine = Engine::getInstance();\n";
            ss << "    engine.shutdown();\n";
        } else {
            // Generate body from child nodes
            for (const auto& child : node->children) {
                ss << "    " << generateNode(child) << "\n";
            }
        }
        
        ss << "}\n";
        
        return ss.str();
    }
    
    std::string generateVariable(std::shared_ptr<TypeScriptAST> node) {
        std::stringstream ss;
        
        // Get type
        std::string type = "auto";
        if (node->properties.find("type") != node->properties.end()) {
            type = mapType(node->properties["type"]);
        }
        
        ss << type << " " << node->value;
        
        // Get value
        if (node->properties.find("value") != node->properties.end()) {
            ss << " = " << node->properties["value"];
        }
        
        ss << ";";
        return ss.str();
    }
    
    std::string generateIfStatement(std::shared_ptr<TypeScriptAST> node) {
        std::stringstream ss;
        ss << "if (" << node->properties["condition"] << ") {\n";
        
        // Generate body from child nodes
        for (const auto& child : node->children) {
            ss << "    " << generateNode(child) << "\n";
        }
        
        ss << "}";
        return ss.str();
    }
    
    std::string generateForStatement(std::shared_ptr<TypeScriptAST> node) {
        std::stringstream ss;
        ss << "for (" << node->properties["initializer"] << "; " 
           << node->properties["condition"] << "; " 
           << node->properties["increment"] << ") {\n";
        
        // Generate body from child nodes
        for (const auto& child : node->children) {
            ss << "    " << generateNode(child) << "\n";
        }
        
        ss << "}";
        return ss.str();
    }
    
    std::string generateWhileStatement(std::shared_ptr<TypeScriptAST> node) {
        std::stringstream ss;
        ss << "while (" << node->properties["condition"] << ") {\n";
        
        // Generate body from child nodes
        for (const auto& child : node->children) {
            ss << "    " << generateNode(child) << "\n";
        }
        
        ss << "}";
        return ss.str();
    }
    
    std::string generateReturnStatement(std::shared_ptr<TypeScriptAST> node) {
        std::stringstream ss;
        ss << "return " << node->properties["value"] << ";";
        return ss.str();
    }
    
    std::string generateExpression(std::shared_ptr<TypeScriptAST> node) {
        // Convert TypeScript expressions to C++
        std::string expression = node->value;
        
        // Replace TypeScript-specific syntax
        expression = std::regex_replace(expression, std::regex("console\\.log"), "std::cout");
        expression = std::regex_replace(expression, std::regex("Math\\."), "std::");
        expression = std::regex_replace(expression, std::regex("foundry\\."), "");
        
        return expression + ";";
    }
    
    std::string mapType(const std::string& tsType) {
        auto it = typeMapping_.find(tsType);
        if (it != typeMapping_.end()) {
            return it->second;
        }
        return tsType; // Return as-is if not found
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
    
    bool compileFile(const std::string& tsFile, const std::string& outputDir) {
        // Read TypeScript file
        std::ifstream file(tsFile);
        if (!file.is_open()) {
            return false;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string typescriptCode = buffer.str();
        file.close();
        
        // Compile to C++
        std::string cppCode = compileToNative(typescriptCode, targetPlatform_);
        
        // Generate output file path
        std::string relativePath = std::filesystem::relative(tsFile, std::filesystem::path(tsFile).parent_path().parent_path()).string();
        std::string outputPath = outputDir + "/" + relativePath;
        
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
        
        outputFile << cppCode;
        outputFile.close();
        
        return true;
    }
    
    void generateMainFile(const std::string& outputDir) {
        std::string mainContent = R"(
#include "GameEngine/core/Engine.h"
#include "GameEngine/typescript/NativeTypeScriptRuntime.h"
#include <iostream>

using namespace FoundryEngine;

int main() {
    std::cout << "Starting FoundryEngine TypeScript Game..." << std::endl;
    
    // Initialize engine
    Engine& engine = Engine::getInstance();
    if (!engine.initialize()) {
        std::cerr << "Failed to initialize engine" << std::endl;
        return -1;
    }
    
    // Initialize TypeScript runtime
    NativeTypeScriptRuntime runtime;
    if (!runtime.initialize(".", PlatformType::)";
    
    mainContent += getPlatformName(targetPlatform_) + R"() {
        std::cerr << "Failed to initialize TypeScript runtime" << std::endl;
        engine.shutdown();
        return -1;
    }
    
    // Execute TypeScript game code
    runtime.executeGameCode();
    
    // Main game loop
    while (engine.isRunning()) {
        float deltaTime = engine.getDeltaTime();
        engine.update(deltaTime);
        runtime.update(deltaTime);
        engine.render();
    }
    
    // Cleanup
    runtime.shutdown();
    engine.shutdown();
    
    return 0;
}
)";
        
        std::ofstream mainFile(outputDir + "/main.cpp");
        if (mainFile.is_open()) {
            mainFile << mainContent;
            mainFile.close();
        }
    }
    
    void generateCMakeLists(const std::string& outputDir) {
        std::string cmakeContent = R"(
cmake_minimum_required(VERSION 3.16)
project(FoundryTypeScriptGame)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find FoundryEngine
find_package(FoundryEngine REQUIRED)

# Source files
file(GLOB_RECURSE SOURCES "*.cpp")

# Create executable
add_executable(game ${SOURCES})

# Link libraries
target_link_libraries(game FoundryEngine)

# Platform-specific settings
)";
        
        switch (targetPlatform_) {
            case PlatformType::WINDOWS:
                cmakeContent += R"(
target_link_libraries(game d3d11 dxgi xaudio2 xinput)
)";
                break;
            case PlatformType::MACOS:
                cmakeContent += R"(
target_link_libraries(game "-framework OpenGL" "-framework Cocoa")
)";
                break;
            case PlatformType::LINUX:
                cmakeContent += R"(
target_link_libraries(game GL X11 pthread)
)";
                break;
            case PlatformType::ANDROID:
                cmakeContent += R"(
target_link_libraries(game log android EGL GLESv2)
)";
                break;
            case PlatformType::IOS:
                cmakeContent += R"(
target_link_libraries(game "-framework OpenGLES" "-framework UIKit")
)";
                break;
            case PlatformType::WEB:
                cmakeContent += R"(
# Web platform uses Emscripten
)";
                break;
        }
        
        std::ofstream cmakeFile(outputDir + "/CMakeLists.txt");
        if (cmakeFile.is_open()) {
            cmakeFile << cmakeContent;
            cmakeFile.close();
        }
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
    
    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(' ');
        if (first == std::string::npos) {
            return "";
        }
        size_t last = str.find_last_not_of(' ');
        return str.substr(first, (last - first + 1));
    }
};

// Public interface implementation
TypeScriptToNativeCompiler::TypeScriptToNativeCompiler() : impl_(std::make_unique<TypeScriptToNativeCompilerImpl>()) {}
TypeScriptToNativeCompiler::~TypeScriptToNativeCompiler() = default;

std::string TypeScriptToNativeCompiler::compileToNative(const std::string& typescriptCode, PlatformType platform) {
    return impl_->compileToNative(typescriptCode, platform);
}

bool TypeScriptToNativeCompiler::compileProject(const std::string& projectRoot, const std::string& outputDir, PlatformType platform) {
    return impl_->compileProject(projectRoot, outputDir, platform);
}

} // namespace FoundryEngine
