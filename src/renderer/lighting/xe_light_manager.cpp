//
// Created by adity on 04-09-2025.
//

#include "renderer/lighting/xe_light_manager.h"

#include <algorithm>
#include <cassert>


namespace xe {
    XELightManager::XELightManager(XEDevice &device, uint32_t framesInFlight, uint32_t initialCapacity):
    device(device), framesInFlight(framesInFlight) {

        LightSSBOPool = XEDescriptorPool::Builder(device)
        .setMaxSets(framesInFlight)
        .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, framesInFlight)
        .build();

        LightSSBOSetLayout = XEDescriptorSetLayout::Builder(device)
        .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
        .build();

        LightSSBODescriptorSets.resize(framesInFlight);

        createBuffers_(std::max(16u, initialCapacity));

        allocateDescriptorSets_();
    }

    XELightManager::~XELightManager() {
        destroyBuffers_();
    }

    void XELightManager::allocateDescriptorSets_() {
        for (int i=0; i<LightSSBODescriptorSets.size(); i++) {
            auto bufferInfo = perFrameLightBuffers[i].buffer->descriptorInfo();
            XEDescriptorWriter(*LightSSBOSetLayout, *LightSSBOPool)
            .writeBuffer(0, &bufferInfo)
            .build(LightSSBODescriptorSets[i]);
        }
    }

    void XELightManager::rewriteDescriptorSets_() {
        for (int i=0; i<LightSSBODescriptorSets.size(); i++) {
            auto bufferInfo = perFrameLightBuffers[i].buffer->descriptorInfo();
            XEDescriptorWriter(*LightSSBOSetLayout, *LightSSBOPool)
            .writeBuffer(0, &bufferInfo)
            .overwrite(LightSSBODescriptorSets[i]);
        }
    }


    void XELightManager::createBuffers_(uint32_t capacityLights) {
        destroyBuffers_();

        capacity = capacityLights;
        perFrameLightBuffers.resize(framesInFlight);

        const VkDeviceSize totalSize = sizeof(LightSSBOHeader) + sizeof(GPULight) * static_cast<VkDeviceSize>(capacityLights);

        for (PerFrame& pf: perFrameLightBuffers) {
            pf.buffer = std::make_unique<XEBuffer>(
                device,
                totalSize,
                1,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            VkResult result = pf.buffer->map();
            assert(result == VK_SUCCESS && "Failed to map LightManager buffer");

            pf.totalSize = totalSize;
        }
    }

    void XELightManager::destroyBuffers_() {
        for (PerFrame& pf: perFrameLightBuffers) {
            if (pf.buffer) {
                pf.buffer.reset();
            }
            pf.totalSize = 0;
        }
        capacity = 0;
    }

    void XELightManager::clear() {
        gpuLights.clear();
    }

    uint32_t XELightManager::addLight(GPULight &light) {
        gpuLights.push_back(light);
        return static_cast<uint32_t>(gpuLights.size() - 1);
    }

    void XELightManager::setLight(uint32_t index, GPULight &light) {
        if (index < gpuLights.size()) {
            gpuLights[index] = light;
        }
    }

    void XELightManager::reserve(uint32_t minCapacity) {
        if (minCapacity <= capacity) return;

        const uint32_t newCapacity = std::max(minCapacity, (capacity > 0) ? (capacity + capacity / 2) : 128u );

        createBuffers_(newCapacity);
        rewriteDescriptorSets_();
    }

    void XELightManager::upload(uint32_t frameIndex) {
        if (gpuLights.size() > capacity) {
            reserve(static_cast<uint32_t>(gpuLights.size()));
        }

        hdr.count = static_cast<uint32_t>(gpuLights.size());

        PerFrame &pf = perFrameLightBuffers[frameIndex];

        // Write Header
        pf.buffer->writeToBuffer(&hdr, sizeof(hdr), 0);

        // Write light positions
        if (!gpuLights.empty()) {
            pf.buffer->writeToBuffer(gpuLights.data(),
                static_cast<VkDeviceSize>(gpuLights.size() * sizeof(GPULight)),
                sizeof(hdr));
        }
    }
}
