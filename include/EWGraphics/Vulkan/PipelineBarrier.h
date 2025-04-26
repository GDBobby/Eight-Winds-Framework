#pragma once

#include "EWGraphics/Vulkan/VulkanHeader.h"

#include <vector>

namespace EWE {
    struct PipelineBarrier {
        VkPipelineStageFlagBits srcStageMask;
        VkPipelineStageFlagBits dstStageMask;
        VkDependencyFlags dependencyFlags;
        std::vector<VkMemoryBarrier> memoryBarriers;
        std::vector<VkImageMemoryBarrier> imageBarriers;
        std::vector<VkBufferMemoryBarrier> bufferBarriers;

		PipelineBarrier();
		PipelineBarrier(PipelineBarrier& copySource) noexcept;
		PipelineBarrier& operator=(PipelineBarrier& copySource) noexcept;
		PipelineBarrier(PipelineBarrier&& moveSource) noexcept;
		PipelineBarrier& operator=(PipelineBarrier&& moveSource) noexcept;

		bool Empty() const {
			return (memoryBarriers.size() + imageBarriers.size() + bufferBarriers.size()) == 0;
		}

		void AddBarrier(VkMemoryBarrier const& memoryBarrier) {
			memoryBarriers.push_back(memoryBarrier);
		}
		void AddBarrier(VkImageMemoryBarrier const& imageBarrier) {
			imageBarriers.push_back(imageBarrier);
		}
		void AddBarrier(VkBufferMemoryBarrier const& bufferBarrier) {
			bufferBarriers.push_back(bufferBarrier);
		}
		void Submit(CommandBuffer& cmdBuf) const;

		//the parameter object passed in is no longer usable, submitting both barriers will potentially lead to errors
		void Merge(PipelineBarrier const& other);

		static void SimplifyVector(std::vector<PipelineBarrier>& barriers);
	};
	namespace Barrier {
		//this only changes the src/dst access mask
		VkImageMemoryBarrier ChangeImageLayout(
			const VkImage image, 
			const VkImageLayout oldImageLayout, 
			const VkImageLayout newImageLayout, 
			VkImageSubresourceRange const& subresourceRange
		);
		VkImageMemoryBarrier TransitionImageLayout(VkImage& image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, uint8_t layerCount = 1);
		
		void TransitionImageLayoutWithBarrier(CommandBuffer cmdBuf, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkImage& image, VkImageLayout srcLayout, VkImageLayout dstLayout, uint32_t mipLevels, uint8_t layerCount);

		void TransferImageStage(CommandBuffer& cmdBuf, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, VkImage const& image);
		void TransferImageStage(CommandBuffer& cmdBuf, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, std::vector<VkImage> const& images);
	} //namespace Barrier
} //namespace EWE
