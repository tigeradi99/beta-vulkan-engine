//
// Created by adity on 27-07-2025.
//

#pragma once


#include "renderer/xe_device.h"
#include "renderer/xe_image_vma.h"


#include <memory>
#include "vulkan/vulkan.h"


namespace xe {
    class XETexture {
    public:
        XETexture(const std::string& fileName, XEDevice& deviceRef, VkFormat imageFormat);
        ~XETexture();

        XETexture(const XETexture&) = delete;
        XETexture& operator=(const XETexture&) = delete;

        void createImageInfo();
        VkDescriptorImageInfo getImageInfo() { return m_descriptorImageInfo; }

    private:
        void createTextureSampler();
        void transitionImageLayout(VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

        // Textures:
        std::unique_ptr<XEImageVMA> xe_image_vma;
        VkSampler m_sampler = VK_NULL_HANDLE;
        VkDescriptorImageInfo m_descriptorImageInfo{};

        XEDevice& device;
    };
}