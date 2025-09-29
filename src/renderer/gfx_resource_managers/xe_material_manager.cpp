//
// Created by adity on 10-08-2025.
//

#include "renderer/gfx_resource_managers/xe_material_manager.h"

namespace xe {
    XEMaterialManager::XEMaterialManager(XETextureManager &texture_manager): textureManager(texture_manager) {
        defaultMaterial.albedoIndex = textureManager.getDefaultAlbedoTextureIndex();
        defaultMaterial.normalIndex = textureManager.getDefaultNormalTextureIndex();

        materials.push_back(defaultMaterial);
        defaultMaterialIndex = static_cast<int>(materials.size() - 1);
    }

    XEMaterialManager::~XEMaterialManager() { }

    int XEMaterialManager::getIndexOrDefault(const std::string &textureFileName, TextureSemantic sem) {
        int defaultIndex = 0;
        if (sem == TextureSemantic::BaseColor) {
            defaultIndex = 0;
        }
        else if (sem == TextureSemantic::Normal) {
            defaultIndex = 1;
        }

        if (textureFileName.empty()) {
            return defaultIndex;
        }

        int i = textureManager.getOrLoadTexture(textureFileName, sem);
        return (i > defaultIndexBound) ? i : defaultIndex;
    }

    int XEMaterialManager::create(const XEMaterialDesc& m_desc) {
        XEMaterial material;

        material.albedoIndex = getIndexOrDefault(m_desc.albedoFileName, TextureSemantic::BaseColor);
        material.normalIndex = getIndexOrDefault(m_desc.normalFileName, TextureSemantic::Normal);

        materials.push_back(material);
        return static_cast<int>(materials.size() - 1);
    }
}
