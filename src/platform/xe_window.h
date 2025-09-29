//
// Created by adity on 07-07-2025.
//

#pragma once

#define GLFW_INCLUDE_VULKAN
#include <string>

#include "GLFW/glfw3.h"

namespace xe {

    class XEWindow {

    public:
        XEWindow(int width, int height, std::string title);
        ~XEWindow();

        bool shouldClose() { return glfwWindowShouldClose(window); }
        VkExtent2D getExtent() { return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)}; }
        bool wasWindowResized() { return frameBufferResized; }
        void resetWindowResizeFlag() { frameBufferResized = false; }
        GLFWwindow* getGLFWwindow() const { return window; }

        void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);

    private:
        static void framebufferResizeCallback(GLFWwindow *window, int width, int height);
        void initWindow();

        int width;
        int height;
        bool frameBufferResized = false;
        std::string title;

        GLFWwindow* window;
    };
}
