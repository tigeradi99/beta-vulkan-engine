#pragma once

#include "platform/xe_window.h"
#include "vulkan/vulkan.h"
#include "vma/vk_mem_alloc.h"

// std lib headers
#include <string>
#include <vector>

namespace xe {

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    struct QueueFamilyIndices {
        uint32_t graphicsFamily;
        uint32_t presentFamily;
        uint32_t transferFamily;
        bool graphicsFamilyHasValue = false;
        bool presentFamilyHasValue = false;
        bool transferFamilyHasValue = false;
        bool isComplete() { return graphicsFamilyHasValue && presentFamilyHasValue && transferFamilyHasValue; }
    };

    class XEDevice {
        public:
    #ifdef NDEBUG
        const bool enableValidationLayers = false;
    #else
        const bool enableValidationLayers = true;
    #endif

        XEDevice(XEWindow &window);
        ~XEDevice();

        // Not copyable or movable
        XEDevice(const XEDevice &) = delete;
        void operator=(const XEDevice &) = delete;
        XEDevice(XEDevice &&) = delete;
        XEDevice &operator=(XEDevice &&) = delete;

        VkInstance getInstance() { return instance; }
        VkCommandPool getCommandPool() { return commandPool; }
        VkPhysicalDevice getPhysicalDevice() { return physicalDevice; }
        VkDevice device() { return device_; }
        VmaAllocator vmaAllocator() { return _allocator; }
        VkSurfaceKHR surface() { return surface_; }
        VkQueue graphicsQueue() { return graphicsQueue_; }
        VkQueue presentQueue() { return presentQueue_; }
        VkQueue transferQueue() { return transferQueue_; }

        VkCommandBuffer getTransferCommandBuffer() { return transferCommandBuffer; }

        SwapChainSupportDetails getSwapChainSupport() { return querySwapChainSupport(physicalDevice); }
        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
        QueueFamilyIndices findPhysicalQueueFamilies() { return findQueueFamilies(physicalDevice); }
        VkFormat findSupportedFormat(
            const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

        // Buffer Helper Functions
        void createBuffer(
            VkDeviceSize size,
            VkBufferUsageFlags usage,
            VkMemoryPropertyFlags properties,
            VkBuffer &buffer,
            VkDeviceMemory &bufferMemory);
        VkCommandBuffer beginSingleTimeCommandsTransfer();
        void endSingleTimeCommandsTransfer(VkCommandBuffer cmdBuffer);
        VkCommandBuffer beginSingleTimeCommandsGraphics();
        void endSingleTimeCommandsGraphics(VkCommandBuffer cmdBuffer);
        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
        void copyBufferToImage(
            VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount);

        void createImageWithInfo(
            const VkImageCreateInfo &imageInfo,
            VkMemoryPropertyFlags properties,
            VkImage &image,
            VkDeviceMemory &imageMemory);

        void createImageWithInfoVMA(
            const VkImageCreateInfo &imageInfo,
            VmaMemoryUsage memoryUsage,
            VkImage& image,
            VmaAllocation& allocation);

        VkPhysicalDeviceProperties properties;

        private:
        void createInstance();
        void setupDebugMessenger();
        void createSurface();
        void pickPhysicalDevice();
        void createLogicalDevice();
        void createVMAAllocator();
        void createCommandPool();
        void createGraphicsCommandBuffers();
        void createTransferCommandBuffers();
        void createGraphicsFences();
        void createTransferFences();

        // helper functions
        bool isDeviceSuitable(VkPhysicalDevice device);
        std::vector<const char *> getRequiredExtensions();
        bool checkValidationLayerSupport();
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
        void hasGflwRequiredInstanceExtensions();
        bool checkDeviceExtensionSupport(VkPhysicalDevice device);
        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        XEWindow &window;
        VkCommandPool commandPool;
        VkCommandPool graphicsCommandPool;
        VkCommandPool transferCommandPool;
        VkCommandBuffer graphicsCommandBuffer  = VK_NULL_HANDLE;
        VkCommandBuffer transferCommandBuffer = VK_NULL_HANDLE;

        VkDevice device_;
        VkSurfaceKHR surface_;
        VkQueue graphicsQueue_;
        VkQueue presentQueue_;
        VkQueue transferQueue_;

        VkFence graphicsFence_;
        VkFence transferFence_;

        // VMA instance
        VmaAllocator _allocator;

        const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
        const std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    };

}  // namespace lve