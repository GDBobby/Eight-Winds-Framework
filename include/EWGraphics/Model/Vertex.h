#pragma once


#include "EWGraphics/Vulkan/Device.hpp"

#include <vector>
#include <map>

#define DEBUGGING_MODEL_LOAD false




#define MAX_BONE_INFLUENCE 4
namespace EWE {
    /*
    template<uint8_t InfluenceCount = MAX_BONE_INFLUENCE> requires(InfluenceCount <= MAX_BONE_INFLUENCE)
    struct BoneInfo {
        int id[InfluenceCount];
        lab::mat4 offset[InfluenceCount];

    };

    struct Position_V {
        lab::vec3 position;
    };
    struct Color_V {
        lab::vec3 color;
    };
    struct Normal_V {
        lab::vec3 normal;
    };
    struct Tangent_V {
        lab::vec3 tangent;
    };
    struct UV_V {
        lab::vec2 uv;
    };

    template <typename... Components>
    struct VertexTemplate : public Components...
    {
        VertexTemplate() = default;

        template <typename... Args>
        VertexTemplate(Args&&... args)
            : Components(std::forward<Args>(args))...
        {}
    };
    */

    struct boneVertex {
        lab::vec3 position{ 0.f };
        lab::vec3 normal{ 0.f };
        lab::vec2 uv{ 0.f };
        lab::vec3 tangent{ 0.f };

        int m_BoneIDs[MAX_BONE_INFLUENCE];
        float m_Weights[MAX_BONE_INFLUENCE];

        static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();

        void swapEndian();
    };
    struct boneVertexNoTangent {
        lab::vec3 position;
        lab::vec3 normal;
        lab::vec2 uv;

        int m_BoneIDs[MAX_BONE_INFLUENCE];
        float m_Weights[MAX_BONE_INFLUENCE];

        static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();

        void swapEndian();
    };
    struct SkyVertex {
        lab::vec3 position{ 0.f };

        static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();
    };
    struct Vertex {
        lab::vec3 position{ 0.f };
        lab::vec3 normal{ 0.f };
        lab::vec2 uv{ 0.f };
        lab::vec3 tangent{ 0.f };

        static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();

        void swapEndian();
    };
    struct VertexNT {
        lab::vec3 position{ 0.f };
        lab::vec3 normal{ 0.f };
        lab::vec2 uv{ 0.f };
        VertexNT() {}
        VertexNT(lab::vec3 position) : position{ position } {}
        VertexNT(lab::vec3 position, lab::vec3 normal, lab::vec2 uv) : position{ position } {}

        static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();

        bool operator==(VertexNT const& other) const {
            return (position == other.position) && (normal == other.normal) && (uv == other.uv);
        }

        void swapEndian();
    };
    struct SimpleVertex {
        lab::vec3 position{ 0.f };
        static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();
        bool operator ==(const SimpleVertex& other) const {
            return position == other.position;
        }
    };
    struct GrassVertex {
        lab::vec3 position{ 0.f };
        lab::vec3 color{ 0.f };
        //float uv;
        static std::vector<VkVertexInputBindingDescription> GetBindingDescriptions();
        static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();
        bool operator ==(const GrassVertex& other) const {
            return position == other.position && color == other.color;
        }
    };
    struct GrassInstance {
        lab::mat4 transform;
        GrassInstance(lab::mat4 transform) : transform{ transform } {}
    };

    struct TransformInstance {
        lab::mat4 transform;
        TransformInstance(lab::mat4 transform) : transform{ transform } {}
    };

    struct EffectVertex {
        lab::vec3 position{ 0.f };
        lab::vec2 uv{ 0.f };
        EffectVertex() {}
        EffectVertex(lab::vec3 position) : position{ position } {}
        EffectVertex(lab::vec3 position, lab::vec2 uv) : position{ position }, uv{ uv } {}

        static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();
        bool operator ==(const EffectVertex& other) const {
            return position == other.position && uv == other.uv;
        }
    };
    struct TileVertex {
        lab::vec2 uv{ 0.f };
        static std::vector<VkVertexInputBindingDescription> GetBindingDescriptions();
        static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();
        bool operator ==(const TileVertex& other) const {
            return uv == other.uv;
        }
    };
    struct TileInstance{
        lab::vec2 uvOffset;
        TileInstance(lab::vec2 uv) : uvOffset{ uv } {}
    };
    //struct Vertex {
    //    lab::vec3 position{ 0.f };
    //    lab::vec3 normal{ 0.f };
    //    lab::vec2 uv{ 0.f };
    //    lab::vec3 color{ 0.f };

    //    static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();

    //    bool operator==(const Vertex& other) const {
    //        return position == other.position && color == other.color && normal == other.normal &&
    //            uv == other.uv;
    //    }
    //};
    struct VertexColor {
        lab::vec3 position{ 0.f };
        lab::vec3 normal{ 0.f };
        lab::vec2 uv{ 0.f };
        lab::vec3 color{ 0.f };

        static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();

        bool operator==(const VertexColor& other) const {
            return position == other.position && color == other.color && normal == other.normal &&
                uv == other.uv;
        }
    };

    struct VertexUI {
        lab::vec2 position{ 0.f };
        lab::vec2 uv{ 0.f };

        static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();
    };
    struct VertexGrid2D {
        lab::vec2 position;
        VertexGrid2D() : position{ 0.f } {}
        VertexGrid2D(float x, float y) : position{ x, y } {}

        static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();
    };

    template <typename V_Type>
    struct MeshData {
        std::vector<V_Type> vertices{};
        std::vector<uint32_t> indices{};

        MeshData() {}
        MeshData(std::vector<V_Type> const& vertices, std::vector<uint32_t> const& indices) : vertices{ vertices }, indices{ indices } {}
        MeshData(std::pair<std::vector<V_Type>, std::vector<uint32_t>> const& pairData) : vertices{ pairData.first }, indices{ pairData.second } {}

        protected:
    };
}