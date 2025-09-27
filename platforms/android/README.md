# FoundryEngine Android Platform

This directory contains the complete Android platform implementation for the FoundryEngine game engine, including native C++ code, Java Activity, and build configuration.

## 📁 File Structure

```
platforms/android/
├── src/
│   └── main/
│       ├── AndroidManifest.xml          # Android app manifest
│       ├── java/
│       │   └── com/foundryengine/game/
│       │       └── GameActivity.java    # Main Android Activity
│       ├── cpp/                        # Native C++ source files
│       │   ├── audio/                   # Audio-related components
│       │   │   ├── AAudioPlatform.cpp   # Android audio platform implementation
│       │   │   └── AAudioPlatform.h     # Android audio platform header
│       │   ├── core/                    # Core platform components
│       │   │   ├── AndroidPlatform.cpp  # Native C++ platform implementation
│       │   │   ├── AndroidPlatform.h    # Native C++ platform header
│       │   │   ├── AndroidPlatformPAL.h # Platform abstraction layer
│       │   │   ├── PlatformServices.cpp # Platform services implementation
│       │   │   └── PlatformServices.h   # Platform services header
│       │   ├── graphics/                # Graphics-related components
│       │   │   ├── GPUDebugger.cpp      # GPU debugging utilities
│       │   │   ├── GPUDebugger.h        # GPU debugging header
│       │   │   ├── VulkanPlatform.cpp   # Vulkan platform implementation
│       │   │   └── VulkanPlatform.h     # Vulkan platform header
│       │   ├── input/                   # Input handling components
│       │   │   ├── GestureManager.cpp   # Touch gesture management
│       │   │   ├── GestureManager.h     # Touch gesture header
│       │   │   ├── StylusManager.cpp    # Stylus input management
│       │   │   └── StylusManager.h      # Stylus input header
│       │   ├── network/                 # Network-related components
│       │   │   ├── EnhancedNetworking.cpp # Advanced networking features
│       │   │   └── EnhancedNetworking.h   # Advanced networking header
│       │   └── system/                  # System-level components
│       │       ├── AccessibilityManager.cpp # Accessibility features
│       │       ├── AccessibilityManager.h   # Accessibility header
│       │       ├── BackgroundTaskManager.h  # Background task management
│       │       ├── PushNotificationManager.cpp # Push notification handling
│       │       ├── PushNotificationManager.h   # Push notification header
│       │       ├── ScopedStorageManager.cpp    # Android scoped storage
│       │       ├── ScopedStorageManager.h      # Android scoped storage header
│       │       ├── ThermalManager.cpp          # Device thermal management
│       │       └── ThermalManager.h            # Device thermal management header
│       └── res/                        # Android resources (drawable, layout, values, etc.)
├── build.gradle                        # Gradle build configuration
├── CMakeLists.txt                      # CMake build configuration
└── README.md                           # This file
```

## 🚀 Features Implemented

### ✅ Complete Android Platform
- **Native C++ Integration**: Full JNI bridge between Java and C++
- **OpenGL ES 3.0**: Hardware-accelerated graphics rendering
- **Touch Input**: Multi-touch gesture support
- **Gamepad Support**: Android controller integration
- **Audio System**: OpenSL ES audio processing
- **File System**: Android asset and storage management
- **Lifecycle Management**: Proper Android activity lifecycle handling

### ✅ Advanced Graphics
- **EGL Context Management**: Proper OpenGL ES context creation
- **Surface Handling**: Native window surface integration
- **Shader Pipeline**: Complete shader compilation and linking
- **Buffer Management**: Vertex and index buffer operations
- **Texture Support**: 2D texture loading and management

### ✅ Input Systems
- **Touch Events**: Multi-touch with gesture recognition
- **Keyboard Input**: Hardware keyboard support
- **Gamepad Support**: Android controller integration with vibration
- **Motion Events**: Accelerometer and gyroscope support

### ✅ Audio Integration
- **OpenSL ES**: Low-latency audio processing
- **Audio Buffers**: Dynamic audio buffer management
- **Spatial Audio**: 3D positional audio support
- **Audio Effects**: Reverb, filtering, and mixing

### ✅ File System
- **Asset Loading**: Loading from APK assets
- **Internal Storage**: Private app data storage
- **External Storage**: SD card and external storage
- **File Operations**: Read, write, delete, list operations

## 🛠️ Building the Android Platform

### Prerequisites

1. **Android Studio**: Latest version with Android SDK
2. **Android NDK**: Version 21+ (for native C++ compilation)
3. **CMake**: Version 3.18+ (for cross-platform builds)
4. **Java JDK**: Version 8+ (for Java compilation)

### Build Steps

#### Method 1: Android Studio (Recommended)

1. Open Android Studio
2. Select "Open an existing Android Studio project"
3. Navigate to `platforms/android/` directory
4. Wait for Gradle sync to complete
5. Build → Make Project (Ctrl+F9)
6. Run → Run 'app' (Shift+F10)

#### Method 2: Command Line

```bash
# Navigate to android directory
cd platforms/android

# Build native library
./gradlew build

# Install to connected device
./gradlew installDebug

# Run on connected device
./gradlew runDebug
```

#### Method 3: CMake (Advanced)

```bash
# Configure with Android NDK
cmake -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
      -DANDROID_ABI=arm64-v8a \
      -DANDROID_PLATFORM=android-21 \
      -DCMAKE_BUILD_TYPE=Release

# Build native library
make foundryengine

# Build APK
./gradlew assembleDebug
```

## 🎮 Usage

### Basic Game Loop

```java
public class MyGameActivity extends GameActivity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Game initialization code here
        initializeGame();
    }

    private void initializeGame() {
        // Access FoundryEngine systems
        // Game-specific initialization
    }
}
```

### Input Handling

```java
// Touch events are automatically handled
@Override
public boolean onTouchEvent(MotionEvent event) {
    // Custom touch handling if needed
    return super.onTouchEvent(event);
}

// Gamepad input
@Override
public boolean onGenericMotionEvent(MotionEvent event) {
    // Gamepad analog stick and trigger handling
    return super.onGenericMotionEvent(event);
}
```

### Asset Loading

```java
// Load assets from APK
AssetManager assets = getAssets();
InputStream inputStream = assets.open("models/player.fbx");

// Load from internal storage
File internalFile = new File(getFilesDir(), "savegame.dat");
```

## 🔧 Configuration

### AndroidManifest.xml Settings

```xml
<!-- Required permissions -->
<uses-permission android:name="android.permission.INTERNET" />
<uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE" />
<uses-permission android:name="android.permission.VIBRATE" />

<!-- OpenGL ES requirement -->
<uses-feature android:name="android.hardware.opengles.aep" android:required="true" />

<!-- Gamepad support (optional) -->
<uses-feature android:name="android.hardware.gamepad" android:required="false" />
```

### Build Configuration

```gradle
android {
    defaultConfig {
        minSdk 21                    // Minimum Android 5.0
        targetSdk 34                 // Target Android 14
        ndk {
            abiFilters 'armeabi-v7a', 'arm64-v8a', 'x86', 'x86_64'
        }
        externalNativeBuild {
            cmake {
                cppFlags "-std=c++17 -frtti -fexceptions"
                arguments "-DANDROID_STL=c++_static"
            }
        }
    }
}
```

## 🎯 Performance Optimization

### Graphics Optimization
- **Texture Compression**: Use ETC2 or ASTC compression
- **Mipmap Generation**: Automatic mipmap generation for textures
- **VBO Indexing**: Use indexed vertex buffers for better performance
- **Shader Optimization**: Pre-compile shaders and use binary shaders

### Memory Management
- **Asset Streaming**: Stream large assets instead of loading all at once
- **Texture Atlasing**: Combine multiple textures into single atlas
- **Object Pooling**: Reuse objects instead of creating/destroying frequently
- **Garbage Collection**: Minimize Java object creation in performance-critical code

### Battery Optimization
- **Frame Rate Limiting**: Cap frame rate when appropriate
- **Background Processing**: Pause rendering when app is backgrounded
- **Power Management**: Use appropriate wake locks and power modes
- **Efficient Updates**: Only update what's visible on screen

## 🐛 Debugging

### Native Code Debugging
```bash
# Enable native debugging in build.gradle
buildTypes {
    debug {
        debuggable true
        ndk {
            debuggable true
        }
    }
}
```

### Logcat Monitoring
```bash
# View all logs
adb logcat

# View only game engine logs
adb logcat -s "GameEngine"

# View logs by priority
adb logcat *:E  # Errors only
adb logcat *:W  # Warnings and errors
adb logcat *:I  # Info and above
```

### Memory Profiling
```bash
# Monitor memory usage
adb shell dumpsys meminfo com.foundryengine.game

# Check for memory leaks
adb shell am dumpheap com.foundryengine.game /sdcard/heap.txt
```

## 📱 Supported Devices

### Minimum Requirements
- **Android Version**: 5.0 (API 21) or higher
- **RAM**: 2GB minimum, 4GB recommended
- **Storage**: 500MB free space
- **GPU**: OpenGL ES 3.0 compatible

### Recommended Devices
- **Android Version**: 8.0+ (API 26+)
- **RAM**: 4GB+
- **Storage**: 1GB+ free space
- **GPU**: Mali, Adreno, or PowerVR with OpenGL ES 3.1+

### Tested Devices
- Samsung Galaxy S8/S9/S10/S20/S21/S22
- Google Pixel 2/3/4/5/6/7
- OnePlus 6/7/8/9/10
- Xiaomi Mi 9/10/11
- Huawei P20/P30/P40

## 🔄 Integration with FoundryEngine

### Engine Initialization
```cpp
// In native C++ code
extern "C" {
    JNIEXPORT void JNICALL Java_com_foundryengine_game_GameActivity_nativeOnCreate(JNIEnv* env, jobject thiz) {
        // Initialize FoundryEngine systems
        World* world = new World();
        world->initialize();

        // Set up platform interface
        AndroidPlatform* platform = new AndroidPlatform();
        platform->setJavaVM(env->GetJavaVM());
    }
}
```

### System Integration
```cpp
// Access Android platform from engine
Platform* platform = Platform::getInstance();
AndroidPlatform* androidPlatform = static_cast<AndroidPlatform*>(platform);

// Get Android-specific features
AssetManager* assets = androidPlatform->getAssetManager();
InputManager* input = androidPlatform->getInputManager();
```

## 🚨 Troubleshooting

### Common Issues

#### Build Failures
- **NDK Not Found**: Ensure Android NDK is installed and configured
- **CMake Issues**: Update CMake to version 3.18+
- **ABI Mismatch**: Check that all ABIs are supported

#### Runtime Crashes
- **Native Library Not Loaded**: Check library path in Java
- **OpenGL ES Context**: Ensure proper EGL context creation
- **Memory Issues**: Monitor memory usage and fix leaks

#### Performance Problems
- **Frame Drops**: Check for GPU bottlenecks
- **Memory Spikes**: Profile memory allocations
- **Battery Drain**: Optimize update loops and rendering

### Getting Help

1. Check the Android Studio logcat for error messages
2. Verify device compatibility and requirements
3. Test on multiple devices to isolate issues
4. Review the official Android NDK documentation

## 📄 License

This Android platform implementation is part of the FoundryEngine project and follows the same licensing terms as the main engine.

---

**FoundryEngine Android Platform - Production Ready** ✅
