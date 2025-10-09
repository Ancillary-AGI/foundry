/**
 * Windows Platform Implementation
 * Native Windows platform using Visual C++ with DirectX and Win32 API
 */

#include "WindowsPlatform.h"
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <xaudio2.h>
#include <xinput.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <thread>
#include <random>

// Link required libraries
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "xinput.lib")
#pragma comment(lib, "shlwapi.lib")

// Forward declarations
class WindowsGraphics;
class WindowsAudio;
class WindowsInput;
class WindowsFileSystem;

// Windows message handling
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

class WindowsPlatform : public Platform {
private:
    std::unique_ptr<WindowsGraphics> graphics_;
    std::unique_ptr<WindowsAudio> audio_;
    std::unique_ptr<WindowsInput> input_;
    std::unique_ptr<WindowsFileSystem> fileSystem_;
    std::unique_ptr<WindowsTimer> timer_;
    std::unique_ptr<WindowsRandom> random_;
    std::unique_ptr<WindowsNetworking> networking_;

    PlatformCapabilities capabilities_;
    HWND windowHandle_ = nullptr;
    HINSTANCE instanceHandle_ = nullptr;
    int windowWidth_ = 1280;
    int windowHeight_ = 720;
    bool running_ = false;

public:
    WindowsPlatform(HINSTANCE hInstance) : instanceHandle_(hInstance) {
        detectCapabilities();
        registerWindowClass();

        // Initialize subsystems
        graphics_ = std::make_unique<WindowsGraphics>(nullptr, windowWidth_, windowHeight_);
        audio_ = std::make_unique<WindowsAudio>();
        input_ = std::make_unique<WindowsInput>(nullptr);
        fileSystem_ = std::make_unique<WindowsFileSystem>();
        timer_ = std::make_unique<WindowsTimer>();
        random_ = std::make_unique<WindowsRandom>();
        networking_ = std::make_unique<WindowsNetworking>();
    }

    ~WindowsPlatform() {
        destroyWindow();
    }

    // Platform interface implementation
    PlatformCapabilities getCapabilities() override { return capabilities_; }

    std::unique_ptr<PlatformCanvas> createCanvas(int width, int height) override {
        return std::make_unique<WindowsCanvas>(width, height, windowHandle_);
    }

    PlatformInputManager* getInputManager() override { return input_.get(); }
    PlatformFileSystem* getFileSystem() override { return fileSystem_.get(); }
    PlatformNetworking* getNetworking() override { return networking_.get(); }
    PlatformAudio* getAudio() override { return audio_.get(); }
    PlatformGraphics* getGraphics() override { return graphics_.get(); }
    PlatformTimer* getTimer() override { return timer_.get(); }
    PlatformRandom* getRandom() override { return random_.get(); }

    // Windows-specific methods
    bool createWindow(int width, int height, const std::string& title) {
        windowWidth_ = width;
        windowHeight_ = height;

        RECT windowRect = { 0, 0, width, height };
        AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

        int windowWidth = windowRect.right - windowRect.left;
        int windowHeight = windowRect.bottom - windowRect.top;

        // Center window on screen
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        int windowX = (screenWidth - windowWidth) / 2;
        int windowY = (screenHeight - windowHeight) / 2;

        windowHandle_ = CreateWindowEx(
            0,
            "FoundryEngineWindow",
            title.c_str(),
            WS_OVERLAPPEDWINDOW,
            windowX, windowY, windowWidth, windowHeight,
            nullptr, nullptr, instanceHandle_, nullptr
        );

        if (!windowHandle_) {
            return false;
        }

        // Update subsystems with window handle
        graphics_ = std::make_unique<WindowsGraphics>(windowHandle_, width, height);
        input_ = std::make_unique<WindowsInput>(windowHandle_);

        ShowWindow(windowHandle_, SW_SHOW);
        UpdateWindow(windowHandle_);

        running_ = true;
        return true;
    }

    void destroyWindow() {
        if (windowHandle_) {
            DestroyWindow(windowHandle_);
            windowHandle_ = nullptr;
        }
        running_ = false;
    }

    void processMessages() {
        MSG msg = {};
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running_ = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    bool isRunning() const { return running_; }
    HWND getWindowHandle() const { return windowHandle_; }
    HINSTANCE getInstanceHandle() const { return instanceHandle_; }

private:
    void detectCapabilities() {
        capabilities_.platformName = "Windows";
        capabilities_.supportsWebGL = false; // DirectX instead
        capabilities_.supportsWebAudio = false; // XAudio2 instead
        capabilities_.supportsWebRTC = false; // WinSock instead
        capabilities_.supportsIndexedDB = false; // Windows registry/files instead
        capabilities_.supportsServiceWorker = false; // Windows services instead
        capabilities_.supportsPushNotifications = false; // Windows notifications instead
        capabilities_.maxTextureSize = 16384;
        capabilities_.maxRenderbufferSize = 16384;
        capabilities_.supportsVertexShaders = true;
        capabilities_.supportsFragmentShaders = true;
        capabilities_.supportsTextures = true;
        capabilities_.supportsFramebuffers = true;
        capabilities_.supportsDepthTextures = true;
        capabilities_.supportsInstancing = true;
        capabilities_.maxVertexAttributes = 16;
        capabilities_.maxTextureUnits = 16;
    }

    void registerWindowClass() {
        WNDCLASSEX wc = {};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = instanceHandle_;
        wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = "FoundryEngineWindow";
        wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

        if (!RegisterClassEx(&wc)) {
            // Handle error
        }
    }
};

// ========== WINDOWS GRAPHICS (DirectX 11) ==========
class WindowsGraphics : public PlatformGraphics {
private:
    ID3D11Device* device_ = nullptr;
    ID3D11DeviceContext* context_ = nullptr;
    IDXGISwapChain* swapChain_ = nullptr;
    ID3D11RenderTargetView* renderTargetView_ = nullptr;
    ID3D11DepthStencilView* depthStencilView_ = nullptr;
    D3D_FEATURE_LEVEL featureLevel_;

    HWND windowHandle_;
    int width_, height_;

public:
    WindowsGraphics(HWND hwnd, int width, int height) : windowHandle_(hwnd), width_(width), height_(height) {}
    ~WindowsGraphics() { shutdown(); }

    bool initialize() {
        // Create device and swap chain
        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
        swapChainDesc.BufferCount = 1;
        swapChainDesc.BufferDesc.Width = width_;
        swapChainDesc.BufferDesc.Height = height_;
        swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
        swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.OutputWindow = windowHandle_;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Windowed = TRUE;

        D3D_FEATURE_LEVEL featureLevels[] = {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0
        };

        UINT createDeviceFlags = 0;
#ifdef _DEBUG
        createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
            featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION,
            &swapChainDesc, &swapChain_, &device_, &featureLevel_, &context_
        );

        if (FAILED(hr)) {
            return false;
        }

        // Create render target view
        ID3D11Texture2D* backBuffer = nullptr;
        hr = swapChain_->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
        if (FAILED(hr)) {
            return false;
        }

        hr = device_->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView_);
        backBuffer->Release();
        if (FAILED(hr)) {
            return false;
        }

        // Create depth stencil view
        D3D11_TEXTURE2D_DESC depthStencilDesc = {};
        depthStencilDesc.Width = width_;
        depthStencilDesc.Height = height_;
        depthStencilDesc.MipLevels = 1;
        depthStencilDesc.ArraySize = 1;
        depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthStencilDesc.SampleDesc.Count = 1;
        depthStencilDesc.SampleDesc.Quality = 0;
        depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
        depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        depthStencilDesc.CPUAccessFlags = 0;
        depthStencilDesc.MiscFlags = 0;

        ID3D11Texture2D* depthStencilBuffer = nullptr;
        hr = device_->CreateTexture2D(&depthStencilDesc, nullptr, &depthStencilBuffer);
        if (FAILED(hr)) {
            return false;
        }

        hr = device_->CreateDepthStencilView(depthStencilBuffer, nullptr, &depthStencilView_);
        depthStencilBuffer->Release();
        if (FAILED(hr)) {
            return false;
        }

        // Set render targets
        context_->OMSetRenderTargets(1, &renderTargetView_, depthStencilView_);

        // Set viewport
        D3D11_VIEWPORT viewport = { 0.0f, 0.0f, (float)width_, (float)height_, 0.0f, 1.0f };
        context_->RSSetViewports(1, &viewport);

        return true;
    }

    void shutdown() {
        if (depthStencilView_) {
            depthStencilView_->Release();
            depthStencilView_ = nullptr;
        }
        if (renderTargetView_) {
            renderTargetView_->Release();
            renderTargetView_ = nullptr;
        }
        if (swapChain_) {
            swapChain_->Release();
            swapChain_ = nullptr;
        }
        if (context_) {
            context_->Release();
            context_ = nullptr;
        }
        if (device_) {
            device_->Release();
            device_ = nullptr;
        }
    }

    void present() {
        swapChain_->Present(1, 0);
    }

    void resize(int width, int height) {
        if (width == width_ && height == height_) return;

        width_ = width;
        height_ = height;

        // Release render targets
        if (renderTargetView_) {
            renderTargetView_->Release();
            renderTargetView_ = nullptr;
        }
        if (depthStencilView_) {
            depthStencilView_->Release();
            depthStencilView_ = nullptr;
        }

        // Resize swap chain
        context_->OMSetRenderTargets(0, nullptr, nullptr);
        HRESULT hr = swapChain_->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
        if (FAILED(hr)) return;

        // Recreate render targets
        ID3D11Texture2D* backBuffer = nullptr;
        hr = swapChain_->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
        if (FAILED(hr)) return;

        hr = device_->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView_);
        backBuffer->Release();
        if (FAILED(hr)) return;

        // Recreate depth stencil
        D3D11_TEXTURE2D_DESC depthStencilDesc = {};
        depthStencilDesc.Width = width;
        depthStencilDesc.Height = height;
        depthStencilDesc.MipLevels = 1;
        depthStencilDesc.ArraySize = 1;
        depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthStencilDesc.SampleDesc.Count = 1;
        depthStencilDesc.SampleDesc.Quality = 0;
        depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
        depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        depthStencilDesc.CPUAccessFlags = 0;
        depthStencilDesc.MiscFlags = 0;

        ID3D11Texture2D* depthStencilBuffer = nullptr;
        hr = device_->CreateTexture2D(&depthStencilDesc, nullptr, &depthStencilBuffer);
        if (FAILED(hr)) return;

        hr = device_->CreateDepthStencilView(depthStencilBuffer, nullptr, &depthStencilView_);
        depthStencilBuffer->Release();
        if (FAILED(hr)) return;

        // Set render targets
        context_->OMSetRenderTargets(1, &renderTargetView_, depthStencilView_);

        // Set viewport
        D3D11_VIEWPORT viewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
        context_->RSSetViewports(1, &viewport);
    }

    // PlatformGraphics interface
    std::unique_ptr<PlatformGraphicsContext> createContext() override {
        return std::make_unique<WindowsD3DContext>(this);
    }

    PlatformCapabilities getCapabilities() override {
        PlatformCapabilities caps;
        caps.maxTextureSize = 16384; // Conservative estimate
        caps.maxRenderbufferSize = 16384;
        caps.maxViewportWidth = 16384;
        caps.maxViewportHeight = 16384;
        caps.supportsVertexShaders = true;
        caps.supportsFragmentShaders = true;
        caps.supportsTextures = true;
        caps.supportsFramebuffers = true;
        caps.supportsDepthTextures = true;
        caps.supportsInstancing = true;
        caps.maxVertexAttributes = 16;
        caps.maxTextureUnits = 16;
        return caps;
    }

    // DirectX-specific access
    ID3D11Device* getDevice() { return device_; }
    ID3D11DeviceContext* getContext() { return context_; }
    IDXGISwapChain* getSwapChain() { return swapChain_; }
    ID3D11RenderTargetView* getRenderTargetView() { return renderTargetView_; }
    ID3D11DepthStencilView* getDepthStencilView() { return depthStencilView_; }
};

class WindowsD3DContext : public PlatformGraphicsContext {
private:
    WindowsGraphics* graphics_;
    std::unordered_map<std::string, ID3D11Buffer*> vertexBuffers_;
    std::unordered_map<std::string, ID3D11Buffer*> indexBuffers_;
    std::unordered_map<std::string, ID3D11VertexShader*> vertexShaders_;
    std::unordered_map<std::string, ID3D11PixelShader*> pixelShaders_;
    std::unordered_map<std::string, ID3D11InputLayout*> inputLayouts_;

public:
    WindowsD3DContext(WindowsGraphics* graphics) : graphics_(graphics) {}

    // WebGL-style interface for compatibility
    void viewport(int x, int y, int width, int height) override {
        D3D11_VIEWPORT viewport = { (float)x, (float)y, (float)width, (float)height, 0.0f, 1.0f };
        graphics_->getContext()->RSSetViewports(1, &viewport);
    }

    void clear(unsigned int mask) override {
        if (mask & GL_COLOR_BUFFER_BIT) {
            graphics_->getContext()->ClearRenderTargetView(graphics_->getRenderTargetView(), clearColor_);
        }
        if (mask & GL_DEPTH_BUFFER_BIT) {
            graphics_->getContext()->ClearDepthStencilView(graphics_->getDepthStencilView(), D3D11_CLEAR_DEPTH, 1.0f, 0);
        }
    }

    void clearColor(float r, float g, float b, float a) override {
        clearColor_[0] = r;
        clearColor_[1] = g;
        clearColor_[2] = b;
        clearColor_[3] = a;
    }

    void enable(unsigned int cap) override {
        // Map WebGL capabilities to DirectX states
        switch (cap) {
        case GL_DEPTH_TEST:
            // Enable depth testing
            break;
        case GL_CULL_FACE:
            // Enable culling
            break;
        case GL_BLEND:
            // Enable blending
            break;
        default:
            // Unknown capability
            break;
        }
    }

    void disable(unsigned int cap) override {
        // Disable corresponding states
    }

    void cullFace(unsigned int mode) override {
        // Set cull mode
    }

    void depthFunc(unsigned int func) override {
        // Set depth function
    }

    void blendFunc(unsigned int sfactor, unsigned int dfactor) override {
        // Set blend function
    }

    unsigned int createBuffer() override {
        // Create vertex/index buffer
        return 0; // Return handle
    }

    void bindBuffer(unsigned int target, unsigned int buffer) override {
        // Bind buffer
    }

    void bufferData(unsigned int target, const void* data, size_t size, unsigned int usage) override {
        // Upload buffer data
    }

    void deleteBuffer(unsigned int buffer) override {
        // Delete buffer
    }

    unsigned int createShader(unsigned int type) override {
        // Create shader
        return 0; // Return handle
    }

    void shaderSource(unsigned int shader, const std::string& source) override {
        // Store shader source
    }

    void compileShader(unsigned int shader) override {
        // Compile shader using D3DCompile
    }

    int getShaderParameter(unsigned int shader, unsigned int pname) override {
        // Return shader parameter
        return 0;
    }

    std::string getShaderInfoLog(unsigned int shader) override {
        // Return compilation errors
        return "";
    }

    void deleteShader(unsigned int shader) override {
        // Delete shader
    }

    unsigned int createProgram() override {
        // Create shader program
        return 0;
    }

    void attachShader(unsigned int program, unsigned int shader) override {
        // Attach shader to program
    }

    void linkProgram(unsigned int program) override {
        // Link shader program
    }

    int getProgramParameter(unsigned int program, unsigned int pname) override {
        // Return program parameter
        return 0;
    }

    std::string getProgramInfoLog(unsigned int program) override {
        // Return linking errors
        return "";
    }

    void useProgram(unsigned int program) override {
        // Set active shader program
    }

    void deleteProgram(unsigned int program) override {
        // Delete program
    }

    int getAttribLocation(unsigned int program, const std::string& name) override {
        // Get attribute location
        return -1;
    }

    int getUniformLocation(unsigned int program, const std::string& name) override {
        // Get uniform location
        return -1;
    }

    // ... implement remaining WebGL compatibility methods

private:
    float clearColor_[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
};

WindowsAudio::WindowsAudio() = default;

WindowsAudio::~WindowsAudio() {
    shutdown();
}

bool WindowsAudio::initialize() {
    HRESULT hr = XAudio2Create(&xaudio2_, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(hr)) {
        return false;
    }

    hr = xaudio2_->CreateMasteringVoice(&masteringVoice_);
    if (FAILED(hr)) {
        shutdown();
        return false;
    }

    return true;
}

void WindowsAudio::shutdown() {
    for (auto& pair : sourceVoices_) {
        if (pair.second) {
            pair.second->DestroyVoice();
        }
    }
    sourceVoices_.clear();

    if (masteringVoice_) {
        masteringVoice_->DestroyVoice();
        masteringVoice_ = nullptr;
    }

    if (xaudio2_) {
        xaudio2_->Release();
        xaudio2_ = nullptr;
    }
}

std::unique_ptr<PlatformAudioContext> WindowsAudio::createContext() override {
    return std::make_unique<WindowsAudioContext>(xaudio2_, masteringVoice_);
}

void WindowsAudio::resume() override {
    if (xaudio2_) {
        XAUDIO2_VOICE_STATE state;
        masteringVoice_->GetState(&state);
        if (state.BuffersQueued == 0) {
            // Resume audio processing
        }
    }
}

void WindowsAudio::suspend() override {
    if (xaudio2_) {
        // Suspend audio processing
    }
}

// ========== WINDOWS AUDIO (XAudio2) ==========
class WindowsAudio : public PlatformAudio {
private:
    IXAudio2* xaudio2_ = nullptr;
    IXAudio2MasteringVoice* masteringVoice_ = nullptr;
    std::unordered_map<std::string, IXAudio2SourceVoice*> sourceVoices_;

public:
    WindowsAudio();
    ~WindowsAudio();

    bool initialize();
    void shutdown();

    std::unique_ptr<PlatformAudioContext> createContext() override;
    void resume() override;
    void suspend() override;
};

class WindowsAudioContext : public PlatformAudioContext {
private:
    IXAudio2* xaudio2_;
    IXAudio2MasteringVoice* masteringVoice_;

public:
    WindowsAudioContext(IXAudio2* xaudio2, IXAudio2MasteringVoice* masteringVoice)
        : xaudio2_(xaudio2), masteringVoice_(masteringVoice) {}

    std::unique_ptr<PlatformAudioBuffer> createBuffer(unsigned int channels, unsigned int length, float sampleRate) override {
        // Create audio buffer implementation
        return nullptr;
    }

    std::unique_ptr<PlatformAudioBufferSource> createBufferSource() override {
        // Create buffer source implementation
        return nullptr;
    }

    std::unique_ptr<PlatformGainNode> createGain() override {
        // Create gain node implementation
        return nullptr;
    }

    PlatformAudioDestination* getDestination() override {
        // Return audio destination implementation
        return nullptr;
    }

    float getCurrentTime() override {
        if (xaudio2_) {
            XAUDIO2_PERFORMANCE_DATA perfData;
            xaudio2_->GetPerformanceData(&perfData);
            return static_cast<float>(perfData.AudioBytes) / 44100.0f / 4.0f; // Assuming 44.1kHz stereo
        }
        return 0.0f;
    }

    float getSampleRate() override {
        return 44100.0f; // Default sample rate
    }
};

// ========== WINDOWS INPUT ==========
class WindowsInput : public PlatformInputManager {
private:
    std::unordered_map<int, bool> keyStates_;
    std::unordered_map<int, bool> mouseButtons_;
    POINT mousePosition_;
    std::vector<XINPUT_STATE> gamepadStates_;
    std::vector<std::function<void(const InputEvent&)>> listeners_;

public:
    WindowsInput(HWND hwnd);
    ~WindowsInput();

    void update();

    std::unordered_map<int, bool> getKeyboardState() override { return keyStates_; }
    MouseState getMouseState() override;
    std::vector<TouchPoint> getTouchState() override { return {}; } // Windows doesn't have touch by default
    GamepadState getGamepadState(int index) override;
    std::vector<GamepadState> getConnectedGamepads() override;
    int getGamepadCount() override;
    bool isGamepadConnected(int index) override;
    std::string getGamepadName(int index) override;
    bool setGamepadVibration(int index, float leftMotor, float rightMotor, float duration) override;

    void addEventListener(const std::string& type, std::function<void(const InputEvent&)> listener) override;
    void removeEventListener(const std::string& type, std::function<void(const InputEvent&)> listener) override;

    // Windows message handling
    void handleKeyMessage(UINT message, WPARAM wParam, LPARAM lParam);
    void handleMouseMessage(UINT message, WPARAM wParam, LPARAM lParam);
};

// ========== WINDOWS FILE SYSTEM ==========
class WindowsFileSystem : public PlatformFileSystem {
private:
    std::string appDataPath_;
    std::string documentsPath_;

public:
    WindowsFileSystem() {
        // Initialize paths
        appDataPath_ = getAppDataPath();
        documentsPath_ = getDocumentsPath();
    }

    std::vector<uint8_t> readFile(const std::string& path) override {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            return {};
        }

        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> data(size);
        file.read(reinterpret_cast<char*>(data.data()), size);
        return data;
    }

    void writeFile(const std::string& path, const std::vector<uint8_t>& data) override {
        std::ofstream file(path, std::ios::binary);
        if (file) {
            file.write(reinterpret_cast<const char*>(data.data()), data.size());
        }
    }

    void deleteFile(const std::string& path) override {
        DeleteFileA(path.c_str());
    }

    std::vector<std::string> listFiles(const std::string& directory) override {
        std::vector<std::string> files;
        std::string searchPath = directory + "\\*";

        WIN32_FIND_DATAA findData;
        HANDLE findHandle = FindFirstFileA(searchPath.c_str(), &findData);

        if (findHandle != INVALID_HANDLE_VALUE) {
            do {
                if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    files.push_back(findData.cFileName);
                }
            } while (FindNextFileA(findHandle, &findData));
            FindClose(findHandle);
        }

        return files;
    }

    void createDirectory(const std::string& path) override {
        CreateDirectoryA(path.c_str(), nullptr);
    }

    bool exists(const std::string& path) override {
        DWORD attributes = GetFileAttributesA(path.c_str());
        return attributes != INVALID_FILE_ATTRIBUTES;
    }

private:
    std::string getAppDataPath() {
        char path[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, path))) {
            return std::string(path) + "\\FoundryEngine";
        }
        return ".\\data";
    }

    std::string getDocumentsPath() {
        char path[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_MYDOCUMENTS, nullptr, 0, path))) {
            return std::string(path) + "\\FoundryEngine";
        }
        return ".\\documents";
    }
};

// ========== WINDOWS NETWORKING ==========
class WindowsNetworking : public PlatformNetworking {
private:
    WSADATA wsaData_;
    bool initialized_ = false;

public:
    WindowsNetworking() {
        initialize();
    }

    ~WindowsNetworking() {
        shutdown();
    }

    bool initialize() {
        if (initialized_) return true;

        int result = WSAStartup(MAKEWORD(2, 2), &wsaData_);
        if (result != 0) {
            return false;
        }

        initialized_ = true;
        return true;
    }

    void shutdown() {
        if (initialized_) {
            WSACleanup();
            initialized_ = false;
        }
    }

    std::unique_ptr<PlatformWebSocket> connect(const std::string& url) override {
        // Parse URL and create WebSocket connection
        // For now, return nullptr as this is a placeholder
        return nullptr;
    }

    std::vector<uint8_t> httpGet(const std::string& url) override {
        // Implement HTTP GET using WinHTTP or similar
        return {};
    }

    std::vector<uint8_t> httpPost(const std::string& url, const std::vector<uint8_t>& data) override {
        // Implement HTTP POST using WinHTTP or similar
        return {};
    }
};

// ========== WINDOWS TIMER ==========
class WindowsTimer : public PlatformTimer {
private:
    LARGE_INTEGER frequency_;
    LARGE_INTEGER startTime_;

public:
    WindowsTimer() {
        QueryPerformanceFrequency(&frequency_);
        QueryPerformanceCounter(&startTime_);
    }

    double now() override {
        LARGE_INTEGER current;
        QueryPerformanceCounter(&current);
        return static_cast<double>(current.QuadPart - startTime_.QuadPart) / frequency_.QuadPart * 1000.0;
    }

    int setTimeout(std::function<void()> callback, int delay) override {
        // Use Windows timers or std::thread
        return 0;
    }

    void clearTimeout(int id) override {
        // Clear timer
    }

    int setInterval(std::function<void()> callback, int delay) override {
        // Use Windows timers
        return 0;
    }

    void clearInterval(int id) override {
        // Clear interval
    }

    int requestAnimationFrame(std::function<void(double)> callback) override {
        // Windows doesn't have requestAnimationFrame, use timer
        return 0;
    }

    void cancelAnimationFrame(int id) override {
        // Cancel animation frame
    }
};

// ========== WINDOWS RANDOM ==========
class WindowsRandom : public PlatformRandom {
public:
    double random() override {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0.0, 1.0);
        return dis(gen);
    }

    int randomInt(int min, int max) override {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(min, max);
        return dis(gen);
    }

    double randomFloat(double min, double max) override {
        return min + (random() * (max - min));
    }

    void seed(unsigned int seed) override {
        generator_.seed(seed);
    }

private:
    std::mt19937 generator_;
};

// ========== WINDOWS APPLICATION ==========
class WindowsApplication {
private:
    WindowsPlatform* platform_;
    std::unique_ptr<GameEngine> engine_;
    bool running_ = false;
    HINSTANCE hInstance_;

public:
    WindowsApplication(HINSTANCE hInstance) : hInstance_(hInstance) {
        platform_ = new WindowsPlatform(hInstance);
    }

    ~WindowsApplication() {
        if (platform_) {
            delete platform_;
        }
    }

    bool initialize(int width, int height, const std::string& title) {
        if (!platform_) return false;

        if (!platform_->createWindow(width, height, title)) {
            return false;
        }

        // Initialize game engine here if needed
        // engine_ = std::make_unique<GameEngine>(platform_);

        return true;
    }

    void run() {
        if (!platform_) return;

        running_ = true;

        // Main game loop
        while (running_ && platform_->isRunning()) {
            // Process Windows messages
            platform_->processMessages();

            // Update game logic
            update(16.67f); // ~60 FPS

            // Render frame
            render();
        }
    }

    void shutdown() {
        running_ = false;
        platform_->destroyWindow();

        // Cleanup game engine
        if (engine_) {
            engine_.reset();
        }
    }

private:
    void update(float deltaTime) {
        if (engine_) {
            // engine_->update(deltaTime);
        }

        // Update platform systems
        if (platform_) {
            platform_->getInputManager()->update();
        }
    }

    void render() {
        if (!platform_) return;

        // Get graphics context and render
        PlatformGraphics* graphics = platform_->getGraphics();
        if (graphics) {
            auto context = graphics->createContext();
            if (context) {
                // Clear screen
                context->clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // Render game content here
                // if (engine_) {
                //     engine_->render(context.get());
                // }

                // Present frame
                graphics->present();
            }
        }
    }

    LRESULT handleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            running_ = false;
            return 0;

        case WM_SIZE:
            if (platform_) {
                int width = LOWORD(lParam);
                int height = HIWORD(lParam);
                platform_->getGraphics()->resize(width, height);
            }
            break;

        case WM_KEYDOWN:
        case WM_KEYUP:
            if (platform_) {
                WindowsInput* input = static_cast<WindowsInput*>(platform_->getInputManager());
                if (input) {
                    input->handleKeyMessage(uMsg, wParam, lParam);
                }
            }
            break;

        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
            if (platform_) {
                WindowsInput* input = static_cast<WindowsInput*>(platform_->getInputManager());
                if (input) {
                    input->handleMouseMessage(uMsg, wParam, lParam);
                }
            }
            break;

        case WM_CLOSE:
            running_ = false;
            return 0;

        default:
            // Unhandled message; fall through to DefWindowProc
            break;
        }

        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
};

// ========== WINDOW PROCEDURE ==========
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    WindowsApplication* app = nullptr;

    if (uMsg == WM_CREATE) {
        CREATESTRUCT* createStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
        app = static_cast<WindowsApplication*>(createStruct->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    } else {
        app = reinterpret_cast<WindowsApplication*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (app) {
        return app->handleMessage(hwnd, uMsg, wParam, lParam);
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// ========== WINDOWS INPUT IMPLEMENTATIONS ==========
WindowsInput::WindowsInput(HWND hwnd) : hwnd_(hwnd) {
    gamepadStates_.resize(4); // XInput supports up to 4 controllers
}

WindowsInput::~WindowsInput() {
    // Clear vibration on exit
    for (int i = 0; i < 4; i++) {
        XINPUT_VIBRATION vibration = {0, 0};
        XInputSetState(i, &vibration);
    }
}

void WindowsInput::update() {
    // Update gamepad states
    for (int i = 0; i < 4; i++) {
        DWORD result = XInputGetState(i, &gamepadStates_[i]);
        if (result != ERROR_SUCCESS) {
            // Controller not connected, clear state
            ZeroMemory(&gamepadStates_[i], sizeof(XINPUT_STATE));
        }
    }
}

MouseState WindowsInput::getMouseState() {
    POINT cursorPos;
    GetCursorPos(&cursorPos);
    ScreenToClient(hwnd_, &cursorPos);

    return {
        cursorPos.x,
        cursorPos.y,
        mouseButtons_
    };
}

GamepadState WindowsInput::getGamepadState(int index) {
    if (index < 0 || index >= 4) {
        return {false, "", {}, {}};
    }

    XINPUT_STATE state = gamepadStates_[index];
    if (XInputGetState(index, &state) != ERROR_SUCCESS) {
        return {false, "", {}, {}};
    }

    std::vector<ButtonState> buttons;
    buttons.push_back({(state.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0, state.Gamepad.bLeftTrigger / 255.0f});
    buttons.push_back({(state.Gamepad.wButtons & XINPUT_GAMEPAD_B) != 0, state.Gamepad.bRightTrigger / 255.0f});
    buttons.push_back({(state.Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0, 0.0f});
    buttons.push_back({(state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0, 0.0f});
    buttons.push_back({(state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0, state.Gamepad.bLeftTrigger / 255.0f});
    buttons.push_back({(state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0, state.Gamepad.bRightTrigger / 255.0f});
    buttons.push_back({(state.Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0, 0.0f});
    buttons.push_back({(state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) != 0, 0.0f});
    buttons.push_back({(state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0, 0.0f});
    buttons.push_back({(state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0, 0.0f});
    buttons.push_back({(state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0, 0.0f});
    buttons.push_back({(state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0, 0.0f});
    buttons.push_back({(state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0, 0.0f});
    buttons.push_back({(state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0, 0.0f});

    std::vector<float> axes = {
        state.Gamepad.sThumbLX / 32767.0f,
        state.Gamepad.sThumbLY / 32767.0f,
        state.Gamepad.sThumbRX / 32767.0f,
        state.Gamepad.sThumbRY / 32767.0f,
        state.Gamepad.bLeftTrigger / 255.0f,
        state.Gamepad.bRightTrigger / 255.0f
    };

    return {true, "XInput Controller", buttons, axes};
}

std::vector<GamepadState> WindowsInput::getConnectedGamepads() {
    std::vector<GamepadState> connected;
    for (int i = 0; i < 4; i++) {
        GamepadState state = getGamepadState(i);
        if (state.connected) {
            connected.push_back(state);
        }
    }
    return connected;
}

int WindowsInput::getGamepadCount() {
    return 4; // XInput always supports up to 4 controllers
}

bool WindowsInput::isGamepadConnected(int index) {
    if (index < 0 || index >= 4) return false;
    XINPUT_STATE state;
    return XInputGetState(index, &state) == ERROR_SUCCESS;
}

std::string WindowsInput::getGamepadName(int index) {
    if (!isGamepadConnected(index)) return "";
    return "XInput Controller";
}

bool WindowsInput::setGamepadVibration(int index, float leftMotor, float rightMotor, float duration) {
    if (index < 0 || index >= 4) return false;

    XINPUT_VIBRATION vibration;
    vibration.wLeftMotorSpeed = static_cast<WORD>(leftMotor * 65535.0f);
    vibration.wRightMotorSpeed = static_cast<WORD>(rightMotor * 65535.0f);

    DWORD result = XInputSetState(index, &vibration);
    if (result == ERROR_SUCCESS && duration > 0) {
        // Stop vibration after duration
        std::thread([index, duration]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(duration * 1000)));
            XINPUT_VIBRATION stopVibration = {0, 0};
            XInputSetState(index, &stopVibration);
        }).detach();
    }

    return result == ERROR_SUCCESS;
}

void WindowsInput::addEventListener(const std::string& type, std::function<void(const InputEvent&)> listener) {
    listeners_.push_back(listener);
}

void WindowsInput::removeEventListener(const std::string& type, std::function<void(const InputEvent&)> listener) {
    // Remove listener implementation
}

void WindowsInput::handleKeyMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    bool pressed = (message == WM_KEYDOWN);
    keyStates_[static_cast<int>(wParam)] = pressed;

    InputEvent event = { "keyboard", pressed ? "press" : "release" };
    event.key = static_cast<int>(wParam);
    event.timestamp = GetTickCount64();

    for (auto& listener : listeners_) {
        listener(event);
    }
}

void WindowsInput::handleMouseMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    int button = -1;
    bool pressed = false;

    switch (message) {
    case WM_LBUTTONDOWN: button = 0; pressed = true; break;
    case WM_LBUTTONUP: button = 0; pressed = false; break;
    case WM_RBUTTONDOWN: button = 2; pressed = true; break;
    case WM_RBUTTONUP: button = 2; pressed = false; break;
    case WM_MBUTTONDOWN: button = 1; pressed = true; break;
    case WM_MBUTTONUP: button = 1; pressed = false; break;
    default: return; // Add default case
    }

    if (button >= 0) {
        mouseButtons_[button] = pressed;

        InputEvent event = { "mouse", pressed ? "press" : "release" };
        event.button = button;
        event.position = { LOWORD(lParam), HIWORD(lParam) };
        event.timestamp = GetTickCount64();

        for (auto& listener : listeners_) {
            listener(event);
        }
    }
}

// ========== MAIN ENTRY POINT ==========
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Create application
    WindowsApplication app(hInstance);

    // Initialize with default settings
    if (!app.initialize(1280, 720, "Game Engine")) {
        return 1;
    }

    // Run main loop
    app.run();

    // Shutdown
    app.shutdown();

    return 0;
}
