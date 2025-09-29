//
// Created by adity on 04-09-2025.
//

#pragma once

#include "renderer/lighting/xe_lights.h"
#include "renderer/xe_descriptors.h"
#include "renderer/xe_device.h"
#include "renderer/xe_buffer.h"
#include "vulkan/vulkan.h"

#include "memory"
#include "vector"

namespace xe {
    struct alignas(16) LightSSBOHeader {
        uint32_t count{0};
        uint32_t pad1{0}, pad2{0}, pad3{0};
    };

    class XELightManager {
    public:
        XELightManager(XEDevice& device, uint32_t framesInFlight,uint32_t initialCapacity);
        ~XELightManager();

        XELightManager(const XELightManager&) = delete;
        XELightManager& operator=(const XELightManager&) = delete;

        // CPU side
        void clear();  // call once per frame, before filling
        uint32_t addLight(GPULight& light);  // return index
        void setLight(uint32_t index, GPULight& light);  // modify properties of light at a specific index
        void reserve(uint32_t minCapacity);  // optional manual grow
        void upload(uint32_t frameIndex);  // Upload SSBO to gpu at index frameIndex

        void createOrthographicProjection();

        // Return descriptor index
        VkDescriptorSet descriptorSet(uint32_t frameIndex) const { return LightSSBODescriptorSets[frameIndex]; }
        VkDescriptorSetLayout getDescriptorLayout() const { return LightSSBOSetLayout->getDescriptorSetLayout(); }

    private:
        struct PerFrame {
            std::unique_ptr<XEBuffer> buffer; // single blob: header + lights[]
            VkDeviceSize totalSize{0};
        };

        void allocateDescriptorSets_();
        void createBuffers_(uint32_t capacityLights);
        void destroyBuffers_();
        void rewriteDescriptorSets_(); // (re)point sets to the per-frame buffers

        XEDevice& device;
        uint32_t framesInFlight;

        // CPU side Buffer
        std::vector<GPULight> gpuLights{};
        LightSSBOHeader hdr{}; // filled during upload

        // GPU side resources
        std::vector<PerFrame> perFrameLightBuffers;
        uint32_t capacity{0};

        std::unique_ptr<XEDescriptorPool> LightSSBOPool{};
        std::unique_ptr<XEDescriptorSetLayout> LightSSBOSetLayout{};
        std::vector<VkDescriptorSet> LightSSBODescriptorSets{};
    };
}
