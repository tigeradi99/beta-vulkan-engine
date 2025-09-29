//
// Created by adity on 31-07-2025.
//

#include "xe_buffer_vma.h"

#include <stdexcept>

namespace xe {
    XEBufferVMA::XEBufferVMA(XEDevice &device, VkDeviceSize instanceSize, uint32_t instanceCount,
        VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) : device {device} {

        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        // Total size of buffer in size
        bufferSize = instanceSize * instanceCount;
        bufferInfo.size = bufferSize;
        // Where the buffer will be used
        bufferInfo.usage = usage;

        // VMA allocation info
        VmaAllocationCreateInfo vmaAllocInfo = {};
        vmaAllocInfo.usage = memoryUsage;

        // allocate the buffer
        if (vmaCreateBuffer(device.vmaAllocator(), &bufferInfo,
            &vmaAllocInfo, &buffer, &allocation, nullptr) != VK_SUCCESS) {
            throw std::runtime_error("failed to create allocate memory and create buffer!!");
        }
    }

    XEBufferVMA::~XEBufferVMA() {
        vmaDestroyBuffer(device.vmaAllocator(), buffer, allocation);
    }

    void XEBufferVMA::map() {
        vmaMapMemory(device.vmaAllocator(), allocation, &mappedMemory);
    }

    /**
     * Copies the specified data to the mapped buffer. Default value writes whole buffer range
     *
     * @param data Pointer to the data to copy
     * @param size (Optional) Size of the data to copy. Pass VK_WHOLE_SIZE to flush the complete buffer
     * range.
     * @param offset (Optional) Byte offset from beginning of mapped region
     *
     */
    void XEBufferVMA::writeToBuffer(void* data, VkDeviceSize size, VkDeviceSize offset) {
        if (size == VK_WHOLE_SIZE) {
            memcpy(mappedMemory, data, bufferSize);
        } else {
            char* memOffset = (char*)mappedMemory;
            memOffset += offset;
            memcpy(memOffset, data, size);
        }
    }

    void XEBufferVMA::unmap() {
        vmaUnmapMemory(device.vmaAllocator(), allocation);
    }

    std::unique_ptr<XEBufferVMA> XEBufferVMA::createBufferAndTransferDataToGPU(XEDevice &device,
        VkDeviceSize instanceSize, uint32_t instanceCount, VkBufferUsageFlags usage, void* data,
        VkDeviceSize size, VkDeviceSize offset) {

        XEBufferVMA staging(
            device,
            instanceSize,
            instanceCount,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_MEMORY_USAGE_CPU_ONLY);

        staging.map();
        staging.writeToBuffer(data, size, offset);
        staging.unmap();

        auto buffer = std::make_unique<XEBufferVMA>(
            device,
            instanceSize,
            instanceCount,
            usage,
            VMA_MEMORY_USAGE_GPU_ONLY);

        device.copyBuffer(staging.getBuffer(), buffer->getBuffer(), buffer->bufferSize);
        return buffer;
    }
}
