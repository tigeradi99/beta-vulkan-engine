//
// Created by adity on 26-07-2025.
//

#pragma once

#include "renderer/xe_pipeline.h"
#include "renderer/xe_device.h"
#include "renderer/lighting/xe_light_manager.h"
#include "scene/xe_game_object.h"
#include "systems/xe_camera.h"
#include "systems/xe_frame_info.h"

#include <memory>

namespace xe {
    class XEPointLightSystem {
    public:
        XEPointLightSystem(XEDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout,
            XELightManager& lightManager);
        ~XEPointLightSystem();

        XEPointLightSystem(const XEPointLightSystem &) = delete;
        XEPointLightSystem &operator=(const XEPointLightSystem &) = delete;

        void render(FrameInfo& frame_info, uint32_t instanceCount);

    private:
        void createPipelineLayout();
        void createPipeline(VkRenderPass renderPass);

        XEDevice& xe_device;
        std::unique_ptr<xe::XEPipeline> xe_pipeline;
        VkPipelineLayout xe_pipeline_layout{VK_NULL_HANDLE};
        XELightManager& lightManager;
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    };
}
