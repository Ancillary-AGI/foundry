/**
 * @file SimulationGPU.cpp
 * @brief GPU-accelerated simulation implementations for Linux, macOS, and iOS
 */

#include "../../include/GameEngine/simulation/SimulationGPU.h"
#include <iostream>
#include <cstring>

// Linux Vulkan GPU Implementation
#ifdef __linux__

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_xlib.h>

class VulkanSimulationGPU : public SimulationGPU {
private:
    VkInstance vkInstance = VK_NULL_HANDLE;
    VkPhysicalDevice vkPhysicalDevice = VK_NULL_HANDLE;
    VkDevice vkDevice = VK_NULL_HANDLE;
    VkQueue vkComputeQueue = VK_NULL_HANDLE;
    VkCommandPool vkCommandPool = VK_NULL_HANDLE;
    VkDescriptorPool vkDescriptorPool = VK_NULL_HANDLE;
    uint32_t computeQueueFamilyIndex = 0;

    // Compute pipelines
    VkPipeline smokePipeline = VK_NULL_HANDLE;
    VkPipeline fluidPipeline = VK_NULL_HANDLE;
    VkPipeline noisePipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

    // Shader modules
    VkShaderModule smokeShader = VK_NULL_HANDLE;
    VkShaderModule fluidShader = VK_NULL_HANDLE;
    VkShaderModule noiseShader = VK_NULL_HANDLE;

    // Descriptor sets
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;

    bool initialized = false;

public:
    VulkanSimulationGPU() {}
    ~VulkanSimulationGPU() { cleanup(); }

    bool initialize() override {
        if (initialized) return true;

        std::cout << "Initializing Vulkan GPU simulation support..." << std::endl;

        if (!createInstance()) {
            std::cerr << "Failed to create Vulkan instance" << std::endl;
            return false;
        }

        if (!selectPhysicalDevice()) {
            std::cerr << "Failed to select physical device" << std::endl;
            return false;
        }

        if (!createDevice()) {
            std::cerr << "Failed to create device" << std::endl;
            return false;
        }

        if (!createCommandPool()) {
            std::cerr << "Failed to create command pool" << std::endl;
            return false;
        }

        if (!createDescriptorPool()) {
            std::cerr << "Failed to create descriptor pool" << std::endl;
            return false;
        }

        if (!createDescriptorSetLayout()) {
            std::cerr << "Failed to create descriptor set layout" << std::endl;
            return false;
        }

        if (!createPipelines()) {
            std::cerr << "Failed to create compute pipelines" << std::endl;
            return false;
        }

        initialized = true;
        std::cout << "Vulkan GPU simulation support initialized" << std::endl;
        return true;
    }

    void cleanup() override {
        if (!initialized) return;

        vkDeviceWaitIdle(vkDevice);

        if (smokePipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(vkDevice, smokePipeline, nullptr);
            smokePipeline = VK_NULL_HANDLE;
        }
        if (fluidPipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(vkDevice, fluidPipeline, nullptr);
            fluidPipeline = VK_NULL_HANDLE;
        }
        if (noisePipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(vkDevice, noisePipeline, nullptr);
            noisePipeline = VK_NULL_HANDLE;
        }
        if (pipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(vkDevice, pipelineLayout, nullptr);
            pipelineLayout = VK_NULL_HANDLE;
        }

        if (smokeShader != VK_NULL_HANDLE) {
            vkDestroyShaderModule(vkDevice, smokeShader, nullptr);
            smokeShader = VK_NULL_HANDLE;
        }
        if (fluidShader != VK_NULL_HANDLE) {
            vkDestroyShaderModule(vkDevice, fluidShader, nullptr);
            fluidShader = VK_NULL_HANDLE;
        }
        if (noiseShader != VK_NULL_HANDLE) {
            vkDestroyShaderModule(vkDevice, noiseShader, nullptr);
            noiseShader = VK_NULL_HANDLE;
        }

        if (descriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(vkDevice, descriptorSetLayout, nullptr);
            descriptorSetLayout = VK_NULL_HANDLE;
        }
        if (vkDescriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(vkDevice, vkDescriptorPool, nullptr);
            vkDescriptorPool = VK_NULL_HANDLE;
        }
        if (vkCommandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(vkDevice, vkCommandPool, nullptr);
            vkCommandPool = VK_NULL_HANDLE;
        }
        if (vkDevice != VK_NULL_HANDLE) {
            vkDestroyDevice(vkDevice, nullptr);
            vkDevice = VK_NULL_HANDLE;
        }
        if (vkInstance != VK_NULL_HANDLE) {
            vkDestroyInstance(vkInstance, nullptr);
            vkInstance = VK_NULL_HANDLE;
        }

        initialized = false;
    }

    bool runSmokeSimulation(const SmokeSimulationData& input, SmokeSimulationData& output) override {
        if (!initialized) return false;

        // Create buffers for input/output
        VkBuffer inputBuffer, outputBuffer;
        VkDeviceMemory inputMemory, outputMemory;

        if (!createBuffer(input.particleCount * sizeof(SmokeParticle), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         inputBuffer, inputMemory)) {
            return false;
        }

        if (!createBuffer(input.particleCount * sizeof(SmokeParticle), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         outputBuffer, outputMemory)) {
            vkDestroyBuffer(vkDevice, inputBuffer, nullptr);
            vkFreeMemory(vkDevice, inputMemory, nullptr);
            return false;
        }

        // Copy input data to GPU
        void* data;
        vkMapMemory(vkDevice, inputMemory, 0, input.particleCount * sizeof(SmokeParticle), 0, &data);
        memcpy(data, input.particles, input.particleCount * sizeof(SmokeParticle));
        vkUnmapMemory(vkDevice, inputMemory);

        // Execute compute shader
        if (!executeComputeShader(smokePipeline, inputBuffer, outputBuffer, input.particleCount)) {
            vkDestroyBuffer(vkDevice, inputBuffer, nullptr);
            vkDestroyBuffer(vkDevice, outputBuffer, nullptr);
            vkFreeMemory(vkDevice, inputMemory, nullptr);
            vkFreeMemory(vkDevice, outputMemory, nullptr);
            return false;
        }

        // Copy results back
        vkMapMemory(vkDevice, outputMemory, 0, input.particleCount * sizeof(SmokeParticle), 0, &data);
        memcpy(output.particles, data, input.particleCount * sizeof(SmokeParticle));
        vkUnmapMemory(vkDevice, outputMemory);

        // Cleanup
        vkDestroyBuffer(vkDevice, inputBuffer, nullptr);
        vkDestroyBuffer(vkDevice, outputBuffer, nullptr);
        vkFreeMemory(vkDevice, inputMemory, nullptr);
        vkFreeMemory(vkDevice, outputMemory, nullptr);

        return true;
    }

    bool runFluidSimulation(const FluidSimulationData& input, FluidSimulationData& output) override {
        if (!initialized) return false;

        // Similar implementation to smoke simulation
        // Create buffers, execute compute shader, copy results
        return executeFluidCompute(input, output);
    }

    bool generateNoise(const NoiseGenerationData& input, std::vector<float>& output) override {
        if (!initialized) return false;

        // Create output buffer
        VkBuffer outputBuffer;
        VkDeviceMemory outputMemory;

        size_t bufferSize = input.width * input.height * sizeof(float);
        if (!createBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         outputBuffer, outputMemory)) {
            return false;
        }

        // Execute noise generation compute shader
        if (!executeNoiseCompute(noisePipeline, outputBuffer, input)) {
            vkDestroyBuffer(vkDevice, outputBuffer, nullptr);
            vkFreeMemory(vkDevice, outputMemory, nullptr);
            return false;
        }

        // Copy results back
        output.resize(input.width * input.height);
        void* data;
        vkMapMemory(vkDevice, outputMemory, 0, bufferSize, 0, &data);
        memcpy(output.data(), data, bufferSize);
        vkUnmapMemory(vkDevice, outputMemory);

        // Cleanup
        vkDestroyBuffer(vkDevice, outputBuffer, nullptr);
        vkFreeMemory(vkDevice, outputMemory, nullptr);

        return true;
    }

private:
    bool createInstance() {
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Foundry Simulation GPU";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Foundry Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_1;

        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        // Enable required extensions
        const char* extensions[] = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_XLIB_SURFACE_EXTENSION_NAME
        };
        createInfo.enabledExtensionCount = 2;
        createInfo.ppEnabledExtensionNames = extensions;

        return vkCreateInstance(&createInfo, nullptr, &vkInstance) == VK_SUCCESS;
    }

    bool selectPhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr);
        if (deviceCount == 0) return false;

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(vkInstance, &deviceCount, devices.data());

        for (const auto& device : devices) {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);

            // Check for compute capability
            VkPhysicalDeviceFeatures deviceFeatures;
            vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

            if (deviceFeatures.shaderFloat64) { // Basic compute capability check
                vkPhysicalDevice = device;
                return true;
            }
        }

        return false;
    }

    bool createDevice() {
        // Find compute queue family
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamilyCount, queueFamilies.data());

        for (uint32_t i = 0; i < queueFamilyCount; ++i) {
            if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                computeQueueFamilyIndex = i;
                break;
            }
        }

        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = computeQueueFamilyIndex;
        queueCreateInfo.queueCount = 1;
        float queuePriority = 1.0f;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount = 1;
        deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;

        return vkCreateDevice(vkPhysicalDevice, &deviceCreateInfo, nullptr, &vkDevice) == VK_SUCCESS &&
               vkGetDeviceQueue(vkDevice, computeQueueFamilyIndex, 0, &vkComputeQueue) == VK_SUCCESS;
    }

    bool createCommandPool() {
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = computeQueueFamilyIndex;

        return vkCreateCommandPool(vkDevice, &poolInfo, nullptr, &vkCommandPool) == VK_SUCCESS;
    }

    bool createDescriptorPool() {
        VkDescriptorPoolSize poolSize = {};
        poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSize.descriptorCount = 10; // Enough for our needs

        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = 10;

        return vkCreateDescriptorPool(vkDevice, &poolInfo, nullptr, &vkDescriptorPool) == VK_SUCCESS;
    }

    bool createDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding layoutBinding = {};
        layoutBinding.binding = 0;
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &layoutBinding;

        return vkCreateDescriptorSetLayout(vkDevice, &layoutInfo, nullptr, &descriptorSetLayout) == VK_SUCCESS;
    }

    bool createPipelines() {
        // Create pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

        if (vkCreatePipelineLayout(vkDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            return false;
        }

        // Note: In a real implementation, you would load SPIR-V shaders
        // For this example, we'll assume shader modules are created elsewhere
        // createShaderModule() would load SPIR-V files

        return true; // Simplified - assume shaders are loaded
    }

    bool createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                     VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(vkDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            return false;
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(vkDevice, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(vkDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            vkDestroyBuffer(vkDevice, buffer, nullptr);
            return false;
        }

        return vkBindBufferMemory(vkDevice, buffer, bufferMemory, 0) == VK_SUCCESS;
    }

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
            if ((typeFilter & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("Failed to find suitable memory type!");
    }

    bool executeComputeShader(VkPipeline pipeline, VkBuffer inputBuffer, VkBuffer outputBuffer, uint32_t particleCount) {
        // Allocate descriptor set
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = vkDescriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &descriptorSetLayout;

        VkDescriptorSet descriptorSet;
        if (vkAllocateDescriptorSets(vkDevice, &allocInfo, &descriptorSet) != VK_SUCCESS) {
            return false;
        }

        // Update descriptor set
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = inputBuffer;
        bufferInfo.offset = 0;
        bufferInfo.range = VK_WHOLE_SIZE;

        VkWriteDescriptorSet descriptorWrite = {};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSet;
        descriptorWrite.dstBinding = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(vkDevice, 1, &descriptorWrite, 0, nullptr);

        // Create command buffer
        VkCommandBuffer commandBuffer;
        VkCommandBufferAllocateInfo allocInfo2 = {};
        allocInfo2.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo2.commandPool = vkCommandPool;
        allocInfo2.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo2.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(vkDevice, &allocInfo2, &commandBuffer) != VK_SUCCESS) {
            return false;
        }

        // Record command buffer
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout,
                               0, 1, &descriptorSet, 0, nullptr);

        // Dispatch compute work
        uint32_t workGroupSize = 256;
        uint32_t workGroups = (particleCount + workGroupSize - 1) / workGroupSize;
        vkCmdDispatch(commandBuffer, workGroups, 1, 1);

        vkEndCommandBuffer(commandBuffer);

        // Submit and wait
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        VkFence fence;
        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        vkCreateFence(vkDevice, &fenceInfo, nullptr, &fence);

        if (vkQueueSubmit(vkComputeQueue, 1, &submitInfo, fence) != VK_SUCCESS) {
            vkDestroyFence(vkDevice, fence, nullptr);
            vkFreeCommandBuffers(vkDevice, vkCommandPool, 1, &commandBuffer);
            return false;
        }

        vkWaitForFences(vkDevice, 1, &fence, VK_TRUE, UINT64_MAX);

        // Cleanup
        vkDestroyFence(vkDevice, fence, nullptr);
        vkFreeCommandBuffers(vkDevice, vkCommandPool, 1, &commandBuffer);

        return true;
    }

    bool executeFluidCompute(const FluidSimulationData& input, FluidSimulationData& output) {
        // Similar to smoke simulation but for fluid particles
        return true; // Placeholder
    }

    bool executeNoiseCompute(VkPipeline pipeline, VkBuffer outputBuffer, const NoiseGenerationData& input) {
        // Execute noise generation compute shader
        return true; // Placeholder
    }
};

#endif // __linux__

// macOS Metal GPU Implementation
#ifdef __APPLE__

#include <Metal/Metal.h>
#include <MetalKit/MetalKit.h>

class MetalSimulationGPU : public SimulationGPU {
private:
    id<MTLDevice> metalDevice = nil;
    id<MTLCommandQueue> metalCommandQueue = nil;
    id<MTLLibrary> metalLibrary = nil;

    // Compute pipelines
    id<MTLComputePipelineState> smokePipeline = nil;
    id<MTLComputePipelineState> fluidPipeline = nil;
    id<MTLComputePipelineState> noisePipeline = nil;

    bool initialized = false;

public:
    MetalSimulationGPU() {}
    ~MetalSimulationGPU() { cleanup(); }

    bool initialize() override {
        if (initialized) return true;

        std::cout << "Initializing Metal GPU simulation support..." << std::endl;

        metalDevice = MTLCreateSystemDefaultDevice();
        if (!metalDevice) {
            std::cerr << "Failed to create Metal device" << std::endl;
            return false;
        }

        metalCommandQueue = [metalDevice newCommandQueue];
        if (!metalCommandQueue) {
            std::cerr << "Failed to create Metal command queue" << std::endl;
            return false;
        }

        // Load Metal library (assuming shaders are compiled into the app bundle)
        NSError* error = nil;
        NSString* libraryPath = [[NSBundle mainBundle] pathForResource:@"SimulationShaders" ofType:@"metallib"];
        if (libraryPath) {
            metalLibrary = [metalDevice newLibraryWithFile:libraryPath error:&error];
        } else {
            // Fallback: try to load from default library
            metalLibrary = [metalDevice newDefaultLibrary];
        }

        if (!metalLibrary) {
            std::cerr << "Failed to load Metal library" << std::endl;
            return false;
        }

        if (!createComputePipelines()) {
            std::cerr << "Failed to create compute pipelines" << std::endl;
            return false;
        }

        initialized = true;
        std::cout << "Metal GPU simulation support initialized" << std::endl;
        return true;
    }

    void cleanup() override {
        if (!initialized) return;

        smokePipeline = nil;
        fluidPipeline = nil;
        noisePipeline = nil;
        metalLibrary = nil;
        metalCommandQueue = nil;
        metalDevice = nil;

        initialized = false;
    }

    bool runSmokeSimulation(const SmokeSimulationData& input, SmokeSimulationData& output) override {
        if (!initialized || !smokePipeline) return false;

        id<MTLCommandBuffer> commandBuffer = [metalCommandQueue commandBuffer];
        if (!commandBuffer) return false;

        id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
        if (!encoder) return false;

        [encoder setComputePipelineState:smokePipeline];

        // Create buffers
        id<MTLBuffer> inputBuffer = [metalDevice newBufferWithBytes:input.particles
                                                            length:input.particleCount * sizeof(SmokeParticle)
                                                           options:MTLResourceStorageModeShared];
        id<MTLBuffer> outputBuffer = [metalDevice newBufferWithLength:input.particleCount * sizeof(SmokeParticle)
                                                              options:MTLResourceStorageModeShared];

        if (!inputBuffer || !outputBuffer) {
            [encoder endEncoding];
            return false;
        }

        // Set buffers
        [encoder setBuffer:inputBuffer offset:0 atIndex:0];
        [encoder setBuffer:outputBuffer offset:0 atIndex:1];

        // Set uniforms
        float deltaTime = input.deltaTime;
        [encoder setBytes:&deltaTime length:sizeof(float) atIndex:2];

        // Dispatch compute work
        NSUInteger threadGroupSize = 256;
        NSUInteger threadGroups = (input.particleCount + threadGroupSize - 1) / threadGroupSize;
        [encoder dispatchThreadgroups:MTLSizeMake(threadGroups, 1, 1)
                threadsPerThreadgroup:MTLSizeMake(threadGroupSize, 1, 1)];

        [encoder endEncoding];

        // Execute
        [commandBuffer commit];
        [commandBuffer waitUntilCompleted];

        // Copy results back
        memcpy(output.particles, [outputBuffer contents],
               input.particleCount * sizeof(SmokeParticle));

        return true;
    }

    bool runFluidSimulation(const FluidSimulationData& input, FluidSimulationData& output) override {
        if (!initialized || !fluidPipeline) return false;

        // Similar implementation to smoke simulation
        id<MTLCommandBuffer> commandBuffer = [metalCommandQueue commandBuffer];
        id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];

        [encoder setComputePipelineState:fluidPipeline];

        // Create and set buffers, dispatch work
        // Implementation similar to smoke simulation

        [encoder endEncoding];
        [commandBuffer commit];
        [commandBuffer waitUntilCompleted];

        return true;
    }

    bool generateNoise(const NoiseGenerationData& input, std::vector<float>& output) override {
        if (!initialized || !noisePipeline) return false;

        id<MTLCommandBuffer> commandBuffer = [metalCommandQueue commandBuffer];
        id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];

        [encoder setComputePipelineState:noisePipeline];

        // Create output buffer
        id<MTLBuffer> outputBuffer = [metalDevice newBufferWithLength:input.width * input.height * sizeof(float)
                                                              options:MTLResourceStorageModeShared];

        [encoder setBuffer:outputBuffer offset:0 atIndex:0];

        // Set noise parameters
        [encoder setBytes:&input length:sizeof(NoiseGenerationData) atIndex:1];

        // Dispatch
        NSUInteger threadGroups = (input.width * input.height + 255) / 256;
        [encoder dispatchThreadgroups:MTLSizeMake(threadGroups, 1, 1)
                threadsPerThreadgroup:MTLSizeMake(256, 1, 1)];

        [encoder endEncoding];
        [commandBuffer commit];
        [commandBuffer waitUntilCompleted];

        // Copy results
        output.resize(input.width * input.height);
        memcpy(output.data(), [outputBuffer contents], output.size() * sizeof(float));

        return true;
    }

private:
    bool createComputePipelines() {
        NSError* error = nil;

        // Smoke simulation pipeline
        id<MTLFunction> smokeFunction = [metalLibrary newFunctionWithName:@"smokeSimulationKernel"];
        if (smokeFunction) {
            smokePipeline = [metalDevice newComputePipelineStateWithFunction:smokeFunction error:&error];
            if (!smokePipeline) {
                std::cerr << "Failed to create smoke pipeline: " << [[error localizedDescription] UTF8String] << std::endl;
                return false;
            }
        }

        // Fluid simulation pipeline
        id<MTLFunction> fluidFunction = [metalLibrary newFunctionWithName:@"fluidSimulationKernel"];
        if (fluidFunction) {
            fluidPipeline = [metalDevice newComputePipelineStateWithFunction:fluidFunction error:&error];
            if (!fluidPipeline) {
                std::cerr << "Failed to create fluid pipeline: " << [[error localizedDescription] UTF8String] << std::endl;
                return false;
            }
        }

        // Noise generation pipeline
        id<MTLFunction> noiseFunction = [metalLibrary newFunctionWithName:@"noiseGenerationKernel"];
        if (noiseFunction) {
            noisePipeline = [metalDevice newComputePipelineStateWithFunction:noiseFunction error:&error];
            if (!noisePipeline) {
                std::cerr << "Failed to create noise pipeline: " << [[error localizedDescription] UTF8String] << std::endl;
                return false;
            }
        }

        return true;
    }
};

#endif // __APPLE__

// Factory function to create appropriate GPU implementation
SimulationGPU* SimulationGPU::create() {
#ifdef __linux__
    return new VulkanSimulationGPU();
#elif defined(__APPLE__)
    return new MetalSimulationGPU();
#else
    // Fallback for other platforms
    return nullptr;
#endif
}

void SimulationGPU::destroy(SimulationGPU* gpu) {
    if (gpu) {
        gpu->cleanup();
        delete gpu;
    }
}