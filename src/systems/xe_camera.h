//
// Created by adity on 20-07-2025.
//

#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"

#include <array>

namespace xe {

    class XECamera {
    public:
        void setOrthographicProjection(
            float left, float right, float top, float bottom, float near, float far);

        void setPerspectiveProjection(float fovy, float aspect, float near, float far);

        void setViewDirection(glm::vec3 position, glm::vec3 direction, glm::vec3 up = glm::vec3(0.0f, -1.0f, 0.0f));
        void setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up = glm::vec3(0.0f, -1.0f, 0.0f));
        void setViewYXZ(glm::vec3 position, glm::vec3 rotation);
        void setCameraParams(float fovy, float aspect, float near, float far);

        const glm::mat4 getProjection() const { return projectionMatrix; }
        const glm::mat4 getView() const { return viewMatrix; }
        const glm::mat4 getInverseView() const { return inverseViewMatrix; }

        const float getNearClip() { return z_near; }
        const float getFarClip() { return z_far; }
        const float getAspect() { return cam_aspect; }
        const float getFOV() { return fov_y; }

    private:
        glm::mat4 projectionMatrix = glm::mat4(1.0f);
        glm::mat4 viewMatrix = glm::mat4(1.0f);
        glm::mat4 inverseViewMatrix = glm::mat4(1.0f);

        float fov_y = 0.0f;
        float cam_aspect = 0.0f;
        float z_near = 0.0f;
        float z_far = 0.0f;
    };
}
