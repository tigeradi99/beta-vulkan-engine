//
// Created by adity on 26-07-2025.
//

#include "systems/xe_point_light_system.h"

#include <stdexcept>
#include <array>
#include <cassert>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"

namespace xe {

    XEPointLightSystem::XEPointLightSystem(XEDevice& device,
        VkRenderPass renderPass,
        VkDescriptorSetLayout globalSetLayout,
        XELightManager& lightManager): xe_device(device), lightManager(lightManager) {

        descriptorSetLayouts.push_back(globalSetLayout);
        descriptorSetLayouts.push_back(lightManager.getDescriptorLayout());

        createPipelineLayout();
        createPipeline(renderPass);
    }

    XEPointLightSystem::~XEPointLightSystem() {
        vkDestroyPipelineLayout(xe_device.device(), xe_pipeline_layout, nullptr);
    }

    void XEPointLightSystem::createPipelineLayout() {
        // VkPushConstantRange push_constant_range = {};
        // push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        // push_constant_range.offset = 0;
        // push_constant_range.size = sizeof(SimplePushConstantData);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;

        if (vkCreatePipelineLayout(xe_device.device(), &pipelineLayoutInfo, nullptr, &xe_pipeline_layout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }
    }

    void XEPointLightSystem::createPipeline(VkRenderPass renderPass) {
        assert(xe_pipeline_layout != nullptr && "Cannot create pipeline before pipeline layout!");

        PipelineConfigInfo pipelineConfig = {};
        XEPipeline::defaultPipelineConfigInfo(pipelineConfig);
        pipelineConfig.attributeDescriptions.clear();
        pipelineConfig.bindingDescriptions.clear();

        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = xe_pipeline_layout;
        xe_pipeline = std::make_unique<XEPipeline>(xe_device,
            "assets\\shaders\\point_light_shader.spv",
            "assets\\shaders\\point_light_fragment.spv",
            pipelineConfig,
            "graphics");
    }

    void XEPointLightSystem::render(FrameInfo& frame_info, uint32_t instanceCount) {
        xe_pipeline->bind(frame_info.commandBuffer);

        vkCmdBindDescriptorSets(
            frame_info.commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            xe_pipeline_layout,
            0, 1,
            &frame_info.globalDescriptorSet,
            0,
            nullptr);

        vkCmdBindDescriptorSets(
            frame_info.commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            xe_pipeline_layout,
            1, 1,
            &frame_info.lightDescriptorSet,
            0,
            nullptr);

        vkCmdDraw(frame_info.commandBuffer, 6, instanceCount, 0, 0);
    }

}
