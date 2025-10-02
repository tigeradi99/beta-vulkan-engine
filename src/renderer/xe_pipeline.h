//
// Created by adity on 07-07-2025.
//

#pragma once
#include "renderer/xe_device.h"
#include "renderer/xe_model.h"

#include <string>
#include <vector>

namespace xe {

    struct PipelineConfigInfo {
        PipelineConfigInfo(const PipelineConfigInfo&) = delete;
        PipelineConfigInfo& operator=(const PipelineConfigInfo&) = delete;

        std::vector<VkVertexInputBindingDescription> bindingDescriptions;
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
        VkPipelineViewportStateCreateInfo viewportInfo{};
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
        VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
        VkPipelineMultisampleStateCreateInfo multisampleInfo{};
        VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
        std::vector<VkDynamicState> dynamicStateEnables{};
        VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
        VkPipelineLayout pipelineLayout = nullptr;
        VkRenderPass renderPass = nullptr;

        uint32_t subpass = 0;
    };

    class XEPipeline {
    public:
        XEPipeline(XEDevice& device,
            const std::string& vertFilePath,
            const std::string& fragFilePath,
            const PipelineConfigInfo& configInfo,
            const std::string& type);

        // For shadow render pass
        XEPipeline(XEDevice& device,
            const std::string& vertFilePath,
            const PipelineConfigInfo& configInfo);
        ~XEPipeline();

        XEPipeline(const XEPipeline&) = delete;
        XEPipeline& operator=(const XEPipeline&) = delete;

        void bind(VkCommandBuffer commandBuffer);

        static void defaultPipelineConfigInfo(PipelineConfigInfo& configInfo);
        static void defaultShadowPipelineConfigInfo(PipelineConfigInfo& configInfo);
        static void defaultSkyboxPipelineConfigInfo(PipelineConfigInfo& configInfo);

    private:
        static std::vector<char> readFile(const std::string& filePath);

        void createGraphicsPipeline(const std::string& vertFilePath,
            const std::string& fragFilePath,
            const PipelineConfigInfo& configInfo);
        void createShadowPassPipeline(const std::string& vertFilePath, const PipelineConfigInfo& configInfo);
        auto createSkyboxPipeline(const std::string &vertFilePath,
                                  const std::string &fragFilePath,
                                  const PipelineConfigInfo &configInfo) -> void;

        void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);

        XEDevice& xe_device;
        VkPipeline graphicsPipeline;
        VkShaderModule vertShaderModule = VK_NULL_HANDLE;
        VkShaderModule fragShaderModule = VK_NULL_HANDLE;
    };
}
