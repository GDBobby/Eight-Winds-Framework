#pragma once

#include "EWGraphics/Vulkan/Device_Buffer.h"
#include "EWGraphics/Vulkan/Device.hpp"
#include "EWGraphics/Model/Vertex.h"

// libs

/*
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
*/
#define MAX_BONE_INFLUENCE 4

#define LEVEL_BUILDER false
// std
#include <memory>
#include <vector>
#include <map>
#include <iostream>

namespace EWE {
    constexpr std::size_t FLOAT_SIZE3 = sizeof(float) * 3;
    constexpr std::size_t FLOAT_SIZE2 = sizeof(float) * 2;

    struct MaterialComponent {
        //
        //lab::vec3 ambient{ 0.f };
        //lab::vec3 diffuse{ 0.f };
        //lab::vec3 specular{ 0.f };
        //float shininess{ 0.f };
        lab::vec4 metallicRoughnessOpacity;
        //float metallic;
        //float roughness;
    };



    class EWEModel {
    public:

        template <typename T>
        static std::vector<VkVertexInputBindingDescription> GetBindingDescriptions() {
            std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
            bindingDescriptions[0].binding = 0;
            //printf("size of BVNT:T - %d:%d \n", sizeof(boneVertexNoTangent),sizeof(T));
            bindingDescriptions[0].stride = sizeof(T);
            bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            return bindingDescriptions;
        }


        EWEModel(void const* verticesData, const std::size_t vertexCount, const std::size_t sizeOfVertex, std::vector<uint32_t> const& indices);
        EWEModel(void const* verticesData, const std::size_t vertexCount, const std::size_t sizeOfVertex);


        void AddInstancing(uint32_t instanceCount, const uint32_t instanceSize, void const* data);


        EWEModel(const EWEModel&) = delete;
        EWEModel& operator=(const EWEModel&) = delete;
#if CALL_TRACING
        static EWEModel* CreateModelFromObj(const std::string& filepath, std::source_location = std::source_location::current());
        static EWEModel* CreateSimpleModelFromObj(const std::string& filePath, std::source_location = std::source_location::current());
        static EWEModel* CreateGrassModelFromObj(const std::string& filePath, std::source_location = std::source_location::current());
#else
        static EWEModel* CreateModelFromObj(const std::string& filepath);
        static EWEModel* CreateSimpleModelFromObj(const std::string& filePath);
        static EWEModel* CreateGrassModelFromObj(const std::string& filePat);
#endif

        void BindAndDraw();
        void BindAndDrawNoIndex();
        void Bind();
        void BindNoIndex();
        void Draw();
        void DrawNoIndex();
        void BindAndDrawInstance();
        void BindAndDrawInstance(uint32_t instanceCount);
        void BindAndDrawInstanceNoIndex();
        void BindAndDrawInstanceNoBuffer(int instanceCount);
        void BindAndDrawInstanceNoBufferNoIndex(int instanceCount);

        uint32_t GetVertexCount() { return vertexCount; }
        uint32_t GetIndexCount() { return indexCount; }

#if DEBUG_NAMING
        void SetDebugNames(std::string const& name);
#endif

        //delete needs to be called on this at destruction, or put it into a smart pointer
        //static EWEBuffer* CreateIndexBuffer(std::vector<uint32_t> const& indices);
        //static EWEBuffer* CreateIndexBuffer(CommandBuffer cmdBuf, std::vector<uint32_t> const& indices);

    protected:
        //void createVertexBuffers(const std::vector<Vertex>& vertices);
        //void createUIVertexBuffers(const std::vector<VertexUI>& vertices);
        //void createBoneVertexBuffers(const std::vector<boneVertex>& vertices);
        //void createBobVertexBuffers(const std::vector <bobVertex>& vertices);

        void VertexBuffers(uint32_t vertexCount, uint32_t vertexSize, void const* data);

        void CreateIndexBuffer(void const* indexData, uint32_t indexCount);
        void CreateIndexBuffers(const std::vector<uint32_t>& indices);


        EWEBuffer* vertexBuffer{ nullptr };
        uint32_t vertexCount;

        bool hasIndexBuffer = false;
        EWEBuffer* indexBuffer{ nullptr };
        uint32_t indexCount;

        bool hasInstanceBuffer = false;
        EWEBuffer* instanceBuffer{ nullptr };
        uint32_t instanceCount;
    };
} //namespace EWE