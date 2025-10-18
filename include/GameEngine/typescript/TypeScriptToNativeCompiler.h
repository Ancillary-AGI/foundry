#pragma once

#include "../platform/PlatformInterface.h"
#include <string>

namespace FoundryEngine {

/**
 * TypeScript to Native C++ Compiler
 * Compiles TypeScript code to native C++ code for each platform
 * Similar to how Flutter compiles Dart to native code
 */
class TypeScriptToNativeCompiler {
public:
    TypeScriptToNativeCompiler();
    ~TypeScriptToNativeCompiler();
    
    /**
     * Compile TypeScript code to native C++ code
     * @param typescriptCode The TypeScript source code
     * @param platform The target platform
     * @return Generated C++ code
     */
    std::string compileToNative(const std::string& typescriptCode, PlatformType platform);
    
    /**
     * Compile an entire TypeScript project to native code
     * @param projectRoot Root directory of the TypeScript project
     * @param outputDir Output directory for generated C++ files
     * @param platform Target platform
     * @return true if compilation succeeded
     */
    bool compileProject(const std::string& projectRoot, const std::string& outputDir, PlatformType platform);

private:
    class TypeScriptToNativeCompilerImpl;
    std::unique_ptr<TypeScriptToNativeCompilerImpl> impl_;
};

} // namespace FoundryEngine
