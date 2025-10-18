package com.foundry.ide.platforms

import kotlinx.serialization.Serializable
import java.io.File

/**
 * Platform abstraction layer for cross-platform development
 * Provides unified interface for platform-specific operations
 */
@Serializable
data class PlatformInfo(
    val name: String,
    val displayName: String,
    val architecture: String,
    val os: String,
    val version: String,
    val capabilities: List<String>
)

@Serializable
data class BuildTarget(
    val platform: String,
    val architecture: String,
    val configuration: String, // "debug", "release"
    val toolchain: String,
    val sdkVersion: String? = null
)

enum class PlatformType {
    WINDOWS, MACOS, LINUX, ANDROID, IOS, WEB
}

interface PlatformInterface {
    val platformType: PlatformType
    val platformInfo: PlatformInfo

    fun isAvailable(): Boolean
    fun getToolchainPath(): String?
    fun getSdkPath(): String?
    fun validateEnvironment(): List<String>
    fun getBuildCommand(target: BuildTarget, sourceFiles: List<String>, outputPath: String): List<String>
    fun getRunCommand(executablePath: String): List<String>
    fun getPackageCommand(buildPath: String, outputPath: String): List<String>
}

class WindowsPlatform : PlatformInterface {
    override val platformType = PlatformType.WINDOWS
    override val platformInfo = PlatformInfo(
        name = "windows",
        displayName = "Windows",
        architecture = System.getProperty("os.arch") ?: "x64",
        os = "Windows",
        version = System.getProperty("os.version") ?: "10",
        capabilities = listOf("desktop", "gaming", "directx", "opengl")
    )

    override fun isAvailable(): Boolean = System.getProperty("os.name")?.contains("Windows") == true

    override fun getToolchainPath(): String? {
        // Check for Visual Studio
        val vsPath = System.getenv("VSINSTALLDIR")
        if (vsPath != null) return vsPath

        // Check for MinGW
        val path = System.getenv("PATH")
        if (path?.contains("mingw") == true) return "mingw"

        return null
    }

    override fun getSdkPath(): String? {
        return System.getenv("WindowsSDKDir") ?: System.getenv("WINDOWSSDKDIR")
    }

    override fun validateEnvironment(): List<String> {
        val issues = mutableListOf<String>()

        if (getToolchainPath() == null) {
            issues.add("No C++ toolchain found. Install Visual Studio or MinGW.")
        }

        if (getSdkPath() == null) {
            issues.add("Windows SDK not found.")
        }

        return issues
    }

    override fun getBuildCommand(target: BuildTarget, sourceFiles: List<String>, outputPath: String): List<String> {
        return when (target.toolchain) {
            "msvc" -> getMSVCBuildCommand(target, sourceFiles, outputPath)
            "mingw" -> getMinGWBuildCommand(target, sourceFiles, outputPath)
            else -> getMSVCBuildCommand(target, sourceFiles, outputPath)
        }
    }

    private fun getMSVCBuildCommand(target: BuildTarget, sourceFiles: List<String>, outputPath: String): List<String> {
        val cmd = mutableListOf("cl.exe")

        // Source files
        cmd.addAll(sourceFiles)

        // Output
        cmd.add("/Fe:$outputPath")

        // Include directories
        cmd.add("/Iinclude")
        cmd.add("/Igenerated/bindings")

        // Libraries
        cmd.add("/link")
        cmd.add("opengl32.lib")
        cmd.add("user32.lib")
        cmd.add("gdi32.lib")

        // Configuration
        if (target.configuration == "debug") {
            cmd.add("/Zi")
            cmd.add("/Od")
        } else {
            cmd.add("/O2")
        }

        return cmd
    }

    private fun getMinGWBuildCommand(target: BuildTarget, sourceFiles: List<String>, outputPath: String): List<String> {
        val cmd = mutableListOf("g++")

        // Source files
        cmd.addAll(sourceFiles)

        // Output
        cmd.add("-o")
        cmd.add(outputPath)

        // Include directories
        cmd.add("-Iinclude")
        cmd.add("-Igenerated/bindings")

        // Libraries
        cmd.add("-lopengl32")
        cmd.add("-luser32")
        cmd.add("-lgdi32")

        // Configuration
        if (target.configuration == "debug") {
            cmd.add("-g")
            cmd.add("-O0")
        } else {
            cmd.add("-O2")
        }

        return cmd
    }

    override fun getRunCommand(executablePath: String): List<String> {
        return listOf(executablePath)
    }

    override fun getPackageCommand(buildPath: String, outputPath: String): List<String> {
        // For Windows, create a simple ZIP or use NSIS for installer
        return listOf("powershell", "Compress-Archive", "-Path", buildPath, "-DestinationPath", outputPath)
    }
}

class MacOSPlatform : PlatformInterface {
    override val platformType = PlatformType.MACOS
    override val platformInfo = PlatformInfo(
        name = "macos",
        displayName = "macOS",
        architecture = System.getProperty("os.arch") ?: "x64",
        os = "macOS",
        version = System.getProperty("os.version") ?: "12.0",
        capabilities = listOf("desktop", "gaming", "metal", "opengl")
    )

    override fun isAvailable(): Boolean = System.getProperty("os.name")?.contains("Mac") == true

    override fun getToolchainPath(): String? {
        // Check for Xcode
        val xcodePath = "/Applications/Xcode.app"
        if (File(xcodePath).exists()) return xcodePath

        // Check for Command Line Tools
        try {
            val process = ProcessBuilder("xcode-select", "-p")
                .redirectErrorStream(true)
                .start()
            if (process.waitFor() == 0) {
                return process.inputStream.bufferedReader().readLine()
            }
        } catch (e: Exception) {
            // Ignore
        }

        return null
    }

    override fun getSdkPath(): String? {
        return try {
            val process = ProcessBuilder("xcrun", "--show-sdk-path")
                .redirectErrorStream(true)
                .start()
            if (process.waitFor() == 0) {
                return process.inputStream.bufferedReader().readLine()
            }
            null
        } catch (e: Exception) {
            null
        }
    }

    override fun validateEnvironment(): List<String> {
        val issues = mutableListOf<String>()

        if (getToolchainPath() == null) {
            issues.add("Xcode or Command Line Tools not found. Install Xcode from App Store.")
        }

        return issues
    }

    override fun getBuildCommand(target: BuildTarget, sourceFiles: List<String>, outputPath: String): List<String> {
        val cmd = mutableListOf("clang++")

        // Source files
        cmd.addAll(sourceFiles)

        // Output
        cmd.add("-o")
        cmd.add(outputPath)

        // Frameworks
        cmd.add("-framework")
        cmd.add("OpenGL")
        cmd.add("-framework")
        cmd.add("Cocoa")

        // Include directories
        cmd.add("-Iinclude")
        cmd.add("-Igenerated/bindings")

        // Configuration
        if (target.configuration == "debug") {
            cmd.add("-g")
            cmd.add("-O0")
        } else {
            cmd.add("-O2")
        }

        return cmd
    }

    override fun getRunCommand(executablePath: String): List<String> {
        return listOf(executablePath)
    }

    override fun getPackageCommand(buildPath: String, outputPath: String): List<String> {
        // Create macOS app bundle
        return listOf("bash", "-c", "mkdir -p '$outputPath' && cp -r '$buildPath' '$outputPath'")
    }
}

class LinuxPlatform : PlatformInterface {
    override val platformType = PlatformType.LINUX
    override val platformInfo = PlatformInfo(
        name = "linux",
        displayName = "Linux",
        architecture = System.getProperty("os.arch") ?: "x64",
        os = "Linux",
        version = System.getProperty("os.version") ?: "5.0",
        capabilities = listOf("desktop", "gaming", "opengl", "vulkan")
    )

    override fun isAvailable(): Boolean = System.getProperty("os.name")?.contains("Linux") == true

    override fun getToolchainPath(): String? {
        // Check for GCC
        try {
            val process = ProcessBuilder("which", "g++")
                .redirectErrorStream(true)
                .start()
            if (process.waitFor() == 0) {
                return "gcc"
            }
        } catch (e: Exception) {
            // Ignore
        }
        return null
    }

    override fun getSdkPath(): String? = null // Linux doesn't have a central SDK

    override fun validateEnvironment(): List<String> {
        val issues = mutableListOf<String>()

        if (getToolchainPath() == null) {
            issues.add("GCC not found. Install build-essential package.")
        }

        // Check for required libraries
        val requiredLibs = listOf("libgl1-mesa-dev", "libx11-dev", "libxrandr-dev")
        // In a real implementation, check if these are installed

        return issues
    }

    override fun getBuildCommand(target: BuildTarget, sourceFiles: List<String>, outputPath: String): List<String> {
        val cmd = mutableListOf("g++")

        // Source files
        cmd.addAll(sourceFiles)

        // Output
        cmd.add("-o")
        cmd.add(outputPath)

        // Libraries
        cmd.add("-lGL")
        cmd.add("-lX11")
        cmd.add("-lXrandr")
        cmd.add("-lpthread")
        cmd.add("-ldl")

        // Include directories
        cmd.add("-Iinclude")
        cmd.add("-Igenerated/bindings")

        // Configuration
        if (target.configuration == "debug") {
            cmd.add("-g")
            cmd.add("-O0")
        } else {
            cmd.add("-O2")
        }

        return cmd
    }

    override fun getRunCommand(executablePath: String): List<String> {
        return listOf(executablePath)
    }

    override fun getPackageCommand(buildPath: String, outputPath: String): List<String> {
        // Create tar.gz archive
        return listOf("tar", "-czf", outputPath, "-C", buildPath, ".")
    }
}

class AndroidPlatform : PlatformInterface {
    override val platformType = PlatformType.ANDROID
    override val platformInfo = PlatformInfo(
        name = "android",
        displayName = "Android",
        architecture = "arm64-v8a", // Default, can be multiple
        os = "Android",
        version = "API 30",
        capabilities = listOf("mobile", "touch", "opengl", "vulkan")
    )

    override fun isAvailable(): Boolean {
        // Check if Android SDK is available
        val androidHome = System.getenv("ANDROID_HOME") ?: System.getenv("ANDROID_SDK_ROOT")
        return androidHome != null && File(androidHome).exists()
    }

    override fun getToolchainPath(): String? {
        return System.getenv("ANDROID_NDK_ROOT") ?: System.getenv("ANDROID_NDK_HOME")
    }

    override fun getSdkPath(): String? {
        return System.getenv("ANDROID_HOME") ?: System.getenv("ANDROID_SDK_ROOT")
    }

    override fun validateEnvironment(): List<String> {
        val issues = mutableListOf<String>()

        if (getSdkPath() == null) {
            issues.add("Android SDK not found. Set ANDROID_HOME environment variable.")
        }

        if (getToolchainPath() == null) {
            issues.add("Android NDK not found. Set ANDROID_NDK_ROOT environment variable.")
        }

        return issues
    }

    override fun getBuildCommand(target: BuildTarget, sourceFiles: List<String>, outputPath: String): List<String> {
        // Use Android NDK build system
        val ndkPath = getToolchainPath() ?: return emptyList()
        val toolchain = "$ndkPath/toolchains/llvm/prebuilt/linux-x86_64/bin"

        val cmd = mutableListOf("$toolchain/aarch64-linux-android30-clang++")

        // Source files
        cmd.addAll(sourceFiles)

        // Output
        cmd.add("-o")
        cmd.add(outputPath)

        // Android-specific flags
        cmd.add("-shared")
        cmd.add("-fPIC")

        // Include directories
        cmd.add("-Iinclude")
        cmd.add("-Igenerated/bindings")
        cmd.add("-I$ndkPath/sysroot/usr/include")

        // Libraries
        cmd.add("-L$ndkPath/sysroot/usr/lib")
        cmd.add("-landroid")
        cmd.add("-lEGL")
        cmd.add("-lGLESv3")

        return cmd
    }

    override fun getRunCommand(executablePath: String): List<String> {
        // Android apps need to be installed and launched through ADB
        return listOf("adb", "shell", "am", "start", "-n", "com.foundry.game/.MainActivity")
    }

    override fun getPackageCommand(buildPath: String, outputPath: String): List<String> {
        // Use Gradle to build APK
        return listOf("./gradlew", "assembleRelease")
    }
}

class IOSPlatform : PlatformInterface {
    override val platformType = PlatformType.IOS
    override val platformInfo = PlatformInfo(
        name = "ios",
        displayName = "iOS",
        architecture = "arm64",
        os = "iOS",
        version = "15.0",
        capabilities = listOf("mobile", "touch", "metal")
    )

    override fun isAvailable(): Boolean = System.getProperty("os.name")?.contains("Mac") == true

    override fun getToolchainPath(): String? = getSdkPath()

    override fun getSdkPath(): String? {
        return try {
            val process = ProcessBuilder("xcrun", "--show-sdk-path", "--sdk", "iphoneos")
                .redirectErrorStream(true)
                .start()
            if (process.waitFor() == 0) {
                return process.inputStream.bufferedReader().readLine()
            }
            null
        } catch (e: Exception) {
            null
        }
    }

    override fun validateEnvironment(): List<String> {
        val issues = mutableListOf<String>()

        if (getSdkPath() == null) {
            issues.add("iOS SDK not found. Install Xcode.")
        }

        return issues
    }

    override fun getBuildCommand(target: BuildTarget, sourceFiles: List<String>, outputPath: String): List<String> {
        // Use Xcode build system or clang directly
        val cmd = mutableListOf("xcrun", "clang++")

        // Source files
        cmd.addAll(sourceFiles)

        // Output
        cmd.add("-o")
        cmd.add(outputPath)

        // iOS-specific flags
        cmd.add("-arch")
        cmd.add("arm64")
        cmd.add("-isysroot")
        cmd.add(getSdkPath() ?: "")
        cmd.add("-miphoneos-version-min=12.0")

        // Frameworks
        cmd.add("-framework")
        cmd.add("UIKit")
        cmd.add("-framework")
        cmd.add("Metal")

        return cmd
    }

    override fun getRunCommand(executablePath: String): List<String> {
        // iOS apps need to be installed through Xcode
        return listOf("xcrun", "simctl", "launch", "booted", "com.foundry.game")
    }

    override fun getPackageCommand(buildPath: String, outputPath: String): List<String> {
        // Use Xcode to create IPA
        return listOf("xcodebuild", "-exportArchive", "-archivePath", buildPath, "-exportPath", outputPath)
    }
}

class WebPlatform : PlatformInterface {
    override val platformType = PlatformType.WEB
    override val platformInfo = PlatformInfo(
        name = "web",
        displayName = "Web",
        architecture = "wasm",
        os = "Web",
        version = "ES2020",
        capabilities = listOf("web", "wasm", "webgl", "webaudio")
    )

    override fun isAvailable(): Boolean = true // WebAssembly is always available in modern browsers

    override fun getToolchainPath(): String? {
        // Emscripten path
        return System.getenv("EMSCRIPTEN_ROOT") ?: "/usr/local/emscripten"
    }

    override fun getSdkPath(): String? = null

    override fun validateEnvironment(): List<String> {
        val issues = mutableListOf<String>()

        if (getToolchainPath() == null || !File(getToolchainPath()!!).exists()) {
            issues.add("Emscripten not found. Install Emscripten SDK.")
        }

        return issues
    }

    override fun getBuildCommand(target: BuildTarget, sourceFiles: List<String>, outputPath: String): List<String> {
        val cmd = mutableListOf("emcc")

        // Source files
        cmd.addAll(sourceFiles)

        // Output
        cmd.add("-o")
        cmd.add(outputPath)

        // WebAssembly flags
        cmd.add("-sWASM=1")
        cmd.add("-sMODULARIZE=1")
        cmd.add("-sEXPORT_ES6=1")

        // Web APIs
        cmd.add("-sUSE_WEBGL2=1")
        cmd.add("-sUSE_SDL=2")

        return cmd
    }

    override fun getRunCommand(executablePath: String): List<String> {
        // Open in default browser
        val os = System.getProperty("os.name").lowercase()
        return when {
            os.contains("win") -> listOf("cmd", "/c", "start", executablePath)
            os.contains("mac") -> listOf("open", executablePath)
            else -> listOf("xdg-open", executablePath)
        }
    }

    override fun getPackageCommand(buildPath: String, outputPath: String): List<String> {
        // Create ZIP archive for web deployment
        return listOf("zip", "-r", outputPath, buildPath)
    }
}

// Platform manager
class PlatformManager {
    private val platforms = mutableMapOf<PlatformType, PlatformInterface>()

    init {
        registerPlatform(WindowsPlatform())
        registerPlatform(MacOSPlatform())
        registerPlatform(LinuxPlatform())
        registerPlatform(AndroidPlatform())
        registerPlatform(IOSPlatform())
        registerPlatform(WebPlatform())
    }

    fun registerPlatform(platform: PlatformInterface) {
        platforms[platform.platformType] = platform
    }

    fun getPlatform(type: PlatformType): PlatformInterface? {
        return platforms[type]
    }

    fun getAvailablePlatforms(): List<PlatformInterface> {
        return platforms.values.filter { it.isAvailable() }
    }

    fun getCurrentPlatform(): PlatformInterface? {
        val os = System.getProperty("os.name").lowercase()
        return when {
            os.contains("win") -> getPlatform(PlatformType.WINDOWS)
            os.contains("mac") -> getPlatform(PlatformType.MACOS)
            os.contains("linux") -> getPlatform(PlatformType.LINUX)
            else -> null
        }
    }

    fun validateAllPlatforms(): Map<PlatformType, List<String>> {
        return platforms.mapValues { it.value.validateEnvironment() }
    }
}

// Global platform manager instance
val platformManager = PlatformManager()