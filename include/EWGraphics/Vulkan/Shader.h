#pragma once

#include "EWGraphics/Vulkan/VulkanHeader.h"
#include "EWGraphics/Vulkan/Descriptors.h"

#include "EWGraphics/Data/magic_enum.hpp"

namespace EWE {
	struct ShaderStage {
		enum Bits {
			Vertex = 0,
			TessControl,
			TessEval,
			Geometry,
			Task,
			Mesh,
			Fragment,
			Compute,

			COUNT
		};
		Bits value;
		constexpr ShaderStage() : value{ Bits::COUNT } {}
		constexpr ShaderStage(Bits v) : value{ v } {}
		constexpr operator Bits() const {
			return value;
		}

		constexpr ShaderStage(VkShaderStageFlagBits vkStage) {
			switch (vkStage) {
				case VK_SHADER_STAGE_VERTEX_BIT: value = Bits::Vertex; break;
				case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT: value = Bits::TessControl; break;
				case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: value = Bits::TessEval; break;
				case VK_SHADER_STAGE_GEOMETRY_BIT: value = Bits::Geometry; break;
				case VK_SHADER_STAGE_TASK_BIT_EXT: value = Bits::Task; break;
				case VK_SHADER_STAGE_MESH_BIT_EXT: value = Bits::Mesh; break;
				case VK_SHADER_STAGE_FRAGMENT_BIT: value = Bits::Fragment; break;
				case VK_SHADER_STAGE_COMPUTE_BIT: value = Bits::Compute; break;
				default:EWE_UNREACHABLE;
			}
		}
		constexpr operator VkShaderStageFlagBits() const {
			switch (value) {
				case Bits::Vertex:		return VK_SHADER_STAGE_VERTEX_BIT;
				case Bits::TessControl:	return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
				case Bits::TessEval:	return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
				case Bits::Geometry:    return VK_SHADER_STAGE_GEOMETRY_BIT;
				case Bits::Task:		return VK_SHADER_STAGE_TASK_BIT_EXT;
				case Bits::Mesh:		return VK_SHADER_STAGE_MESH_BIT_EXT;
				case Bits::Fragment:    return VK_SHADER_STAGE_FRAGMENT_BIT;
				case Bits::Compute:		return VK_SHADER_STAGE_COMPUTE_BIT;
			}
			EWE_UNREACHABLE;
		}

		constexpr bool operator==(Bits bits) const {
			return value == bits;
		}
		constexpr bool operator==(ShaderStage const other) const {
			return value == other.value;
		}
		constexpr bool operator==(VkShaderStageFlagBits bits) const {
			return value == ShaderStage(bits);
		}
	};



	struct Shader {

		enum ShaderFundamentalType {
			ST_INT,
			ST_UINT,
			ST_BOOL,
			ST_FLOAT,
			ST_DOUBLE,

			ST_COUNT
		};

		struct SpecializationEntry {
#if PIPELINE_HOT_RELOAD
			std::string name{};
#endif
			ShaderFundamentalType type;
			uint32_t constantID;
			uint8_t elementCount = 1;
			char value[64]; //64 being the size of a 4x4 matrix, the largest size im supporting rn. i dont even know if thats a thing for spec constants
		};

		struct VkSpecInfo_RAII {
			VkSpecInfo_RAII() {}
			VkSpecInfo_RAII(std::vector<SpecializationEntry> const& specEntries);
			VkSpecInfo_RAII(VkSpecInfo_RAII const& copy);
			VkSpecInfo_RAII& operator=(VkSpecInfo_RAII const& copy) = delete;
			VkSpecInfo_RAII(VkSpecInfo_RAII&& move) noexcept;
			VkSpecInfo_RAII& operator=(VkSpecInfo_RAII&& move) = delete;
			~VkSpecInfo_RAII();
			std::vector<VkSpecializationMapEntry> mapEntries{};
			VkSpecializationInfo specInfo{};
			uint64_t memPtr = 0;
		};


		std::string filepath{}; 
		VkPipelineShaderStageCreateInfo shaderStageCreateInfo;

		DescriptorLayoutPack* descriptorSets;
		std::vector<VkVertexInputAttributeDescription> vertexInputAttributes{};
		VkPushConstantRange pushRange{};

		std::vector<SpecializationEntry> defaultSpecConstants{};

		explicit Shader(std::string_view fileLocation);
		Shader(std::string_view fileLocation, const std::size_t dataSize, const void* data);
		Shader();
		~Shader();

		bool ValidateVertexInputAttributes(std::vector<VkVertexInputAttributeDescription> const& cpu_side) const;
		VkShaderModule GetVkShader() const {
			return shaderStageCreateInfo.module;
		}
#if PIPELINE_HOT_RELOAD
		std::vector<VkShaderModule> staleModules{};
		void HotReload();
#endif
		void DrawImgui();

	//protected:
		void CompileModule(const std::size_t dataSize, const void* data);
		void ReadReflection(const std::size_t dataSize, const void* data);
	};

	Shader* GetShader(std::string_view filepath);
	Shader* CreateShader(std::string_view filepath, const std::size_t dataSize, const void* data);
	void DestroyShader(Shader& shader);
	void DestroyAllShaders();
}