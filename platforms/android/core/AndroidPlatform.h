/**
 * Android Platform Header
 * Native Android platform interface definitions
 */

#ifndef ANDROID_PLATFORM_H
#define ANDROID_PLATFORM_H

#include "../../core/Platform.h"
#include "../../core/System.h"
#include "../../math/Vector3.h"
#include "../../math/Quaternion.h"
#include "../../math/Matrix4.h"

#include <android/native_window.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <jni.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>

namespace FoundryEngine {

// Platform interface implementations
class AndroidCanvas : public PlatformCanvas {
private:
    int width_, height_;

public:
    AndroidCanvas(int width, int height) : width_(width), height_(height) {}

    int getWidth() const override { return width_; }
    int getHeight() const override { return height_; }
    void setWidth(int width) override { width_ = width; }
    void setHeight(int height) override { height_ = height; }

    PlatformGraphicsContext* getContext(const std::string& contextType) override;
    void addEventListener(const std::string& event, std::function<void(const InputEvent&)> listener) override;
    void removeEventListener(const std::string& event, std::function<void(const InputEvent&)> listener) override;
};

class AndroidGraphics : public PlatformGraphics {
public:
    PlatformCapabilities getCapabilities() override;
    std::unique_ptr<PlatformGraphicsContext> createContext() override;
};

class AndroidAudio : public PlatformAudio {
public:
    std::unique_ptr<PlatformAudioContext> createContext() override;
    void resume() override;
    void suspend() override;
};

class AndroidInput : public PlatformInputManager {
private:
    std::vector<GamepadState> gamepadStates_;
    JNIEnv* env_;
    jobject inputManager_;

public:
    AndroidInput(JNIEnv* env, jobject inputManager);
    ~AndroidInput();

    std::unordered_map<int, bool> getKeyboardState() override;
    MouseState getMouseState() override;
    std::vector<TouchPoint> getTouchState() override;
    GamepadState getGamepadState(int index) override;
    std::vector<GamepadState> getConnectedGamepads() override;
    int getGamepadCount() override;
    bool isGamepadConnected(int index) override;
    std::string getGamepadName(int index) override;
    bool setGamepadVibration(int index, float leftMotor, float rightMotor, float duration) override;

    void addEventListener(const std::string& type, std::function<void(const InputEvent&)> listener) override;
    void removeEventListener(const std::string& type, std::function<void(const InputEvent&)> listener) override;

    // Android-specific methods
    void updateGamepadStates();
    void onGamepadConnected(int deviceId);
    void onGamepadDisconnected(int deviceId);
};

class AndroidFileSystem : public PlatformFileSystem {
public:
    std::vector<uint8_t> readFile(const std::string& path) override;
    void writeFile(const std::string& path, const std::vector<uint8_t>& data) override;
    void deleteFile(const std::string& path) override;
    std::vector<std::string> listFiles(const std::string& directory) override;
    void createDirectory(const std::string& path) override;
    bool exists(const std::string& path) override;
};

class AndroidNetworking : public PlatformNetworking {
public:
    std::unique_ptr<PlatformWebSocket> connect(const std::string& url) override;
    std::vector<uint8_t> httpGet(const std::string& url) override;
    std::vector<uint8_t> httpPost(const std::string& url, const std::vector<uint8_t>& data) override;
};

class AndroidTimer : public PlatformTimer {
public:
    double now() override;
    int setTimeout(std::function<void()> callback, int delay) override;
    void clearTimeout(int id) override;
    int setInterval(std::function<void()> callback, int delay) override;
    void clearInterval(int id) override;
    int requestAnimationFrame(std::function<void(double)> callback) override;
    void cancelAnimationFrame(int id) override;
};

class AndroidRandom : public PlatformRandom {
public:
    double random() override;
    int randomInt(int min, int max) override;
    double randomFloat(double min, double max) override;
    void seed(unsigned int seed) override;
};

// Android-specific classes
class AndroidGLContext : public PlatformGraphicsContext {
private:
    EGLDisplay display_;
    EGLSurface surface_;
    EGLContext context_;

public:
    AndroidGLContext();
    ~AndroidGLContext();

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

    // WebGL constants
    static const unsigned int TRIANGLES = GL_TRIANGLES;
    static const unsigned int TRIANGLE_STRIP = GL_TRIANGLE_STRIP;
    static const unsigned int TRIANGLE_FAN = GL_TRIANGLE_FAN;
    static const unsigned int LINES = GL_LINES;
    static const unsigned int LINE_STRIP = GL_LINE_STRIP;
    static const unsigned int LINE_LOOP = GL_LINE_LOOP;
    static const unsigned int POINTS = GL_POINTS;

    static const unsigned int DEPTH_BUFFER_BIT = GL_DEPTH_BUFFER_BIT;
    static const unsigned int STENCIL_BUFFER_BIT = GL_STENCIL_BUFFER_BIT;
    static const unsigned int COLOR_BUFFER_BIT = GL_COLOR_BUFFER_BIT;

    static const unsigned int DEPTH_TEST = GL_DEPTH_TEST;
    static const unsigned int STENCIL_TEST = GL_STENCIL_TEST;
    static const unsigned int BLEND = GL_BLEND;
    static const unsigned int CULL_FACE = GL_CULL_FACE;

    static const unsigned int NEVER = GL_NEVER;
    static const unsigned int LESS = GL_LESS;
    static const unsigned int EQUAL = GL_EQUAL;
    static const unsigned int LEQUAL = GL_LEQUAL;
    static const unsigned int GREATER = GL_GREATER;
    static const unsigned int NOTEQUAL = GL_NOTEQUAL;
    static const unsigned int GEQUAL = GL_GEQUAL;
    static const unsigned int ALWAYS = GL_ALWAYS;

    static const unsigned int ZERO = GL_ZERO;
    static const unsigned int ONE = GL_ONE;
    static const unsigned int SRC_COLOR = GL_SRC_COLOR;
    static const unsigned int ONE_MINUS_SRC_COLOR = GL_ONE_MINUS_SRC_COLOR;
    static const unsigned int SRC_ALPHA = GL_SRC_ALPHA;
    static const unsigned int ONE_MINUS_SRC_ALPHA = GL_ONE_MINUS_SRC_ALPHA;
    static const unsigned int DST_ALPHA = GL_DST_ALPHA;
    static const unsigned int ONE_MINUS_DST_ALPHA = GL_ONE_MINUS_DST_ALPHA;
    static const unsigned int DST_COLOR = GL_DST_COLOR;
    static const unsigned int ONE_MINUS_DST_COLOR = GL_ONE_MINUS_DST_COLOR;
    static const unsigned int SRC_ALPHA_SATURATE = GL_SRC_ALPHA_SATURATE;

    static const unsigned int FRONT = GL_FRONT;
    static const unsigned int BACK = GL_BACK;
    static const unsigned int FRONT_AND_BACK = GL_FRONT_AND_BACK;

    static const unsigned int VERTEX_SHADER = GL_VERTEX_SHADER;
    static const unsigned int FRAGMENT_SHADER = GL_FRAGMENT_SHADER;

    static const unsigned int ARRAY_BUFFER = GL_ARRAY_BUFFER;
    static const unsigned int ELEMENT_ARRAY_BUFFER = GL_ELEMENT_ARRAY_BUFFER;
    static const unsigned int STATIC_DRAW = GL_STATIC_DRAW;
    static const unsigned int DYNAMIC_DRAW = GL_DYNAMIC_DRAW;
    static const unsigned int STREAM_DRAW = GL_STREAM_DRAW;

    static const unsigned int FLOAT = GL_FLOAT;
    static const unsigned int UNSIGNED_BYTE = GL_UNSIGNED_BYTE;
    static const unsigned int UNSIGNED_SHORT = GL_UNSIGNED_SHORT;
    static const unsigned int UNSIGNED_INT = GL_UNSIGNED_INT;

    static const unsigned int COMPILE_STATUS = GL_COMPILE_STATUS;
    static const unsigned int LINK_STATUS = GL_LINK_STATUS;
};

} // namespace FoundryEngine

#endif // ANDROID_PLATFORM_H
