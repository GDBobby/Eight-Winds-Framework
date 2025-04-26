#pragma once

#include "EWGraphics/Vulkan/Device.hpp"
#include "EWGraphics/Vulkan/Pipeline.h"
#include "EWGraphics/Model/Model.h"
#include "EWGraphics/Data/EngineDataTypes.h"
#if PIPELINE_HOT_RELOAD
#include "EWGraphics/Data/magic_enum.hpp"
#endif

#include <unordered_map>
#include <memory>

namespace EWE {
	class PipelineSystem {
	protected:
		static std::unordered_map<PipelineID, PipelineSystem*> pipelineSystem;
#if EWE_DEBUG
		static PipelineID currentPipe;
		PipelineSystem(PipelineID pipeID) : myID{pipeID} {}
#else
		PipelineSystem() {}
#endif
		virtual void CreatePipeLayout() = 0;
		virtual void CreatePipeline() = 0;

	public:
		virtual ~PipelineSystem() {}
		static PipelineSystem* At(PipelineID pipeID);

#if PIPELINE_HOT_RELOAD
		static void Emplace(std::string const& pipeName, PipelineID pipeID, PipelineSystem* pipeSys);
		template<typename T>
		static void Emplace(T pipeID, PipelineSystem* pipeSys) {
			const std::string pipeName = std::string(magic_enum::enum_name(pipeID));
			Emplace(pipeName, pipeID, pipeSys);
		}
		static void RenderPipelinesIMGUI();
#else
		static void Emplace(PipelineID pipeID, PipelineSystem* pipeSys);
#endif
		static void Destruct();
		static void DestructAt(PipelineID pipeID);

		void BindPipeline();

		void BindModel(EWEModel* model);
		void BindDescriptor(uint8_t descSlot, VkDescriptorSet* descSet);

		void Push(void* push);
		virtual void PushAndDraw(void* push);
		void DrawModel();
		virtual void DrawInstanced(EWEModel* model);
		
		[[nodiscard]] EWEDescriptorSetLayout* GetDSL() {
			return eDSL;
		}
#if PIPELINE_HOT_RELOAD
		EWEPipeline* pipe{ nullptr };
	protected:
#else
	protected:
		EWEPipeline* pipe{ nullptr };
#endif

		VkPipelineLayout pipeLayout{};
		VkDescriptorSet bindedTexture = VK_NULL_HANDLE;
		//VkPipelineCache cache{VK_NULL_HANDLE};
		EWEModel* bindedModel = nullptr;
		VkShaderStageFlags pushStageFlags;
		uint32_t pushSize; 
		EWEDescriptorSetLayout* eDSL{};
#if EWE_DEBUG
		PipelineID myID;
#endif
	};
}