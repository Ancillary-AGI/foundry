#ifndef FOUNDRYENGINE_VULKAN_PLATFORM_H
#define FOUNDRYENGINE_VULKAN_PLATFORM_H

#include "../../core/Platform.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_android.h>
#include <android/native_window.h>
#include <vector>
#include <memory>
#include <string>

namespace FoundryEngine {

// Vulkan Physical Device Selection
struct VulkanPhysicalDevice {
    VkPhysicalDevice device;
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties memoryProperties;
    std::vector<VkQueueFamilyProperties> queueFamilyProperties;
    std::vector<VkExtensionProperties> extensions;
    uint32_t graphicsQueueFamilyIndex;
    uint32_t presentQueueFamilyIndex;
    uint32_t computeQueueFamilyIndex;
    bool isDiscreteGPU;
    bool supportsPresentation;
    bool supportsCompute;
};

// Vulkan Swapchain Support
struct VulkanSwapchainSupport {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

// Vulkan Graphics Pipeline
class VulkanGraphicsPipeline {
public:
    VkPipeline pipeline;
    VkPipelineLayout layout;
    VkRenderPass renderPass;
    std::vector<VkFramebuffer> framebuffers;
    std::vector<VkCommandBuffer> commandBuffers;
    VkCommandPool commandPool;

    VulkanGraphicsPipeline() : pipeline(VK_NULL_HANDLE), layout(VK_NULL_HANDLE),
                              renderPass(VK_NULL_HANDLE), commandPool(VK_NULL_HANDLE) {}
    ~VulkanGraphicsPipeline();
};

// Vulkan Platform Implementation
class VulkanPlatform : public PlatformGraphics {
private:
    // Core Vulkan objects
    VkInstance instance_;
    VkSurfaceKHR surface_;
    VkDevice device_;
    VkQueue graphicsQueue_;
    VkQueue presentQueue_;
    VkQueue computeQueue_;

    // Physical device selection
    VulkanPhysicalDevice physicalDevice_;
    std::vector<VulkanPhysicalDevice> availableDevices_;

    // Swapchain
    VkSwapchainKHR swapchain_;
    VulkanSwapchainSupport swapchainSupport_;
    std::vector<VkImage> swapchainImages_;
    std::vector<VkImageView> swapchainImageViews_;
    VkFormat swapchainImageFormat_;
    VkExtent2D swapchainExtent_;

    // Render pass and pipeline
    VkRenderPass renderPass_;
    VulkanGraphicsPipeline graphicsPipeline_;

    // Command pools and buffers
    VkCommandPool commandPool_;
    std::vector<VkCommandBuffer> commandBuffers_;

    // Synchronization objects
    std::vector<VkSemaphore> imageAvailableSemaphores_;
    std::vector<VkSemaphore> renderFinishedSemaphores_;
    std::vector<VkFence> inFlightFences_;
    std::vector<VkFence> imagesInFlight_;
    size_t currentFrame_ = 0;

    // Surface and window
    ANativeWindow* window_ = nullptr;
    VkSurfaceKHR androidSurface_ = VK_NULL_HANDLE;

    // Validation layers
    bool enableValidationLayers_ = true;
    VkDebugUtilsMessengerEXT debugMessenger_;

    // Extension support
    std::vector<const char*> requiredExtensions_;
    std::vector<const char*> requiredDeviceExtensions_;

public:
    VulkanPlatform();
    ~VulkanPlatform();

    // PlatformGraphics interface
    std::unique_ptr<PlatformGraphicsContext> createContext() override;
    PlatformCapabilities getCapabilities() override;

    // Vulkan-specific methods
    bool initialize(ANativeWindow* window);
    void shutdown();

    // Device management
    bool selectPhysicalDevice();
    bool createLogicalDevice();
    bool createSurface(ANativeWindow* window);

    // Swapchain management
    bool createSwapchain();
    void recreateSwapchain();
    bool querySwapchainSupport(VkPhysicalDevice device, VulkanSwapchainSupport& support);

    // Graphics pipeline
    bool createRenderPass();
    bool createGraphicsPipeline();
    bool createFramebuffers();
    bool createCommandPool();
    bool createCommandBuffers();
    bool createSyncObjects();

    // Drawing and presentation
    void drawFrame();
    VkResult acquireNextImage(uint32_t* imageIndex);
    VkResult submitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex);
    VkResult presentFrame(uint32_t* imageIndex);

    // Utility functions
    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();
    void setupDebugMessenger();
    bool isDeviceSuitable(VkPhysicalDevice device);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat findDepthFormat();

    // Queue family indices
    struct QueueFamilyIndices {
        uint32_t graphicsFamily = UINT32_MAX;
        uint32_t presentFamily = UINT32_MAX;
        uint32_t computeFamily = UINT32_MAX;

        bool isComplete() {
            return graphicsFamily != UINT32_MAX && presentFamily != UINT32_MAX;
        }
    };

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

    // Getters
    VkDevice getDevice() const { return device_; }
    VkPhysicalDevice getPhysicalDevice() const { return physicalDevice_.device; }
    VkQueue getGraphicsQueue() const { return graphicsQueue_; }
    VkQueue getPresentQueue() const { return presentQueue_; }
    VkQueue getComputeQueue() const { return computeQueue_; }
    VkSwapchainKHR getSwapchain() const { return swapchain_; }
    VkRenderPass getRenderPass() const { return renderPass_; }
    const VulkanSwapchainSupport& getSwapchainSupport() const { return swapchainSupport_; }

private:
    // Validation layer callback
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDebugUtilsMessengerEXT* pDebugMessenger);

    void DestroyDebugUtilsMessengerEXT(VkInstance instance,
        VkDebugUtilsMessengerEXT debugMessenger,
        const VkAllocationCallbacks* pAllocator);
};

// Vulkan Context Implementation
class VulkanContext : public PlatformGraphicsContext {
private:
    VulkanPlatform* vulkanPlatform_;
    VkCommandBuffer commandBuffer_;
    VkFramebuffer framebuffer_;
    VkImageView imageView_;

public:
    VulkanContext(VulkanPlatform* platform);
    ~VulkanContext();

    // PlatformGraphicsContext interface implementation
    void viewport(int x, int y, int width, int height) override;
    void clear(unsigned int mask) override;
    void clearColor(float r, float g, float b, float a) override;
    void enable(unsigned int cap) override;
    void disable(unsigned int cap) override;
    void cullFace(unsigned int mode) override;
    void depthFunc(unsigned int func) override;
    void blendFunc(unsigned int sfactor, unsigned int dfactor) override;

    // Buffer operations
    unsigned int createBuffer() override;
    void bindBuffer(unsigned int target, unsigned int buffer) override;
    void bufferData(unsigned int target, const void* data, size_t size, unsigned int usage) override;
    void deleteBuffer(unsigned int buffer) override;

    // Shader operations
    unsigned int createShader(unsigned int type) override;
    void shaderSource(unsigned int shader, const std::string& source) override;
    void compileShader(unsigned int shader) override;
    int getShaderParameter(unsigned int shader, unsigned int pname) override;
    std::string getShaderInfoLog(unsigned int shader) override;
    void deleteShader(unsigned int shader) override;

    // Program operations
    unsigned int createProgram() override;
    void attachShader(unsigned int program, unsigned int shader) override;
    void linkProgram(unsigned int program) override;
    int getProgramParameter(unsigned int program, unsigned int pname) override;
    std::string getProgramInfoLog(unsigned int program) override;
    void useProgram(unsigned int program) override;
    void deleteProgram(unsigned int program) override;

    // Attribute and uniform operations
    int getAttribLocation(unsigned int program, const std::string& name) override;
    int getUniformLocation(unsigned int program, const std::string& name) override;
    void vertexAttribPointer(unsigned int index, int size, unsigned int type, bool normalized, int stride, unsigned int offset) override;
    void enableVertexAttribArray(unsigned int index) override;
    void disableVertexAttribArray(unsigned int index) override;

    // Uniform operations
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

    // Drawing operations
    void drawArrays(unsigned int mode, int first, int count) override;
    void drawElements(unsigned int mode, int count, unsigned int type, unsigned int offset) override;

    // Vulkan-specific methods
    void beginRenderPass();
    void endRenderPass();
    void beginCommandBuffer();
    void endCommandBuffer();
    void bindPipeline();
    void setViewport();
    void setScissor();

    // Getters
    VkCommandBuffer getCommandBuffer() const { return commandBuffer_; }
    VkFramebuffer getFramebuffer() const { return framebuffer_; }
    VkImageView getImageView() const { return imageView_; }
};

} // namespace FoundryEngine

#endif // FOUNDRYENGINE_VULKAN_PLATFORM_H
