#include "EWGraphics/Vulkan/ComputePipeline.h"

namespace EWE{
    void ComputePipeline::BindDescriptor(uint8_t binding, VkDescriptorSet* descSet) const {
        EWE_VK(vkCmdBindDescriptorSets,
            VK::Object->GetFrameBuffer(),
            VK_PIPELINE_BIND_POINT_COMPUTE,
            pipeLayout, 
            binding, 1, 
            descSet,
            0, nullptr
        );
    }

    void ComputePipeline::Push(void* pushData) const {
        EWE_VK(vkCmdPushConstants, VK::Object->GetFrameBuffer(), pipeLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, static_cast<uint32_t>(sizeof(pushData)), pushData);
    }

    void ComputePipeline::BindPipeline() const {
        EWE_VK(vkCmdBindPipeline, VK::Object->GetFrameBuffer(), VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    }

    void ComputePipeline::Dispatch(uint16_t x, uint16_t y, uint16_t z) const {
        EWE_VK(vkCmdDispatch, VK::Object->GetFrameBuffer(), x, y, z);
    }
}