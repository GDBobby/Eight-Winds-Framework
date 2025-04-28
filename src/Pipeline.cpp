#include "EWGraphics/Vulkan/Pipeline.h"


#include "EWGraphics/Model/Model.h"
#include "EWGraphics/Vulkan/Renderer.h"

#if PIPELINE_HOT_RELOAD
#include "EWGraphics/Data/magic_enum.hpp"
#include "EWGraphics/imgui/imgui.h"

#include <algorithm>
#endif

#include <fstream>
#include <iostream>
#include <cassert>

#ifndef SHADER_DIR
#define SHADER_DIR "shaders/"
#endif

namespace EWE {

	namespace Pipeline_Helper_Functions {
		std::vector<char> ReadFile(const std::string& filepath) {

			std::string enginePath = SHADER_DIR + filepath;

			std::ifstream shaderFile{};
			shaderFile.open(enginePath, std::ios::binary);
			if(!shaderFile.is_open()){
				printf("failed ot open shader file : %s\n", enginePath.c_str());
				assert(shaderFile.is_open() && "failed to open shader");
			}
			shaderFile.seekg(0, std::ios::end);
			const std::size_t fileSize = static_cast<std::size_t>(shaderFile.tellg());
			assert(fileSize > 0 && "shader is empty");

			shaderFile.seekg(0, std::ios::beg);
			std::vector<char> returnVec(fileSize);
			shaderFile.read(returnVec.data(), fileSize);
			shaderFile.close();
			return returnVec;
		}
		void CreateShaderModule(std::string const& file_path, VkShaderModule* shaderModule) {
			auto data = ReadFile(file_path);
			VkShaderModuleCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.pNext = nullptr;
			createInfo.flags = 0;
			createInfo.codeSize = data.size();
			//printf("template data size : %d \n", data.size());
			createInfo.pCode = reinterpret_cast<const uint32_t*>(data.data());

			EWE_VK(vkCreateShaderModule, VK::Object->vkDevice, &createInfo, nullptr, shaderModule);
		}
		void CreateShaderModule(const std::vector<uint32_t>& data, VkShaderModule* shaderModule) {
			VkShaderModuleCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.pNext = nullptr;
			createInfo.flags = 0;
			createInfo.codeSize = data.size() * 4;
			//printf("uint32_t data size : %d \n", data.size());
			createInfo.pCode = data.data();

			EWE_VK(vkCreateShaderModule, VK::Object->vkDevice, &createInfo, nullptr, shaderModule);
		}
		template <typename T>
		void CreateShaderModule(const std::vector<T>& data, VkShaderModule* shaderModule) {
#if EWE_DEBUG
			//printf("creating sahder module\n");
#endif
			VkShaderModuleCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.pNext = nullptr;
			createInfo.flags = 0;
			createInfo.codeSize = data.size() * sizeof(T);
			//printf("template data size : %d \n", data.size());
			createInfo.pCode = reinterpret_cast<const uint32_t*>(data.data());

			EWE_VK(vkCreateShaderModule, VK::Object->vkDevice, &createInfo, nullptr, shaderModule);
		}
		void CreateShaderModule(const void* data, std::size_t dataSize, VkShaderModule* shaderModule) {
			VkShaderModuleCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.pNext = nullptr;
			createInfo.flags = 0;
			createInfo.codeSize = dataSize;
			createInfo.pCode = reinterpret_cast<const uint32_t*>(data);
			//printf("uint32_t data size : %d \n", data.size());
			EWE_VK(vkCreateShaderModule, VK::Object->vkDevice, &createInfo, nullptr, shaderModule);
		}
	}

	// ~~~~~~~ COMPUTE PIPELINE ~~~~~~~~~~~~~~~
	EWE_Compute_Pipeline EWE_Compute_Pipeline::CreatePipeline(std::vector<VkDescriptorSetLayout> computeDSL, std::string compute_path) {
		EWE_Compute_Pipeline ret;
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(computeDSL.size());
		pipelineLayoutInfo.pSetLayouts = computeDSL.data();

		EWE_VK(vkCreatePipelineLayout, VK::Object->vkDevice, &pipelineLayoutInfo, nullptr, &ret.pipe_layout);
		

		VkComputePipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.layout = ret.pipe_layout;

		std::string computePath = "compute/";
		computePath += compute_path;
		Pipeline_Helper_Functions::CreateShaderModule(computePath, &ret.shader);
		VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
		computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		computeShaderStageInfo.module = ret.shader;
		computeShaderStageInfo.pName = "main";
		pipelineInfo.stage = computeShaderStageInfo;
		EWE_VK(vkCreateComputePipelines, VK::Object->vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &ret.pipeline);
		return ret;
	}
	EWE_Compute_Pipeline EWE_Compute_Pipeline::CreatePipeline(VkPipelineLayout pipe_layout, std::string compute_path) {
		EWE_Compute_Pipeline ret;
		ret.pipe_layout = pipe_layout;

		VkComputePipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.layout = ret.pipe_layout;
		Pipeline_Helper_Functions::CreateShaderModule(compute_path, &ret.shader);
		VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
		computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		computeShaderStageInfo.module = ret.shader;
		computeShaderStageInfo.pName = "computeMain";
		pipelineInfo.stage = computeShaderStageInfo;
		EWE_VK(vkCreateComputePipelines, VK::Object->vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &ret.pipeline);
		return ret;
	}

	// ~~~~~~~~~~~~~~~~~~~~ END COMPUTE PIPELINE ~~~~~~~~~~~~~~~~~~~~~~

#if EWE_DEBUG
	void Validate(ShaderStringStruct const& stringStruct) {
		bool hasNormalPipeline = false;
		hasNormalPipeline |= stringStruct.filepath[Shader::vert].size() > 0;
		hasNormalPipeline |= stringStruct.filepath[Shader::geom].size() > 0;
		hasNormalPipeline |= stringStruct.filepath[Shader::tessControl].size() > 0;
		hasNormalPipeline |= stringStruct.filepath[Shader::tessEval].size() > 0;

		bool hasMeshPipeline = false;
		hasMeshPipeline |= stringStruct.filepath[Shader::task].size() > 0;
		hasMeshPipeline |= stringStruct.filepath[Shader::mesh].size() > 0;

		assert(hasMeshPipeline ^ hasNormalPipeline);
		//throw std::runtime_error("invalid shader filepaths");
	}
#endif


	uint8_t ShaderStringStruct::Count() const {
		uint8_t ret = 0;
		for (uint8_t i = 0; i < Shader::Stage::COUNT; i++) {
			ret += filepath[i].size() > 0;
		}
		return ret;
	}

	struct ShaderModuleTracker {
		VkShaderModule shader;
		int16_t usageCount;
		ShaderModuleTracker(VkShaderModule shader) : shader{ shader }, usageCount{ 1 } {}
	};
	std::map<std::string, ShaderModuleTracker> shaderModuleMap;
	std::mutex shaderMapMutex;


	void GetShader(std::string const& filepath, VkShaderModule& shader) {
		auto modFind = shaderModuleMap.find(filepath);
		if (modFind == shaderModuleMap.end()) {
			const auto shaderCode = Pipeline_Helper_Functions::ReadFile(filepath);
			Pipeline_Helper_Functions::CreateShaderModule(shaderCode, &shader);
			shaderMapMutex.lock();
			shaderModuleMap.try_emplace(filepath, shader);
			shaderMapMutex.unlock();
		}
		else {
			shaderMapMutex.lock();
			modFind->second.usageCount++;
			shader = modFind->second.shader;
			shaderMapMutex.unlock();
		}
	}
	void DestroyShader(VkShaderModule shader, bool lock = true) {
		if (shader != VK_NULL_HANDLE) {
			if (lock) {
				shaderMapMutex.lock();
			}
			for (auto& shaderTracker : shaderModuleMap) {
				if (shaderTracker.second.shader == shader) {
					shaderTracker.second.usageCount--;
					if (shaderTracker.second.usageCount <= 0) {
						EWE_VK(vkDestroyShaderModule, VK::Object->vkDevice, shader, nullptr);
					}
					shaderModuleMap.erase(shaderTracker.first);

					if (lock) {
						shaderMapMutex.unlock();
					}
					return;
				}
			}
#if PIPELINE_HOT_RELOAD
			EWE_VK(vkDestroyShaderModule, VK::Object->vkDevice, shader, nullptr);
			return;
#endif
			EWE_UNREACHABLE;
		}
	}

	VkPipelineRenderingCreateInfo* EWEPipeline::PipelineConfigInfo::pipelineRenderingInfoStatic;


	EWEPipeline::EWEPipeline(ShaderStringStruct const& stringStruct, PipelineConfigInfo const& configInfo)
#if PIPELINE_HOT_RELOAD
		: copyConfigInfo{configInfo},
		copyStringStruct{stringStruct}
#endif
	{
#if EWE_DEBUG
		Validate(stringStruct);
#endif

		for (uint8_t i = 0; i < Shader::Stage::COUNT; i++) {
			if (stringStruct.filepath[i].size() > 0) {
				GetShader(stringStruct.filepath[i], shaderModules[i]);
			}
		}
		CreateGraphicsPipeline(configInfo);
	}

	EWEPipeline::EWEPipeline(std::array<VkShaderModule, Shader::Stage::COUNT> const& shaderModules, const PipelineConfigInfo& configInfo)
	: shaderModules{shaderModules}
#if PIPELINE_HOT_RELOAD
		, copyConfigInfo{ configInfo },
		copyStringStruct{}
#endif 
	{
		CreateGraphicsPipeline(configInfo);
	}
	

	EWEPipeline::~EWEPipeline() {
		EWE_VK(vkDestroyPipeline, VK::Object->vkDevice, graphicsPipeline, nullptr);
		shaderMapMutex.lock();
		for (uint8_t i = 0; i < Shader::Stage::COUNT; i++) {
			DestroyShader(shaderModules[i], false);
		}
		shaderMapMutex.unlock();
	}

	void EWEPipeline::Bind() {
		EWERenderer::BindGraphicsPipeline(graphicsPipeline);
	}

#if DEBUG_NAMING
	void EWEPipeline::SetDebugName(std::string const& name) {
		DebugNaming::SetObjectName(graphicsPipeline, VK_OBJECT_TYPE_PIPELINE, name.c_str());
	}
#endif

	void EWEPipeline::CreateGraphicsPipeline(const PipelineConfigInfo& configInfo) {

		assert(configInfo.pipelineLayout != VK_NULL_HANDLE && "Cannot create graphics pipeline:: no pipelineLayout provided in configInfo");
		//assert(configInfo.renderPass != VK_NULL_HANDLE && "Cannot create graphics pipeline:: no renderPass provided in configInfo");

		uint8_t currentStage = 0; 

		uint8_t shaderCount = 0;
		for (uint8_t i = 0; i < Shader::Stage::COUNT; i++) {
			shaderCount += shaderModules[i] != VK_NULL_HANDLE;
		}
		if (shaderCount < 2) {
			//the material system generates the modules sometimes, it wont show in the strings
			shaderCount = 2;
		}

		std::vector<VkPipelineShaderStageCreateInfo> shaderStages{ shaderCount };

		if (shaderModules[Shader::mesh] != VK_NULL_HANDLE) {
			assert(shaderModules[Shader::vert] == VK_NULL_HANDLE);
			assert(shaderModules[Shader::tessControl] == VK_NULL_HANDLE);
			assert(shaderModules[Shader::tessEval] == VK_NULL_HANDLE);
			assert(shaderModules[Shader::geom] == VK_NULL_HANDLE);

			if (shaderModules[Shader::task] != VK_NULL_HANDLE) {
				shaderStages[currentStage].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				shaderStages[currentStage].stage = VK_SHADER_STAGE_TASK_BIT_EXT;
				shaderStages[currentStage].module = shaderModules[Shader::task];
				shaderStages[currentStage].pName = "main";
				shaderStages[currentStage].flags = 0;
				shaderStages[currentStage].pNext = nullptr;
				shaderStages[currentStage].pSpecializationInfo = nullptr;

				currentStage++;
			}

			shaderStages[currentStage].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStages[currentStage].stage = VK_SHADER_STAGE_MESH_BIT_EXT;
			shaderStages[currentStage].module = shaderModules[Shader::mesh];
			shaderStages[currentStage].pName = "main";
			shaderStages[currentStage].flags = 0;
			shaderStages[currentStage].pNext = nullptr;
			shaderStages[currentStage].pSpecializationInfo = nullptr;
			currentStage++;
		}
		else {
			shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
			shaderStages[0].module = shaderModules[Shader::vert];
			shaderStages[0].pName = "main";
			shaderStages[0].flags = 0;
			shaderStages[0].pNext = nullptr;
			shaderStages[0].pSpecializationInfo = nullptr;
			currentStage = 1;

			if (shaderModules[Shader::tessControl] != VK_NULL_HANDLE) {
				{
					auto& tescStage = shaderStages[currentStage];
					currentStage++;
					tescStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
					tescStage.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
					tescStage.module = shaderModules[Shader::tessControl];
					tescStage.pName = "main";
					tescStage.flags = 0;
					tescStage.pNext = nullptr;
					tescStage.pSpecializationInfo = nullptr;
				}
				{
					auto& teseStage = shaderStages[currentStage];
					currentStage++;
					teseStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
					teseStage.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
					teseStage.module = shaderModules[Shader::tessEval];
					teseStage.pName = "main";
					teseStage.flags = 0;
					teseStage.pNext = nullptr;
					teseStage.pSpecializationInfo = nullptr;
				}
			}

			if (shaderModules[Shader::geom] != VK_NULL_HANDLE) {
				auto& geomStage = shaderStages[currentStage];
				currentStage++;
				geomStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				geomStage.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
				geomStage.module = shaderModules[Shader::geom];
				geomStage.pName = "main";
				geomStage.flags = 0;
				geomStage.pNext = nullptr;
				geomStage.pSpecializationInfo = nullptr;
			}
		}

		
		auto& fragStage = shaderStages[currentStage];
		fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragStage.module = shaderModules[Shader::frag];
		fragStage.pName = "main";
		fragStage.flags = 0;
		fragStage.pNext = nullptr;
		fragStage.pSpecializationInfo = nullptr;

		auto& bindingDescriptions = configInfo.bindingDescriptions;
		auto& attributeDescriptions = configInfo.attributeDescriptions;
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
		vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.pNext = &configInfo.pipelineRenderingInfo;
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineInfo.pStages = shaderStages.data();
		if (shaderModules[Shader::mesh] != VK_NULL_HANDLE) {
			pipelineInfo.pVertexInputState = nullptr;
			pipelineInfo.pInputAssemblyState = nullptr;
		}
		else {
			pipelineInfo.pVertexInputState = &vertexInputInfo;
			pipelineInfo.pInputAssemblyState = &configInfo.inputAssemblyInfo;
		}
		pipelineInfo.pViewportState = &configInfo.viewportInfo;
		pipelineInfo.pRasterizationState = &configInfo.rasterizationInfo;
		pipelineInfo.pMultisampleState = &configInfo.multisampleInfo;

		pipelineInfo.pColorBlendState = &configInfo.colorBlendInfo;
		pipelineInfo.pDepthStencilState = &configInfo.depthStencilInfo;
		pipelineInfo.pDynamicState = &configInfo.dynamicStateInfo;

		pipelineInfo.layout = configInfo.pipelineLayout;
		pipelineInfo.subpass = configInfo.subpass;
		if(shaderModules[Shader::tessControl] != VK_NULL_HANDLE){
			pipelineInfo.pTessellationState = &configInfo.tessCreateInfo;
		}

		EWE_VK(vkCreateGraphicsPipelines, VK::Object->vkDevice, configInfo.cache, 1, &pipelineInfo, nullptr, &graphicsPipeline);
	}

	void EWEPipeline::Enable2DConfig(PipelineConfigInfo& configInfo) {
		configInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	}

#if PIPELINE_HOT_RELOAD
	EWEPipeline::PipelineConfigInfo::PipelineConfigInfo(PipelineConfigInfo const& other) {

		bindingDescriptions = other.bindingDescriptions;
		attributeDescriptions = other.attributeDescriptions;

		inputAssemblyInfo = other.inputAssemblyInfo;
		viewportInfo = other.viewportInfo;
		rasterizationInfo = other.rasterizationInfo;
		multisampleInfo = other.multisampleInfo;
		colorBlendAttachment = other.colorBlendAttachment;
		colorBlendInfo = other.colorBlendInfo;
		colorBlendInfo.pAttachments = &colorBlendAttachment;
		depthStencilInfo = other.depthStencilInfo;
		tessCreateInfo = other.tessCreateInfo;

		pipelineLayout = other.pipelineLayout;
		cache = other.cache;

		dynamicStateEnables = other.dynamicStateEnables;
		dynamicStateInfo.sType = other.dynamicStateInfo.sType;
		dynamicStateInfo.pNext = nullptr; //idk if this is correct or if i need some more complicated handling of this
		dynamicStateInfo.pDynamicStates = dynamicStateEnables.data();
		dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());

		pipelineRenderingInfo = *PipelineConfigInfo::pipelineRenderingInfoStatic;
	}
#endif

	void EWEPipeline::DefaultPipelineConfigInfo(PipelineConfigInfo& configInfo) {

		configInfo.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		configInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		configInfo.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

		configInfo.viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		configInfo.viewportInfo.viewportCount = 1;
		configInfo.viewportInfo.pViewports = nullptr;
		configInfo.viewportInfo.scissorCount = 1;
		configInfo.viewportInfo.pScissors = nullptr;

		configInfo.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		configInfo.rasterizationInfo.depthClampEnable = VK_FALSE;
		configInfo.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
		configInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
		//configInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_LINE;
		configInfo.rasterizationInfo.lineWidth = 1.0f;
		configInfo.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
		configInfo.rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
		configInfo.rasterizationInfo.depthBiasEnable = VK_FALSE;
		configInfo.rasterizationInfo.depthBiasConstantFactor = 0.0f;  // Optional
		configInfo.rasterizationInfo.depthBiasClamp = 0.0f;           // Optional
		configInfo.rasterizationInfo.depthBiasSlopeFactor = 0.0f;     // Optional

		configInfo.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		configInfo.multisampleInfo.sampleShadingEnable = VK_FALSE;
		configInfo.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		configInfo.multisampleInfo.minSampleShading = 1.0f;           // Optional
		configInfo.multisampleInfo.pSampleMask = nullptr;             // Optional
		configInfo.multisampleInfo.alphaToCoverageEnable = VK_FALSE;  // Optional
		configInfo.multisampleInfo.alphaToOneEnable = VK_FALSE;       // Optional

		configInfo.colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		configInfo.colorBlendAttachment.blendEnable = VK_FALSE;
		configInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
		configInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
		configInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;              // Optional
		configInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
		configInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
		configInfo.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;              // Optional

		configInfo.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		configInfo.colorBlendInfo.logicOpEnable = VK_FALSE;
		configInfo.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;  // Optional
		configInfo.colorBlendInfo.attachmentCount = 1;
		configInfo.colorBlendInfo.pAttachments = &configInfo.colorBlendAttachment;
		configInfo.colorBlendInfo.blendConstants[0] = 0.0f;  // Optional
		configInfo.colorBlendInfo.blendConstants[1] = 0.0f;  // Optional
		configInfo.colorBlendInfo.blendConstants[2] = 0.0f;  // Optional
		configInfo.colorBlendInfo.blendConstants[3] = 0.0f;  // Optional

		configInfo.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		configInfo.depthStencilInfo.depthTestEnable = VK_TRUE;
		configInfo.depthStencilInfo.depthWriteEnable = VK_TRUE;
		configInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
		configInfo.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
		configInfo.depthStencilInfo.minDepthBounds = 0.0f;  // Optional
		configInfo.depthStencilInfo.maxDepthBounds = 1.0f;  // Optional
		configInfo.depthStencilInfo.stencilTestEnable = VK_FALSE;
		configInfo.depthStencilInfo.front = {};  // Optional
		configInfo.depthStencilInfo.back = {};   // Optional

		configInfo.dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		configInfo.dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		configInfo.dynamicStateInfo.pDynamicStates = configInfo.dynamicStateEnables.data();
		configInfo.dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(configInfo.dynamicStateEnables.size());
		configInfo.dynamicStateInfo.flags = 0;

		configInfo.pipelineRenderingInfo = *PipelineConfigInfo::pipelineRenderingInfoStatic;
		/*
		std::vector<VkVertexInputBindingDescription> bindingDescription(1);
		bindingDescription[0].binding = 0;                            // Binding index
		bindingDescription[0].stride = 0;                             // No per-vertex data
		bindingDescription[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Input rate

		// Define the vertex input attribute description
		std::vector<VkVertexInputAttributeDescription> attributeDescription(1);
		attributeDescription[0].binding = 0;        // Binding index (should match the binding description)
		attributeDescription[0].location = 0;       // Location in the vertex shader
		attributeDescription[0].format = VK_FORMAT_R32G32B32A32_SFLOAT; // Format of the attribute data
		attributeDescription[0].offset = 0;

		configInfo.bindingDescriptions = bindingDescription;
		configInfo.attributeDescriptions = attributeDescription;
		*/
	}

	void EWEPipeline::EnableAlphaBlending(PipelineConfigInfo& configInfo) {
		configInfo.colorBlendAttachment.blendEnable = VK_TRUE;

		configInfo.colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		configInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		configInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		configInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;

		//may want to change these 3 later
		configInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		configInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		configInfo.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	}
	void EWEPipeline::CleanShaderModules() {
		for (auto iter = shaderModuleMap.begin(); iter != shaderModuleMap.end(); iter++) {
			EWE_VK(vkDestroyShaderModule, VK::Object->vkDevice, iter->second.shader, nullptr);
		}
		shaderModuleMap.clear();
	}

#if PIPELINE_HOT_RELOAD
	void EWEPipeline::ReloadShaderModules() {
		for (uint8_t i = 0; i < Shader::Stage::COUNT; i++) {
			if (copyStringStruct.filepath[i].size() > 0) {
				auto modFind = shaderModuleMap.find(copyStringStruct.filepath[i]);
				if (modFind == shaderModuleMap.end()) {
					assert(false && "this should be supported");
				}
				else {
					shaderMapMutex.lock();
					const auto shaderCode = Pipeline_Helper_Functions::ReadFile(copyStringStruct.filepath[i]);
					Pipeline_Helper_Functions::CreateShaderModule(shaderCode, &shaderModules[i]);
					modFind->second = shaderModules[i];
					shaderMapMutex.unlock();
				}
			}
			else if (shaderModules[i] != VK_NULL_HANDLE) {
				printf("this is 100 percent a gpu memory leak. im just too lazy to fix it rn\n");
				shaderModules[i] = VK_NULL_HANDLE;
				//DestroyShader(shaderModules[i]);
			}
		}

		CreateGraphicsPipeline(copyConfigInfo);
	}
	void EWEPipeline::HotReloadPipeline(bool reloadShaders) {
		/*
		static EWEPipeline* pipeCopy = nullptr;
		if (pipeCopy != nullptr) {
			Deconstruct(pipeCopy);
		}
		pipeCopy = Construct<EWEPipeline>({ copyStringStruct, copyConfigInfo });
		*/
		
		stalePipeline = graphicsPipeline;
		graphicsPipeline = VK_NULL_HANDLE;
		if (reloadShaders) {
			printf("this is a vram leak, old shaders wont get freed. revisit later\n");
			ReloadShaderModules();
		}
		else {
			CreateGraphicsPipeline(copyConfigInfo);
		}
		//std::swap(stalePipeline, graphicsPipeline);
		printf("recreated a pipeline, sleepign\n");
		//std::this_thread::sleep_for(std::chrono::seconds(3));
	}

	void ShaderStringStruct::RenderIMGUI() {
		static int currentImguiIndex = 0;
		if (imguiIndex < 0) {
			imguiIndex = currentImguiIndex;
			currentImguiIndex++;
		}

		std::string extension = "##p";
		extension += std::to_string(imguiIndex);

		std::string treeName = "shaderStrings";
		treeName += extension;

		const auto str_callback = [](ImGuiInputTextCallbackData* data) -> int {
			if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
			{
				auto* str = static_cast<std::string*>(data->UserData);
				str->resize(data->BufTextLen);
				data->Buf = str->data();
			}
			return 0;
		};


		if (ImGui::TreeNode(treeName.c_str())) {
			for (uint8_t i = 0; i < Shader::Stage::COUNT; i++) {
				ImGui::InputText(magic_enum::enum_name(static_cast<Shader::Stage>(i)).data(), filepath[i].data(), filepath[i].capacity() + 1, ImGuiInputTextFlags_CallbackResize, str_callback, &filepath[i]);
			}

			ImGui::TreePop();
		}
	}


	void imgui_vkbool(std::string const& name, VkBool32& vkBool) {
		bool loe = vkBool;
		ImGui::Checkbox(name.c_str(), &loe);
		vkBool = loe;
	}

	template<typename T>
	void imgui_enum(std::string const& name, T& val, int min, int max) {

		ImGui::SliderInt(name.c_str(), reinterpret_cast<int*>(&val), min, max, magic_enum::enum_name(val).data());
	}


	void EWEPipeline::PipelineConfigInfo::RenderIMGUI() {
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
				ImGui::DragFloat4(optionStr.c_str(), colorBlendInfo.blendConstants, 0.01, 0.f, 1.f);

				optionStr = "logic op";
				optionStr += extension;
				//ImGui::SliderInt(optionStr.c_str(), reinterpret_cast<int*>(&colorBlendInfo.logicOp), 0, 15, magic_enum::enum_name(colorBlendInfo.logicOp).data());
				imgui_enum(optionStr, colorBlendInfo.logicOp, 0, 15);

				optionStr = "logic op enable";
				optionStr += extension;
				imgui_vkbool(optionStr, colorBlendInfo.logicOpEnable);
				ImGui::TreePop();
			}


			//if(ImGui::TreeNode(optionStr.c_str(), ))

			ImGui::TreePop();
		}

		
	}
#endif
}