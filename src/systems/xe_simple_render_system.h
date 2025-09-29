//
// Created by adity on 10-07-2025.
//

#pragma once

#include "renderer/xe_pipeline.h"
#include "renderer/xe_device.h"
#include "scene/xe_game_object.h"
#include "systems/xe_camera.h"
#include "systems/xe_frame_info.h"
#include "renderer/xe_descriptors.h"
#include "renderer/gfx_resource_managers/xe_texture_manager.h"
#include "renderer/gfx_resource_managers/xe_material_manager.h"
#include "renderer/lighting/xe_light_manager.h"

#include <memory>
#include <vector>

namespace xe {
    class XESimpleRenderSystem {
    public:
        XESimpleRenderSystem(XEDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout,
            XETextureManager& textureManager, XEMaterialManager& materialManager,
            VkDescriptorSetLayout shadowSamplerLayout, XELightManager& lightManager);
        ~XESimpleRenderSystem();

        XESimpleRenderSystem(const XESimpleRenderSystem &) = delete;
        XESimpleRenderSystem &operator=(const XESimpleRenderSystem &) = delete;

        void renderGameObjects(FrameInfo& frame_info, VkDescriptorSet shadowSamplerDescriptorSet);

    private:
        void createPipelineLayout();
        void createPipeline(VkRenderPass renderPass);

        XEDevice& xe_device;
        std::unique_ptr<xe::XEPipeline> xe_pipeline;
        VkPipelineLayout xe_pipeline_layout{VK_NULL_HANDLE};

        XETextureManager& textureManager;
        XEMaterialManager& materialManager;
        XELightManager& lightManager;
        VkDescriptorSet textureSet;
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts;

    };
}
