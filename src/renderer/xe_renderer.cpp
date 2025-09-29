//
// Created by adity on 10-07-2025.
//

#include "xe_renderer.h"

#include <stdexcept>
#include <array>
#include <cassert>

namespace xe {
    XERenderer::XERenderer(XEWindow& window, XEDevice& device): xe_window(window), xe_device(device) {
        recreateSwapChain();
        createCommandBuffers();
    }

    XERenderer::~XERenderer() {
        freeCommandBuffers();
    }

    void XERenderer::recreateSwapChain() {
        auto extent = xe_window.getExtent();

        while (extent.width == 0 || extent.height == 0) {
            extent = xe_window.getExtent();
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(xe_device.device());

        if (xe_swap_chain == nullptr) {
            xe_swap_chain = std::make_unique<XESwapChain>(xe_device, extent);
        } else {
            std::shared_ptr<XESwapChain> oldSwapChain = std::move(xe_swap_chain);
            xe_swap_chain = std::make_unique<XESwapChain>(xe_device, extent, oldSwapChain);

            if (!oldSwapChain->compareSwapChainFormats(*xe_swap_chain.get())) {
                throw std::runtime_error("Swap image(or depth) formats don't match");

            }
        }
    }

    void XERenderer::createCommandBuffers() {
        xe_command_buffers.resize(XESwapChain::MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo cmdBufferAllocateInfo = {};
        cmdBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdBufferAllocateInfo.commandPool = xe_device.getCommandPool();
        cmdBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(xe_command_buffers.size());

        if (vkAllocateCommandBuffers(xe_device.device(), &cmdBufferAllocateInfo, xe_command_buffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    void XERenderer::freeCommandBuffers() {
        vkFreeCommandBuffers(xe_device.device(),
            xe_device.getCommandPool(),
            static_cast<uint32_t>(xe_command_buffers.size()),
            xe_command_buffers.data());
        xe_command_buffers.clear();
    }

    VkCommandBuffer XERenderer::beginFrame() {
        assert(!isFrameStarted && "Can't call beginFrame when already in progress");
        auto result = xe_swap_chain->acquireNextImage(&currentImageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return nullptr;
        }

        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        isFrameStarted = true;

        auto commandBuffer = getCurrentCommandBuffer();
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin command buffer operation!");
        }

        return commandBuffer;
    }

    void XERenderer::endFrame() {
        assert(isFrameStarted && "Can't call endFrame without starting frame render!!");
        auto commandBuffer = getCurrentCommandBuffer();

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer operation!");
        }

        auto result = xe_swap_chain->submitCommandBuffers(&commandBuffer, &currentImageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || xe_window.wasWindowResized()) {
            xe_window.resetWindowResizeFlag();
            recreateSwapChain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to submit swap chain command buffer operation!");
        }

        isFrameStarted = false;
        currentFrameIndex = (currentFrameIndex + 1) % XESwapChain::MAX_FRAMES_IN_FLIGHT;
    }

    void XERenderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer) {
        assert(isFrameStarted && "Can't call swapChainRenderPass without starting frame render!!");
        assert(commandBuffer == getCurrentCommandBuffer() &&
            "Can't begin a render pass on a command buffer from a different frame!");

        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = xe_swap_chain->getRenderPass();
        renderPassInfo.framebuffer = xe_swap_chain->getFrameBuffer(currentImageIndex);

        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = xe_swap_chain->getSwapChainExtent();

        std::array<VkClearValue, 2> clearValues = {};
        clearValues[0].color = {0.1f, 0.1f, 0.1f, 1.0f};
        clearValues[1].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(xe_swap_chain->getSwapChainExtent().width);
        viewport.height = static_cast<float>(xe_swap_chain->getSwapChainExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor = {};
        scissor.extent = xe_swap_chain->getSwapChainExtent();
        scissor.offset = {0, 0};

        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    }

    void XERenderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer) {
        assert(isFrameStarted && "Can't call swapChainRenderPass without starting frame render!!");
        assert(commandBuffer == getCurrentCommandBuffer() &&
            "Can't begin a render pass on a command buffer from a different frame!");

        vkCmdEndRenderPass(commandBuffer);
    }

}
