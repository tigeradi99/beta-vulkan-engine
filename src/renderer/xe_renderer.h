//
// Created by adity on 10-07-2025.
//

#pragma once

#include "platform/xe_window.h"
#include "renderer/xe_device.h"
#include "renderer/xe_swap_chain.h"

#include <memory>
#include <cassert>

namespace xe {

    class XERenderer {
    public:
        XERenderer(XEWindow& xe_window, XEDevice& xe_device);
        ~XERenderer();

        XERenderer(const XERenderer &) = delete;
        XERenderer &operator=(const XERenderer &) = delete;

        bool isFrameInProgress() const { return isFrameStarted; }
        float getAspectRatio() const { return xe_swap_chain->extentAspectRatio(); }
        VkRenderPass getSwapChainRenderPass() const { return xe_swap_chain->getRenderPass(); }

        VkCommandBuffer getCurrentCommandBuffer() const {
            assert(isFrameStarted && "Cannot get command buffer when frame not in progress.");
            return xe_command_buffers[currentFrameIndex];
        }

        int getFrameIndex() const {
            assert(isFrameStarted && "Cannot get frame index when frame not in progress.");
            return currentFrameIndex;
        }

        VkCommandBuffer beginFrame();
        void endFrame();
        void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
        void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

    private:
        void createCommandBuffers();
        void freeCommandBuffers();
        void recreateSwapChain();

        XEWindow& xe_window;
        XEDevice& xe_device;
        std::unique_ptr<XESwapChain> xe_swap_chain;
        std::vector<VkCommandBuffer> xe_command_buffers;

        uint32_t currentImageIndex = 0;
        int currentFrameIndex = 0;
        bool isFrameStarted = false;

    };
}