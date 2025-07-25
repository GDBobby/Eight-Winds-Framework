#pragma once

//#include "DescriptorHandler.h"
#include "EWGraphics/Vulkan/Device.hpp"
#include "EWGraphics/Vulkan/Descriptors.h"

#include <map>
#include <array>


/*
#define PIPELINE_DERIVATIVES 0 //pipeline derivatives are not currently recommended by hardware vendors
https://developer.nvidia.com/blog/vulkan-dos-donts/ */

//#define DYNAMIC_PIPE_LAYOUT_COUNT 24 //MAX_TEXTURE_COUNT * 4 //defined in descriptorhandler.h

namespace EWE {

	namespace Pipeline_Helper_Functions {
		void CreateShaderModule(std::string const& file_path, VkShaderModule* shaderModule);
		std::vector<char> ReadFile(const std::string& filepath);

		void CreateShaderModule(const std::vector<uint32_t>& data, VkShaderModule* shaderModule);
		void CreateShaderModule(const char* data, std::size_t dataSize, VkShaderModule* shaderModule);

		template <typename T>
		void CreateShaderModule(const std::vector<T>& data, VkShaderModule* shaderModule);
	}

	namespace Shader {
		enum Stage { //i dont know if the order matters
			vert = 0,
			tessControl,
			tessEval,
			geom,
			task,
			mesh,
			frag,

			COUNT
		};
	}
	struct ShaderTrackingStruct {
		struct IndividualShaderTrackingStruct{
			std::string filepath{};
			VkShaderModule shader{VK_NULL_HANDLE};
		};

		std::array<IndividualShaderTrackingStruct, Shader::Stage::COUNT> shaderData{};

		uint8_t Count() const;
		void Validate() const;

		void RenderIMGUI();
		int16_t imguiIndex = -1;
	};

	class EWE_Compute_Pipeline {
	public:
		VkPipelineLayout pipe_layout;
		VkPipeline pipeline;

		static EWE_Compute_Pipeline CreatePipeline(std::vector<VkDescriptorSetLayout> computeDSL, std::string compute_path);
		static EWE_Compute_Pipeline CreatePipeline(VkPipelineLayout pipe_layout, std::string compute_path);
		void Bind(CommandBuffer& cmdBuf) {
			EWE_VK(vkCmdBindPipeline, cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
		}
	private:
		VkShaderModule shader;
	};

	

	class EWEPipeline {
	public:

		static VkShaderModule CheckIfShaderExist(std::string const& path);

		struct PipelineConfigInfo {
			PipelineConfigInfo() = default;
			void RenderIMGUI();
			int16_t imguiIndex = -1;
#if PIPELINE_HOT_RELOAD
			PipelineConfigInfo(PipelineConfigInfo const&);
#else
			PipelineConfigInfo(const PipelineConfigInfo&) = delete;
#endif
			PipelineConfigInfo& operator=(PipelineConfigInfo const&) = delete;
			PipelineConfigInfo(PipelineConfigInfo&&) = delete;
			PipelineConfigInfo& operator=(PipelineConfigInfo&&) = delete;

			//void AddGeomShaderModule(const MaterialFlags flags); //i need to figure out a different way to do this

			//VkViewport viewport;
			//VkRect2D scissor;

			std::vector<VkVertexInputBindingDescription> bindingDescriptions{};
			std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

			VkPipelineViewportStateCreateInfo viewportInfo{};
			VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
			VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
			VkPipelineMultisampleStateCreateInfo multisampleInfo{};
			VkPipelineColorBlendAttachmentState colorBlendAttachment{};
			VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
			VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};

			std::vector<VkDynamicState> dynamicStateEnables{};
			VkPipelineDynamicStateCreateInfo dynamicStateInfo{};

			VkPipelineTessellationStateCreateInfo tessCreateInfo{};

			VkPipelineLayout pipelineLayout = nullptr;
			//VkPipelineRenderingCreateInfo const& pipeRenderInfo = nullptr;
			VkPipelineRenderingCreateInfo pipelineRenderingInfo;
			uint32_t subpass = 0;
			void* pNext;
		#if PIPELINE_DERIVATIVES
			int32_t basePipelineIndex = -1;
			EWEPipeline* basePipelineHandle = nullptr;
			VkPipelineCreateFlags flags = 0;
		#endif

			VkPipelineCache cache = VK_NULL_HANDLE;


			static VkPipelineRenderingCreateInfo* pipelineRenderingInfoStatic;
		};

		EWEPipeline(ShaderTrackingStruct const& stringStruct, PipelineConfigInfo const& configInfo);

		~EWEPipeline();

		EWEPipeline(EWEPipeline const&) = delete;
		EWEPipeline& operator=(EWEPipeline const&) = delete;

		void Bind();
		static void DefaultPipelineConfigInfo(PipelineConfigInfo& configInfo);
		static void Enable2DConfig(PipelineConfigInfo& configInfo);
		static void EnableAlphaBlending(PipelineConfigInfo& configInfo);

		static void CleanShaderModules();
#if DEBUG_NAMING
		void SetDebugName(std::string const& name);
#endif
#if PIPELINE_HOT_RELOAD
		void ReloadShaderModules();
		void HotReloadPipeline(bool reloadShaders);

		//void RecompileShaderModules(ShaderStringStruct const& stringStruct);
		//void ReloadConfig(PipelineConfigInfo const& configInfo);

		PipelineConfigInfo copyConfigInfo;
		uint16_t framesSinceSwap = 0;
		VkPipeline stalePipeline = VK_NULL_HANDLE; //going to let it tick for MAX_FRAMES_IN_FLIGHT + 1 then delete it
		int32_t imguiIndex = -1;
		ShaderTrackingStruct shaderModules{};

		VkPipeline graphicsPipeline;
	private:
#else
	private:
		ShaderTrackingStruct shaderModules{};

#endif




		void CreateGraphicsPipeline(PipelineConfigInfo const& configInfo);

	};
}