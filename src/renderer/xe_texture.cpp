//
// Created by adity on 27-07-2025.
//

#include "renderer/xe_buffer.h"
#include "renderer/xe_texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


#include <stdexcept>
#include <iostream>
#include <filesystem>

namespace xe {
    XETexture::XETexture(const std::string& fileName, XEDevice& deviceRef, VkFormat imageFormat) : device(deviceRef) {
        int width, height, channels;
        uint32_t mipLevels;

        stbi_uc* pixels = stbi_load(fileName.c_str(), &width, &height, &channels, STBI_rgb_alpha);
        std::cout << "Loading image: " << fileName << " width: " << width << " height: " << height << " channels: " << channels << std::endl;

        if (!pixels) {
            std::string reason = stbi_failure_reason() ? stbi_failure_reason() : "unknown";
            std::cerr << "[Texture] Failed to load: " << fileName
                << " (wd=" << std::filesystem::current_path().string()
                << ") reason=" << reason << "\n";
            throw std::runtime_error("Failed to load texture");
        }

        if (channels == 3 || channels == 2 || channels == 1) {
            channels = 4; // default to four channels
        }

        mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

        xe_image_vma = std::make_unique<XEImageVMA>(device, width, height, channels, mipLevels, (void*) pixels,
            imageFormat, VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY);

        stbi_image_free(pixels);
        createTextureSampler();
        createImageInfo();
        std::cout<<"Loaded texture: "<<fileName<<"\n";
    }

    XETexture::~XETexture() {
        vkDestroySampler(device.device(), m_sampler, nullptr);
    }

    void XETexture::createTextureSampler() {
        VkSamplerCreateInfo samplerInfo{};

        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = device.properties.limits.maxSamplerAnisotropy;

        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = VK_LOD_CLAMP_NONE;

        if (vkCreateSampler(device.device(), &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create texture sampler!!");
        }
    }

    void XETexture::createImageInfo() {
        m_descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        m_descriptorImageInfo.imageView = xe_image_vma->getImageView();
        m_descriptorImageInfo.sampler = m_sampler;
    }

}
