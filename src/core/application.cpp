//
// Created by adity on 07-07-2025.
//

#include "core/application.h"
#include "systems/xe_camera.h"
#include "renderer/xe_buffer.h"
#include "platform/xe_movement_controller.h"
#include "systems/xe_frame_info.h"
#include "systems/xe_simple_render_system.h"
#include "systems/xe_point_light_system.h"
#include "systems/xe_shadow_system.h"

#include <stdexcept>
#include <array>
#include <cassert>
#include <chrono>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"

#include "vma/vk_mem_alloc.h"

namespace xe {

    struct GlobalUbo {
        glm::mat4 projection{1.f};
        glm::mat4 view{1.f};
        glm::mat4 inverseView{1.f};
        glm::vec4 ambientLightColor{1.f, 1.f, 1.f, 0.2f}; // w is intensity
    };

    Application::Application() {
        globalPool = XEDescriptorPool::Builder(xe_device)
        .setMaxSets(XESwapChain::MAX_FRAMES_IN_FLIGHT)
        .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, XESwapChain::MAX_FRAMES_IN_FLIGHT)
        .build();

        createImguiDescriptorPool();

        loadGameObjects();

        // ImGUI init
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForVulkan(xe_window.getGLFWwindow(), true);
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = xe_device.getInstance();
        init_info.PhysicalDevice = xe_device.getPhysicalDevice();
        init_info.Device = xe_device.device();
        init_info.QueueFamily = xe_device.findPhysicalQueueFamilies().graphicsFamily;
        init_info.Queue = xe_device.graphicsQueue();
        init_info.PipelineCache = VK_NULL_HANDLE;
        init_info.DescriptorPool = imGuiDescriptorPool;

        //init_info.RenderPass = xe_renderer.getSwapChainRenderPass();
        //init_info.Subpass = 0;

        init_info.MinImageCount = XESwapChain::MAX_FRAMES_IN_FLIGHT;
        init_info.ImageCount = XESwapChain::MAX_FRAMES_IN_FLIGHT;

        //init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        //init_info.Allocator = g_Allocator;
        //init_info.CheckVkResultFn = check_vk_result;

        ImGui_ImplVulkan_Init(&init_info);
    }

    Application::~Application() {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        vkDestroyDescriptorPool(xe_device.device(), imGuiDescriptorPool, nullptr);
    }

    void Application::run() {
        std::vector<std::unique_ptr<XEBuffer>> uboBuffers(XESwapChain::MAX_FRAMES_IN_FLIGHT);

        for (int i = 0; i < uboBuffers.size(); i++) {
            uboBuffers[i] = std::make_unique<XEBuffer>(
            xe_device,
            sizeof(GlobalUbo),
            1,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            xe_device.properties.limits.minUniformBufferOffsetAlignment);

            uboBuffers[i]->map();
        }

        auto globalSetLayout = XEDescriptorSetLayout::Builder(xe_device)
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
        .build();

        std::vector<VkDescriptorSet> globalDescriptorSets(XESwapChain::MAX_FRAMES_IN_FLIGHT);
        for (int i=0; i<globalDescriptorSets.size(); i++) {
            auto bufferInfo = uboBuffers[i]->descriptorInfo();
            XEDescriptorWriter(*globalSetLayout, *globalPool)
            .writeBuffer(0, &bufferInfo)
            .build(globalDescriptorSets[i]);
        }

        XELightManager lightManager{xe_device, XESwapChain::MAX_FRAMES_IN_FLIGHT, 128};

        XEShadowSystem shadowSystem{xe_device, lightManager};
        XESimpleRenderSystem simpleRenderSystem{xe_device, xe_renderer.getSwapChainRenderPass(),
            globalSetLayout->getDescriptorSetLayout(), textureManager, materialManager,
            shadowSystem.getDescriptorSetLayout(), lightManager};
        XEPointLightSystem pointLightSystem{xe_device, xe_renderer.getSwapChainRenderPass(),
            globalSetLayout->getDescriptorSetLayout(), lightManager};
        XECamera camera{};

        camera.setViewTarget(glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 0.f, 2.5f));

        auto viewerObject = XEGameObject::createGameObject();
        viewerObject.transform.translation = glm::vec3(5.983f, 0.795f, 0.518);
        // viewerObject.transform.translation = glm::vec3(-0.f, 0.f, 0.f);
        KeyboardMovementController cameraController{};

        //set lights
        glm::vec3 originToSun = glm::normalize(glm::vec3(1.0f, 1.0f, -0.2f));

        GPULight sunLight = {
            glm::vec4(1.f, 1.f, 0.95f, 3.0f),
            glm::vec4(0, 0, 0, 0),
            glm::vec4(originToSun, 0.0f),
            {0,0,0, 64.0f}              // specPower=64
        };
        bool sunlightOn = true;

        GPULight pointLight{};
        pointLight.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        pointLight.position = glm::vec4(0.0f, 0.0f, 0.0f, 1);
        pointLight.param = {3, 0, 0, 32.0f};
        std::vector<GPULight> pointLights{pointLight};

        std::array<float, 3> lightPos{2.414, 0.795f, 0.433};
        std::array<float, 3> lightColor{1.f, 1.f, 1.f};
        std::array<float, 3> sunPos{-4.390f, 2.438f, -10.812f};
        std::array<float, 4> sunColor{1.f, 1.f, 0.95f, 3.0f};
        VmaBudget budgets[VK_MAX_MEMORY_HEAPS];
        VkPhysicalDeviceMemoryProperties memoryProperties{};

        vkGetPhysicalDeviceMemoryProperties(xe_device.getPhysicalDevice(), &memoryProperties);
        vmaGetHeapBudgets(xe_device.vmaAllocator(), budgets);


        auto currentTime = std::chrono::high_resolution_clock::now();

        while (!xe_window.shouldClose()) {
            glfwPollEvents();

            auto newTime = std::chrono::high_resolution_clock::now();
            float frameTime =
                std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
            currentTime = newTime;

            cameraController.moveInPlaneXZ(xe_window.getGLFWwindow(), frameTime, viewerObject);
            camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

            // glm::mat4 viewTest = glm::lookAtLH({sunLight.position.x, sunLight.position.y, sunLight.position.z},
            //                                 glm::vec3(0.0f, 0.0f, 0.0f),
            //                                 glm::vec3(0.0f, 1.0f, 0.0f));

            float aspect = xe_renderer.getAspectRatio();
            camera.setPerspectiveProjection(glm::radians(60.0f), aspect, 0.1f, 200.0f);
            camera.setCameraParams(glm::radians(60.0f), aspect, 0.1f, 200.0f);
            // glm::mat4 projectionTest = glm::orthoLH_ZO(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 1000.0f);

            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // -------------------- Your ImGui UI here --------------------
            ImGui::Begin("Adjust Lighting");
            ImGui::Text("Drag to manipulate light X,Y &Z");
            ImGui::SliderFloat3("light Pos", &lightPos[0], -10.f, 10.f);
            ImGui::Text("Drag to manipulate light color components");
            ImGui::SliderFloat3("light Color", &lightColor[0], 0.f, 1.f);
            ImGui::Checkbox("Turn Sunlight on/off", &sunlightOn);
            ImGui::SliderFloat3("Sunlight Color", &sunColor[0], 0.f, 1.f);
            ImGui::SliderFloat("Sunlight Intensity", &sunColor[3], 0.f, 10.f);
            ImGui::Separator();
            ImGui::Text("Current coordinates: X: %.3f, Y: %.3f, Z: %.3f", viewerObject.transform.translation.x, viewerObject.transform.translation.y, viewerObject.transform.translation.z);

            // ------------------ VMA statistics --------------------------
            ImGui::Separator();
            ImGui::Text("Memory Details");
            for (uint32_t i = 0; i < memoryProperties.memoryHeapCount; i++) {
                if (memoryProperties.memoryHeaps[i].flags && VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                    const auto& budget = budgets[i];
                    ImGui::Text("Device name: %s", xe_device.properties.deviceName);
                    ImGui::Text("Heap: %u",i);
                    ImGui::Text("Usage: %.2f MB", budget.usage / (1024.f * 1024.f));
                    ImGui::Text("Budget: %.2f MB", budget.budget / (1024.f * 1024.f));
                    ImGui::Text("Allocated: %.2f MB", budget.statistics.allocationBytes / (1024.f * 1024.f));
                }

            }
            ImGui::End();
            // -----------------------------------------------------------

            ImGui::Render();

            if (auto commandBuffer = xe_renderer.beginFrame()) {
                int frameIndex = xe_renderer.getFrameIndex();
                FrameInfo frameInfo{
                    frameIndex,
                    frameTime,
                    commandBuffer,
                    camera,
                    globalDescriptorSets[frameIndex],
                    lightManager.descriptorSet(frameIndex),
                    gameObjects
                };

                // Update
                GlobalUbo ubo{};
                ubo.projection = camera.getProjection();
                ubo.view = camera.getView();
                ubo.inverseView = camera.getInverseView();
                // ubo.projection = projectionTest;
                // ubo.view = viewTest;
                // ubo.inverseView = glm::inverse(viewTest);
                uboBuffers[frameIndex]->writeToBuffer(&ubo);
                uboBuffers[frameIndex]->flush();

                // Sunlight on/off
                if (!sunlightOn) {
                    sunLight.color = glm::vec4(0.f, 0.f, 0.f, 3.0f);
                }
                else {
                    sunLight.color = {sunColor[0], sunColor[1], sunColor[2], sunColor[3]};
                }

                // Update light position
                pointLights[0].position.x = lightPos[0];
                pointLights[0].position.y = lightPos[1];
                pointLights[0].position.z = lightPos[2];

                // Update Light color
                pointLights[0].color.x = lightColor[0];
                pointLights[0].color.y = lightColor[1];
                pointLights[0].color.z = lightColor[2];

                // Frame light settings
                lightManager.clear();
                lightManager.addLight(sunLight);
                for (auto& pl : pointLights) lightManager.addLight(pl);
                lightManager.upload(frameIndex);

                // Shadow Pass
                shadowSystem.renderGameObjects(frameInfo, sunLight);

                // Render items
                xe_renderer.beginSwapChainRenderPass(commandBuffer);
                simpleRenderSystem.renderGameObjects(frameInfo,
                    shadowSystem.getDescriptorSet(frameIndex));
                pointLightSystem.render(frameInfo, pointLights.size());
                // ImGui draw
                ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
                xe_renderer.endSwapChainRenderPass(commandBuffer);
                xe_renderer.endFrame();
            }
        }

        vkDeviceWaitIdle(xe_device.device());
    }

    void Application::loadGameObjects() {
        // std::shared_ptr<XEModel> xe_model = XEModel::createModelFromFile(xe_device, materialManager,
        //     "assets\\niagara_bistro\\bistrox.gltf");

        std::shared_ptr<XEModel> xe_model = XEModel::createModelFromFile(xe_device, materialManager,
            "assets\\sponza-gltf-pbr\\sponza.glb");

        auto gameObj1 = XEGameObject::createGameObject();
        gameObj1.model = xe_model;
        gameObj1.canCastShadow = true;
        gameObj1.transform.translation = {0.0f, 0.0f, 0.0f};
        gameObj1.transform.rotation = {0.0f, 0.0f, 0.0f};
        gameObj1.transform.scale = {1.0f, 1.0f, 1.0f};
        gameObjects.emplace(gameObj1.getId(), std::move(gameObj1));

        // std::shared_ptr<XEModel> xe_model = XEModel::createModelFromFile(xe_device, materialManager,
        //     "assets\\BoomBox\\BoomBox.gltf");
        //
        // auto gameObj1 = XEGameObject::createGameObject();
        // gameObj1.model = xe_model;
        // gameObj1.canCastShadow = true;
        // gameObj1.transform.translation = {0.0f, 1.0f, 0.0f};
        // gameObj1.transform.rotation = {0.0f, 0.0f, 0.0f};
        // gameObj1.transform.scale = {50.0f, 50.0f, 50.0f};
        // gameObjects.emplace(gameObj1.getId(), std::move(gameObj1));
        //
        // std::shared_ptr<XEModel> quad = XEModel::createModelFromFile(xe_device, materialManager,
        //     "assets\\models\\quad.obj");
        //
        // auto floor = XEGameObject::createGameObject();
        // floor.model = quad;
        // floor.canCastShadow = false;
        // floor.transform.translation = {0.0f, 0.0f, 0.0f};
        // floor.transform.rotation = {0.0f, 0.0f, 0.0f};
        // floor.transform.scale = {10.0f, 10.0f, 10.0f};
        // gameObjects.emplace(floor.getId(), std::move(floor));
    }

    void Application::createImguiDescriptorPool() {
        std::array<VkDescriptorPoolSize, 11> imguiPoolSizes = {
            VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_SAMPLER, 100},
            VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100},
            VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 100},
            VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100},
            VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 100},
            VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 100},
            VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100},
            VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100},
            VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 100},
            VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 100},
            VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 100}
        };

        VkDescriptorPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;  // Important!
        pool_info.maxSets = 100 * static_cast<uint32_t>(imguiPoolSizes.size());
        pool_info.poolSizeCount = static_cast<uint32_t>(imguiPoolSizes.size());
        pool_info.pPoolSizes = imguiPoolSizes.data();

        if (vkCreateDescriptorPool(xe_device.device(), &pool_info, nullptr, &imGuiDescriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create ImGui descriptor pool");
        }
    }
}
