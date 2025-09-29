//
// Created by adity on 06-09-2025.
//

#pragma once

#include "vulkan/vulkan.h"
#include "renderer/xe_pipeline.h"
#include "renderer/xe_device.h"
#include "renderer/xe_image_vma.h"
#include "scene/xe_game_object.h"
#include "systems/xe_camera.h"
#include "systems/xe_frame_info.h"
#include "renderer/lighting/xe_light_manager.h"
#include "renderer/xe_descriptors.h"
#include "renderer/xe_buffer.h"
#include "renderer/lighting/xe_lights.h"
#include "utils/xe_utils.h"

#include <memory>
#include <array>

#define SHADOW_MAP_CASCADE_COUNT 4

namespace xe {
    struct ShadowUbo {
        std::array<glm::mat4, SHADOW_MAP_CASCADE_COUNT> cascadeLightProjections;
        std::array<glm::mat4, SHADOW_MAP_CASCADE_COUNT> cascadeLightViews;
        std::array<float, SHADOW_MAP_CASCADE_COUNT> splitDepths;
    };

    struct LightBBResult {
        glm::mat4 lightProjection{1.f};
        glm::mat4 lightView{1.f};
    };

    struct CascadeInfo {
        std::array<VkFramebuffer, SHADOW_MAP_CASCADE_COUNT> frameBuffers;
        std::array<VkImageView, SHADOW_MAP_CASCADE_COUNT> shadowLayerImageViews;
    };

    class XEShadowSystem {
    public:
        XEShadowSystem(XEDevice& device, XELightManager& lightManager);
        ~XEShadowSystem();

        XEShadowSystem(const XEShadowSystem &) = delete;
        XEShadowSystem &operator=(const XEShadowSystem &) = delete;

        void renderGameObjects(FrameInfo& frame_info, GPULight sunLight);
        VkDescriptorSetLayout getDescriptorSetLayout() { return shadowPassDescriptorSetLayout->getDescriptorSetLayout(); }
        VkDescriptorSet getDescriptorSet(int index) { return shadowPassDescriptorSets[index]; }

    private:
        void createPipelineLayout();
        void createShadowRenderPass();
        void createPipeline();
        void createDescriptorPool();
        void createDescriptorSetLayout();
        void initializeDescriptorSet();
        void createDepthImagesAndViews();
        void createSamplers();
        void createFramebuffers();

        void beginShadowRenderPass(VkCommandBuffer commandBuffer, int frameIndex, int cascade);
        void endShadowRenderPass(VkCommandBuffer commandBuffer);

        void calculateSplitDepths(float nearClip, float farClip);
        const std::array<glm::vec3, 4> getNearPlane(float fov_y, float cam_aspect, float z_near);
        const std::array<glm::vec3, 4> getFarPlane(float fov_y, float cam_aspect, float z_far);

        LightBBResult createViewVolumeAndCalcLightPos(const std::array<glm::vec3, 4>& zNearPlane,
            const std::array<glm::vec3, 4>& zFarPlane, const glm::mat4& invViewMatrix,
            const glm::vec3& directionalLightDir);

        XEDevice& xe_device;
        std::unique_ptr<xe::XEPipeline> xe_pipeline;
        VkPipelineLayout xe_pipeline_layout{VK_NULL_HANDLE};
        VkRenderPass shadowRenderPass{VK_NULL_HANDLE};
        
        XELightManager& lightManager;

        std::unique_ptr<XEDescriptorPool> shadowPassDescriptorPool;
        std::unique_ptr<XEDescriptorSetLayout> shadowPassDescriptorSetLayout;
        std::vector<std::unique_ptr<XEImageVMA>> shadowXEImages;
        std::vector<VkDescriptorImageInfo> shadowDescriptorImageInfo;
        VkSampler shadowDepthSampler = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> shadowPassDescriptorSets;
        std::vector<std::unique_ptr<XEBuffer>> shadowUboBuffers;
        std::vector<CascadeInfo> cascadeInfos;

        int shadowMapWidth = 2048;
        int shadowMapHeight = 2048;
        std::array<int, 4> shadowMapDimensions{4096, 4096, 2048, 2048};

        ShadowUbo shadowUbo{};

        float cascadeSplitLambda = 0.73f;
        const float slope[4]  = {1.25f, 1.5f, 1.75f, 2.0f};
        const float constB[4] = {0.0005f, 0.0010f, 0.0020f, 0.0030f};
    };
}

