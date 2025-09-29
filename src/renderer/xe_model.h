//
// Created by adity on 08-07-2025.
//

#pragma once

#include "renderer/xe_device.h"
#include "renderer/xe_buffer_vma.h"
#include "renderer/materials/xe_materials.h"
#include "gfx_resource_managers/xe_texture_manager.h"
#include "gfx_resource_managers/xe_material_manager.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"

// ASSIMP imports
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include <vector>
#include <memory>


namespace xe {

    class XEModel {
    public:

        struct Vertex {
            glm::vec3 position{};
            glm::vec3 color{};
            glm::vec3 normal{};
            glm::vec2 uv{};
            glm::vec4 tangent{}; // xyz = tangent dir, w = handedness (+1/-1)

            static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
            static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

            bool operator==(const Vertex& other) const {
                return position == other.position && color == other.color && normal == other.normal && uv == other.uv;
            }
        };

        struct XEMesh {
            uint32_t firstIndex;
            uint32_t indexCount;
            int32_t vertexOffset;
            uint32_t vertexCount;
            int materialIndex = 0; // texture index
        };

        struct Builder {
            std::vector<Vertex> vertices{};
            std::vector<uint32_t> indices{};
            std::vector<XEMesh> meshes{};
            XEMaterialManager* materialMgr = nullptr;
            std::string modelDir;

            void loadModel(const std::string& path);
            void processNode(aiNode* node, const aiScene* scene, const aiMatrix4x4& parentTransform);
            void processMesh(aiMesh* mesh, const aiScene* scene, const aiMatrix4x4& transform);
        };

        XEModel(XEDevice& deviceRef, XEModel::Builder&& builder);
        ~XEModel();

        XEModel(const XEModel &) = delete;
        XEModel &operator=(const XEModel &) = delete;

        static std::unique_ptr<XEModel> createModelFromFile(XEDevice& device, XEMaterialManager& materialManager,
            const std::string& modelPath);

        void bind(VkCommandBuffer cmdBuffer);
        void draw(VkCommandBuffer cmdBuffer);
        void drawMesh(VkCommandBuffer cmdBuffer, const XEMesh& mesh);

        // Access Meshes
        std::vector<XEMesh>& getMeshes() { return meshes; }

    private:
        XEDevice& deviceRef;

        // Vertex
        std::unique_ptr<XEBufferVMA> vertexBuffer;
        uint32_t vertexCount;

        // Index
        std::unique_ptr<XEBufferVMA> indexBuffer;
        uint32_t indexCount;

        // Meshes:
        std::vector<XEMesh> meshes;

        // Materials:
        std::vector<XEMaterial> materials;

        bool hasIndexBuffer = false;

        void createVertexBuffer(const std::vector<Vertex> &vertices);
        void createIndexBuffer(const std::vector<uint32_t> &indices);
    };
}
