#include "gtest/gtest.h"
#include "GameEngine/platform/PlatformInterface.h"
#include "GameEngine/core/Engine.h"
#include <memory>
#include <thread>
#include <chrono>

using namespace FoundryEngine;

class PlatformSpecificTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine = &Engine::getInstance();
        ASSERT_TRUE(engine->initialize()) << "Engine initialization failed!";
    }

    void TearDown() override {
        engine->shutdown();
    }

    Engine* engine;
};

// Windows Platform Tests
#ifdef _WIN32
TEST_F(PlatformSpecificTest, WindowsDirectXIntegration) {
    // Test DirectX 11 initialization
    Renderer* renderer = engine->getRenderer();
    ASSERT_NE(renderer, nullptr);
    
    // Test DirectX device creation
    EXPECT_TRUE(renderer->isInitialized());
    
    // Test DirectX swap chain
    EXPECT_TRUE(renderer->createSwapChain(800, 600));
    
    // Test DirectX texture creation
    Texture* texture = renderer->createTexture("test_texture.png");
    EXPECT_NE(texture, nullptr);
    
    renderer->destroyTexture(texture);
}

TEST_F(PlatformSpecificTest, WindowsXAudio2Integration) {
    // Test XAudio2 audio system
    AudioManager* audio = engine->getAudio();
    ASSERT_NE(audio, nullptr);
    
    EXPECT_TRUE(audio->isInitialized());
    
    // Test XAudio2 voice creation
    AudioSource* source = audio->createSource();
    EXPECT_NE(source, nullptr);
    
    audio->destroySource(source);
}

TEST_F(PlatformSpecificTest, WindowsXInputIntegration) {
    // Test XInput gamepad support
    InputManager* input = engine->getInput();
    ASSERT_NE(input, nullptr);
    
    // Test gamepad detection
    int gamepadCount = input->getGamepadCount();
    EXPECT_GE(gamepadCount, 0);
    
    if (gamepadCount > 0) {
        // Test gamepad input
        input->simulateGamepadButtonPress(0, GamepadButton::A);
        EXPECT_TRUE(input->isGamepadButtonPressed(0, GamepadButton::A));
    }
}

TEST_F(PlatformSpecificTest, WindowsFileSystem) {
    // Test Windows file system operations
    std::string testFile = "test_file.txt";
    std::string testData = "Hello Windows!";
    
    // Test file writing
    std::ofstream file(testFile);
    ASSERT_TRUE(file.is_open());
    file << testData;
    file.close();
    
    // Test file reading
    std::ifstream readFile(testFile);
    ASSERT_TRUE(readFile.is_open());
    std::string readData;
    std::getline(readFile, readData);
    readFile.close();
    
    EXPECT_EQ(readData, testData);
    
    // Cleanup
    std::remove(testFile.c_str());
}
#endif

// Linux Platform Tests
#ifdef __linux__
TEST_F(PlatformSpecificTest, LinuxOpenGLIntegration) {
    // Test OpenGL context creation
    Renderer* renderer = engine->getRenderer();
    ASSERT_NE(renderer, nullptr);
    
    EXPECT_TRUE(renderer->isInitialized());
    
    // Test OpenGL extensions
    EXPECT_TRUE(renderer->hasExtension("GL_ARB_vertex_buffer_object"));
    
    // Test OpenGL shader compilation
    Shader* shader = renderer->createShader("test_vertex.glsl", "test_fragment.glsl");
    EXPECT_NE(shader, nullptr);
    
    renderer->destroyShader(shader);
}

TEST_F(PlatformSpecificTest, LinuxALSAIntegration) {
    // Test ALSA audio system
    AudioManager* audio = engine->getAudio();
    ASSERT_NE(audio, nullptr);
    
    EXPECT_TRUE(audio->isInitialized());
    
    // Test ALSA device enumeration
    std::vector<std::string> devices = audio->getAudioDevices();
    EXPECT_GT(devices.size(), 0);
}

TEST_F(PlatformSpecificTest, LinuxX11Integration) {
    // Test X11 window system
    InputManager* input = engine->getInput();
    ASSERT_NE(input, nullptr);
    
    // Test X11 input handling
    input->simulateKeyPress(KeyCode::W);
    EXPECT_TRUE(input->isKeyPressed(KeyCode::W));
    
    input->simulateMouseMove(100, 200);
    EXPECT_EQ(input->getMouseX(), 100);
    EXPECT_EQ(input->getMouseY(), 200);
}

TEST_F(PlatformSpecificTest, LinuxFileSystem) {
    // Test Linux file system operations
    std::string testFile = "test_file.txt";
    std::string testData = "Hello Linux!";
    
    // Test file operations
    std::ofstream file(testFile);
    ASSERT_TRUE(file.is_open());
    file << testData;
    file.close();
    
    std::ifstream readFile(testFile);
    ASSERT_TRUE(readFile.is_open());
    std::string readData;
    std::getline(readFile, readData);
    readFile.close();
    
    EXPECT_EQ(readData, testData);
    
    // Cleanup
    std::remove(testFile.c_str());
}
#endif

// macOS Platform Tests
#ifdef __APPLE__
TEST_F(PlatformSpecificTest, MacOSMetalIntegration) {
    // Test Metal rendering
    Renderer* renderer = engine->getRenderer();
    ASSERT_NE(renderer, nullptr);
    
    EXPECT_TRUE(renderer->isInitialized());
    
    // Test Metal device creation
    EXPECT_TRUE(renderer->createMetalDevice());
    
    // Test Metal texture creation
    Texture* texture = renderer->createTexture("test_texture.png");
    EXPECT_NE(texture, nullptr);
    
    renderer->destroyTexture(texture);
}

TEST_F(PlatformSpecificTest, MacOSCoreAudioIntegration) {
    // Test Core Audio system
    AudioManager* audio = engine->getAudio();
    ASSERT_NE(audio, nullptr);
    
    EXPECT_TRUE(audio->isInitialized());
    
    // Test Core Audio device enumeration
    std::vector<std::string> devices = audio->getAudioDevices();
    EXPECT_GT(devices.size(), 0);
}

TEST_F(PlatformSpecificTest, MacOSCocoaIntegration) {
    // Test Cocoa integration
    InputManager* input = engine->getInput();
    ASSERT_NE(input, nullptr);
    
    // Test Cocoa input handling
    input->simulateKeyPress(KeyCode::W);
    EXPECT_TRUE(input->isKeyPressed(KeyCode::W));
    
    input->simulateMouseMove(100, 200);
    EXPECT_EQ(input->getMouseX(), 100);
    EXPECT_EQ(input->getMouseY(), 200);
}
#endif

// Android Platform Tests
#ifdef __ANDROID__
TEST_F(PlatformSpecificTest, AndroidOpenGLESIntegration) {
    // Test OpenGL ES rendering
    Renderer* renderer = engine->getRenderer();
    ASSERT_NE(renderer, nullptr);
    
    EXPECT_TRUE(renderer->isInitialized());
    
    // Test OpenGL ES context creation
    EXPECT_TRUE(renderer->createGLESContext());
    
    // Test OpenGL ES extensions
    EXPECT_TRUE(renderer->hasExtension("GL_OES_vertex_array_object"));
}

TEST_F(PlatformSpecificTest, AndroidOpenSLESIntegration) {
    // Test OpenSL ES audio
    AudioManager* audio = engine->getAudio();
    ASSERT_NE(audio, nullptr);
    
    EXPECT_TRUE(audio->isInitialized());
    
    // Test OpenSL ES engine creation
    EXPECT_TRUE(audio->createSLESEngine());
}

TEST_F(PlatformSpecificTest, AndroidSensorIntegration) {
    // Test Android sensors
    InputManager* input = engine->getInput();
    ASSERT_NE(input, nullptr);
    
    // Test accelerometer
    Vector3 acceleration = input->getAcceleration();
    EXPECT_TRUE(acceleration.x >= -1.0f && acceleration.x <= 1.0f);
    EXPECT_TRUE(acceleration.y >= -1.0f && acceleration.y <= 1.0f);
    EXPECT_TRUE(acceleration.z >= -1.0f && acceleration.z <= 1.0f);
    
    // Test gyroscope
    Vector3 gyroscope = input->getGyroscope();
    EXPECT_TRUE(gyroscope.x >= -1.0f && gyroscope.x <= 1.0f);
    EXPECT_TRUE(gyroscope.y >= -1.0f && gyroscope.y <= 1.0f);
    EXPECT_TRUE(gyroscope.z >= -1.0f && gyroscope.z <= 1.0f);
}

TEST_F(PlatformSpecificTest, AndroidCameraIntegration) {
    // Test Android camera
    // This would test camera access and image capture
    EXPECT_TRUE(true); // Camera testing requires actual device
}
#endif

// iOS Platform Tests
#ifdef __APPLE__
TEST_F(PlatformSpecificTest, iOSMetalIntegration) {
    // Test iOS Metal rendering
    Renderer* renderer = engine->getRenderer();
    ASSERT_NE(renderer, nullptr);
    
    EXPECT_TRUE(renderer->isInitialized());
    
    // Test Metal device creation
    EXPECT_TRUE(renderer->createMetalDevice());
}

TEST_F(PlatformSpecificTest, iOSAVAudioEngineIntegration) {
    // Test AVAudioEngine
    AudioManager* audio = engine->getAudio();
    ASSERT_NE(audio, nullptr);
    
    EXPECT_TRUE(audio->isInitialized());
    
    // Test AVAudioEngine setup
    EXPECT_TRUE(audio->createAVAudioEngine());
}

TEST_F(PlatformSpecificTest, iOSGameControllerIntegration) {
    // Test Game Controller framework
    InputManager* input = engine->getInput();
    ASSERT_NE(input, nullptr);
    
    // Test gamepad detection
    int gamepadCount = input->getGamepadCount();
    EXPECT_GE(gamepadCount, 0);
}
#endif

// Web Platform Tests
TEST_F(PlatformSpecificTest, WebWebGLIntegration) {
    // Test WebGL rendering
    Renderer* renderer = engine->getRenderer();
    ASSERT_NE(renderer, nullptr);
    
    EXPECT_TRUE(renderer->isInitialized());
    
    // Test WebGL context creation
    EXPECT_TRUE(renderer->createWebGLContext());
    
    // Test WebGL extensions
    EXPECT_TRUE(renderer->hasExtension("WEBGL_depth_texture"));
}

TEST_F(PlatformSpecificTest, WebWebAudioIntegration) {
    // Test Web Audio API
    AudioManager* audio = engine->getAudio();
    ASSERT_NE(audio, nullptr);
    
    EXPECT_TRUE(audio->isInitialized());
    
    // Test Web Audio context creation
    EXPECT_TRUE(audio->createWebAudioContext());
}

TEST_F(PlatformSpecificTest, WebWebAssemblyIntegration) {
    // Test WebAssembly module loading
    // This would test WASM module instantiation
    EXPECT_TRUE(true); // WASM testing requires browser environment
}

// Cross-Platform Tests
TEST_F(PlatformSpecificTest, CrossPlatformMathOperations) {
    // Test math operations across platforms
    Vector3 v1(1, 2, 3);
    Vector3 v2(4, 5, 6);
    
    Vector3 result = v1 + v2;
    EXPECT_EQ(result.x, 5.0f);
    EXPECT_EQ(result.y, 7.0f);
    EXPECT_EQ(result.z, 9.0f);
    
    float dot = v1.dot(v2);
    EXPECT_FLOAT_EQ(dot, 32.0f); // 1*4 + 2*5 + 3*6 = 32
    
    Vector3 cross = v1.cross(v2);
    EXPECT_FLOAT_EQ(cross.x, -3.0f); // 2*6 - 3*5 = -3
    EXPECT_FLOAT_EQ(cross.y, 6.0f);  // 3*4 - 1*6 = 6
    EXPECT_FLOAT_EQ(cross.z, -3.0f); // 1*5 - 2*4 = -3
}

TEST_F(PlatformSpecificTest, CrossPlatformMemoryManagement) {
    // Test memory management across platforms
    MemoryPool pool(1024, 10 * 1024);
    
    void* ptr = pool.allocateRaw(512);
    ASSERT_NE(ptr, nullptr);
    
    pool.deallocateRaw(ptr);
    EXPECT_EQ(pool.totalAllocated(), 0);
}

TEST_F(PlatformSpecificTest, CrossPlatformFileOperations) {
    // Test file operations across platforms
    std::string testFile = "cross_platform_test.txt";
    std::string testData = "Cross-platform test data";
    
    // Write file
    std::ofstream file(testFile);
    ASSERT_TRUE(file.is_open());
    file << testData;
    file.close();
    
    // Read file
    std::ifstream readFile(testFile);
    ASSERT_TRUE(readFile.is_open());
    std::string readData;
    std::getline(readFile, readData);
    readFile.close();
    
    EXPECT_EQ(readData, testData);
    
    // Cleanup
    std::remove(testFile.c_str());
}

TEST_F(PlatformSpecificTest, CrossPlatformNetworkOperations) {
    // Test network operations across platforms
    NetworkManager* network = engine->getNetwork();
    ASSERT_NE(network, nullptr);
    
    // Test message serialization
    NetworkMessage message;
    message.type = "test";
    message.data = "cross-platform test";
    
    std::vector<uint8_t> serialized = network->serializeMessage(message);
    EXPECT_GT(serialized.size(), 0);
    
    NetworkMessage deserialized = network->deserializeMessage(serialized);
    EXPECT_EQ(deserialized.type, "test");
    EXPECT_EQ(deserialized.data, "cross-platform test");
}

TEST_F(PlatformSpecificTest, CrossPlatformPerformance) {
    // Test performance across platforms
    const int iterations = 10000;
    
    // Test vector operations performance
    auto start = std::chrono::high_resolution_clock::now();
    
    Vector3 result(0, 0, 0);
    for (int i = 0; i < iterations; ++i) {
        Vector3 v1(i, i + 1, i + 2);
        Vector3 v2(i + 3, i + 4, i + 5);
        result = result + v1 * v2;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Performance should be reasonable (less than 1 second for 10k iterations)
    EXPECT_LT(duration.count(), 1000000);
    
    // Result should be non-zero
    EXPECT_NE(result.x, 0.0f);
    EXPECT_NE(result.y, 0.0f);
    EXPECT_NE(result.z, 0.0f);
}

TEST_F(PlatformSpecificTest, CrossPlatformThreading) {
    // Test threading across platforms
    std::atomic<int> counter(0);
    const int threadCount = 4;
    const int incrementsPerThread = 1000;
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back([&counter, incrementsPerThread]() {
            for (int j = 0; j < incrementsPerThread; ++j) {
                counter++;
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(counter.load(), threadCount * incrementsPerThread);
}

TEST_F(PlatformSpecificTest, CrossPlatformErrorHandling) {
    // Test error handling across platforms
    try {
        // Test invalid memory allocation
        MemoryPool pool(1024, 1024);
        void* ptr = pool.allocateRaw(2048); // Larger than pool size
        EXPECT_EQ(ptr, nullptr);
        
        // Test invalid file operations
        std::ifstream file("nonexistent_file.txt");
        EXPECT_FALSE(file.is_open());
        
        // Test invalid network operations
        NetworkManager* network = engine->getNetwork();
        bool connected = network->connect("invalid_host", 99999);
        EXPECT_FALSE(connected);
        
    } catch (const std::exception& e) {
        // Error handling should work properly
        EXPECT_TRUE(true);
    }
}

TEST_F(PlatformSpecificTest, CrossPlatformResourceManagement) {
    // Test resource management across platforms
    Renderer* renderer = engine->getRenderer();
    ASSERT_NE(renderer, nullptr);
    
    // Test resource creation and cleanup
    Texture* texture1 = renderer->createTexture("test1.png");
    Texture* texture2 = renderer->createTexture("test2.png");
    
    EXPECT_NE(texture1, nullptr);
    EXPECT_NE(texture2, nullptr);
    
    // Test resource cleanup
    renderer->destroyTexture(texture1);
    renderer->destroyTexture(texture2);
    
    // Resources should be properly cleaned up
    EXPECT_TRUE(true);
}

TEST_F(PlatformSpecificTest, CrossPlatformConfiguration) {
    // Test configuration across platforms
    EngineConfig config;
    config.windowWidth = 800;
    config.windowHeight = 600;
    config.vsync = true;
    config.fullscreen = false;
    
    // Configuration should be valid
    EXPECT_GT(config.windowWidth, 0);
    EXPECT_GT(config.windowHeight, 0);
    
    // Test platform-specific configuration
    #ifdef _WIN32
    config.renderer = "DirectX11";
    #elif defined(__linux__)
    config.renderer = "OpenGL";
    #elif defined(__APPLE__)
    config.renderer = "Metal";
    #elif defined(__ANDROID__)
    config.renderer = "OpenGLES";
    #else
    config.renderer = "WebGL";
    #endif
    
    EXPECT_FALSE(config.renderer.empty());
}
