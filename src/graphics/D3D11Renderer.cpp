#include "../../include/GameEngine/graphics/Renderer.h"
#include "../../include/GameEngine/core/SystemImpl.h"
#include <iostream>
#include <vector>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace FoundryEngine {

class D3D11RendererImpl : public SystemImplBase<D3D11RendererImpl> {
private:
    ID3D11Device* device_ = nullptr;
    ID3D11DeviceContext* context_ = nullptr;
    IDXGISwapChain* swapChain_ = nullptr;
    ID3D11RenderTargetView* renderTargetView_ = nullptr;
    ID3D11DepthStencilView* depthStencilView_ = nullptr;

    RenderSettings settings_;
    int drawCalls_ = 0;
    int triangles_ = 0;
    int vertices_ = 0;

    friend class SystemImplBase<D3D11RendererImpl>;

    bool onInitialize() override {
        std::cout << "Initializing D3D11 Renderer..." << std::endl;

        // Create device and swap chain
        D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
        UINT createDeviceFlags = 0;

#ifdef _DEBUG
        createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        HRESULT hr = D3D11CreateDevice(
            nullptr,                    // Default adapter
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,                    // No software rasterizer
            createDeviceFlags,
            &featureLevel,
            1,
            D3D11_SDK_VERSION,
            &device_,
            nullptr,
            &context_
        );

        if (FAILED(hr)) {
            std::cerr << "Failed to create D3D11 device: " << std::hex << hr << std::endl;
            return false;
        }

        std::cout << "D3D11 Renderer initialized successfully" << std::endl;
        return true;
    }

    void onShutdown() override {
        std::cout << "Shutting down D3D11 Renderer..." << std::endl;

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

        std::cout << "D3D11 Renderer shutdown complete" << std::endl;
    }

    void onUpdate(float deltaTime) override {
        // Handle any per-frame updates
    }

public:
    D3D11RendererImpl() : SystemImplBase("D3D11Renderer") {}

    void beginFrame() {
        if (!isInitialized()) return;

        // Clear render targets
        const float clearColor[] = {0.0f, 0.0f, 0.0f, 1.0f};
        context_->ClearRenderTargetView(renderTargetView_, clearColor);
        context_->ClearDepthStencilView(depthStencilView_, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

        resetStats();
    }

    void endFrame() {
        if (!isInitialized()) return;
        // End frame operations
    }

    void present() {
        if (!isInitialized() || !swapChain_) return;

        HRESULT hr = swapChain_->Present(1, 0); // V-sync enabled
        if (FAILED(hr)) {
            std::cerr << "Failed to present: " << std::hex << hr << std::endl;
        }
    }

    void drawMesh(Mesh* mesh, Material* material, const Matrix4& transform) {
        if (!isInitialized() || !mesh || !material) return;

        // Set up vertex and index buffers
        // Apply material properties
        // Set transformation matrices
        // Draw the mesh

        drawCalls_++;
        // triangles_ += mesh->getTriangleCount();
        // vertices_ += mesh->getVertexCount();
    }

    void drawInstanced(Mesh* mesh, Material* material, const std::vector<Matrix4>& transforms) {
        if (!isInitialized() || !mesh || !material || transforms.empty()) return;

        // Set up instanced rendering
        // Create instance buffer with transforms
        // Draw instanced

        drawCalls_++;
        // triangles_ += mesh->getTriangleCount() * transforms.size();
        // vertices_ += mesh->getVertexCount() * transforms.size();
    }

    void drawSkybox(Texture* skybox) {
        if (!isInitialized()) return;
        // Implement skybox rendering
    }

    void drawUI() {
        if (!isInitialized()) return;
        // Implement UI rendering
    }

    void setCamera(Camera* camera) {
        if (!isInitialized() || !camera) return;
        // Set camera matrices in constant buffers
    }

    void setViewport(int x, int y, int width, int height) {
        if (!isInitialized()) return;

        D3D11_VIEWPORT viewport;
        viewport.TopLeftX = static_cast<float>(x);
        viewport.TopLeftY = static_cast<float>(y);
        viewport.Width = static_cast<float>(width);
        viewport.Height = static_cast<float>(height);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;

        context_->RSSetViewports(1, &viewport);
    }

    void setLights(const std::vector<Light*>& lights) {
        if (!isInitialized()) return;
        // Update light constant buffer
    }

    void setEnvironmentMap(Texture* envMap) {
        if (!isInitialized()) return;
        // Set environment map for reflections
    }

    RenderTarget* createRenderTarget(int width, int height, bool hdr) {
        if (!isInitialized()) return nullptr;

        // Create render target texture and views
        // Return RenderTarget wrapper
        return nullptr; // Placeholder
    }

    void setRenderTarget(RenderTarget* target) {
        if (!isInitialized()) return;

        if (target) {
            // Set custom render target
        } else {
            // Set default render target
            context_->OMSetRenderTargets(1, &renderTargetView_, depthStencilView_);
        }
    }

    void clearRenderTarget(const Vector3& color, float depth) {
        if (!isInitialized()) return;

        const float clearColor[] = {color.x, color.y, color.z, 1.0f};
        context_->ClearRenderTargetView(renderTargetView_, clearColor);
        context_->ClearDepthStencilView(depthStencilView_, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, depth, 0);
    }

    void applyPostProcessing() {
        if (!isInitialized()) return;
        // Implement post-processing pipeline
    }

    void setExposure(float exposure) {
        if (!isInitialized()) return;
        // Update post-processing parameters
    }

    void setGamma(float gamma) {
        if (!isInitialized()) return;
        // Update gamma correction parameters
    }

    void setRenderSettings(const RenderSettings& settings) { settings_ = settings; }
    RenderSettings getRenderSettings() const { return settings_; }

    void drawDebugLine(const Vector3& start, const Vector3& end, const Vector3& color) {
        if (!isInitialized()) return;
        // Implement debug line rendering
    }

    void drawDebugSphere(const Vector3& center, float radius, const Vector3& color) {
        if (!isInitialized()) return;
        // Implement debug sphere rendering
    }

    void drawDebugBox(const Vector3& center, const Vector3& size, const Vector3& color) {
        if (!isInitialized()) return;
        // Implement debug box rendering
    }

    int getDrawCalls() const { return drawCalls_; }
    int getTriangles() const { return triangles_; }
    int getVertices() const { return vertices_; }
    void resetStats() { drawCalls_ = 0; triangles_ = 0; vertices_ = 0; }

    std::string getStatistics() const override {
        char buffer[256];
        snprintf(buffer, sizeof(buffer),
                 "D3D11 Stats - Draw Calls: %d, Triangles: %d, Vertices: %d",
                 drawCalls_, triangles_, vertices_);
        return std::string(buffer);
    }
};

D3D11Renderer::D3D11Renderer() : impl_(std::make_unique<D3D11RendererImpl>()) {}
D3D11Renderer::~D3D11Renderer() = default;

bool D3D11Renderer::initialize() { return impl_->initialize(); }
void D3D11Renderer::shutdown() { impl_->shutdown(); }
void D3D11Renderer::beginFrame() { static_cast<D3D11RendererImpl*>(impl_.get())->beginFrame(); }
void D3D11Renderer::endFrame() { static_cast<D3D11RendererImpl*>(impl_.get())->endFrame(); }
void D3D11Renderer::present() { static_cast<D3D11RendererImpl*>(impl_.get())->present(); }
void D3D11Renderer::drawMesh(Mesh* mesh, Material* material, const Matrix4& transform) { static_cast<D3D11RendererImpl*>(impl_.get())->drawMesh(mesh, material, transform); }
void D3D11Renderer::drawInstanced(Mesh* mesh, Material* material, const std::vector<Matrix4>& transforms) { static_cast<D3D11RendererImpl*>(impl_.get())->drawInstanced(mesh, material, transforms); }
void D3D11Renderer::drawSkybox(Texture* skybox) { static_cast<D3D11RendererImpl*>(impl_.get())->drawSkybox(skybox); }
void D3D11Renderer::drawUI() { static_cast<D3D11RendererImpl*>(impl_.get())->drawUI(); }
void D3D11Renderer::setCamera(Camera* camera) { static_cast<D3D11RendererImpl*>(impl_.get())->setCamera(camera); }
void D3D11Renderer::setViewport(int x, int y, int width, int height) { static_cast<D3D11RendererImpl*>(impl_.get())->setViewport(x, y, width, height); }
void D3D11Renderer::setLights(const std::vector<Light*>& lights) { static_cast<D3D11RendererImpl*>(impl_.get())->setLights(lights); }
void D3D11Renderer::setEnvironmentMap(Texture* envMap) { static_cast<D3D11RendererImpl*>(impl_.get())->setEnvironmentMap(envMap); }
RenderTarget* D3D11Renderer::createRenderTarget(int width, int height, bool hdr) { return static_cast<D3D11RendererImpl*>(impl_.get())->createRenderTarget(width, height, hdr); }
void D3D11Renderer::setRenderTarget(RenderTarget* target) { static_cast<D3D11RendererImpl*>(impl_.get())->setRenderTarget(target); }
void D3D11Renderer::clearRenderTarget(const Vector3& color, float depth) { static_cast<D3D11RendererImpl*>(impl_.get())->clearRenderTarget(color, depth); }
void D3D11Renderer::applyPostProcessing() { static_cast<D3D11RendererImpl*>(impl_.get())->applyPostProcessing(); }
void D3D11Renderer::setExposure(float exposure) { static_cast<D3D11RendererImpl*>(impl_.get())->setExposure(exposure); }
void D3D11Renderer::setGamma(float gamma) { static_cast<D3D11RendererImpl*>(impl_.get())->setGamma(gamma); }
void D3D11Renderer::setRenderSettings(const RenderSettings& settings) { static_cast<D3D11RendererImpl*>(impl_.get())->setRenderSettings(settings); }
RenderSettings D3D11Renderer::getRenderSettings() const { return static_cast<D3D11RendererImpl*>(impl_.get())->getRenderSettings(); }
void D3D11Renderer::drawDebugLine(const Vector3& start, const Vector3& end, const Vector3& color) { static_cast<D3D11RendererImpl*>(impl_.get())->drawDebugLine(start, end, color); }
void D3D11Renderer::drawDebugSphere(const Vector3& center, float radius, const Vector3& color) { static_cast<D3D11RendererImpl*>(impl_.get())->drawDebugSphere(center, radius, color); }
void D3D11Renderer::drawDebugBox(const Vector3& center, const Vector3& size, const Vector3& color) { static_cast<D3D11RendererImpl*>(impl_.get())->drawDebugBox(center, size, color); }
int D3D11Renderer::getDrawCalls() const { return static_cast<D3D11RendererImpl*>(impl_.get())->getDrawCalls(); }
int D3D11Renderer::getTriangles() const { return static_cast<D3D11RendererImpl*>(impl_.get())->getTriangles(); }
int D3D11Renderer::getVertices() const { return static_cast<D3D11RendererImpl*>(impl_.get())->getVertices(); }
void D3D11Renderer::resetStats() { static_cast<D3D11RendererImpl*>(impl_.get())->resetStats(); }

} // namespace FoundryEngine
