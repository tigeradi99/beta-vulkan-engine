//
// Created by adity on 10-08-2025.
//

#pragma once

#include "renderer/xe_texture.h"
#include "renderer/materials/xe_materials.h"
#include "renderer/gfx_resource_managers/xe_texture_manager.h"

namespace xe {
    class XEMaterialManager {
    public:
        XEMaterialManager(XETextureManager& texture_manager);
        ~XEMaterialManager();

        XEMaterialManager(const XEMaterialManager &) = delete;
        XEMaterialManager(XEMaterialManager &&) = delete;

        int create(const XEMaterialDesc& m_desc); // return material index after creation
        const XEMaterial& getMaterial(int id) const { return materials[id]; }
        const int getDefaultMaterialIndex() const { return defaultMaterialIndex; }

    private:
        int getIndexOrDefault(const std::string& textureFileName, TextureSemantic sem);

        std::vector<XEMaterial> materials;
        XETextureManager& textureManager;
        XEMaterial defaultMaterial{};
        int defaultMaterialIndex = 0;
        int defaultIndexBound = 1; // 0, 1 are default albedo and normal textures respectively. Increment
    };
}
