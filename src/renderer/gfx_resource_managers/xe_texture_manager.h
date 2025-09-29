//
// Created by adity on 03-08-2025.
//

#pragma once

#include "vulkan/vulkan.h"

#include "renderer/xe_texture.h"
#include "renderer/xe_descriptors.h"
#include "renderer/xe_device.h"

#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

namespace xe {
    enum class TextureSemantic { BaseColor, Normal, ORM, Emissive };

    class XETextureManager {
    public:
        XETextureManager(
            XEDevice& device,
            uint32_t maxTextures);

        ~XETextureManager();

        XETextureManager(const XETextureManager&) = delete;
        XETextureManager& operator=(const XETextureManager&) = delete;

        int getOrLoadTexture(const std::string& path, TextureSemantic semantic);
        int getDefaultAlbedoTextureIndex() const { return defaultAlbedoTextureIndex; }
        int getDefaultNormalTextureIndex() const { return defaultNormalTextureIndex; }
        VkDescriptorSet getDescriptorSet() const {return textureDescriptorSet; }
        VkDescriptorSetLayout getDescriptorLayout() const { return textureSetLayout->getDescriptorSetLayout(); }

    private:
        void updateDescriptorSet();
        void createDefaultAlbedoTexture();
        void createDefaultNormalTexture();
        void initializeDescriptorSet();

        XEDevice& device;

        std::unique_ptr<XEDescriptorPool> texturePool{};
        std::unique_ptr<XEDescriptorSetLayout> textureSetLayout{};
        VkDescriptorSet textureDescriptorSet{VK_NULL_HANDLE};

        std::vector<std::shared_ptr<XETexture>> textures;
        std::unordered_map<std::string, int> texturesIndexMap;
        std::vector<VkDescriptorImageInfo> imageInfos;

        std::shared_ptr<XETexture> defaultAlbedoTexture;
        std::shared_ptr<XETexture> defaultNormalTexture;
        int defaultAlbedoTextureIndex = 0;
        int defaultNormalTextureIndex = 0;

        uint32_t maxTextures{0};
    };
}