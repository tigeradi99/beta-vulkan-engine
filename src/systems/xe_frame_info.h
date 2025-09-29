//
// Created by adity on 23-07-2025.
//

#pragma once

#include "systems/xe_camera.h"
#include "scene/xe_game_object.h"

#include "vulkan/vulkan.h"

namespace xe {
    struct FrameInfo {
        int frameIndex;
        float frameTime;
        VkCommandBuffer commandBuffer;
        XECamera &camera;
        VkDescriptorSet globalDescriptorSet;
        VkDescriptorSet lightDescriptorSet;
        XEGameObject::Map &gameObjects;
    };
}
