#include "EWGraphics/Vulkan/PipelineBarrier.h"
#include "EWGraphics/Vulkan/Device.hpp"


#include <iterator>

namespace EWE {
    PipelineBarrier::PipelineBarrier() :
        srcStageMask{},
        dstStageMask{},
        dependencyFlags{ 0 },
        memoryBarriers{},
        imageBarriers{},
        bufferBarriers{}
    {}
    PipelineBarrier::PipelineBarrier(PipelineBarrier& copySource) noexcept :
        srcStageMask{ copySource.srcStageMask },
        dstStageMask{ copySource.dstStageMask },
        dependencyFlags{ copySource.dependencyFlags },
        memoryBarriers{ std::move(copySource.memoryBarriers) },
        imageBarriers{ std::move(copySource.imageBarriers) },
        bufferBarriers{ std::move(copySource.bufferBarriers) }
    {}
    PipelineBarrier& PipelineBarrier::operator=(PipelineBarrier& copySource) noexcept {
        srcStageMask = copySource.srcStageMask;
        dstStageMask = copySource.dstStageMask;
        dependencyFlags = copySource.dependencyFlags;
        memoryBarriers = std::move(copySource.memoryBarriers);
        imageBarriers = std::move(copySource.imageBarriers);
        bufferBarriers = std::move(copySource.bufferBarriers);

        return *this;
    }
    PipelineBarrier::PipelineBarrier(PipelineBarrier&& moveSource) noexcept :
        srcStageMask{ moveSource.srcStageMask },
        dstStageMask{ moveSource.dstStageMask },
        dependencyFlags{ moveSource.dependencyFlags },
        memoryBarriers{ std::move(moveSource.memoryBarriers) },
        imageBarriers{ std::move(moveSource.imageBarriers) },
        bufferBarriers{ std::move(moveSource.bufferBarriers) }
    {}
    PipelineBarrier& PipelineBarrier::operator=(PipelineBarrier&& moveSource) noexcept {
        srcStageMask = moveSource.srcStageMask;
        dstStageMask = moveSource.dstStageMask;
        dependencyFlags = moveSource.dependencyFlags;
        memoryBarriers = std::move(moveSource.memoryBarriers);
        imageBarriers = std::move(moveSource.imageBarriers);
        bufferBarriers = std::move(moveSource.bufferBarriers);

        return *this;
    }


	void PipelineBarrier::Submit(CommandBuffer& cmdBuf) const {
        //need pool synchronization here
		EWE_VK(vkCmdPipelineBarrier, cmdBuf,
			srcStageMask, dstStageMask,
			dependencyFlags,
			static_cast<uint32_t>(memoryBarriers.size()), memoryBarriers.data(),
            static_cast<uint32_t>(bufferBarriers.size()), bufferBarriers.data(),
            static_cast<uint32_t>(imageBarriers.size()), imageBarriers.data()
		);
	}
	//the parameter object passed in is no longer usable, submitting both barriers will potentially lead to errors
	void PipelineBarrier::Merge(PipelineBarrier const& other) {
		//idk if i need if operators for empty vectors
		std::copy(other.memoryBarriers.begin(), other.memoryBarriers.end(), std::back_inserter(memoryBarriers));
		std::copy(other.bufferBarriers.begin(), other.bufferBarriers.end(), std::back_inserter(bufferBarriers));
		std::copy(other.imageBarriers.begin(), other.imageBarriers.end(), std::back_inserter(imageBarriers));
	}

	void PipelineBarrier::SimplifyVector(std::vector<PipelineBarrier>& barriers) {
		if (barriers.size() <= 1) {
			return;
		}
#if EWE_DEBUG
		assert(barriers.size() < 256 && "too many barriers"); //reduce the barrier count. if not possible, change the values and data types here
#endif

        uint8_t lastComparisonIndex = 0;
		uint8_t currentComparisonIndex = 1;
		uint8_t nextComparisonIndex = -1;

		//c short for comparison
        while (currentComparisonIndex < barriers.size()) {
            nextComparisonIndex = static_cast<uint8_t>(barriers.size());

            const VkPipelineStageFlagBits cSrcStageMask = barriers[currentComparisonIndex].srcStageMask;
            const VkPipelineStageFlagBits cDstStageMask = barriers[currentComparisonIndex].dstStageMask;
            const VkDependencyFlags cDependencyFlags = barriers[currentComparisonIndex].dependencyFlags;

            for ( ; currentComparisonIndex < static_cast<uint8_t>(barriers.size()); currentComparisonIndex++) {
                const bool srcComp{ cSrcStageMask == barriers[currentComparisonIndex].srcStageMask };
                const bool dstComp{ cDstStageMask == barriers[currentComparisonIndex].dstStageMask };
                const bool dfComp{ cDependencyFlags == barriers[currentComparisonIndex].dependencyFlags };
                if (srcComp && dstComp && dfComp) {
                    barriers[lastComparisonIndex].Merge(barriers[currentComparisonIndex]);
                    barriers.erase(barriers.begin() + currentComparisonIndex);
                    currentComparisonIndex--;
                }
                else if (nextComparisonIndex > currentComparisonIndex) {
                    nextComparisonIndex = currentComparisonIndex;
                }
            }
            lastComparisonIndex = nextComparisonIndex;
            currentComparisonIndex = lastComparisonIndex + 1;
        }
	}

	namespace Barrier {
		VkImageMemoryBarrier ChangeImageLayout(
			const VkImage image,
			const VkImageLayout oldImageLayout,
			const VkImageLayout newImageLayout,
			VkImageSubresourceRange const& subresourceRange
		) {
			VkImageMemoryBarrier imageMemoryBarrier{};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.pNext = nullptr;
			imageMemoryBarrier.oldLayout = oldImageLayout;
			imageMemoryBarrier.newLayout = newImageLayout;
			imageMemoryBarrier.image = image;
			imageMemoryBarrier.subresourceRange = subresourceRange;
            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

			if ((oldImageLayout == VK_IMAGE_LAYOUT_UNDEFINED) && (newImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)) {
				imageMemoryBarrier.srcAccessMask = 0;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			}
			else if (oldImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newImageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {

				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			}
			else {
				assert(false && "unsupported layout transition");
			}

			return imageMemoryBarrier;
		}

        VkImageMemoryBarrier TransitionImageLayout(VkImage& image, VkImageLayout srcLayout, VkImageLayout dstLayout, uint32_t mipLevels, uint8_t layerCount) {

            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.pNext = nullptr;
            barrier.oldLayout = srcLayout;
            barrier.newLayout = dstLayout;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

            barrier.image = image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = mipLevels;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = layerCount;

            switch (srcLayout) {
            case VK_IMAGE_LAYOUT_UNDEFINED:
                // Image layout is undefined (or does not matter).
                // Only valid as initial layout. No flags required.
                barrier.srcAccessMask = 0;
                break;

            case VK_IMAGE_LAYOUT_PREINITIALIZED:
                // Image is preinitialized.
                // Only valid as initial layout for linear images; preserves memory
                // contents. Make sure host writes have finished.
                barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                // Image is a color attachment.
                // Make sure writes to the color buffer have finished
                barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                // Image is a depth/stencil attachment.
                // Make sure any writes to the depth/stencil buffer have finished.
                barrier.srcAccessMask
                    = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                // Image is a transfer source.
                // Make sure any reads from the image have finished
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                // Image is a transfer destination.
                // Make sure any writes to the image have finished.
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                // Image is read by a shader.
                // Make sure any shader reads from the image have finished
                barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                break;
            case VK_IMAGE_LAYOUT_GENERAL:
                barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                barrier.srcAccessMask |= VK_ACCESS_SHADER_READ_BIT * (dstLayout == VK_IMAGE_LAYOUT_GENERAL);
                break;
            default:
                /* Value not used by callers, so not supported. */
                assert(false && "unsupported src layout transition");
            }

            // Target layouts (new)
            // The destination access mask controls the dependency for the new image
            // layout.
            switch (dstLayout) {
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                // Image will be used as a transfer destination.
                // Make sure any writes to the image have finished.
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                // Image will be used as a transfer source.
                // Make sure any reads from and writes to the image have finished.
                barrier.srcAccessMask |= VK_ACCESS_TRANSFER_READ_BIT;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                break;

            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                // Image will be used as a color attachment.
                // Make sure any writes to the color buffer have finished.
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                // Image layout will be used as a depth/stencil attachment.
                // Make sure any writes to depth/stencil buffer have finished.
                barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                // Image will be read in a shader (sampler, input attachment).
                // Make sure any writes to the image have finished.
                if (barrier.srcAccessMask == 0) {
                    barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
                }
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                break;
            case VK_IMAGE_LAYOUT_GENERAL:
                barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
                break;
            default:
                /* Value not used by callers, so not supported. */
#if EWE_DEBUG
                assert(false && "unsupported dst layout transition");
#else
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
                __assume(false);
#else // GCC, Clang
                __builtin_unreachable();
#endif
#endif
            }

            return barrier;
        }
	
        void TransitionImageLayoutWithBarrier(CommandBuffer cmdBuf, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkImage& image, VkImageLayout srcLayout, VkImageLayout dstLayout, uint32_t mipLevels, uint8_t layerCount) {
            VkImageMemoryBarrier imageBarrier{ TransitionImageLayout(image, srcLayout, dstLayout, mipLevels, layerCount) };
            EWE_VK(vkCmdPipelineBarrier, cmdBuf,
                srcStageMask, dstStageMask,
                0,
                0, nullptr,
                0, nullptr,
                1, &imageBarrier
            );
        }

#define BARRIER_DEBUGGING false
        void TransferImageStage(CommandBuffer& cmdBuf, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, VkImage const& image) {
            VkImageMemoryBarrier imageBarrier{};
            imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageBarrier.pNext = nullptr;
            imageBarrier.image = image;
#if EWE_DEBUG
            assert(imageBarrier.image != VK_NULL_HANDLE && "transfering a null image?");
#endif
            imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // or VK_IMAGE_ASPECT_DEPTH_BIT for depth images
            imageBarrier.subresourceRange.baseMipLevel = 0;
            imageBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
            imageBarrier.subresourceRange.baseArrayLayer = 0;
            imageBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

            imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

            if ((srcStage & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT)
                && ((dstStage & (VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)))
                ) {
#if BARRIER_DEBUGGING
                printf(" COMPUTE TO GRAPHICS IMAGE TRANSFER \n");
#endif
                imageBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
                imageBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT; // Access mask for compute shader writes
                imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT; // Access mask for transfer read operation

                EWE_VK(vkCmdPipelineBarrier,
                    cmdBuf,
                    srcStage, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, // pipeline stage
                    0, //dependency flags
                    0, nullptr, //memory barrier
                    0, nullptr, //buffer barrier
                    1, &imageBarrier //image barrier
                );
            }
            else if (((srcStage & VK_PIPELINE_STAGE_VERTEX_SHADER_BIT) || (srcStage & VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)) &&
                (dstStage & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT))
            {
#if BARRIER_DEBUGGING
                printf(" GRAPHICS TO COMPUTE IMAGE TRANSFER \n");
#endif
                imageBarrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                imageBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT; // Access mask for transfer read operation
                EWE_VK(vkCmdPipelineBarrier,
                    cmdBuf,
                    srcStage, dstStage, // pipeline stage
                    0, //dependency flags
                    0, nullptr, //memory barrier
                    0, nullptr, //buffer barrier
                    1, &imageBarrier //image barrier
                );
            }
            else if (srcStage == dstStage && (srcStage & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT)) {
#if BARRIER_DEBUGGING
                printf("COMPUTE TO COMPUTE image barrier \n");
#endif
                imageBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
                imageBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                imageBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT; // Access mask for compute shader writes
                imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT; // Access mask for transfer read operation
                EWE_VK(vkCmdPipelineBarrier,
                    cmdBuf,
                    srcStage, dstStage, // pipeline stage
                    0, //dependency flags
                    0, nullptr, //memory barrier
                    0, nullptr, //buffer barrier
                    1, &imageBarrier //image barrier
                );
            }

        }
        void TransferImageStage(CommandBuffer& cmdBuf, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, std::vector<VkImage> const& images) {
            assert(images.size() > 0);
            const uint32_t imageCount = static_cast<uint32_t>(images.size());

            std::vector<VkImageMemoryBarrier> imageBarriers{};
            imageBarriers.resize(imageCount);
            imageBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageBarriers[0].pNext = nullptr;
            imageBarriers[0].image = images[0];
#if EWE_DEBUG
            assert(imageBarriers[0].image != VK_NULL_HANDLE && "transfering a null image?");
#endif

            imageBarriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // or VK_IMAGE_ASPECT_DEPTH_BIT for depth images
            imageBarriers[0].subresourceRange.baseMipLevel = 0;
            imageBarriers[0].subresourceRange.levelCount = 1;
            imageBarriers[0].subresourceRange.baseArrayLayer = 0;
            imageBarriers[0].subresourceRange.layerCount = 1;
            if (VK::Object->queueIndex[Queue::compute] != VK::Object->queueIndex[Queue::graphics]) {
                if (!VK::Object->queueEnabled[Queue::compute]) {
                    throw std::runtime_error("misisng comptue queue but still using it");
                }
                imageBarriers[0].srcQueueFamilyIndex = VK::Object->queueIndex[Queue::compute];
                imageBarriers[0].dstQueueFamilyIndex = VK::Object->queueIndex[Queue::graphics];
            }
            else {
                imageBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                imageBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            }

            if ((srcStage & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT) &&
                ((dstStage & VK_PIPELINE_STAGE_VERTEX_SHADER_BIT) || (dstStage & VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT))
                ) {

                //std::cout << " COMPUTE TO GRAPHICS IMAGE TRANSFER \n";
                imageBarriers[0].oldLayout = VK_IMAGE_LAYOUT_GENERAL;
                imageBarriers[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageBarriers[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT; // Access mask for compute shader writes
                imageBarriers[0].dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT; // Access mask for transfer read operation
            }
            else if (((srcStage & VK_PIPELINE_STAGE_VERTEX_SHADER_BIT) || (srcStage & VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)) &&
                (dstStage & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT))
            {
                //std::cout << " GRAPHICS TO COMPUTE IMAGE TRANSFER \n";
                imageBarriers[0].oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageBarriers[0].newLayout = VK_IMAGE_LAYOUT_GENERAL;
                imageBarriers[0].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT; // Access mask for compute shader writes
                imageBarriers[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT; // Access mask for transfer read operation
            }

            for (uint8_t i = 1; i < imageCount; i++) {
                imageBarriers[i] = imageBarriers[0];
                imageBarriers[i].image = images[i];
#if EWE_DEBUG
                assert(imageBarriers[i].image != VK_NULL_HANDLE && "transfering a null image?");
#endif
            }

            EWE_VK(vkCmdPipelineBarrier,
                cmdBuf,
                srcStage,  // pipeline stage
                dstStage,
                0,
                0, nullptr,
                0, nullptr,
                imageCount, &imageBarriers[0]
            );
        }
}//namespace Barrier

} //namespace EWE