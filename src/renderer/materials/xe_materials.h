//
// Created by adity on 30-08-2025.
//

#pragma once

#include "glm/glm.hpp"
#include <string>


namespace xe {
    struct XEMaterial {
        int albedoIndex = 0;
        int normalIndex = 0;
    };

    struct XEMaterialDesc {
        std::string albedoFileName;
        std::string normalFileName;
    };
}
