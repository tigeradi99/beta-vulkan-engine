//
// Created by adity on 23-07-2025.
//

#pragma once

#include "renderer/xe_device.h"

#include <memory>
#include <unordered_map>
#include <vector>

#include "imgui_impl_vulkan.h"

namespace xe
{
    class XEDescriptorSetLayout
    {
    public:
        class Builder
        {
        public:
            Builder(XEDevice &xe_device) : xe_device{xe_device} {}

            Builder &addBinding(
                uint32_t binding,
                VkDescriptorType descriptorType,
                VkShaderStageFlags stageFlags,
                uint32_t count = 1);
            std::unique_ptr<XEDescriptorSetLayout> build() const;

        private:
            XEDevice &xe_device;
            std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};
        };

        XEDescriptorSetLayout(
            XEDevice &xe_device, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings);
        ~XEDescriptorSetLayout();
        XEDescriptorSetLayout(const XEDescriptorSetLayout &) = delete;
        XEDescriptorSetLayout &operator=(const XEDescriptorSetLayout &) = delete;

        VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }

    private:
        XEDevice &xe_device;
        VkDescriptorSetLayout descriptorSetLayout;
        std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;

        friend class XEDescriptorWriter;
    };

    class XEDescriptorPool
    {
    public:
        class Builder
        {
        public:
            Builder(XEDevice &xe_device) : xe_device{xe_device} {}

            Builder &addPoolSize(VkDescriptorType descriptorType, uint32_t count);
            Builder &setPoolFlags(VkDescriptorPoolCreateFlags flags);
            Builder &setMaxSets(uint32_t count);
            std::unique_ptr<XEDescriptorPool> build() const;

        private:
            XEDevice &xe_device;
            std::vector<VkDescriptorPoolSize> poolSizes{};
            uint32_t maxSets = 1000;
            VkDescriptorPoolCreateFlags poolFlags = 0;
        };

        XEDescriptorPool(
            XEDevice &xe_device,
            uint32_t maxSets,
            VkDescriptorPoolCreateFlags poolFlags,
            const std::vector<VkDescriptorPoolSize> &poolSizes);
        ~XEDescriptorPool();
        XEDescriptorPool(const XEDescriptorPool &) = delete;
        XEDescriptorPool &operator=(const XEDescriptorPool &) = delete;

        bool allocateDescriptor(
            const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet &descriptor) const;

        void freeDescriptors(std::vector<VkDescriptorSet> &descriptors) const;

        void resetPool();

    private:
        XEDevice &xe_device;
        VkDescriptorPool descriptorPool;

        friend class XEDescriptorWriter;
    };

    class XEDescriptorWriter
    {
    public:
        XEDescriptorWriter(XEDescriptorSetLayout &setLayout, XEDescriptorPool &pool);

        XEDescriptorWriter &writeBuffer(uint32_t binding, VkDescriptorBufferInfo *bufferInfo);
        XEDescriptorWriter &writeImage(uint32_t binding, VkDescriptorImageInfo *imageInfo);
        XEDescriptorWriter &writeImageArray(uint32_t binding, std::vector<VkDescriptorImageInfo>& imageInfos);

        bool build(VkDescriptorSet &set);
        void overwrite(VkDescriptorSet &set);

    private:
        XEDescriptorSetLayout &setLayout;
        XEDescriptorPool &pool;
        std::vector<VkWriteDescriptorSet> writes;
    };
}
