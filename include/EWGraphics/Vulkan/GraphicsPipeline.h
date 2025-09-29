#pragma once

//#include "DescriptorHandler.h"
#include "EWGraphics/Vulkan/Device.hpp"
#include "EWGraphics/Vulkan/Descriptors.h"
#include "EWGraphics/Vulkan/Shader.h"
#include "EWGraphics/PipelineSystem.h"


/*
#define PIPELINE_DERIVATIVES 0 //pipeline derivatives are not currently recommended by hardware vendors
https://developer.nvidia.com/blog/vulkan-dos-donts/ */

//#define DYNAMIC_PIPE_LAYOUT_COUNT 24 //MAX_TEXTURE_COUNT * 4 //defined in descriptorhandler.h

namespace EWE {

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

		void EnableAlphaBlending();
		void Enable2DConfig();
		//static PipelineConfigInfo DefaultPipelineConfigInfo(); //requires the move operator? something RVO
		void SetToDefaults();

		VkGraphicsPipelineCreateInfo CreateGraphicsInfo(PipeLayout& pipeLayout) const;


		std::vector<VkVertexInputBindingDescription> bindingDescriptions{};
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

		VkPipelineViewportStateCreateInfo viewportInfo{};
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
		VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
		VkPipelineMultisampleStateCreateInfo multisampleInfo{};
		uint32_t sampleMask[2];
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
		VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};

		std::vector<VkDynamicState> dynamicStateEnables{};
		VkPipelineDynamicStateCreateInfo dynamicStateInfo{};

		VkPipelineTessellationStateCreateInfo tessCreateInfo{};

		VkPipelineRenderingCreateInfo pipelineRenderingInfo;
		uint32_t subpass = 0;
		void* pNext;
#if PIPELINE_DERIVATIVES
		int32_t basePipelineIndex = -1;
		EWEPipeline* basePipelineHandle = nullptr;
		VkPipelineCreateFlags flags = 0;
#endif


		static VkPipelineRenderingCreateInfo* pipelineRenderingInfoStatic;
	};

	struct GraphicsPipeline : public Pipeline {
		GraphicsPipeline(PipelineID pipeID, PipeLayout* layout, PipelineConfigInfo& configInfo, std::vector<KeyValuePair<ShaderStage, std::vector<Shader::SpecializationEntry>>> const& specInfo);
		GraphicsPipeline(PipelineID pipeID, PipeLayout* layout, PipelineConfigInfo& configInfo);

		void CreateVkPipeline(PipelineConfigInfo& configInfo);
		
#if PIPELINE_HOT_RELOAD
		void HotReload(bool layoutReload = true) override final;
		PipelineConfigInfo copyConfigInfo;
#endif
	protected:
		//internal
		void CreateVkPipeline_SecondStage(PipelineConfigInfo& configInfo, VkGraphicsPipelineCreateInfo& pipelineInfo);
	};
}