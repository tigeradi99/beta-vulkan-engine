//
// Created by adity on 20-07-2025.
//

#pragma once

#include "scene/xe_game_object.h"
#include "platform/xe_window.h"

namespace xe {

    class KeyboardMovementController {
        public:
        struct KeyMappings {
            int moveLeft = GLFW_KEY_A;
            int moveRight = GLFW_KEY_D;
            int moveForward = GLFW_KEY_W;
            int moveBackward = GLFW_KEY_S;

            int moveUp = GLFW_KEY_E;
            int moveDown = GLFW_KEY_Q;

            int lookUp = GLFW_KEY_UP;
            int lookDown = GLFW_KEY_DOWN;
            int lookLeft = GLFW_KEY_LEFT;
            int lookRight = GLFW_KEY_RIGHT;
        };

        void moveInPlaneXZ(GLFWwindow* window, float dt, XEGameObject& gameObject);

        KeyMappings keys;
        float moveSpeed = 3.0f;
        float lookSpeed = 1.0f;
    };
}