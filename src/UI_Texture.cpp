#include "EWGraphics/Texture/UI_Texture.h"
#include "EWGraphics/Texture/Sampler.h"

#include "EWGraphics/Texture/ImageFunctions.h"

#include "EWGraphics/Data/EWE_Memory.h"

#include <stb/stb_image.h>
#include <cmath>


namespace EWE {
    namespace UI_Texture {

        //namespace internal {
        void CreateUIImage(ImageInfo& uiImageInfo, std::vector<PixelPeek> const& pixelPeek, bool mipmapping) {
            std::size_t layerSize = pixelPeek[0].width * pixelPeek[0].height * 4;
            uiImageInfo.arrayLayers = static_cast<uint16_t>(pixelPeek.size());
            const VkDeviceSize imageSize = layerSize * uiImageInfo.arrayLayers;
#if EWE_DEBUG
            assert(pixelPeek.size() > 1 && "creating an array without an array of images?");
            const VkDeviceSize assertionSize = pixelPeek[0].width * pixelPeek[0].height * 4;
            for (uint16_t i = 1; i < pixelPeek.size(); i++) {
                assert(assertionSize == (pixelPeek[i].width * pixelPeek[i].height * 4) && "size is not equal");
            }
#endif

            void* data;
#if USING_VMA
            StagingBuffer* stagingBuffer = Construct<StagingBuffer>({ imageSize });
            vmaMapMemory(VK::Object->vmaAllocator, stagingBuffer->vmaAlloc, &data);
#else
            StagingBuffer* stagingBuffer = Construct<StagingBuffer>({ imageSize });
            stagingBuffer->Map(data);
#endif
            uint64_t memAddress = reinterpret_cast<uint64_t>(data);
            if (MIPMAP_ENABLED && mipmapping) {
                uiImageInfo.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(pixelPeek[0].width, pixelPeek[0].height))) + 1);
            }

            for (uint16_t i = 0; i < pixelPeek.size(); i++) {
                assert((memAddress + layerSize) <= (reinterpret_cast<uint64_t>(data) + stagingBuffer->bufferSize));
                memcpy(reinterpret_cast<void*>(memAddress), pixelPeek[i].pixels, layerSize); //static_cast<void*> unnecessary>?
                stbi_image_free(pixelPeek[i].pixels);
                memAddress += layerSize;
            }
#if USING_VMA
            vmaUnmapMemory(VK::Object->vmaAllocator, stagingBuffer->vmaAlloc);
#else
            EWE_VK(vkUnmapMemory, VK::Object->vkDevice, stagingBuffer->memory);
#endif

            VkImageCreateInfo imageCreateInfo{};
            imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageCreateInfo.pNext = nullptr;
            imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
            imageCreateInfo.extent.width = pixelPeek[0].width;
            imageCreateInfo.extent.height = pixelPeek[0].height;
            imageCreateInfo.extent.depth = 1;
            imageCreateInfo.mipLevels = uiImageInfo.mipLevels;
            imageCreateInfo.arrayLayers = uiImageInfo.arrayLayers;

            imageCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
            imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            if (MIPMAP_ENABLED && mipmapping) {
                imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            }
            imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageCreateInfo.flags = 0;// VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT;

            imageCreateInfo.queueFamilyIndexCount = 0;
            imageCreateInfo.pQueueFamilyIndices = nullptr;

            Image::CreateImageWithInfo(imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, uiImageInfo.image, uiImageInfo.memory);
#if DEBUG_NAMING
            DebugNaming::SetObjectName(reinterpret_cast<void*>(uiImageInfo.image), VK_OBJECT_TYPE_IMAGE, pixelPeek[0].debugName.c_str());
#endif
#if IMAGE_DEBUGGING
            uiImageInfo.imageName = pixelPeek[0].debugName;
#endif
            Image::CreateImageCommands(uiImageInfo, imageCreateInfo, stagingBuffer, mipmapping);
        }

        void CreateUIImageView(ImageInfo& uiImageInfo) {

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.pNext = nullptr;
            viewInfo.image = uiImageInfo.image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = uiImageInfo.mipLevels;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = uiImageInfo.arrayLayers;
            viewInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };

            EWE_VK(vkCreateImageView, VK::Object->vkDevice, &viewInfo, nullptr, &uiImageInfo.imageView);
        }

        void CreateUISampler(ImageInfo& uiImageInfo) {

            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.pNext = nullptr;

            //if(tType == tType_2d){
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;

            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;

            samplerInfo.addressModeV = samplerInfo.addressModeU;
            samplerInfo.addressModeW = samplerInfo.addressModeU;

            samplerInfo.anisotropyEnable = VK_TRUE;
            samplerInfo.maxAnisotropy = VK::Object->properties.limits.maxSamplerAnisotropy;

            samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;

            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.mipLodBias = 0.f;

            //force sampler to not use lowest level by changing this value
            // i.e. samplerInfo.minLod = static_cast<float>(mipLevels / 2);
            samplerInfo.minLod = 0.f;
            samplerInfo.maxLod = 1.f;

            uiImageInfo.sampler = Sampler::GetSampler(samplerInfo);
        }
        //} //namespace internal
    } //namespace UI_Texture
} //namespace EWE