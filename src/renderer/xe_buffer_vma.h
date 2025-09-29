//
// Created by adity on 31-07-2025.
//

#pragma once

#include "renderer/xe_device.h"
#include "vulkan/vulkan.h"
#include "vma/vk_mem_alloc.h"
#include <memory>

namespace xe {
    class XEBufferVMA {
    public:
        XEBufferVMA(XEDevice& device,
                    VkDeviceSize instanceSize,
                    uint32_t instanceCount,
                    VkBufferUsageFlags usage,
                    VmaMemoryUsage memoryUsage);
        ~XEBufferVMA();

        XEBufferVMA(const XEBufferVMA&) = delete;
        XEBufferVMA& operator=(const XEBufferVMA&) = delete;

        void map();
        void writeToBuffer(void* data, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        void unmap();

        static std::unique_ptr<XEBufferVMA> createBufferAndTransferDataToGPU(XEDevice &device,
            VkDeviceSize instanceSize, uint32_t instanceCount, VkBufferUsageFlags usage, void* data,
            VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

        VkBuffer getBuffer() { return buffer; }
        VkDeviceSize getBufferSize() const { return bufferSize; }

    private:
        XEDevice& device;
        VkBuffer buffer;
        VmaAllocation allocation;
        VkDeviceSize bufferSize;

        void *mappedMemory = nullptr;
    };
}
