//
// Created by adity on 07-07-2025.
//

#pragma once

#include "platform/xe_window.h"
#include "renderer/xe_device.h"
#include "renderer/xe_renderer.h"
#include "scene/xe_game_object.h"
#include "renderer/xe_descriptors.h"
#include "renderer/gfx_resource_managers/xe_texture_manager.h"
#include "renderer/gfx_resource_managers/xe_material_manager.h"
#include "renderer/lighting/xe_light_manager.h"

#include <memory>

namespace xe {

    class Application {
    public:
        static constexpr int WIDTH = 1280;
        static constexpr int HEIGHT = 720;

        Application();
        ~Application();

        Application(const Application &) = delete;
        Application &operator=(const Application &) = delete;
        void run();

    private:
        void loadGameObjects();
        void createImguiDescriptorPool();

        XEWindow xe_window{WIDTH, HEIGHT, "Hello Vulkan!"};
        XEDevice xe_device{xe_window};
        XERenderer xe_renderer{xe_window, xe_device};
        std::unique_ptr<XEDescriptorPool> globalPool{};
        XEGameObject::Map gameObjects;

        //ImGui specific descriptor pool
        VkDescriptorPool imGuiDescriptorPool;

        XETextureManager textureManager{xe_device, 1000};
        XEMaterialManager materialManager{textureManager};
    };
}