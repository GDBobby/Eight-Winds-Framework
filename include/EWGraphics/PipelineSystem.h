#pragma once

#include "EWGraphics/Vulkan/Device.hpp"
#include "EWGraphics/Model/Model.h"
#include "EWGraphics/Data/EngineDataTypes.h"
#include "EWGraphics/Vulkan/PipeLayout.h"
#if PIPELINE_HOT_RELOAD
#include "EWGraphics/Data/magic_enum.hpp"
#endif

#include <unordered_map>
#include <memory>

namespace EWE {
	struct Pipeline {
		PipeLayout* pipeLayout;
		VkPipeline vkPipe;

		const PipelineID myID;
		PipelineID GetID() const { return myID; };

		Pipeline(PipelineID id, PipeLayout* layout);
		Pipeline(PipelineID id, PipeLayout* layout, std::vector<KeyValuePair<ShaderStage, std::vector<Shader::SpecializationEntry>>> const& specInfo);

		Pipeline(Pipeline&) = delete;
		Pipeline(Pipeline&&) = delete;
		Pipeline& operator=(Pipeline&) = delete;
		Pipeline& operator=(Pipeline&&) = delete;

#if PIPELINE_HOT_RELOAD
		uint16_t framesSinceSwap = 0;
		VkPipeline stalePipeline = VK_NULL_HANDLE; //going to let it tick for MAX_FRAMES_IN_FLIGHT + 1 then delete it

		virtual void HotReload(bool layoutReload = true) = 0;

		bool enabled = true;
#endif
		std::vector<KeyValuePair<ShaderStage, std::vector<Shader::SpecializationEntry>>> copySpecInfo;

		void BindDescriptor(uint8_t descSlot, VkDescriptorSet* descSet);
		void BindPipeline();
		void BindPipelineWithVPScissor();

		void Push(void* push, uint8_t pushIndex = 0);
#if DEBUG_NAMING
		void SetDebugName(const char* name);
#endif
	};

	namespace PipelineSystem {


		Pipeline* At(PipelineID pipeID);

#if PIPELINE_HOT_RELOAD
		void Emplace(std::string const& pipeName, PipelineID pipeID, Pipeline* pipe);

		template<typename T>
		void Emplace(T pipeID, Pipeline* pipeSys) {
			const std::string pipeName = std::string(magic_enum::enum_name(pipeID));
			Emplace(pipeName, pipeID, pipeSys);
		}
#else
		void Emplace(PipelineID pipeID, PipelineSystem* pipeSys);
#endif
		void DestructAll();

		bool OptionalDestructAt(PipelineID pipeID);
		void DestructAt(PipelineID pipeID);

		void DrawImgui();
	}
}