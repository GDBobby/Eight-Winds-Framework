#include "EWEngine/Graphics/Texture/Cube_Texture.h"


#include "EWEngine/Graphics/Texture/Image_Manager.h"
#include "EWEngine/Graphics/Texture/Sampler.h"
#include "EWEngine/Graphics/TransferCommandManager.h"

#include "EWEngine/Graphics/Texture/ImageFunctions.h"

#include <stb/stb_image.h>

namespace EWE {

#ifndef SKYBOX_DIR
#define SKYBOX_DIR "textures/skybox/"
#endif
    namespace Cube_Texture {
        void CreateCubeImage(ImageInfo& cubeImage, std::vector<PixelPeek>& pixelPeek) {
            uint64_t layerSize = pixelPeek[0].width * pixelPeek[0].height * 4;
            cubeImage.arrayLayers = 6;
            VkDeviceSize imageSize = layerSize * cubeImage.arrayLayers;

            void* data;
            StagingBuffer* stagingBuffer = Construct<StagingBuffer>({ imageSize });
            stagingBuffer->Map(data);
            uint64_t memAddress = reinterpret_cast<uint64_t>(data);
            cubeImage.mipLevels = 1;
            for (int i = 0; i < 6; i++) {
                memcpy(reinterpret_cast<void*>(memAddress), pixelPeek[i].pixels, static_cast<std::size_t>(layerSize)); //static_cast<void*> unnecessary>?
                stbi_image_free(pixelPeek[i].pixels);
                memAddress += layerSize;
            }
            stagingBuffer->Unmap();

            VkImageCreateInfo imageCreateInfo;
            imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageCreateInfo.pNext = nullptr;
            imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
            imageCreateInfo.extent.width = pixelPeek[0].width;
            imageCreateInfo.extent.height = pixelPeek[0].height;
            imageCreateInfo.extent.depth = 1;
            imageCreateInfo.mipLevels = 1;
            imageCreateInfo.arrayLayers = cubeImage.arrayLayers;

            imageCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
            imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

            imageCreateInfo.queueFamilyIndexCount = 0;
            imageCreateInfo.pQueueFamilyIndices = nullptr;

            Image::CreateImageWithInfo(imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, cubeImage.image, cubeImage.memory);
#if DEBUG_NAMING
            DebugNaming::SetObjectName(cubeImage.image, VK_OBJECT_TYPE_IMAGE, pixelPeek[0].debugName.c_str());
#endif

#if IMAGE_DEBUGGING
            cubeImage.imageName = pixelPeek[0].debugName;
#endif
            Image::CreateImageCommands(cubeImage, imageCreateInfo, stagingBuffer, false);
        }

        void CreateCubeImageView(ImageInfo& cubeImage) {

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.pNext = nullptr;
            viewInfo.image = cubeImage.image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
            viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = cubeImage.mipLevels;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = cubeImage.arrayLayers;
            viewInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };

            EWE_VK(vkCreateImageView, VK::Object->vkDevice, &viewInfo, nullptr, &cubeImage.imageView);
        }

        void CreateCubeSampler(ImageInfo& cubeImage) {

            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.pNext = nullptr;

            //if(tType == tType_2d){
            samplerInfo.magFilter = VK_FILTER_NEAREST;
            samplerInfo.minFilter = VK_FILTER_NEAREST;

            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

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

            cubeImage.sampler = Sampler::GetSampler(samplerInfo);
        }

        ImageID CreateCubeImage(std::string texPath, std::string extension) {
            {
                ImageID foundImage = Image_Manager::FindByPath(texPath);
                if (foundImage != IMAGE_INVALID) {
                    return foundImage;
                }
            }
            const std::array<std::string, 6> cubeNames = {
                "px", "nx", "py", "ny", "pz", "nz"
            };
            std::vector<PixelPeek> pixelPeeks{};
            pixelPeeks.reserve(6);

            for (int i = 0; i < 6; i++) {
                std::string individualPath = SKYBOX_DIR;
                individualPath += texPath;
                individualPath += cubeNames[i];
                individualPath += extension;

                pixelPeeks.emplace_back(individualPath);

                assert(!((i > 0) && ((pixelPeeks[i].width != pixelPeeks[i - 1].width) || (pixelPeeks[i].height != pixelPeeks[i - 1].height))) && "failed to load cube texture, bad dimensions");
            }
            
            Image_Manager::ImageReturn cubeTracker = Image_Manager::ConstructEmptyImageTracker(texPath);
            ImageInfo& cubeImage = cubeTracker.imgTracker->imageInfo;

            cubeImage.descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            cubeImage.destinationImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            

            CreateCubeImage(cubeImage, pixelPeeks);
            CreateCubeImageView(cubeImage);
            CreateCubeSampler(cubeImage);

            cubeImage.descriptorImageInfo.sampler = cubeImage.sampler;
            cubeImage.descriptorImageInfo.imageView = cubeImage.imageView;

            return cubeTracker.imgID;
        }
    }//namespace Cube_Texture
}//namespace EWE