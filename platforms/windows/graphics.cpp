/**
 * Windows Graphics Implementation
 * DirectX 11 Graphics Pipeline for Windows Platform
 */

#include "WindowsPlatform.h"
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>

// Link required DirectX libraries
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

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
        if (swapChain_) {
            swapChain_->Present(1, 0);
        }
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
        if (context_) {
            context_->OMSetRenderTargets(0, nullptr, nullptr);
        }

        if (swapChain_) {
            HRESULT hr = swapChain_->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
            if (FAILED(hr)) return;
        }

        // Recreate render targets
        if (swapChain_ && device_) {
            ID3D11Texture2D* backBuffer = nullptr;
            HRESULT hr = swapChain_->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
            if (SUCCEEDED(hr)) {
                hr = device_->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView_);
                backBuffer->Release();
            }

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
            if (SUCCEEDED(hr)) {
                hr = device_->CreateDepthStencilView(depthStencilBuffer, nullptr, &depthStencilView_);
                depthStencilBuffer->Release();
            }

            // Set render targets
            if (context_) {
                context_->OMSetRenderTargets(1, &renderTargetView_, depthStencilView_);

                // Set viewport
                D3D11_VIEWPORT viewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
                context_->RSSetViewports(1, &viewport);
            }
        }
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
        if (graphics_ && graphics_->getContext()) {
            D3D11_VIEWPORT viewport = { (float)x, (float)y, (float)width, (float)height, 0.0f, 1.0f };
            graphics_->getContext()->RSSetViewports(1, &viewport);
        }
    }

    void clear(unsigned int mask) override {
        if (!graphics_) return;

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
