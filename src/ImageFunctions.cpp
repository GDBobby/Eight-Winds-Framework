#include "EWEngine/Graphics/Texture/ImageFunctions.h"

#include "EWEngine/Systems/SyncHub.h"
#include "EWEngine/Graphics/Texture/Sampler.h"


#include <stb/stb_image.h>
#include <cmath>

#ifndef TEXTURE_DIR
#define TEXTURE_DIR "textures/"
#endif


namespace EWE {
    namespace Image {
#if USING_VMA
        void CreateImageWithInfo(const VkImageCreateInfo& imageCreateInfo, const VkMemoryPropertyFlags properties, VkImage& image, VmaAllocation vmaAlloc) {
            VmaAllocationInfo vmaAllocInfo{};

            VmaAllocationCreateInfo vmaAllocCreateInfo{};
            vmaAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
            //if(imageCreateInfo.width * height > some amount){
            vmaAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT | VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT;
            //}

#if DEBUGGING_MEMORY_WITH_VMA
            //vmaAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
#endif

            EWE_VK(vmaCreateImage, VK::Object->vmaAllocator, &imageCreateInfo, &vmaAllocCreateInfo, &image, &vmaAlloc, nullptr);

        }
#else
        void CreateImageWithInfo(const VkImageCreateInfo& imageCreateInfo, const VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
            EWE_VK(vkCreateImage, VK::Object->vkDevice, &imageCreateInfo, nullptr, &image);

            VkMemoryRequirements memRequirements;
            EWE_VK(vkGetImageMemoryRequirements, VK::Object->vkDevice, image, &memRequirements);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.pNext = nullptr;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

            EWE_VK(vkAllocateMemory, VK::Object->vkDevice, &allocInfo, nullptr, &imageMemory);

            EWE_VK(vkBindImageMemory, VK::Object->vkDevice, image, imageMemory, 0);
        }
#endif

        void CopyBufferToImage(CommandBuffer& cmdBuf, VkBuffer& buffer, VkImage& image, uint32_t width, uint32_t height, uint32_t layerCount) {

            VkBufferImageCopy region{};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;

            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = layerCount;

            region.imageOffset = { 0, 0, 0 };
            region.imageExtent.width = width;
            region.imageExtent.height = height;
            region.imageExtent.depth = 1;

            EWE_VK(vkCmdCopyBufferToImage,
                cmdBuf,
                buffer,
                image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &region
            );
        }
        void GenerateMipMapsForMultipleImagesTransferQueue(CommandBuffer& cmdBuf, std::vector<ImageInfo*>& imageInfos) {
            assert(VK::Object->queueEnabled[Queue::transfer]);
            //printf("before mip map loop? size of image : %d \n", image.size());

            //printf("after beginning single time command \n");
            uint8_t maxMipLevels = 0;
            for (auto const& imageInfo : imageInfos) {
                if (imageInfo->mipLevels > maxMipLevels) {
                    maxMipLevels = imageInfo->mipLevels;
#if EWE_DEBUG
                    assert(imageInfo->mipLevels >= 1);
#endif
                }
            }


            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.pNext = nullptr;
            barrier.srcQueueFamilyIndex = VK::Object->queueIndex[Queue::transfer];
            barrier.dstQueueFamilyIndex = VK::Object->queueIndex[Queue::graphics]; //graphics queue is the only queue that can support vkCmdBlitImage
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            {
                const uint32_t mipLevel = 1;
                { //prebarrier, transfers ownership, prepares the current miplevel to be a transfer src
                    std::vector<VkImageMemoryBarrier> preBarriers{};
                    preBarriers.reserve(imageInfos.size());
                    for (auto const& imageInfo : imageInfos) {

                        barrier.image = imageInfo->image;
                        barrier.subresourceRange.levelCount = imageInfo->arrayLayers;
                        preBarriers.push_back(barrier);
                    }

                    EWE_VK(vkCmdPipelineBarrier, cmdBuf,
                        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                        0, nullptr,
                        0, nullptr,
                        static_cast<uint32_t>(preBarriers.size()), preBarriers.data()
                    );
                }
                { //blit, copies the current image to the next with linear scaling
                    VkImageBlit blit{};
                    blit.srcOffsets[0] = { 0, 0, 0 };
                    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    blit.srcSubresource.mipLevel = mipLevel - 1;
                    blit.srcSubresource.baseArrayLayer = 0;
                    blit.dstOffsets[0] = { 0, 0, 0 };
                    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    blit.dstSubresource.mipLevel = mipLevel;
                    blit.dstSubresource.baseArrayLayer = 0;
                    for (auto const& imageInfo : imageInfos) {
                        blit.srcOffsets[1] = VkOffset3D{ static_cast<int32_t>(imageInfo->width), static_cast<int32_t>(imageInfo->height), 1 };
                        blit.dstOffsets[1] = { imageInfo->width > 1 ? static_cast<int32_t>(imageInfo->width) / 2 : 1, imageInfo->height > 1 ? static_cast<int32_t>(imageInfo->height) / 2 : 1, 1 };
                        blit.dstSubresource.layerCount = imageInfo->arrayLayers;
                        blit.srcSubresource.layerCount = imageInfo->arrayLayers;
                        //printf("before blit image \n");
                        EWE_VK(vkCmdBlitImage, cmdBuf,
                            imageInfo->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                            imageInfo->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            1, &blit,
                            VK_FILTER_LINEAR
                        );
                    }
                }
                //printf("after blit image \n");
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                { //post barrier, finalizes the current mip level and prepares it for reading in the fragment stage
                    std::vector<VkImageMemoryBarrier> postBarriers{};
                    postBarriers.reserve(imageInfos.size());
                    for (auto& imageInfo : imageInfos) {
                        barrier.image = imageInfo->image;
                        barrier.subresourceRange.levelCount = imageInfo->arrayLayers;
                        postBarriers.push_back(barrier);
                        if (imageInfo->width > 1) { imageInfo->width /= 2; }
                        if (imageInfo->height > 1) { imageInfo->height /= 2; }
                    }
                    EWE_VK(vkCmdPipelineBarrier, cmdBuf,
                        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                        0, nullptr,
                        0, nullptr,
                        static_cast<uint32_t>(postBarriers.size()), postBarriers.data()
                    );
                }
                //printf("after pipeline barrier 2 \n");
            }

            for (uint32_t mipLevel = 2; mipLevel < maxMipLevels; mipLevel++) {
                barrier.subresourceRange.baseMipLevel = mipLevel - 1;

                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;


                { //prebarrier, transfers ownership, prepares the current miplevel to be a transfer src
                    std::vector<VkImageMemoryBarrier> preBarriers{};
                    preBarriers.reserve(imageInfos.size());
                    for (auto const& imageInfo : imageInfos) {

                        if (imageInfo->mipLevels <= mipLevel) {
                            continue;
                        }
                        barrier.image = imageInfo->image;
                        barrier.subresourceRange.levelCount = imageInfo->arrayLayers;
                        preBarriers.push_back(barrier);
                        //printf("before cmd pipeline barrier \n");
                        //this barrier right here needs a transfer queue partner
                    }

                    EWE_VK(vkCmdPipelineBarrier, cmdBuf,
                        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                        0, nullptr,
                        0, nullptr,
                        static_cast<uint32_t>(preBarriers.size()), preBarriers.data()
                    );
                }

                { //blit, copies the current image to the next with linear scaling
                    VkImageBlit blit{};
                    blit.srcOffsets[0] = { 0, 0, 0 };
                    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    blit.srcSubresource.mipLevel = mipLevel - 1;
                    blit.srcSubresource.baseArrayLayer = 0;
                    blit.dstOffsets[0] = { 0, 0, 0 };
                    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    blit.dstSubresource.mipLevel = mipLevel;
                    blit.dstSubresource.baseArrayLayer = 0;
                    for (auto const& imageInfo : imageInfos) {
                        if (imageInfo->mipLevels <= mipLevel) {
                            continue;
                        }

                        blit.srcOffsets[1] = VkOffset3D{ static_cast<int32_t>(imageInfo->width), static_cast<int32_t>(imageInfo->height), 1 };
                        blit.dstOffsets[1] = { imageInfo->width > 1 ? static_cast<int32_t>(imageInfo->width) / 2 : 1, imageInfo->height > 1 ? static_cast<int32_t>(imageInfo->height) / 2 : 1, 1 };
                        blit.dstSubresource.layerCount = imageInfo->arrayLayers;
                        blit.srcSubresource.layerCount = imageInfo->arrayLayers;
                        //printf("before blit image \n");
                        EWE_VK(vkCmdBlitImage, cmdBuf,
                            imageInfo->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                            imageInfo->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            1, &blit,
                            VK_FILTER_LINEAR
                        );
                    }
                }
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                { //post barrier, finalizes the current mip level and prepares it for reading in the fragment stage
                    std::vector<VkImageMemoryBarrier> postBarriers{};
                    postBarriers.reserve(imageInfos.size());

                    for (auto& imageInfo : imageInfos) {

                        if (imageInfo->mipLevels <= mipLevel) {
                            continue;
                        }
                        barrier.image = imageInfo->image;
                        barrier.subresourceRange.levelCount = imageInfo->arrayLayers;
                        postBarriers.push_back(barrier);
                        if (imageInfo->width > 1) { imageInfo->width /= 2; }
                        if (imageInfo->height > 1) { imageInfo->height /= 2; }
                    }
                    EWE_VK(vkCmdPipelineBarrier, cmdBuf,
                        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                        0, nullptr,
                        0, nullptr,
                        static_cast<uint32_t>(postBarriers.size()), postBarriers.data()
                    );
                }
            }

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            { //transition of the final mip level, which was copied to but never copied from, to be readable by the fragment shader
                std::vector<VkImageMemoryBarrier> finalBarriers{ imageInfos.size() };
                for (uint8_t i = 0; i < imageInfos.size(); i++) {
                    barrier.subresourceRange.baseMipLevel = imageInfos[i]->mipLevels - 1;
                    barrier.image = imageInfos[i]->image;
                    barrier.subresourceRange.levelCount = imageInfos[i]->arrayLayers;
                    finalBarriers[i] = barrier;
                }

                EWE_VK(vkCmdPipelineBarrier, cmdBuf,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                    0, nullptr,
                    0, nullptr,
                    static_cast<uint32_t>(finalBarriers.size()), finalBarriers.data()
                );
            }
        }

        void GenerateMipmaps(CommandBuffer& cmdBuf, ImageInfo* imageInfo, Queue::Enum srcQueue) {
            //printf("before mip map loop? size of image : %d \n", image.size());

            //printf("after beginning single time command \n");

            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.pNext = nullptr;
            barrier.image = imageInfo->image;
            barrier.srcQueueFamilyIndex = VK::Object->queueIndex[srcQueue];
            barrier.dstQueueFamilyIndex = VK::Object->queueIndex[Queue::graphics]; //graphics queue is the only queue that can support vkCmdBlitImage
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = imageInfo->arrayLayers;
            barrier.subresourceRange.levelCount = 1;

            VkImageBlit blit{};
            blit.srcOffsets[0] = { 0, 0, 0 };
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = imageInfo->arrayLayers;
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstOffsets[0] = { 0, 0, 0 };
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = imageInfo->arrayLayers;
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

            for (uint32_t i = 1; i < imageInfo->mipLevels; i++) {
                barrier.subresourceRange.baseMipLevel = i - 1;
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                //printf("before cmd pipeline barrier \n");
                //this barrier right here needs a transfer queue partner
                EWE_VK(vkCmdPipelineBarrier, cmdBuf,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier
                );
                //printf("after cmd pipeline barreir \n");
                blit.srcSubresource.mipLevel = i - 1;
                blit.srcOffsets[1] = VkOffset3D{ static_cast<int32_t>(imageInfo->width), static_cast<int32_t>(imageInfo->height), 1 };
                blit.dstOffsets[1] = { imageInfo->width > 1 ? static_cast<int32_t>(imageInfo->width) / 2 : 1, imageInfo->height > 1 ? static_cast<int32_t>(imageInfo->height) / 2 : 1, 1 };
                blit.dstSubresource.mipLevel = i;
                //printf("before blit image \n");
                EWE_VK(vkCmdBlitImage, cmdBuf,
                    imageInfo->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    imageInfo->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1, &blit,
                    VK_FILTER_LINEAR
                );
                //printf("after blit image \n");
                //this is going to be set again for each mip level, extremely small performance hit, potentially optimized away
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                EWE_VK(vkCmdPipelineBarrier, cmdBuf,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier
                );
                //printf("after pipeline barrier 2 \n");
                if (imageInfo->width > 1) { imageInfo->width /= 2; }
                if (imageInfo->height > 1) { imageInfo->height /= 2; }
            }

            barrier.subresourceRange.baseMipLevel = imageInfo->mipLevels - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            //printf("before pipeline barrier 3 \n");
            EWE_VK(vkCmdPipelineBarrier,
                cmdBuf,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );
        }


        void CreateImageCommands(ImageInfo& imageInfo, VkImageCreateInfo const& imageCreateInfo, StagingBuffer* stagingBuffer, bool mipmapping) {
            SyncHub* syncHub = SyncHub::GetSyncHubInstance();
            CommandBuffer& cmdBuf = syncHub->BeginSingleTimeCommand();
            VkImageSubresourceRange subresourceRange = CreateSubresourceRange(imageInfo);
            {
                VkImageMemoryBarrier imageBarrier = Barrier::ChangeImageLayout(imageInfo.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);
                imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                EWE_VK(vkCmdPipelineBarrier, cmdBuf,
                    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                    0,
                    0, nullptr,
                    0, nullptr,
                    1, &imageBarrier
                );
            }
            //printf("before copy buffer to image \n");
            
            Image::CopyBufferToImage(cmdBuf, stagingBuffer->buffer, imageInfo.image, imageCreateInfo.extent.width, imageCreateInfo.extent.height, imageInfo.arrayLayers);

            const bool inMainThread = VK::Object->CheckMainThread();

            if (inMainThread || (!VK::Object->queueEnabled[Queue::transfer])) {
                GraphicsCommand graphicsCommand{};
                graphicsCommand.command = &cmdBuf;
                graphicsCommand.stagingBuffer = stagingBuffer;
                if (mipmapping && MIPMAP_ENABLED) {
                    syncHub->EndSingleTimeCommandGraphics(graphicsCommand);
                    VkFormatProperties formatProperties;
                    EWE_VK(vkGetPhysicalDeviceFormatProperties, VK::Object->physicalDevice, imageCreateInfo.format, &formatProperties);
                    assert((formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) && "texture image format does not support linear blitting");

                    SyncHub* syncHub = SyncHub::GetSyncHubInstance();
                    GraphicsCommand mipCommand{};
                    mipCommand.command = &syncHub->BeginSingleTimeCommand();
                    mipCommand.imageInfo = &imageInfo;

                    GenerateMipmaps(*mipCommand.command, &imageInfo, Queue::graphics);

                    syncHub->EndSingleTimeCommandGraphics(mipCommand);
                }
                else {
                    graphicsCommand.imageInfo = &imageInfo;
                    VkImageMemoryBarrier imageBarrier = Barrier::ChangeImageLayout(imageInfo.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange);
                    imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    EWE_VK(vkCmdPipelineBarrier, cmdBuf,
                        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                        0,
                        0, nullptr,
                        0, nullptr,
                        1, &imageBarrier
                    );

                    syncHub->EndSingleTimeCommandGraphics(graphicsCommand);
                }
            }
            else {
                VkImageMemoryBarrier imageBarrier{};
                imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                imageBarrier.pNext = nullptr;
                imageBarrier.image = imageInfo.image;
                imageBarrier.srcQueueFamilyIndex = VK::Object->queueIndex[Queue::transfer];
                imageBarrier.dstQueueFamilyIndex = VK::Object->queueIndex[Queue::graphics];
                imageBarrier.subresourceRange = subresourceRange;
                imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

                if (mipmapping && MIPMAP_ENABLED) {
                    //pipeBarrier.AddBarrier(imageBarrier);
                    //pipeBarrier.SubmitBarrier(cmdBuf);
                    VkFormatProperties formatProperties;
                    EWE_VK(vkGetPhysicalDeviceFormatProperties, VK::Object->physicalDevice, imageCreateInfo.format, &formatProperties);
                    assert((formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) && "texture image format does not support linear blitting");


                    //this matches the barrier in generate mips, the only difference, as far as i know, is levelCount == 1 instead of == mipLevels
                    //i honestly dont know why that isn't producing a bug, but it has been working so i'll leave it
                    imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    imageBarrier.subresourceRange.baseArrayLayer = 0;
                    imageBarrier.subresourceRange.layerCount = imageInfo.arrayLayers;
                    imageBarrier.subresourceRange.levelCount = 1;

                    imageBarrier.subresourceRange.baseMipLevel = 0;
                    imageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                    imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                    //printf("before cmd pipeline barrier \n");
                    EWE_VK(vkCmdPipelineBarrier, cmdBuf,
                        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                        0, nullptr,
                        0, nullptr,
                        1, &imageBarrier
                    );

                    imageInfo.descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

                    TransferCommand command{};
                    command.commands.push_back(&cmdBuf);
                    command.stagingBuffers.push_back(stagingBuffer);
                    command.images.push_back(&imageInfo);
                    syncHub->EndSingleTimeCommandTransfer(command);
                }
                else {
                    PipelineBarrier pipeBarrier{};
                    pipeBarrier.srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
                    pipeBarrier.dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
                    imageBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    pipeBarrier.AddBarrier(imageBarrier);
                    pipeBarrier.Submit(cmdBuf);

                    TransferCommand command{};
                    command.commands.push_back(&cmdBuf);
                    command.stagingBuffers.push_back(stagingBuffer);
                    command.images.push_back(&imageInfo);
                    command.pipeBarriers.push_back(std::move(pipeBarrier));
                    syncHub->EndSingleTimeCommandTransfer(command);
                }
                
            }
            
        }
        VkImageSubresourceRange CreateSubresourceRange(ImageInfo const& imageInfo) {
            VkImageSubresourceRange subresourceRange{};

            subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresourceRange.baseMipLevel = 0;
            subresourceRange.levelCount = imageInfo.mipLevels;
            subresourceRange.baseArrayLayer = 0;
            subresourceRange.layerCount = imageInfo.arrayLayers;

            return subresourceRange;
        }



        StagingBuffer* StageImage(PixelPeek& pixelPeek) {
            const int width = pixelPeek.width;
            const int height = pixelPeek.height;

            const VkDeviceSize imageSize = width * height * 4;
#if USING_VMA
            StagingBuffer* stagingBuffer = Construct<StagingBuffer>({ imageSize, pixelPeek.pixels });
#else
            StagingBuffer* stagingBuffer = Construct<StagingBuffer>({ imageSize, pixelPeek.pixels });
#endif
            //printf("freeing pixels \n");
            stbi_image_free(pixelPeek.pixels);

            return stagingBuffer;
        }
        StagingBuffer* StageImage(std::vector<PixelPeek>& pixelPeek) {
            const int width = pixelPeek[0].width;
            const int height = pixelPeek[0].height;
            const uint64_t layerSize = width * height * 4;
            const VkDeviceSize imageSize = layerSize * pixelPeek.size();
            void* data;

#if USING_VMA
            StagingBuffer* stagingBuffer = Construct<StagingBuffer>({ imageSize });
#else
            StagingBuffer* stagingBuffer = Construct<StagingBuffer>({ imageSize });
#endif
            stagingBuffer->Map(data);
            uint64_t memAddress = reinterpret_cast<uint64_t>(data);

            for (int i = 0; i < pixelPeek.size(); i++) {
                memcpy(reinterpret_cast<void*>(memAddress), pixelPeek[i].pixels, static_cast<std::size_t>(layerSize)); //static_cast<void*> unnecessary>?
                stbi_image_free(pixelPeek[i].pixels);
                memAddress += layerSize;
            }
            stagingBuffer->Unmap();

            return stagingBuffer;
        }

        void CreateTextureImage(ImageInfo& imageInfo, PixelPeek& pixelPeek, bool mipmapping) {

            StagingBuffer* stagingBuffer = StageImage(pixelPeek);
            //printf("image dimensions : %d:%d \n", width[i], height[i]);
            //printf("beginning of create image, dimensions - %d : %d : %d \n", width[i], height[i], pixelPeek[i].channels);
            if (MIPMAP_ENABLED && mipmapping) {
                imageInfo.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(pixelPeek.width, pixelPeek.height))) + 1);
            }
            //printf("before creating buffer \n");

            VkImageCreateInfo imageCreateInfo{};
            imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageCreateInfo.pNext = nullptr;
            imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
            imageCreateInfo.extent.width = pixelPeek.width;
            imageCreateInfo.extent.height = pixelPeek.height;
            imageCreateInfo.extent.depth = 1;
            imageCreateInfo.mipLevels = imageInfo.mipLevels;
            imageCreateInfo.arrayLayers = imageInfo.arrayLayers;

            imageCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
            imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            if (MIPMAP_ENABLED && mipmapping) {
                imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            }
            imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageCreateInfo.flags = 0; // Optional

            imageCreateInfo.queueFamilyIndexCount = 1;

            //printf("before image info \n");
            Image::CreateImageWithInfo(imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, imageInfo.image, imageInfo.memory);
#if DEBUG_NAMING
            DebugNaming::SetObjectName(imageInfo.image, VK_OBJECT_TYPE_IMAGE, pixelPeek.debugName.c_str());
#endif
            //printf("before transition \n");
#if IMAGE_DEBUGGING
            imageInfo.imageName = pixelPeek.debugName;
#endif
            CreateImageCommands(imageInfo, imageCreateInfo, stagingBuffer, mipmapping);
        }

        void CreateTextureImageView(ImageInfo& imageInfo) {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.pNext = nullptr;
            viewInfo.image = imageInfo.image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = imageInfo.mipLevels;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = imageInfo.arrayLayers;

            EWE_VK(vkCreateImageView, VK::Object->vkDevice, &viewInfo, nullptr, &imageInfo.imageView);
        }

        void CreateTextureSampler(ImageInfo& imageInfo) {
            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.pNext = nullptr;

            //if(tType == tType_2d){
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;

            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

            samplerInfo.anisotropyEnable = VK_TRUE;
            samplerInfo.maxAnisotropy = VK::Object->properties.limits.maxSamplerAnisotropy;

            samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;

            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.mipLodBias = 0.0f;

            //force sampler to not use lowest level by changing this value
            // i.e. samplerInfo.minLod = static_cast<float>(mipLevels / 2);
            samplerInfo.minLod = 0.0f;
            samplerInfo.maxLod = static_cast<float>(imageInfo.mipLevels);

            imageInfo.sampler = Sampler::GetSampler(samplerInfo);
        }

        void CreateImage(ImageInfo* imageInfo, std::string const& path, bool mipmap) {
            PixelPeek pixelPeek{ path };
            return CreateImage(imageInfo, pixelPeek, mipmap);
        }
        void CreateImage(ImageInfo* imageInfo, PixelPeek& pixelPeek, bool mipmap) {
            const bool inMainThread = VK::Object->CheckMainThread();
            if (inMainThread) {
                imageInfo->descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfo->destinationImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            }
            else {
                imageInfo->descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                imageInfo->destinationImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            }
            imageInfo->width = pixelPeek.width;
            imageInfo->height = pixelPeek.height;
            CreateTextureImage(*imageInfo, pixelPeek, mipmap);
            CreateTextureImageView(*imageInfo);
            if (imageInfo->sampler == VK_NULL_HANDLE) {
                CreateTextureSampler(*imageInfo);
            }
            imageInfo->descriptorImageInfo.sampler = imageInfo->sampler;
            imageInfo->descriptorImageInfo.imageView = imageInfo->imageView;
        }

        void Destroy(ImageInfo& imageInfo) {
            Sampler::RemoveSampler(imageInfo.sampler);

            EWE_VK(vkDestroyImageView, VK::Object->vkDevice, imageInfo.imageView, nullptr);
#if USING_VMA
            vmaDestroyImage(VK::Object->vmaAllocator, imageInfo.image, imageInfo.memory);
#else
            //printf("after image view destruction \n");
            EWE_VK(vkDestroyImage, VK::Object->vkDevice, imageInfo.image, nullptr);
            //printf("after image destruction \n");
            EWE_VK(vkFreeMemory, VK::Object->vkDevice, imageInfo.memory, nullptr);
#endif
        }
    } //namespace Image
} //namespace EWE