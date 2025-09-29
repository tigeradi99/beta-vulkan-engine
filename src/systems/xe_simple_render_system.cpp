//
// Created by adity on 10-07-2025.
//

#include "systems/xe_simple_render_system.h"

#include <stdexcept>
#include <array>
#include <cassert>
#include <iostream>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"

namespace xe {

    struct SimplePushConstantData {
        glm::mat4 modelMatrix{1.f};
        int textureIndex{0};
        int normalIndex{0};
        int pad1{0};
        int pad2{0};
    };

    XESimpleRenderSystem::XESimpleRenderSystem(XEDevice& device,
        VkRenderPass renderPass,
        VkDescriptorSetLayout globalSetLayout,
        XETextureManager& textureManager,
        XEMaterialManager& materialManager,
        VkDescriptorSetLayout shadowSamplerLayout,
        XELightManager& lightManager): xe_device(device), textureManager(textureManager), lightManager(lightManager),
        materialManager(materialManager) {

        descriptorSetLayouts.push_back(globalSetLayout);
        descriptorSetLayouts.push_back(textureManager.getDescriptorLayout());
        descriptorSetLayouts.push_back(lightManager.getDescriptorLayout());
        descriptorSetLayouts.push_back(shadowSamplerLayout);

        createPipelineLayout();
        createPipeline(renderPass);
    }

    XESimpleRenderSystem::~XESimpleRenderSystem() {
        vkDestroyPipelineLayout(xe_device.device(), xe_pipeline_layout, nullptr);
    }

    void XESimpleRenderSystem::createPipelineLayout() {
        VkPushConstantRange push_constant_range = {};
        push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        push_constant_range.offset = 0;
        push_constant_range.size = sizeof(SimplePushConstantData);


        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &push_constant_range;

        if (vkCreatePipelineLayout(xe_device.device(), &pipelineLayoutInfo, nullptr, &xe_pipeline_layout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }
    }

    void XESimpleRenderSystem::createPipeline(VkRenderPass renderPass) {
        assert(xe_pipeline_layout != nullptr && "Cannot create pipeline before pipeline layout!");

        PipelineConfigInfo pipelineConfig = {};
        XEPipeline::defaultPipelineConfigInfo(pipelineConfig);
        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = xe_pipeline_layout;
        xe_pipeline = std::make_unique<XEPipeline>(xe_device,
            "assets\\shaders\\simple_shader.spv",
            "assets\\shaders\\simple_fragment.spv",
            pipelineConfig);
    }

    void XESimpleRenderSystem::renderGameObjects(FrameInfo& frame_info, VkDescriptorSet shadowSamplerDescriptorSet) {
        xe_pipeline->bind(frame_info.commandBuffer);

        vkCmdBindDescriptorSets(
            frame_info.commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            xe_pipeline_layout,
            0, 1,
            &frame_info.globalDescriptorSet,
            0,
            nullptr);

        textureSet = textureManager.getDescriptorSet();

        vkCmdBindDescriptorSets(
            frame_info.commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            xe_pipeline_layout,
            1, 1,
            &textureSet,
            0,
            nullptr);

        vkCmdBindDescriptorSets(
            frame_info.commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            xe_pipeline_layout,
            2, 1,
            &frame_info.lightDescriptorSet,
            0,
            nullptr);

        vkCmdBindDescriptorSets(
            frame_info.commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            xe_pipeline_layout,
            3, 1,
            &shadowSamplerDescriptorSet,
            0,
            nullptr);

        for (auto& kv: frame_info.gameObjects) {
            auto& obj = kv.second;
            obj.model->bind(frame_info.commandBuffer);

            for (auto& mesh: obj.model->getMeshes()) {
                SimplePushConstantData push = {};
                XEMaterial material = materialManager.getMaterial(mesh.materialIndex);
                push.modelMatrix = obj.transform.mat4();
                push.textureIndex = material.albedoIndex;
                push.normalIndex = material.normalIndex;

                vkCmdPushConstants(
                    frame_info.commandBuffer,
                    xe_pipeline_layout,
                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    0,
                    sizeof(SimplePushConstantData),
                    &push);
                obj.model->drawMesh(frame_info.commandBuffer, mesh);
            }
        }
    }

}
