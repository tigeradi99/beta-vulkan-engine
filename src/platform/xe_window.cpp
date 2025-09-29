//
// Created by adity on 07-07-2025.
//

#include "platform/xe_window.h"
#include <stdexcept>

namespace xe {

    XEWindow::XEWindow(int width, int height, std::string title) : width(width), height(height) , title(title) {
        initWindow();
    }

    XEWindow::~XEWindow() {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void XEWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR*surface) {
        if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
    }

    void XEWindow::framebufferResizeCallback(GLFWwindow *window, int width, int height) {
        auto xe_window = reinterpret_cast<XEWindow*>(glfwGetWindowUserPointer(window));
        xe_window->frameBufferResized = true;
        xe_window->width = width;
        xe_window->height = height;
    }

    void XEWindow::initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    }

}
