#pragma once
#include "EWGraphics/Vulkan/VulkanHeader.h"
#include "EWGraphics/Data/EngineDataTypes.h"

#include <unordered_map>


namespace EWE{
    struct ComputePipeline{
    protected:
		static std::unordered_map<PipelineID, PipelineSystem*> pipelineSystem;
    public:


#if PIPELINE_HOT_RELOAD
		static void Emplace(std::string const& pipeName, PipelineID pipeID, ComputePipeline* pipeSys);
		template<typename T>
		static void Emplace(T pipeID, ComputePipeline* pipeSys) {
			const std::string pipeName = std::string(magic_enum::enum_name(pipeID));
			Emplace(pipeName, pipeID, pipeSys);
		}
		static void RenderPipelinesIMGUI();
#else
		static void Emplace(PipelineID pipeID, PipelineSystem* pipeSys);
#endif
		static void Destruct();
		static void DestructAt(PipelineID pipeID);

        VkDescriptorSetLayout descLayout;
        VkPipelineLayout pipeLayout;
        VkPipeline pipeline;

        VkShaderModule shaderModule;

        uint32_t pushSize = 0;

        void Push(void* pushData) const;
        void BindDescriptor(uint8_t binding, VkDescriptorSet* descSet) const;
        void BindPipeline() const;
        void Dispatch(uint16_t x, uint16_t y, uint16_t z) const;
    };
}