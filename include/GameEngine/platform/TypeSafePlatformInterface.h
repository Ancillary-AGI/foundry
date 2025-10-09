#pragma once

#include <string>
#include <memory>
#include <functional>
#include <unordered_map>
#include <mutex>
#include "../core/MemoryPool.h"

namespace FoundryEngine {

/**
 * @brief Platform types for type-safe handle management
 */
enum class PlatformHandleType {
    WINDOW,
    DISPLAY,
    CONTEXT,
    DEVICE,
    SURFACE,
    INSTANCE,
    SOCKET,
    FILE,
    THREAD,
    MUTEX,
    SEMAPHORE,
    UNKNOWN
};

/**
 * @brief Type-safe platform handle wrapper
 */
template<PlatformHandleType HandleType>
class PlatformHandle {
public:
    /**
     * @brief Construct invalid handle
     */
    PlatformHandle() = default;

    /**
     * @brief Construct handle with native pointer
     * @param nativePtr Native platform handle
     */
    explicit PlatformHandle(void* nativePtr) : nativePtr_(nativePtr) {}

    /**
     * @brief Check if handle is valid
     * @return true if handle contains valid native pointer
     */
    bool isValid() const { return nativePtr_ != nullptr; }

    /**
     * @brief Get native handle pointer
     * @return Native platform handle
     */
    void* getNative() const { return nativePtr_; }

    /**
     * @brief Get handle type
     * @return Platform handle type
     */
    PlatformHandleType getType() const { return HandleType; }

    /**
     * @brief Reset handle to invalid state
     */
    void reset() { nativePtr_ = nullptr; }

    /**
     * @brief Compare handles for equality
     */
    bool operator==(const PlatformHandle& other) const {
        return nativePtr_ == other.nativePtr_;
    }

    /**
     * @brief Compare handles for inequality
     */
    bool operator!=(const PlatformHandle& other) const {
        return nativePtr_ != other.nativePtr_;
    }

    /**
     * @brief Check if handle is valid (bool conversion)
     */
    explicit operator bool() const { return isValid(); }

private:
    void* nativePtr_ = nullptr;
};

/**
 * @brief Type-safe window handle
 */
using WindowHandle = PlatformHandle<PlatformHandleType::WINDOW>;

/**
 * @brief Type-safe display handle
 */
using DisplayHandle = PlatformHandle<PlatformHandleType::DISPLAY>;

/**
 * @brief Type-safe graphics context handle
 */
using GraphicsContextHandle = PlatformHandle<PlatformHandleType::CONTEXT>;

/**
 * @brief Type-safe device handle
 */
using DeviceHandle = PlatformHandle<PlatformHandleType::DEVICE>;

/**
 * @brief Type-safe surface handle
 */
using SurfaceHandle = PlatformHandle<PlatformHandleType::SURFACE>;

/**
 * @brief Type-safe instance handle
 */
using InstanceHandle = PlatformHandle<PlatformHandleType::INSTANCE>;

/**
 * @brief Type-safe socket handle
 */
using SocketHandle = PlatformHandle<PlatformHandleType::SOCKET>;

/**
 * @brief Type-safe file handle
 */
using FileHandle = PlatformHandle<PlatformHandleType::FILE>;

/**
 * @brief Type-safe thread handle
 */
using ThreadHandle = PlatformHandle<PlatformHandleType::THREAD>;

/**
 * @brief Type-safe mutex handle
 */
using MutexHandle = PlatformHandle<PlatformHandleType::MUTEX>;

/**
 * @brief Type-safe semaphore handle
 */
using SemaphoreHandle = PlatformHandle<PlatformHandleType::SEMAPHORE>;

/**
 * @brief Platform capabilities with type safety
 */
struct TypeSafePlatformCapabilities {
    std::string platformName;
    std::string platformVersion;
    bool supportsOpenGL = false;
    bool supportsVulkan = false;
    bool supportsD3D11 = false;
    bool supportsD3D12 = false;
    bool supportsMetal = false;
    bool supportsWebGL = false;

    int maxTextureSize = 4096;
    int maxRenderTargets = 8;
    int maxUniformBufferSize = 65536;
    int maxVertexAttributes = 16;

    bool supportsComputeShaders = false;
    bool supportsGeometryShaders = false;
    bool supportsTessellationShaders = false;
    bool supportsMultiDrawIndirect = false;

    bool supportsMultithreading = true;
    int maxThreadCount = std::thread::hardware_concurrency();

    size_t systemMemoryMB = 1024;
    size_t availableMemoryMB = 512;

    bool supportsHDR = false;
    bool supportsSRGB = false;
    bool supportsASTC = false;
    bool supportsBC = false;

    std::unordered_map<std::string, std::string> extensions;
    std::unordered_map<std::string, bool> features;
};

/**
 * @brief Type-safe platform interface
 */
class TypeSafePlatformInterface {
public:
    virtual ~TypeSafePlatformInterface() = default;

    /**
     * @brief Initialize platform interface
     * @param capabilities Platform capabilities
     * @return true if initialization succeeded
     */
    virtual bool initialize(const TypeSafePlatformCapabilities& capabilities) = 0;

    /**
     * @brief Shutdown platform interface
     */
    virtual void shutdown() = 0;

    /**
     * @brief Process platform events
     */
    virtual void processEvents() = 0;

    /**
     * @brief Check if platform interface is initialized
     * @return true if platform is ready for use
     */
    virtual bool isInitialized() const = 0;

    // Window Management (Type-Safe)
    virtual WindowHandle createWindow(const std::string& title, int width, int height, bool fullscreen = false) = 0;
    virtual void destroyWindow(WindowHandle handle) = 0;
    virtual bool isWindowValid(WindowHandle handle) const = 0;
    virtual void showWindow(WindowHandle handle) = 0;
    virtual void hideWindow(WindowHandle handle) = 0;
    virtual void setWindowTitle(WindowHandle handle, const std::string& title) = 0;
    virtual void setWindowSize(WindowHandle handle, int width, int height) = 0;
    virtual void getWindowSize(WindowHandle handle, int& width, int& height) const = 0;
    virtual void setWindowPosition(WindowHandle handle, int x, int y) = 0;
    virtual void getWindowPosition(WindowHandle handle, int& x, int& y) const = 0;

    // Graphics Context Management (Type-Safe)
    virtual GraphicsContextHandle createGraphicsContext(WindowHandle window) = 0;
    virtual void destroyGraphicsContext(GraphicsContextHandle handle) = 0;
    virtual bool isGraphicsContextValid(GraphicsContextHandle handle) const = 0;
    virtual bool makeCurrent(GraphicsContextHandle handle) = 0;
    virtual void swapBuffers(GraphicsContextHandle handle) = 0;
    virtual void setSwapInterval(GraphicsContextHandle handle, int interval) = 0;

    // Input Management (Type-Safe)
    virtual bool isKeyPressed(WindowHandle handle, int keyCode) const = 0;
    virtual bool isMouseButtonPressed(WindowHandle handle, int button) const = 0;
    virtual void getMousePosition(WindowHandle handle, double& x, double& y) const = 0;
    virtual void setMousePosition(WindowHandle handle, double x, double y) = 0;
    virtual bool getJoystickState(int joystickId, std::vector<float>& axes, std::vector<bool>& buttons) const = 0;

    // Network Management (Type-Safe)
    virtual SocketHandle createSocket(int domain, int type, int protocol) = 0;
    virtual void destroySocket(SocketHandle handle) = 0;
    virtual bool isSocketValid(SocketHandle handle) const = 0;
    virtual bool bindSocket(SocketHandle handle, const std::string& address, int port) = 0;
    virtual bool listenSocket(SocketHandle handle, int backlog) = 0;
    virtual SocketHandle acceptSocket(SocketHandle handle) = 0;
    virtual bool connectSocket(SocketHandle handle, const std::string& address, int port) = 0;
    virtual int sendSocket(SocketHandle handle, const void* data, size_t size, int flags = 0) = 0;
    virtual int receiveSocket(SocketHandle handle, void* buffer, size_t size, int flags = 0) = 0;

    // Thread Management (Type-Safe)
    virtual ThreadHandle createThread(std::function<void()> threadFunc) = 0;
    virtual void destroyThread(ThreadHandle handle) = 0;
    virtual bool isThreadValid(ThreadHandle handle) const = 0;
    virtual void joinThread(ThreadHandle handle) = 0;
    virtual void detachThread(ThreadHandle handle) = 0;
    virtual ThreadHandle getCurrentThread() = 0;

    // Synchronization Management (Type-Safe)
    virtual MutexHandle createMutex() = 0;
    virtual void destroyMutex(MutexHandle handle) = 0;
    virtual bool isMutexValid(MutexHandle handle) const = 0;
    virtual void lockMutex(MutexHandle handle) = 0;
    virtual bool tryLockMutex(MutexHandle handle) = 0;
    virtual void unlockMutex(MutexHandle handle) = 0;

    virtual SemaphoreHandle createSemaphore(int initialCount = 0, int maxCount = 1) = 0;
    virtual void destroySemaphore(SemaphoreHandle handle) = 0;
    virtual bool isSemaphoreValid(SemaphoreHandle handle) const = 0;
    virtual void waitSemaphore(SemaphoreHandle handle) = 0;
    virtual bool tryWaitSemaphore(SemaphoreHandle handle) = 0;
    virtual void postSemaphore(SemaphoreHandle handle, int count = 1) = 0;

    // File System Management (Type-Safe)
    virtual FileHandle openFile(const std::string& path, const std::string& mode) = 0;
    virtual void closeFile(FileHandle handle) = 0;
    virtual bool isFileValid(FileHandle handle) const = 0;
    virtual size_t readFile(FileHandle handle, void* buffer, size_t size) = 0;
    virtual size_t writeFile(FileHandle handle, const void* data, size_t size) = 0;
    virtual bool seekFile(FileHandle handle, size_t offset, int whence) = 0;
    virtual size_t tellFile(FileHandle handle) = 0;
    virtual bool flushFile(FileHandle handle) = 0;
    virtual bool isFileEOF(FileHandle handle) = 0;

    // Time Management
    virtual double getTime() const = 0;
    virtual uint64_t getFrequency() const = 0;
    virtual void sleep(uint32_t milliseconds) = 0;

    // Memory Management Integration
    virtual void setMemoryPool(MemoryPool* pool) = 0;
    virtual MemoryPool* getMemoryPool() const = 0;

    // Platform Information
    virtual const TypeSafePlatformCapabilities& getCapabilities() const = 0;
    virtual std::string getPlatformName() const = 0;
    virtual std::string getPlatformVersion() const = 0;

    // Error Handling
    virtual bool hasError() const = 0;
    virtual std::string getLastError() const = 0;
    virtual void clearError() = 0;

    // Event Callbacks (Type-Safe)
    using WindowEventCallback = std::function<void(WindowHandle, int, int, int)>;
    using KeyboardEventCallback = std::function<void(WindowHandle, int, int, int, int)>;
    using MouseEventCallback = std::function<void(WindowHandle, int, int, int, double, double)>;
    using NetworkEventCallback = std::function<void(SocketHandle, const void*, size_t)>;

    virtual void setWindowEventCallback(WindowEventCallback callback) = 0;
    virtual void setKeyboardEventCallback(KeyboardEventCallback callback) = 0;
    virtual void setMouseEventCallback(MouseEventCallback callback) = 0;
    virtual void setNetworkEventCallback(NetworkEventCallback callback) = 0;

protected:
    /**
     * @brief Validate handle with type checking
     * @tparam HandleType Expected handle type
     * @param handle Handle to validate
     * @return true if handle is valid and of correct type
     */
    template<PlatformHandleType HandleType>
    bool validateHandle(const PlatformHandle<HandleType>& handle) const {
        return handle.isValid();
    }

    /**
     * @brief Convert type-safe handle to native pointer
     * @tparam HandleType Handle type
     * @param handle Type-safe handle
     * @return Native pointer
     */
    template<PlatformHandleType HandleType>
    void* getNativeHandle(const PlatformHandle<HandleType>& handle) const {
        return handle.getNative();
    }
};

/**
 * @brief Platform handle registry for tracking and validation
 */
class PlatformHandleRegistry {
public:
    /**
     * @brief Register a platform handle
     * @tparam HandleType Handle type
     * @param handle Handle to register
     * @param name Optional name for debugging
     * @return Registration ID
     */
    template<PlatformHandleType HandleType>
    uint64_t registerHandle(const PlatformHandle<HandleType>& handle, const std::string& name = "") {
        std::lock_guard<std::mutex> lock(mutex_);

        HandleInfo info;
        info.type = HandleType;
        info.nativePtr = handle.getNative();
        info.name = name;
        info.registrationTime = std::chrono::steady_clock::now();

        uint64_t id = nextId_++;
        registry_[id] = info;

        return id;
    }

    /**
     * @brief Unregister a platform handle
     * @param id Registration ID
     */
    void unregisterHandle(uint64_t id) {
        std::lock_guard<std::mutex> lock(mutex_);
        registry_.erase(id);
    }

    /**
     * @brief Check if handle is registered
     * @param nativePtr Native pointer to check
     * @return true if handle is registered
     */
    bool isHandleRegistered(void* nativePtr) const {
        std::lock_guard<std::mutex> lock(mutex_);

        for (const auto& pair : registry_) {
            if (pair.second.nativePtr == nativePtr) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Get handle information
     * @param id Registration ID
     * @return Handle information or nullptr if not found
     */
    const HandleInfo* getHandleInfo(uint64_t id) const {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = registry_.find(id);
        return it != registry_.end() ? &it->second : nullptr;
    }

    /**
     * @brief Get all registered handles of specific type
     * @tparam HandleType Handle type to search for
     * @return Vector of matching handles
     */
    template<PlatformHandleType HandleType>
    std::vector<void*> getHandlesByType() const {
        std::lock_guard<std::mutex> lock(mutex_);

        std::vector<void*> handles;
        for (const auto& pair : registry_) {
            if (pair.second.type == HandleType) {
                handles.push_back(pair.second.nativePtr);
            }
        }
        return handles;
    }

    /**
     * @brief Clear all registered handles
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        registry_.clear();
    }

    /**
     * @brief Get registry statistics
     * @return Number of registered handles
     */
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return registry_.size();
    }

private:
    struct HandleInfo {
        PlatformHandleType type;
        void* nativePtr;
        std::string name;
        std::chrono::steady_clock::time_point registrationTime;
    };

    std::unordered_map<uint64_t, HandleInfo> registry_;
    std::atomic<uint64_t> nextId_{1};
    mutable std::mutex mutex_;
};

/**
 * @brief Global platform handle registry instance
 */
extern PlatformHandleRegistry g_platformHandleRegistry;

} // namespace FoundryEngine
