//
// Created by adity on 03-08-2025.
//

#include "renderer/gfx_resource_managers/xe_texture_manager.h"
#include <algorithm>
#include <iostream>
#include <__msvc_ostream.hpp>

namespace xe {
    XETextureManager::XETextureManager(XEDevice &device, uint32_t maxTextures): device(device), maxTextures(maxTextures) {
        texturePool = XEDescriptorPool::Builder(device)
        .setMaxSets(1)
        .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxTextures)
        .build();

        textureSetLayout = XEDescriptorSetLayout::Builder(device)
        .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, maxTextures)
        .build();

        XEDescriptorWriter(*textureSetLayout, *texturePool)
        .build(textureDescriptorSet);

        createDefaultAlbedoTexture();
        createDefaultNormalTexture();
        initializeDescriptorSet();
    }

    XETextureManager::~XETextureManager() { }

    void XETextureManager::createDefaultAlbedoTexture() {
        const std::string path = "assets\\models\\checkerboard\\tiles_0059_color_1k.jpg";
        std::string key = path;
        std::replace(key.begin(), key.end(), '\\', '/');

        defaultAlbedoTexture = std::make_shared<XETexture>(key, device, VK_FORMAT_R8G8B8A8_SRGB);
        textures.push_back(defaultAlbedoTexture);
        defaultAlbedoTextureIndex = static_cast<int>(textures.size()) - 1;

        texturesIndexMap[key] = defaultAlbedoTextureIndex;
    }

    void XETextureManager::createDefaultNormalTexture() {
        const std::string path = "assets\\models\\checkerboard\\tiles_0059_normal_direct_1k.png";
        std::string key = path;
        std::replace(key.begin(), key.end(), '\\', '/');

        defaultNormalTexture = std::make_shared<XETexture>(key, device, VK_FORMAT_R8G8B8A8_UNORM);
        textures.push_back(defaultNormalTexture);
        defaultNormalTextureIndex = static_cast<int>(textures.size()) - 1;

        texturesIndexMap[key] = defaultNormalTextureIndex;
    }

    void XETextureManager::initializeDescriptorSet() {
        imageInfos.resize(maxTextures);

        for (uint32_t i = 0; i < maxTextures; i++) {
            imageInfos[i] = defaultAlbedoTexture->getImageInfo();
        }

        imageInfos[defaultAlbedoTextureIndex] = defaultAlbedoTexture->getImageInfo();
        imageInfos[defaultNormalTextureIndex] = defaultNormalTexture->getImageInfo();

        updateDescriptorSet();
    }

    int XETextureManager::getOrLoadTexture(const std::string &path, TextureSemantic semantic) {
        // Normalize path
        std::string key = path;
        std::replace(key.begin(), key.end(), '\\', '/');
        auto it = texturesIndexMap.find(key);
        if (it != texturesIndexMap.end()) {
            return it->second; // Texture has already been loaded
        }

        VkFormat imgFormat{};

        if (semantic == TextureSemantic::BaseColor) { imgFormat = VK_FORMAT_R8G8B8A8_SRGB; }
        else if (semantic == TextureSemantic::Normal) { imgFormat = VK_FORMAT_R8G8B8A8_UNORM; }
        else { std::cerr << "Unknown semantic" << std::endl; }

        auto tex = std::make_shared<XETexture>(key, device, imgFormat);
        textures.push_back(tex);

        int index = static_cast<int>(textures.size()) - 1;
        texturesIndexMap[key] = index;
        imageInfos[index] = tex->getImageInfo();

        updateDescriptorSet();
        return index;
    }

    void XETextureManager::updateDescriptorSet() {
        XEDescriptorWriter(*textureSetLayout, *texturePool)
        .writeImageArray(0, imageInfos)
        .overwrite(textureDescriptorSet);
    }
}
