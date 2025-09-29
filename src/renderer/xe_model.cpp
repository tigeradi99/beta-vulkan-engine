//
// Created by adity on 08-07-2025.
//

#include "renderer/xe_model.h"
#include "utils/xe_utils.h"

//#define TINYOBJLOADER_IMPLEMENTATION
//#include "tiny_obj_loader.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <cassert>
#include <cstring>
#include <iostream>
#include <unordered_map>


namespace std {
    template <>
    struct hash<xe::XEModel::Vertex> {
        size_t operator()(xe::XEModel::Vertex const& v) const {
            size_t seed = 0;
            xe::hashCombine(seed, v.position, v.color, v.normal, v.uv);
            return seed;
        }
    };
}


namespace xe {
    std::vector<VkVertexInputBindingDescription> XEModel::Vertex::getBindingDescriptions() {
        std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);

        bindingDescriptions[0].binding = 0;
        bindingDescriptions[0].stride = sizeof(Vertex);
        bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescriptions;
    }

    std::vector<VkVertexInputAttributeDescription> XEModel::Vertex::getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(5);

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, position);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, normal);

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[3].offset = offsetof(Vertex, uv);

        attributeDescriptions[4].binding = 0;
        attributeDescriptions[4].location = 4;
        attributeDescriptions[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[4].offset = offsetof(Vertex, tangent);

        return attributeDescriptions;
    }

    XEModel::XEModel(XEDevice &deviceRef, XEModel::Builder&& builder): deviceRef{deviceRef},
    meshes(std::move(builder.meshes)) {
        createVertexBuffer(builder.vertices);
        createIndexBuffer(builder.indices);
    }

    XEModel::~XEModel() {}

    void XEModel::createVertexBuffer(const std::vector<Vertex> &vertices) {
        vertexCount = static_cast<uint32_t>(vertices.size());
        assert(vertexCount >= 3 && "vertex count must be greater than 3");
        //VkDeviceSize vertexBufferSize = sizeof(vertices[0]) * vertexCount;
        uint32_t vertexSize = sizeof(vertices[0]);

        vertexBuffer = XEBufferVMA::createBufferAndTransferDataToGPU(
            deviceRef,
            vertexSize,
            vertexCount,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            (void *)vertices.data());
    }

    void XEModel::createIndexBuffer(const std::vector<uint32_t> &indices) {
        indexCount = static_cast<uint32_t>(indices.size());
        hasIndexBuffer = indexCount > 0;

        if (!hasIndexBuffer) {
            return;
        }

        // VkDeviceSize indexBufferSize = sizeof(indices[0]) * indexCount;
        uint32_t indexSize = sizeof(indices[0]);

        indexBuffer = XEBufferVMA::createBufferAndTransferDataToGPU(
            deviceRef,
            indexSize,
            indexCount,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            (void *)indices.data());
    }

    std::unique_ptr<XEModel> XEModel::createModelFromFile(XEDevice &device, XEMaterialManager& materialManager,
        const std::string &modelPath) {
        Builder builder{};
        builder.materialMgr = &materialManager;

        auto pos = modelPath.find_last_of("/\\");
        builder.modelDir = (pos == std::string::npos) ? std::string{} : modelPath.substr(0, pos + 1);

        builder.loadModel(modelPath);

        std::cout<<"Loaded model "<<modelPath<<std::endl;
        std::cout<<"Vertex Count: "<<builder.vertices.size()<<std::endl;
        return std::make_unique<XEModel>(device, std::move(builder));
    }

    void XEModel::bind(VkCommandBuffer cmdBuffer) {
        VkBuffer buffers[] = {vertexBuffer->getBuffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmdBuffer, 0, 1, buffers, offsets);

        if (hasIndexBuffer) {
            vkCmdBindIndexBuffer(cmdBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
        }
    }

    void XEModel::draw(VkCommandBuffer cmdBuffer) {
        if (hasIndexBuffer) {
            for (const auto& mesh: meshes) {
                vkCmdDrawIndexed(
                    cmdBuffer,
                    mesh.indexCount,
                    1,
                    mesh.firstIndex,
                    mesh.vertexOffset,
                    0);
            }
            // vkCmdDrawIndexed(cmdBuffer, indexCount, 1, 0, 0, 0);
        } else {
            for (const auto& mesh: meshes) {
                vkCmdDraw(
                    cmdBuffer,
                    mesh.vertexCount,
                    1,
                    mesh.vertexOffset,
                    0);
            }
        }
    }

    void XEModel::drawMesh(VkCommandBuffer cmdBuffer, const XEMesh& mesh) {
        if (hasIndexBuffer) {
            vkCmdDrawIndexed(
                cmdBuffer,
                mesh.indexCount,
                1,
                mesh.firstIndex,
                mesh.vertexOffset,
                0);
        } else {
            vkCmdDraw(
                cmdBuffer,
                mesh.vertexCount,
                1,
                mesh.vertexOffset,
                0);
        }
    }

    static std::string joinPath(const std::string& baseDir, const std::string& rel) {
        if (rel.empty()) return rel;
        // If rel already looks absolute, return as is (basic heuristic)
        if (rel.size() > 1 && (rel[1] == ':' || rel[0] == '/' || rel[0] == '\\')) return rel;
        std::string p = baseDir;
        if (!p.empty() && p.back() != '/' && p.back() != '\\') p.push_back('/');
        p += rel;
        // Normalize slashes for platform
        std::replace(p.begin(), p.end(), '\\', '/');
        return p;
    }

    void XEModel::Builder::loadModel(const std::string &path) {
        Assimp::Importer importer{};

        const aiScene* scene = importer.ReadFile(path,
            aiProcess_Triangulate |
            aiProcess_CalcTangentSpace |
            aiProcess_FlipUVs |
            aiProcess_MakeLeftHanded);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
            throw std::runtime_error(importer.GetErrorString());
        }

        processNode(scene->mRootNode, scene, aiMatrix4x4());
    }

    void XEModel::Builder::processNode(aiNode *node, const aiScene *scene, const aiMatrix4x4& parentTransform) {
        // combine parent transform with this node's transform
        aiMatrix4x4 globalTransform = parentTransform * node->mTransformation;

        // process all node meshes
        for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
            processMesh(mesh, scene, globalTransform);
        }

        // Recursively traverse through all child nodes
        for (unsigned int i = 0; i < node->mNumChildren; ++i) {
            processNode(node->mChildren[i], scene, globalTransform);
        }
    }

    void XEModel::Builder::processMesh(aiMesh *mesh, const aiScene *scene, const aiMatrix4x4& transform) {
        XEMesh meshInfo{};
        meshInfo.vertexOffset = static_cast<int32_t>(vertices.size());
        meshInfo.vertexCount = mesh->mNumVertices;
        meshInfo.firstIndex = indices.size();

        aiMatrix3x3 M3(transform);
        aiMatrix3x3 N3 = M3;  // copy
        N3.Inverse();
        N3.Transpose();

        // Process vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
            Vertex vertex;

            // apply transform to positions
            aiVector3D pos = transform * mesh->mVertices[i];

            // Positions
            vertex.position = {pos.x, pos.y, pos.z};

            // Normals
            if (mesh->HasNormals()) {
                aiVector3D normal = N3 * mesh->mNormals[i];

                // normalize
                float len = std::sqrt(normal.x*normal.x + normal.y*normal.y + normal.z*normal.z);
                if (len > 0) { normal.x/=len; normal.y/=len; normal.z/=len; }
                vertex.normal = { normal.x, normal.y, normal.z };
            }

            if (mesh->HasTangentsAndBitangents()) {
                aiVector3D T = N3 * mesh->mTangents[i];
                aiVector3D B = N3 * mesh->mBitangents[i];
                aiVector3D N = N3 * mesh->mNormals[i];

                // Orthonormalize T against N via Gram-Schmidt Process
                // T = normalize(t - n * dot(n,t))
                float ndott = N.x * T.x + N.y * T.y + N.z * T.z;
                T.x -= N.x * ndott;
                T.y -= N.y * ndott;
                T.z -= N.z * ndott;
                float t_len = std::sqrt(T.x * T.x + T.y * T.y + T.z * T.z);
                if (t_len > 0) { T.x /= t_len; T.y /= t_len; T.z /= t_len; }

                // Handedness: sign = dot(cross(N,T), B) >= 0 ? +1 : -1
                aiVector3D c = aiVector3D(
                    N.y*T.z - N.z*T.y,
                    N.z*T.x - N.x*T.z,
                    N.x*T.y - N.y*T.x
                );
                float sign = (c.x * B.x + c.y * B.y + c.z * B.z) < 0.0f ? 1.0f : -1.0f;
                vertex.tangent = {T.x, T.y, T.z, sign};
            } else {
                vertex.tangent = {1,0,0, 1};
            }

            // Texture coordinates
            if (mesh->HasTextureCoords(0)) {
                glm::vec2 uv;
                uv.x = mesh->mTextureCoords[0][i].x;
                uv.y = mesh->mTextureCoords[0][i].y;
                vertex.uv = uv;
            }

            // colors
            if (mesh->HasVertexColors(0)) {
                vertex.color = {
                    mesh->mColors[0][i].r,
                    mesh->mColors[0][i].g,
                    mesh->mColors[0][i].b
                };
            } else {
                vertex.color = {1.0f, 1.0f, 1.0f}; // default white
            }
            vertices.push_back(vertex);
        }

        // process indices
        for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; ++j) {
                indices.push_back(face.mIndices[j]);
            }
        }
        meshInfo.indexCount = indices.size() - meshInfo.firstIndex;

        int materialIndex = materialMgr->getDefaultMaterialIndex();

        if (scene->HasMaterials() && mesh->mMaterialIndex >= 0 && materialMgr) {
            aiMaterial *mat = scene->mMaterials[mesh->mMaterialIndex];
            aiString texPath;
            XEMaterialDesc materialDesc{};

            auto readTex = [&](aiTextureType type) -> std::string {
                if (mat->GetTexture(type, 0, &texPath) == AI_SUCCESS) {
                    std::string full_path = joinPath(modelDir, std::string(texPath.C_Str()));

                    return full_path;
                }
                return {};
            };

            materialDesc.albedoFileName = readTex(aiTextureType_BASE_COLOR);
            if (materialDesc.albedoFileName.empty()) { materialDesc.albedoFileName = readTex(aiTextureType_DIFFUSE); }
            materialDesc.normalFileName = readTex(aiTextureType_NORMALS);
            if (materialDesc.normalFileName.empty()) { materialDesc.normalFileName = readTex(aiTextureType_HEIGHT); }

            materialIndex = materialMgr->create(materialDesc);
        }

        // Push back meshInfo
        meshInfo.materialIndex = materialIndex;
        meshes.push_back(meshInfo);
    }
}
