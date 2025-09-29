//
// Created by adity on 04-09-2025.
//

#pragma once

#include "glm/glm.hpp"

namespace xe {
    // 16B alignment
    struct GPULight {
        glm::vec4 color; // rgb, a=intensity
        glm::vec4 position; // xyz, w=type (0/1/2)
        glm::vec4 direction; // xyz, w=unused
        glm::vec4 param; // x=range, y=innerCos, z=outerCos, w=specPow
    };
}

