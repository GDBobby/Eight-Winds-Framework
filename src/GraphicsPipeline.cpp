#include "EWGraphics/Vulkan/GraphicsPipeline.h"

#include "EWGraphics/Model/Model.h"
#include "EWGraphics/Vulkan/Renderer.h"

#if PIPELINE_HOT_RELOAD
#include "EWGraphics/Data/magic_enum.hpp"
#include "EWGraphics/imgui/imgui.h"

#include <algorithm>
#endif

#include <cassert>

namespace EWE {

	void imgui_vkbool(std::string_view name, VkBool32& vkBool) {
		bool loe = vkBool;
		ImGui::Checkbox(name.data(), &loe);
		vkBool = loe;
	}

	template<typename T>
	void imgui_enum(std::string_view name, T& val, int min, int max) {

		ImGui::SliderInt(name.data(), reinterpret_cast<int*>(&val), min, max, magic_enum::enum_name(val).data());
	}

	VkPipelineRenderingCreateInfo* PipelineConfigInfo::pipelineRenderingInfoStatic;

	void PipelineConfigInfo::Enable2DConfig() {
		depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	}

#if PIPELINE_HOT_RELOAD
	PipelineConfigInfo::PipelineConfigInfo(PipelineConfigInfo const& other) {

		bindingDescriptions = other.bindingDescriptions;
		attributeDescriptions = other.attributeDescriptions;

		inputAssemblyInfo = other.inputAssemblyInfo;
		viewportInfo = other.viewportInfo;
		rasterizationInfo = other.rasterizationInfo;
		multisampleInfo = other.multisampleInfo;
		if (other.multisampleInfo.pSampleMask != other.sampleMask) {
			printf("invalid copy of sampel mask, BE WARNED\n");
		}
		memcpy(sampleMask, other.sampleMask, sizeof(uint32_t) * 2);
		multisampleInfo.pSampleMask = sampleMask;          // Optional

		colorBlendAttachment = other.colorBlendAttachment;
		colorBlendInfo = other.colorBlendInfo;
		colorBlendInfo.pAttachments = &colorBlendAttachment;
		depthStencilInfo = other.depthStencilInfo;
		tessCreateInfo = other.tessCreateInfo;

		dynamicStateEnables = other.dynamicStateEnables;
		dynamicStateInfo.sType = other.dynamicStateInfo.sType;
		dynamicStateInfo.pNext = nullptr;
		dynamicStateInfo.pDynamicStates = dynamicStateEnables.data();
		dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());

		pipelineRenderingInfo = *PipelineConfigInfo::pipelineRenderingInfoStatic;
	}
#endif

	void PipelineConfigInfo::SetToDefaults() {
		inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

		viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportInfo.viewportCount = 1;
		viewportInfo.pViewports = nullptr;
		viewportInfo.scissorCount = 1;
		viewportInfo.pScissors = nullptr;

		rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationInfo.depthClampEnable = VK_FALSE;
		rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
		rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationInfo.lineWidth = 1.0f;
		rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
		rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizationInfo.depthBiasEnable = VK_FALSE;
		rasterizationInfo.depthBiasConstantFactor = 0.0f;  // Optional
		rasterizationInfo.depthBiasClamp = 0.0f;           // Optional
		rasterizationInfo.depthBiasSlopeFactor = 0.0f;     // Optional

		multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleInfo.sampleShadingEnable = VK_FALSE;
		multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampleInfo.minSampleShading = 1.0f;           // Optional
		sampleMask[0] = UINT32_MAX;
		sampleMask[1] = UINT32_MAX;
		multisampleInfo.pSampleMask = sampleMask;          // Optional
		multisampleInfo.alphaToCoverageEnable = VK_FALSE;  // Optional
		multisampleInfo.alphaToOneEnable = VK_FALSE;       // Optional

		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;              // Optional
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;              // Optional

		colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendInfo.logicOpEnable = VK_FALSE;
		colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;  // Optional
		colorBlendInfo.attachmentCount = 1;
		colorBlendInfo.pAttachments = &colorBlendAttachment;
		colorBlendInfo.blendConstants[0] = 0.0f;  // Optional
		colorBlendInfo.blendConstants[1] = 0.0f;  // Optional
		colorBlendInfo.blendConstants[2] = 0.0f;  // Optional
		colorBlendInfo.blendConstants[3] = 0.0f;  // Optional

		depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilInfo.depthTestEnable = VK_TRUE;
		depthStencilInfo.depthWriteEnable = VK_TRUE;
		depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
		depthStencilInfo.minDepthBounds = 0.0f;  // Optional
		depthStencilInfo.maxDepthBounds = 1.0f;  // Optional
		depthStencilInfo.stencilTestEnable = VK_FALSE;
		depthStencilInfo.front = {};  // Optional
		depthStencilInfo.back = {};   // Optional

		dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateInfo.pDynamicStates = dynamicStateEnables.data();
		dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
		dynamicStateInfo.flags = 0;

		pipelineRenderingInfo = *PipelineConfigInfo::pipelineRenderingInfoStatic;
	}

	void PipelineConfigInfo::EnableAlphaBlending() {
		colorBlendAttachment.blendEnable = VK_TRUE;

		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;

		//may want to change these 3 later
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	}
	void PipelineConfigInfo::RenderIMGUI() {
		static int currentImguiIndex = 0;
		if (imguiIndex < 0) {
			imguiIndex = currentImguiIndex;
			currentImguiIndex++;
		}


		std::string extension = "##p";
		extension += std::to_string(imguiIndex);

		std::string treeName = "pipeline config info";
		treeName += extension;

		std::string optionStr;
		if (ImGui::TreeNode(treeName.c_str())) {

			optionStr = "viewport info";
			optionStr += extension;
			if (ImGui::TreeNode(optionStr.c_str())) {

				optionStr = "topology";
				optionStr += extension;
				imgui_enum(optionStr, inputAssemblyInfo.topology, 0, 10);

				optionStr = "primitive restart enable";
				optionStr += extension;
				imgui_vkbool(optionStr, inputAssemblyInfo.primitiveRestartEnable);

				ImGui::TreePop();
			}
			//input assembly
			optionStr = "rasterzation info";
			optionStr += extension;
			if (ImGui::TreeNode(optionStr.c_str())) {

				optionStr = "depth clamp enable";
				optionStr += extension;
				imgui_vkbool(optionStr, rasterizationInfo.depthClampEnable);

				optionStr = "rasterizer discard enable";
				optionStr += extension;
				imgui_vkbool(optionStr, rasterizationInfo.rasterizerDiscardEnable);

				optionStr = "polygonMode";
				optionStr += extension;
				imgui_enum(optionStr, rasterizationInfo.polygonMode, 0, 2);

				optionStr = "cullMode";
				optionStr += extension;
				ImGui::DragInt(optionStr.c_str(), reinterpret_cast<int*>(&rasterizationInfo.cullMode), 1, 0, 100);
				VkCullModeFlagBits copyCull = static_cast<VkCullModeFlagBits>(rasterizationInfo.cullMode);
				imgui_enum(optionStr.c_str(), copyCull, 0, 3);
				rasterizationInfo.cullMode = copyCull;

				optionStr = "frontFace";
				optionStr += extension;
				imgui_enum(optionStr, rasterizationInfo.frontFace, 0, 1);

				optionStr = "depthBiasEnable";
				optionStr += extension;
				imgui_vkbool(optionStr, rasterizationInfo.depthBiasEnable);

				optionStr = "depth bias constant factor";
				optionStr += extension;
				ImGui::DragFloat(optionStr.c_str(), &rasterizationInfo.depthBiasConstantFactor, 0.1f, 0.f, 100.f);

				optionStr = "depth bias clamp";
				optionStr += extension;
				ImGui::DragFloat(optionStr.c_str(), &rasterizationInfo.depthBiasClamp, 0.1f, 0.f, 100.f);

				optionStr = "depth bias slope factor";
				optionStr += extension;
				ImGui::DragFloat(optionStr.c_str(), &rasterizationInfo.depthBiasSlopeFactor, 0.1f, 0.f, 100.f);

				optionStr = "line width";
				optionStr += extension;
				ImGui::DragFloat(optionStr.c_str(), &rasterizationInfo.lineWidth, 0.1f, 0.f, 100.f);


				ImGui::TreePop();
			}

			optionStr = "color blend";
			optionStr += extension;
			if (ImGui::TreeNode(optionStr.c_str())) {

				optionStr = "blend constants";
				optionStr += extension;
				ImGui::DragFloat4(optionStr.c_str(), colorBlendInfo.blendConstants, 0.01f, 0.f, 1.f);

				optionStr = "logic op";
				optionStr += extension;
				//ImGui::SliderInt(optionStr.c_str(), reinterpret_cast<int*>(&colorBlendInfo.logicOp), 0, 15, magic_enum::enum_name(colorBlendInfo.logicOp).data());
				imgui_enum(optionStr, colorBlendInfo.logicOp, 0, 15);

				optionStr = "logic op enable";
				optionStr += extension;
				imgui_vkbool(optionStr, colorBlendInfo.logicOpEnable);
				ImGui::TreePop();
			}
			optionStr = "dynamic states";
			optionStr += extension;
			if (ImGui::TreeNode(optionStr.c_str())) {
				for (auto const& dynState : dynamicStateEnables) {
					ImGui::Text("%s - %d", magic_enum::enum_name(dynState).data(), dynState);
				}
				ImGui::TreePop();
			}

			//if(ImGui::TreeNode(optionStr.c_str(), ))

			ImGui::TreePop();
		}


	}

	void GraphicsPipeline::CreateVkPipeline_SecondStage(PipelineConfigInfo& configInfo, VkGraphicsPipelineCreateInfo& pipelineInfo) {

		pipelineInfo.pNext = &configInfo.pipelineRenderingInfo;
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(configInfo.attributeDescriptions.size());
		vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(configInfo.bindingDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = configInfo.attributeDescriptions.data();
		vertexInputInfo.pVertexBindingDescriptions = configInfo.bindingDescriptions.data();
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &configInfo.inputAssemblyInfo;

		pipelineInfo.pViewportState = &configInfo.viewportInfo;
		pipelineInfo.pRasterizationState = &configInfo.rasterizationInfo;
		configInfo.multisampleInfo.pSampleMask = configInfo.sampleMask;
		pipelineInfo.pMultisampleState = &configInfo.multisampleInfo;

		pipelineInfo.pColorBlendState = &configInfo.colorBlendInfo;
		pipelineInfo.pDepthStencilState = &configInfo.depthStencilInfo;
		pipelineInfo.pDynamicState = &configInfo.dynamicStateInfo;

		pipelineInfo.layout = pipeLayout->vkLayout;
		pipelineInfo.subpass = configInfo.subpass;

		EWE_VK(vkCreateGraphicsPipelines, VK::Object->vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &vkPipe);
	}

	void GraphicsPipeline::CreateVkPipeline(PipelineConfigInfo& configInfo) {
		std::vector<KeyValuePair<ShaderStage, Shader::VkSpecInfo_RAII>> temp{};
		for (auto& stage : copySpecInfo) {
			temp.push_back(KeyValuePair<ShaderStage, Shader::VkSpecInfo_RAII>(stage.key, Shader::VkSpecInfo_RAII(stage.value)));
		}
		VkGraphicsPipelineCreateInfo pipelineInfo{};
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages = pipeLayout->GetStageData(temp);
		pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineInfo.pStages = shaderStages.data();

		CreateVkPipeline_SecondStage(configInfo, pipelineInfo);
	}


	GraphicsPipeline::GraphicsPipeline(PipelineID pipeID, PipeLayout* layout, PipelineConfigInfo& configInfo) : Pipeline{ pipeID, layout }
#if PIPELINE_HOT_RELOAD
		, copyConfigInfo{ configInfo }
#endif
	{
		//read the default spec info
		CreateVkPipeline(configInfo);
	}
	GraphicsPipeline::GraphicsPipeline(PipelineID pipeID, PipeLayout* layout, PipelineConfigInfo& configInfo, std::vector<KeyValuePair<ShaderStage, std::vector<Shader::SpecializationEntry>>> const& specInfo) : Pipeline{ pipeID, layout, specInfo }
#if PIPELINE_HOT_RELOAD
		, copyConfigInfo{ configInfo }
#endif
	{
		CreateVkPipeline(configInfo);
	}

#if PIPELINE_HOT_RELOAD
	void GraphicsPipeline::HotReload(bool layoutReload) {
		//layout->Reload();
		if (layoutReload) {
			pipeLayout->HotReload();
		}
		stalePipeline = vkPipe;
		vkPipe = VK_NULL_HANDLE;
		CreateVkPipeline(copyConfigInfo);
		//std::swap(vkPipe, stalePipeline);
	}
#endif
}