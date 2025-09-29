//
// Created by adity on 09-07-2025.
//

#pragma once

#include "renderer/xe_model.h"
#include "renderer/xe_texture.h"

#include "glm/gtc/matrix_transform.hpp"

#include <memory>
#include <unordered_map>


namespace xe {

    struct TransformComponent {
        glm::vec3 translation{};
        glm::vec3 scale{.1f, .1f, .1f};
        glm::vec3 rotation{};

        glm::mat4 mat4();

        glm::mat3 normalMatrix();
    };

    class XEGameObject {
    public:
        using id_t = unsigned int;
        using Map = std::unordered_map<id_t, XEGameObject>;

        static XEGameObject createGameObject() {
            static id_t id = 0;
            return XEGameObject{id++};
        }

        XEGameObject(const XEGameObject&) = delete;
        XEGameObject& operator=(const XEGameObject&) = delete;
        XEGameObject(XEGameObject&&) = default;
        XEGameObject& operator=(XEGameObject&&) = default;

        id_t getId() const { return id; }

        std::shared_ptr<XEModel> model{};
        glm::vec3 color{};
        TransformComponent transform{};
        bool canCastShadow = false;

    private:
        XEGameObject(id_t objId): id(objId) {};
        id_t id;
    };
}
