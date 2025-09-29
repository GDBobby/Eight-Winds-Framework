#include "EWGraphics/Vulkan/ComputePipeline.h"

#if PIPELINE_HOT_RELOAD
#include "EWGraphics/imgui/imgui.h"
#endif

namespace EWE{

	void ComputePipeline::CreateVkPipeline() {
		VkComputePipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.pNext = nullptr;
		pipelineInfo.layout = pipeLayout->vkLayout;
		pipelineInfo.stage = pipeLayout->shaders[ShaderStage::Compute]->shaderStageCreateInfo;

		Shader::VkSpecInfo_RAII temp{ copySpecInfo[0].value };
		pipelineInfo.stage.pSpecializationInfo = &temp.specInfo;
		EWE_VK(vkCreateComputePipelines, EWE::VK::Object->vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &vkPipe);

	}

	ComputePipeline::ComputePipeline(PipelineID pipeID, PipeLayout* layout) 
		: Pipeline{ pipeID, layout }
	{
		this->pipeLayout = layout;
		CreateVkPipeline();
	}

	ComputePipeline::ComputePipeline(PipelineID pipeID, PipeLayout* layout, std::vector<Shader::SpecializationEntry> const& specInfo) : 
		Pipeline{ pipeID, layout, std::vector<KeyValuePair<ShaderStage, std::vector<Shader::SpecializationEntry>>>{KeyValuePair<ShaderStage, std::vector<Shader::SpecializationEntry>>(ShaderStage::Compute, specInfo)} }
	{
		this->pipeLayout = layout;
		CreateVkPipeline();
	}

#if PIPELINE_HOT_RELOAD
	void ComputePipeline::HotReload(bool layoutReload) {
		//layout should ALWAYS be reloaded
		pipeLayout->HotReload();
		stalePipeline = vkPipe;
		vkPipe = VK_NULL_HANDLE;
		CreateVkPipeline();
	}
#endif
}