//
// Created by adity on 06-09-2025.
//

#include "systems/xe_shadow_system.h"

#include <iostream>
#include <stdexcept>
#include <algorithm>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"

#include "renderer/xe_swap_chain.h"

namespace xe {

    struct SimplePushConstantData {
        glm::mat4 modelMatrix{1.f};
        alignas(16) int32_t cascadeIndex{0};
    };

    XEShadowSystem::XEShadowSystem(XEDevice &device, XELightManager &lightManager):
      xe_device(device), lightManager(lightManager){

        shadowUboBuffers.resize(XESwapChain::MAX_FRAMES_IN_FLIGHT);

        for (int i = 0; i < shadowUboBuffers.size(); i++) {
            shadowUboBuffers[i] = std::make_unique<XEBuffer>(
            xe_device,
            sizeof(ShadowUbo),
            1,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            xe_device.properties.limits.minUniformBufferOffsetAlignment);

            shadowUboBuffers[i]->map();
        }

        // cascade info per frame
        cascadeInfos.resize(XESwapChain::MAX_FRAMES_IN_FLIGHT);

        createShadowRenderPass();
        createDepthImagesAndViews();
        createSamplers();
        createFramebuffers();

        createDescriptorPool();
        createDescriptorSetLayout();
        initializeDescriptorSet();

        createPipelineLayout();
        createPipeline();
    }

    XEShadowSystem::~XEShadowSystem() {
        vkDestroyPipelineLayout(xe_device.device(), xe_pipeline_layout, nullptr);


        vkDestroySampler(xe_device.device(), shadowDepthSampler, nullptr);

        for (CascadeInfo& cascadeInfo : cascadeInfos) {
            for (VkFramebuffer& frameBuffer: cascadeInfo.frameBuffers) {
                vkDestroyFramebuffer(xe_device.device(), frameBuffer, nullptr);
            }
            for (VkImageView& imageView: cascadeInfo.shadowLayerImageViews) {
                vkDestroyImageView(xe_device.device(), imageView, nullptr);
            }
        }
        vkDestroyRenderPass(xe_device.device(), shadowRenderPass, nullptr);

    }

    void XEShadowSystem::renderGameObjects(FrameInfo &frame_info, GPULight sunLight) {
        calculateSplitDepths(frame_info.camera.getNearClip(), frame_info.camera.getFarClip());

        for (int cascade = 0; cascade < SHADOW_MAP_CASCADE_COUNT; cascade++) {
            beginShadowRenderPass(frame_info.commandBuffer, frame_info.frameIndex, cascade);

            xe_pipeline->bind(frame_info.commandBuffer);

            glm::vec3 sunLightDirToOrigin = -glm::normalize(glm::vec3(sunLight.direction));

            float zNear = (cascade == 0) ? frame_info.camera.getNearClip() : shadowUbo.splitDepths[cascade - 1];
            float zFar = shadowUbo.splitDepths[cascade];

            std::array<glm::vec3, 4> nearPlane = getNearPlane(frame_info.camera.getFOV(), frame_info.camera.getAspect(),
                zNear);
            std::array<glm::vec3, 4> farPlane = getFarPlane(frame_info.camera.getFOV(), frame_info.camera.getAspect(),
                zFar);

            LightBBResult result = createViewVolumeAndCalcLightPos(nearPlane, farPlane, frame_info.camera.getInverseView(), sunLightDirToOrigin);

            shadowUbo.cascadeLightProjections[cascade] = result.lightProjection;
            shadowUbo.cascadeLightViews[cascade] = result.lightView;
            shadowUboBuffers[frame_info.frameIndex]->writeToBuffer(&shadowUbo);
            shadowUboBuffers[frame_info.frameIndex]->flush();

            vkCmdBindDescriptorSets(
                frame_info.commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                xe_pipeline_layout,
                0, 1,
                &shadowPassDescriptorSets[frame_info.frameIndex],
                0,
                nullptr);

            for (auto& kv: frame_info.gameObjects) {
                auto& obj = kv.second;

                //if (!obj.canCastShadow) { continue; }
                obj.model->bind(frame_info.commandBuffer);

                for (auto& mesh: obj.model->getMeshes()) {
                    SimplePushConstantData push = {};
                    push.modelMatrix = obj.transform.mat4();
                    push.cascadeIndex = cascade;

                    vkCmdPushConstants(
                        frame_info.commandBuffer,
                        xe_pipeline_layout,
                        VK_SHADER_STAGE_VERTEX_BIT,
                        0,
                        sizeof(SimplePushConstantData),
                        &push);

                    obj.model->drawMesh(frame_info.commandBuffer, mesh);
                }
            }

            endShadowRenderPass(frame_info.commandBuffer);
        }
    }

    void XEShadowSystem::createPipelineLayout() {
        VkPushConstantRange push_constant_range = {};
        push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        push_constant_range.offset = 0;
        push_constant_range.size = sizeof(SimplePushConstantData);

        std::array<VkDescriptorSetLayout, 1> setLayouts = {shadowPassDescriptorSetLayout->getDescriptorSetLayout()};

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
        pipelineLayoutInfo.pSetLayouts = setLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &push_constant_range;

        if (vkCreatePipelineLayout(xe_device.device(), &pipelineLayoutInfo, nullptr, &xe_pipeline_layout) != VK_SUCCESS) {
            std::cerr << "failed to create pipeline layout" << std::endl;
            throw std::runtime_error("failed to create pipeline layout!");
        }
    }

    void XEShadowSystem::createShadowRenderPass() {
        VkAttachmentDescription depth = {};

        depth.format = VK_FORMAT_D32_SFLOAT;
        depth.samples = VK_SAMPLE_COUNT_1_BIT;
        depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depth.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkAttachmentReference depthRef = {};
        depthRef.attachment = 0;
        depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 0;
        subpass.pDepthStencilAttachment = &depthRef;

        // Optional: external deps to be explicit about ordering
        std::array<VkSubpassDependency, 2> deps{};
        // External -> subpass (write depth)
        deps[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        deps[0].dstSubpass = 0;
        deps[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        deps[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        deps[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        deps[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        deps[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        // Subpass -> external (read depth)
        deps[1].srcSubpass    = 0;
        deps[1].dstSubpass    = VK_SUBPASS_EXTERNAL;
        deps[1].srcStageMask  = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        deps[1].dstStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        deps[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        deps[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        deps[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &depth;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = static_cast<uint32_t>(deps.size());
        renderPassInfo.pDependencies = deps.data();

        if (vkCreateRenderPass(xe_device.device(), &renderPassInfo, nullptr, &shadowRenderPass) != VK_SUCCESS) {
            std::cerr << "failed to create render pass" << std::endl;
            throw std::runtime_error("failed to create render pass!");
        }
    }

    void XEShadowSystem::createPipeline() {
        assert(xe_pipeline_layout != nullptr && "Cannot create pipeline before pipeline layout!");

        PipelineConfigInfo pipelineConfig = {};
        XEPipeline::defaultShadowPipelineConfigInfo(pipelineConfig);
        
        pipelineConfig.renderPass = shadowRenderPass;
        pipelineConfig.pipelineLayout = xe_pipeline_layout;
        pipelineConfig.subpass = 0;

        xe_pipeline = std::make_unique<XEPipeline>(xe_device,
            "assets\\shaders\\shadow_shader.spv",
            pipelineConfig);

    }

    void XEShadowSystem::createDescriptorPool() {
        shadowPassDescriptorPool = XEDescriptorPool::Builder(xe_device)
        .setMaxSets(XESwapChain::MAX_FRAMES_IN_FLIGHT)
        .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, XESwapChain::MAX_FRAMES_IN_FLIGHT)
        .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, XESwapChain::MAX_FRAMES_IN_FLIGHT)
        .build();
    }

    void XEShadowSystem::createDescriptorSetLayout() {
        shadowPassDescriptorSetLayout = XEDescriptorSetLayout::Builder(xe_device)
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
        .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .build();
    }

    void XEShadowSystem::initializeDescriptorSet() {
        shadowPassDescriptorSets.resize(XESwapChain::MAX_FRAMES_IN_FLIGHT);

        for (int i = 0; i < shadowPassDescriptorSets.size(); i++) {
            auto bufferInfo = shadowUboBuffers[i]->descriptorInfo();
            XEDescriptorWriter(*shadowPassDescriptorSetLayout, *shadowPassDescriptorPool)
            .writeBuffer(0, &bufferInfo)
            .writeImage(1, &shadowDescriptorImageInfo[i])
            .build(shadowPassDescriptorSets[i]);
        }

    }

    void XEShadowSystem::createDepthImagesAndViews() {
        shadowXEImages.resize(XESwapChain::MAX_FRAMES_IN_FLIGHT);

        for (int i = 0; i < shadowXEImages.size(); i++) {
            shadowXEImages[i] = std::make_unique<XEImageVMA>(xe_device, shadowMapWidth, shadowMapHeight,
                SHADOW_MAP_CASCADE_COUNT,
                VK_FORMAT_D32_SFLOAT,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY);
        }

        // create a single layer image view per cascade for each frame. [frame][cascade]
        for (int f = 0; f < cascadeInfos.size(); f++) {
            VkImage image = shadowXEImages[f]->getImage();

            for (int i = 0; i < cascadeInfos[f].shadowLayerImageViews.size(); i++) {
                // Image view for this cascade's layer (inside the depth map)
                // This view is used to render to that specific depth image layer
                VkImageViewCreateInfo viewInfo{};
                viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                viewInfo.image = image;
                viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                viewInfo.format = VK_FORMAT_D32_SFLOAT;
                viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                viewInfo.subresourceRange.baseMipLevel = 0;
                viewInfo.subresourceRange.levelCount = 1;
                viewInfo.subresourceRange.baseArrayLayer = i;
                viewInfo.subresourceRange.layerCount = 1;

                if (vkCreateImageView(xe_device.device(), &viewInfo, nullptr,
                    &cascadeInfos[f].shadowLayerImageViews[i]) != VK_SUCCESS) {
                    throw std::runtime_error("Failed to create shadow image view!!");
                }
            }
        }
    }

    void XEShadowSystem::createSamplers() {
        VkSamplerCreateInfo samplerInfo{};

        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        samplerInfo.anisotropyEnable = VK_FALSE;

        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_TRUE;
        samplerInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        if (vkCreateSampler(xe_device.device(), &samplerInfo, nullptr, &shadowDepthSampler) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create texture sampler!!");
        }

        shadowDescriptorImageInfo.resize(XESwapChain::MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < shadowDescriptorImageInfo.size(); i++) {
            shadowDescriptorImageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            shadowDescriptorImageInfo[i].imageView = shadowXEImages[i]->getImageView();
            shadowDescriptorImageInfo[i].sampler = shadowDepthSampler;
        }
    }

    void XEShadowSystem::createFramebuffers() {

        for (CascadeInfo& cascadeInfo : cascadeInfos) {

            for (int i = 0; i < cascadeInfo.frameBuffers.size(); i++) {
                VkImageView attachments = cascadeInfo.shadowLayerImageViews[i];

                VkFramebufferCreateInfo framebufferInfo = {};
                framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebufferInfo.renderPass = shadowRenderPass;
                framebufferInfo.attachmentCount = 1;
                framebufferInfo.pAttachments = &attachments;
                framebufferInfo.width = shadowMapWidth;
                framebufferInfo.height = shadowMapHeight;
                framebufferInfo.layers = 1;

                if (vkCreateFramebuffer(xe_device.device(), &framebufferInfo, nullptr,
                    &cascadeInfo.frameBuffers[i]) != VK_SUCCESS) {
                    std::cerr << "failed to create shadow pass framebuffer" << std::endl;
                    throw std::runtime_error("failed to create shadow pass framebuffer!");
                }
            }

        }
    }

    void XEShadowSystem::beginShadowRenderPass(VkCommandBuffer commandBuffer, int frameIndex, int cascade) {
        VkRenderPassBeginInfo shadowRenderPassInfo = {};
        shadowRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        shadowRenderPassInfo.renderPass = shadowRenderPass;
        shadowRenderPassInfo.framebuffer = cascadeInfos[frameIndex].frameBuffers[cascade];
        shadowRenderPassInfo.renderArea.offset = {0, 0};
        shadowRenderPassInfo.renderArea.extent = {static_cast<uint32_t>(shadowMapWidth),
            static_cast<uint32_t>(shadowMapHeight)};

        VkClearValue clearDepthValues = {};
        clearDepthValues.depthStencil = {1.0f, 0};

        shadowRenderPassInfo.clearValueCount = 1;
        shadowRenderPassInfo.pClearValues = &clearDepthValues;

        vkCmdBeginRenderPass(commandBuffer, &shadowRenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(shadowMapWidth);
        viewport.height = static_cast<float>(shadowMapHeight);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{{0,0}, {static_cast<uint32_t>(shadowMapWidth),
                        static_cast<uint32_t>(shadowMapHeight)}};

        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        // Tune these two at runtime to fight acne/peter-panning
        vkCmdSetDepthBias(commandBuffer, constB[cascade], 0.0f, slope[cascade]);
    }

    void XEShadowSystem::endShadowRenderPass(VkCommandBuffer commandBuffer) {
        vkCmdEndRenderPass(commandBuffer);
    }

    void XEShadowSystem::calculateSplitDepths(float nearClip, float farClip) {
        float clipRange = farClip - nearClip;

        float minZ = nearClip;
        float maxZ = nearClip + clipRange;

        float range = maxZ - minZ;
        float ratio = maxZ / minZ;

        for (int i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
            float p = (i + 1) / static_cast<float>(SHADOW_MAP_CASCADE_COUNT);
            float log = minZ * std::pow(ratio, p);
            float uniform = minZ + range * p;
            shadowUbo.splitDepths[i] = cascadeSplitLambda * (log - uniform) + uniform;
        }
    }

    const std::array<glm::vec3, 4> XEShadowSystem::getNearPlane(float fov_y, float cam_aspect, float z_near) {
        const float tanHalfFovy = tan(fov_y / 2.f);
        float znear_y = z_near * tanHalfFovy;
        float znear_x = znear_y * cam_aspect;

        std::array<glm::vec3, 4> nearPlane = {glm::vec3(znear_x, znear_y, z_near),
                      glm::vec3(znear_x, -znear_y, z_near),
                      glm::vec3(-znear_x, -znear_y, z_near),
                      glm::vec3(-znear_x, znear_y, z_near)};

        return nearPlane;
    }

    const std::array<glm::vec3, 4> XEShadowSystem::getFarPlane(float fov_y, float cam_aspect, float z_far) {
        const float tanHalfFovy = tan(fov_y / 2.f);
        float zfar_y = z_far * tanHalfFovy;
        float zfar_x = zfar_y * cam_aspect;

        std::array<glm::vec3, 4> farPlane = {glm::vec3(zfar_x, zfar_y, z_far),
                        glm::vec3(zfar_x, -zfar_y, z_far),
                        glm::vec3(-zfar_x, -zfar_y, z_far),
                        glm::vec3(-zfar_x, zfar_y, z_far)};

        return farPlane;
    }

    LightBBResult XEShadowSystem::createViewVolumeAndCalcLightPos(const std::array<glm::vec3, 4>& zNearPlane,
                                                                  const std::array<glm::vec3, 4>& zFarPlane, const glm::mat4& invViewMatrix, const glm::vec3& directionalLightDir) {

        // Frustum coordinates in world space
        glm::vec3 nearTopRightWorld = glm::vec3(invViewMatrix * glm::vec4(zNearPlane[0] ,1.0));
        glm::vec3 nearBottomRightWorld = glm::vec3(invViewMatrix * glm::vec4(zNearPlane[1] ,1.0));
        glm::vec3 nearBottomLeftWorld = glm::vec3(invViewMatrix * glm::vec4(zNearPlane[2] ,1.0));
        glm::vec3 nearTopLeftWorld = glm::vec3(invViewMatrix * glm::vec4(zNearPlane[3] ,1.0));

        glm::vec3 farTopRightWorld = glm::vec3(invViewMatrix * glm::vec4(zFarPlane[0] ,1.0));
        glm::vec3 farBottomRightWorld = glm::vec3(invViewMatrix * glm::vec4(zFarPlane[1] ,1.0));
        glm::vec3 farBottomLeftWorld = glm::vec3(invViewMatrix * glm::vec4(zFarPlane[2] ,1.0));
        glm::vec3 farTopLeftWorld = glm::vec3(invViewMatrix * glm::vec4(zFarPlane[3] ,1.0));

        std::vector<glm::vec3> frustumWS = { nearTopRightWorld, nearBottomRightWorld, nearBottomLeftWorld,
            nearTopLeftWorld, farTopRightWorld, farBottomRightWorld, farBottomLeftWorld, farTopLeftWorld};

        // Frustum center in world
        glm::vec3 centerWorldSpace = glm::vec3(0.0f, 0.0f, 0.0f);
        for (const auto& p: frustumWS) {
            centerWorldSpace += p;
        }
        centerWorldSpace /=  8.0f;

        // Radius  - max distance from center (used to calculate light view matrix)
        float radius = 0.0f;
        for (const auto& p: frustumWS) {
            radius = std::max(radius, glm::length(p - centerWorldSpace));
        }
        radius = std::ceil(radius * 16.0f) / 16.0f;

        glm::vec3 maxExtents = glm::vec3{radius, radius, radius};
        glm::vec3 minExtents = -maxExtents;

        // Build LightViewMatrix (Left handed)
        const glm::vec3 worldUp(0,1,0);
        glm::vec3 up = (std::abs(glm::dot(directionalLightDir, worldUp)) > 0.99f) ? glm::vec3(0,0,1) : worldUp;
        glm::vec3 eye = centerWorldSpace - directionalLightDir * -minExtents.z;

        glm::mat4 lightView = glm::lookAtLH(eye, centerWorldSpace, up);
        glm::mat4 lightProjection = glm::orthoLH_ZO(minExtents.x, maxExtents.x,
                                              minExtents.y, maxExtents.y,
                                              0.0f, maxExtents.z - minExtents.z);
        lightProjection[1][1] *= -1;

        return LightBBResult{lightProjection, lightView};
    }

}
