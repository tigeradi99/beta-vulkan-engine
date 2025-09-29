//
// Created by adity on 10-08-2025.
//

#pragma once

#include "renderer/xe_device.h"
#include "vulkan/vulkan.h"
#include "vma/vk_mem_alloc.h"
#include <memory>

namespace xe {
    class XEImageVMA {
    public:
        // constructor for stbi path
        XEImageVMA(XEDevice& device,
                   int width,
                   int height,
                   int n_channels,
                   uint32_t mipLevels,
                   void* pixels,
                   VkFormat format,
                   VkImageTiling tiling,
                   VkImageUsageFlags usage,
                   VmaMemoryUsage memoryUsage);

        // constructor for shadowmap Image
        XEImageVMA(XEDevice& device,
                   int width,
                   int height,
                   uint32_t n_cascades,
                   VkFormat format,
                   VkImageTiling tiling,
                   VkImageUsageFlags usage,
                   VmaMemoryUsage memoryUsage);

        ~XEImageVMA();

        XEImageVMA(const XEImageVMA&) = delete;
        XEImageVMA& operator=(const XEImageVMA&) = delete;

        VkImage getImage() { return image; }
        VkImageView getImageView() { return imageView; }

    private:
        XEDevice& device;
        VkImage image;
        VkImageView imageView;
        VmaAllocation allocation;
        void* mappedMemory = nullptr;

        void transitionImageLayout(VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
        void generateMipmaps(int32_t width, int32_t height, uint32_t mipLevels);
        void createImage(int width, int height, uint32_t mipLevels, uint32_t n_layers, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
        VmaMemoryUsage memoryUsage);
        void createImageView(uint32_t mipLevels, uint32_t n_layers, VkFormat format, VkImageAspectFlags aspect,
            VkImageViewType viewType);
    };
}