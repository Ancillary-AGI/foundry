/**
 * Windows Platform Header
 * Native Windows platform interface definitions
 */

#ifndef WINDOWS_PLATFORM_H
#define WINDOWS_PLATFORM_H

#include "../../core/Platform.h"
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <xaudio2.h>
#include <xinput.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>

// Platform interface implementations
class WindowsCanvas : public PlatformCanvas {
private:
    int width_, height_;
    HWND hwnd_;

public:
    WindowsCanvas(int width, int height, HWND hwnd) : width_(width), height_(height), hwnd_(hwnd) {}

    int getWidth() const override { return width_; }
    int getHeight() const override { return height_; }
    void setWidth(int width) override { width_ = width; }
    void setHeight(int height) override { height_ = height; }

    PlatformGraphicsContext* getContext(const std::string& contextType) override;
    void addEventListener(const std::string& event, std::function<void(const InputEvent&)> listener) override {}
    void removeEventListener(const std::string& event, std::function<void(const InputEvent&)> listener) override {}
};

class WindowsGraphics : public PlatformGraphics {
private:
    ID3D11Device* device_ = nullptr;
    ID3D11DeviceContext* context_ = nullptr;
    IDXGISwapChain* swapChain_ = nullptr;
    ID3D11RenderTargetView* renderTargetView_ = nullptr;
    ID3D11DepthStencilView* depthStencilView_ = nullptr;

public:
    WindowsGraphics(HWND hwnd, int width, int height);
    ~WindowsGraphics();

    bool initialize();
    void shutdown();
    void present();
    void resize(int width, int height);

    PlatformCapabilities getCapabilities() override;
    std::unique_ptr<PlatformGraphicsContext> createContext() override;

    // DirectX-specific access
    ID3D11Device* getDevice() { return device_; }
    ID3D11DeviceContext* getContext() { return context_; }
    IDXGISwapChain* getSwapChain() { return swapChain_; }
    ID3D11RenderTargetView* getRenderTargetView() { return renderTargetView_; }
    ID3D11DepthStencilView* getDepthStencilView() { return depthStencilView_; }
};

class WindowsAudio : public PlatformAudio {
private:
    IXAudio2* xaudio2_ = nullptr;
    IXAudio2MasteringVoice* masteringVoice_ = nullptr;

public:
    WindowsAudio();
    ~WindowsAudio();

    bool initialize();
    void shutdown();

    std::unique_ptr<PlatformAudioContext> createContext() override;
    void resume() override;
    void suspend() override;
};

class WindowsInput : public PlatformInputManager {
private:
    std::unordered_map<int, bool> keyStates_;
    std::unordered_map<int, bool> mouseButtons_;
    POINT mousePosition_;
    std::vector<XINPUT_STATE> gamepadStates_;
    std::vector<std::function<void(const InputEvent&)>> listeners_;
    HWND hwnd_;

public:
    WindowsInput(HWND hwnd);
    ~WindowsInput();

    void update();

    std::unordered_map<int, bool> getKeyboardState() override { return keyStates_; }
    MouseState getMouseState() override;
    std::vector<TouchPoint> getTouchState() override { return {}; }
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

class WindowsFileSystem : public PlatformFileSystem {
private:
    std::string appDataPath_;
    std::string documentsPath_;

public:
    WindowsFileSystem();

    std::vector<uint8_t> readFile(const std::string& path) override;
    void writeFile(const std::string& path, const std::vector<uint8_t>& data) override;
    void deleteFile(const std::string& path) override;
    std::vector<std::string> listFiles(const std::string& directory) override;
    void createDirectory(const std::string& path) override;
    bool exists(const std::string& path) override;

private:
    std::string getAppDataPath();
    std::string getDocumentsPath();
};

class WindowsNetworking : public PlatformNetworking {
public:
    std::unique_ptr<PlatformWebSocket> connect(const std::string& url) override;
    std::vector<uint8_t> httpGet(const std::string& url) override { return {}; }
    std::vector<uint8_t> httpPost(const std::string& url, const std::vector<uint8_t>& data) override { return {}; }
};

class WindowsTimer : public PlatformTimer {
private:
    LARGE_INTEGER frequency_;
    LARGE_INTEGER startTime_;

public:
    WindowsTimer();

    double now() override;
    int setTimeout(std::function<void()> callback, int delay) override;
    void clearTimeout(int id) override;
    int setInterval(std::function<void()> callback, int delay) override;
    void clearInterval(int id) override;
    int requestAnimationFrame(std::function<void(double)> callback) override;
    void cancelAnimationFrame(int id) override;
};

class WindowsRandom : public PlatformRandom {
public:
    double random() override;
    int randomInt(int min, int max) override;
    double randomFloat(double min, double max) override;
    void seed(unsigned int seed) override;
};

// Windows-specific classes
class WindowsD3DContext : public PlatformGraphicsContext {
private:
    WindowsGraphics* graphics_;
    std::unordered_map<std::string, ID3D11Buffer*> vertexBuffers_;
    std::unordered_map<std::string, ID3D11Buffer*> indexBuffers_;
    std::unordered_map<std::string, ID3D11VertexShader*> vertexShaders_;
    std::unordered_map<std::string, ID3D11PixelShader*> pixelShaders_;
    std::unordered_map<std::string, ID3D11InputLayout*> inputLayouts_;
    float clearColor_[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

public:
    WindowsD3DContext(WindowsGraphics* graphics) : graphics_(graphics) {}

    // WebGL-style interface for compatibility
    void viewport(int x, int y, int width, int height) override;
    void clear(unsigned int mask) override;
    void clearColor(float r, float g, float b, float a) override;
    void enable(unsigned int cap) override;
    void disable(unsigned int cap) override;
    void cullFace(unsigned int mode) override;
    void depthFunc(unsigned int func) override;
    void blendFunc(unsigned int sfactor, unsigned int dfactor) override;

    unsigned int createBuffer() override;
    void bindBuffer(unsigned int target, unsigned int buffer) override;
    void bufferData(unsigned int target, const void* data, size_t size, unsigned int usage) override;
    void deleteBuffer(unsigned int buffer) override;

    unsigned int createShader(unsigned int type) override;
    void shaderSource(unsigned int shader, const std::string& source) override;
    void compileShader(unsigned int shader) override;
    int getShaderParameter(unsigned int shader, unsigned int pname) override;
    std::string getShaderInfoLog(unsigned int shader) override;
    void deleteShader(unsigned int shader) override;

    unsigned int createProgram() override;
    void attachShader(unsigned int program, unsigned int shader) override;
    void linkProgram(unsigned int program) override;
    int getProgramParameter(unsigned int program, unsigned int pname) override;
    std::string getProgramInfoLog(unsigned int program) override;
    void useProgram(unsigned int program) override;
    void deleteProgram(unsigned int program) override;

    int getAttribLocation(unsigned int program, const std::string& name) override;
    int getUniformLocation(unsigned int program, const std::string& name) override;

    void vertexAttribPointer(unsigned int index, int size, unsigned int type, bool normalized, int stride, unsigned int offset) override;
    void enableVertexAttribArray(unsigned int index) override;
    void disableVertexAttribArray(unsigned int index) override;

    void uniform1f(int location, float x) override;
    void uniform2f(int location, float x, float y) override;
    void uniform3f(int location, float x, float y, float z) override;
    void uniform4f(int location, float x, float y, float z, float w) override;
    void uniform1i(int location, int x) override;
    void uniform2i(int location, int x, int y) override;
    void uniform3i(int location, int x, int y, int z) override;
    void uniform4i(int location, int x, int y, int z, int w) override;
    void uniform1fv(int location, const Float32Array& v) override;
    void uniform2fv(int location, const Float32Array& v) override;
    void uniform3fv(int location, const Float32Array& v) override;
    void uniform4fv(int location, const Float32Array& v) override;
    void uniformMatrix2fv(int location, bool transpose, const Float32Array& value) override;
    void uniformMatrix3fv(int location, bool transpose, const Float32Array& value) override;
    void uniformMatrix4fv(int location, bool transpose, const Float32Array& value) override;

    void drawArrays(unsigned int mode, int first, int count) override;
    void drawElements(unsigned int mode, int count, unsigned int type, unsigned int offset) override;

    // DirectX constants (mapped to WebGL equivalents)
    static const unsigned int TRIANGLES = 4; // D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
    static const unsigned int TRIANGLE_STRIP = 5; // D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP
    static const unsigned int TRIANGLE_FAN = 6; // Not directly supported in D3D11
    static const unsigned int LINES = 2; // D3D11_PRIMITIVE_TOPOLOGY_LINELIST
    static const unsigned int LINE_STRIP = 3; // D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP
    static const unsigned int LINE_LOOP = 2; // Not directly supported, use LINE_STRIP
    static const unsigned int POINTS = 1; // D3D11_PRIMITIVE_TOPOLOGY_POINTLIST

    static const unsigned int DEPTH_BUFFER_BIT = 0x100;
    static const unsigned int STENCIL_BUFFER_BIT = 0x400;
    static const unsigned int COLOR_BUFFER_BIT = 0x4000;

    static const unsigned int DEPTH_TEST = 0x0B71;
    static const unsigned int STENCIL_TEST = 0x0B90;
    static const unsigned int BLEND = 0x0BE2;
    static const unsigned int CULL_FACE = 0x0B44;

    static const unsigned int NEVER = 0x0200;
    static const unsigned int LESS = 0x0201;
    static const unsigned int EQUAL = 0x0202;
    static const unsigned int LEQUAL = 0x0203;
    static const unsigned int GREATER = 0x0204;
    static const unsigned int NOTEQUAL = 0x0205;
    static const unsigned int GEQUAL = 0x0206;
    static const unsigned int ALWAYS = 0x0207;

    static const unsigned int ZERO = 0;
    static const unsigned int ONE = 1;
    static const unsigned int SRC_COLOR = 0x0300;
    static const unsigned int ONE_MINUS_SRC_COLOR = 0x0301;
    static const unsigned int SRC_ALPHA = 0x0302;
    static const unsigned int ONE_MINUS_SRC_ALPHA = 0x0303;
    static const unsigned int DST_ALPHA = 0x0304;
    static const unsigned int ONE_MINUS_DST_ALPHA = 0x0305;
    static const unsigned int DST_COLOR = 0x0306;
    static const unsigned int ONE_MINUS_DST_COLOR = 0x0307;
    static const unsigned int SRC_ALPHA_SATURATE = 0x0308;

    static const unsigned int FRONT = 0x0404;
    static const unsigned int BACK = 0x0405;
    static const unsigned int FRONT_AND_BACK = 0x0408;

    static const unsigned int VERTEX_SHADER = 0x8B31;
    static const unsigned int FRAGMENT_SHADER = 0x8B30;

    static const unsigned int ARRAY_BUFFER = 0x8892;
    static const unsigned int ELEMENT_ARRAY_BUFFER = 0x8893;
    static const unsigned int STATIC_DRAW = 0x88E4;
    static const unsigned int DYNAMIC_DRAW = 0x88E8;
    static const unsigned int STREAM_DRAW = 0x88E0;

    static const unsigned int FLOAT = 0x1406;
    static const unsigned int UNSIGNED_BYTE = 0x1401;
    static const unsigned int UNSIGNED_SHORT = 0x1403;
    static const unsigned int UNSIGNED_INT = 0x1405;

    static const unsigned int COMPILE_STATUS = 0x8B81;
    static const unsigned int LINK_STATUS = 0x8B82;
};

// Windows Application class
class WindowsApplication {
private:
    WindowsPlatform* platform_;
    std::unique_ptr<GameEngine> engine_;
    bool running_ = false;

public:
    WindowsApplication(HINSTANCE hInstance);
    ~WindowsApplication();

    bool initialize(int width, int height, const std::string& title);
    void run();
    void shutdown();

private:
    void update(float deltaTime);
    void render();
    LRESULT handleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

#endif // WINDOWS_PLATFORM_H
