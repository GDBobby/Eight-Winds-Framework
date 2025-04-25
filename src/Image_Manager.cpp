#include "EWEngine/Graphics/Texture/Image_Manager.h"
#include "EWEngine/Graphics/Texture/UI_Texture.h"

#ifndef TEXTURE_DIR
#define TEXTURE_DIR "textures/"
#endif

namespace EWE {


    Image_Manager* Image_Manager::imgMgrPtr{ nullptr };


    Image_Manager::Image_Manager() {// : imageTrackerBucket{sizeof(ImageTracker)} {
        assert(imgMgrPtr == nullptr);
        imgMgrPtr = this;
    }

    void Image_Manager::Cleanup() {
        //call at end of program
#if DECONSTRUCTION_DEBUG
        printf("beginning of texture cleanup \n");
#endif
        //uint32_t tracker = 0;

        for (auto& image : imageTrackerIDMap) {
            //printf("%d tracking \n", tracker++);
            Image::Destroy(image.second->imageInfo);
            Deconstruct(image.second);
            //imageTrackerBucket.FreeDataChunk(image.second);
        }
        for (auto& texDSL : simpleTextureLayouts) {
            Deconstruct(texDSL.second);
        }
        simpleTextureLayouts.clear();

        //globalPool.reset();
#if DECONSTRUCTION_DEBUG
        printf("end of texture cleanup \n");
#endif
    }

    //return value <flags, textureID>
    /*
    void Image_Manager::ClearSceneTextures() {
        //everythign created with a mode texture needs to be destroyed. if it persist thru modes, it needs to be a global texture
#if EWE_DEBUG
        //DBEUUGGIG TEXUTRE BEING CLEARED INCORRECTLY
        //printf("clearing texutre 29 - %s \n", textureMap.at(29).textureData.path.c_str());
        printf("clearing scene textures \n");
#endif

        for (auto& sceneID : sceneIDs) {

            RemoveMaterialTexture(sceneID);


            std::vector<ImageTracker*>& imageTrackers = textureImages.at(sceneID);
            for (auto& imageTracker : imageTrackers) {
#if EWE_DEBUG
                assert(imageTracker->usedInTexture.erase(sceneID) > 0 && "descriptor is using an image that isn't used in descriptor?");
#else
                imageTracker->usedInTexture.erase(sceneID);
#endif
                if (imageTracker->usedInTexture.size() == 0) {
                    Image::Destroy(imageTracker->imageInfo);
                    for (auto iter = imageMap.begin(); iter != imageMap.end(); iter++) {
                        if (iter->second == imageTracker) {
                            
                            imageMap.erase(iter);
                            break;
                        }
                    }
                    imageTracker->~ImageTracker();
                    imageTrackerBucket.FreeDataChunk(imageTracker);

                    textureImages.erase(sceneID);
                }
            }
            //textureMap.erase(sceneID);
        }
        sceneIDs.clear();
        printf("clear mode textures end \n");

    }
    void Image_Manager::RemoveMaterialTexture(TextureDesc removeID) {
        for (auto iter = existingMaterials.begin(); iter != existingMaterials.end(); iter++) {
            if (iter->second.texture == removeID) {
                existingMaterials.erase(iter);
                break;

            }
        }
    }
    */

    void Image_Manager::RemoveImage(ImageID imgID) {
        std::unique_lock<std::mutex> uniq_lock(imgMgrPtr->imageMutex);
#if EWE_DEBUG
        assert(imgMgrPtr->imageTrackerIDMap.contains(imgID));
#endif
        auto& atRet = imgMgrPtr->imageTrackerIDMap.at(imgID);
#if EWE_DEBUG
        assert(atRet->usageCount > 1);
#endif
        atRet->usageCount--;
        if (atRet->usageCount == 0 && atRet->zeroUsageDelete) {
            imgMgrPtr->imageTrackerIDMap.erase(imgID);
            auto materialFindRet = imgMgrPtr->existingMaterialsByID.find(imgID);
            if (materialFindRet != imgMgrPtr->existingMaterialsByID.end()) {
                imgMgrPtr->existingMaterialsByID.erase(imgID);
            }
        }
    }


    ImageID Image_Manager::ConstructImageTracker(std::string const& path, bool mipmap, bool zeroUsageDelete) {
        //ImageTracker* imageTracker = reinterpret_cast<ImageTracker*>(imgMgrPtr->imageTrackerBucket.GetDataChunk());

        //new(imageTracker) ImageTracker(path, mipmap, zeroUsageDelete);
        ImageTracker* imageTracker = Construct<ImageTracker>({ path, mipmap, zeroUsageDelete });
        std::unique_lock<std::mutex> uniq_lock(imgMgrPtr->imageMutex);
        imgMgrPtr->imageTrackerIDMap.try_emplace(imgMgrPtr->currentImageCount, imageTracker);
        return imgMgrPtr->currentImageCount++;
    }
    ImageID Image_Manager::ConstructImageTracker(std::string const& path, VkSampler sampler, bool mipmap, bool zeroUsageDelete) {
        //ImageTracker* imageTracker = reinterpret_cast<ImageTracker*>(imgMgrPtr->imageTrackerBucket.GetDataChunk());

        //new(imageTracker) ImageTracker(path, mipmap, zeroUsageDelete);
        ImageTracker* imageTracker = Construct<ImageTracker>({ path, sampler, mipmap, zeroUsageDelete });

        std::unique_lock<std::mutex> uniq_lock(imgMgrPtr->imageMutex);
        imgMgrPtr->imageTrackerIDMap.try_emplace(imgMgrPtr->currentImageCount, imageTracker);
        return imgMgrPtr->currentImageCount++;
    }
    ImageID Image_Manager::ConstructImageTracker(std::string const& path, ImageInfo& imageInfo, bool zeroUsageDelete) {
        //ImageTracker* imageTracker = reinterpret_cast<ImageTracker*>(imgMgrPtr->imageTrackerBucket.GetDataChunk());
        //new(imageTracker) ImageTracker(imageInfo, zeroUsageDelete);
        ImageTracker* imageTracker = Construct<ImageTracker>({ imageInfo, zeroUsageDelete });

        std::unique_lock<std::mutex> uniq_lock(imgMgrPtr->imageMutex);
        imgMgrPtr->imageTrackerIDMap.try_emplace(imgMgrPtr->currentImageCount, imageTracker);
        return imgMgrPtr->currentImageCount++;
    }

    Image_Manager::ImageReturn Image_Manager::ConstructEmptyImageTracker(std::string const& path, bool zeroUsageDelete) {

        //ImageTracker* imageTracker = reinterpret_cast<ImageTracker*>(imgMgrPtr->imageTrackerBucket.GetDataChunk());
        //new(imageTracker) ImageTracker(zeroUsageDelete);
        ImageTracker* imageTracker = Construct<ImageTracker>({ zeroUsageDelete });

        std::unique_lock<std::mutex> uniq_lock(imgMgrPtr->imageMutex);
        const ImageID tempID = imgMgrPtr->currentImageCount++;
        return ImageReturn{ tempID, imgMgrPtr->imageTrackerIDMap.try_emplace(tempID, imageTracker).first->second };
    }


    ImageID Image_Manager::CreateImageArray(std::vector<PixelPeek> const& pixelPeeks, bool mipmapping) {

        ImageTracker* imageTracker = Construct<ImageTracker>({});
        ImageInfo* arrayImageInfo = &imageTracker->imageInfo;
#if EWE_DEBUG
        printf("before ui image\n");
#endif

        if (VK::Object->CheckMainThread()) {
            arrayImageInfo->descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            arrayImageInfo->destinationImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }
        else {
            arrayImageInfo->descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            arrayImageInfo->destinationImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }

        arrayImageInfo->width = pixelPeeks[0].width;
        arrayImageInfo->height = pixelPeeks[0].height;
        UI_Texture::CreateUIImage(*arrayImageInfo, pixelPeeks, mipmapping);
#if EWE_DEBUG
        printf("after ui image\n");
#endif
        UI_Texture::CreateUIImageView(*arrayImageInfo);
        UI_Texture::CreateUISampler(*arrayImageInfo);

        arrayImageInfo->descriptorImageInfo.sampler = arrayImageInfo->sampler;
        arrayImageInfo->descriptorImageInfo.imageView = arrayImageInfo->imageView;
        //arrayImageInfo->descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        std::unique_lock<std::mutex> uniq_lock(imgMgrPtr->imageMutex);
        ImageID ret = imgMgrPtr->currentImageCount;
        imgMgrPtr->imageTrackerIDMap.try_emplace(ret, imageTracker);
        imgMgrPtr->currentImageCount++;
        return ret;
        //return arrayImageInfo;
    }
    ImageID Image_Manager::CreateUIImage() {
        const std::vector<std::string> uiNames = {
            "NineUI.png",
            "NineFade.png",
            "clickyBox.png",
            "bracketButton.png",
            "bracketSlide.png",
            "unchecked.png",
            "ButtonUp.png",
            "checked.png",
            "menuBase.png",
            //"roundButton.png",
        };
        std::vector<PixelPeek> pixelPeeks{};
        pixelPeeks.reserve(uiNames.size());

        for (auto const& name : uiNames) {
            std::string individualPath = "textures/UI/";
            individualPath += name;

            pixelPeeks.emplace_back(individualPath);

        }
        return CreateImageArray(pixelPeeks, false);
        //GetimgMgrPtr()->UI_image = uiImageInfo;
    }

    ImageID Image_Manager::CreateImageArrayFromSingleFile(std::string const& filePath, uint32_t layerWidth, uint32_t layerHeight, uint8_t channelCount) {

        PixelPeek firstImage{ filePath };
        assert(((firstImage.width % layerWidth) == 0) && ((firstImage.height % layerHeight) == 0));
        const uint32_t imageCount = firstImage.width / layerWidth * firstImage.height / layerHeight;
        const uint32_t pixelCount = layerWidth * layerHeight;
        const std::size_t layerSize = pixelCount * layerHeight;

        ImageTracker* imageTracker = Construct<ImageTracker>({});
        ImageInfo* arrayImageInfo = &imageTracker->imageInfo;
        if (VK::Object->CheckMainThread()) {
            arrayImageInfo->descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            arrayImageInfo->destinationImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }
        else {
            arrayImageInfo->descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            arrayImageInfo->destinationImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }
        arrayImageInfo->width = layerWidth;
        arrayImageInfo->height = layerHeight;
        arrayImageInfo->arrayLayers = imageCount;

        const std::size_t totalSize = firstImage.width * firstImage.height * channelCount;
        StagingBuffer* stagingBuffer = Construct<StagingBuffer>({ totalSize });

        void* data; //void* normally, but I want to be able to control it by the byte
        stagingBuffer->Map(data);
        stagingBuffer->Stage(data, totalSize);

        const std::size_t verticalCount = firstImage.height / layerHeight;
        const std::size_t horiCount = firstImage.width / layerWidth;
        const std::size_t tileDataSize = layerWidth * layerHeight * channelCount;
        const std::size_t baseImageRowSize = firstImage.width * channelCount;
        const std::size_t singleTileRowMemorySize = layerWidth * channelCount;

        //reorganizing the data, in layers, the entire first layer image is contiguous. in a png, each full row from the full image is contiguous
        uint8_t* basePixels = reinterpret_cast<uint8_t*>(firstImage.pixels);
        uint8_t* dstPixels = reinterpret_cast<uint8_t*>(data);

        std::size_t currentTile = 0;
        for (std::size_t currentTileRow = 0; currentTileRow < verticalCount; currentTileRow++) {

            const std::size_t rowBeginningTile = currentTileRow * horiCount;
            const std::size_t rowEndingTile = rowBeginningTile + horiCount;
            const std::size_t rowBeginningMemory = baseImageRowSize * layerHeight * currentTileRow;

            for (std::size_t pixelRowWithinTile = 0; pixelRowWithinTile < layerHeight; pixelRowWithinTile++) {
                const std::size_t innerRowBeginningMemory = rowBeginningMemory + (pixelRowWithinTile * baseImageRowSize);

                for (std::size_t currentTile = rowBeginningTile; currentTile < rowEndingTile; currentTile++) {
                    const std::size_t singleTileBaseMemOffset = singleTileRowMemorySize * (currentTile - rowBeginningTile);
                    const std::size_t currentBaseOffset = innerRowBeginningMemory + singleTileBaseMemOffset;

                    const std::size_t dstInnerOffset = pixelRowWithinTile * singleTileRowMemorySize;

                    memcpy(dstPixels + (currentTile * tileDataSize + dstInnerOffset), basePixels + currentBaseOffset, singleTileRowMemorySize);
                }
            }
        }

        //memcpy(data, firstImage.pixels, totalSize);
        free(firstImage.pixels);
        stagingBuffer->Unmap();
        //mips here, if desired. currently no

        VkImageCreateInfo imageCreateInfo{};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.pNext = nullptr;
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.extent.width = layerWidth;
        imageCreateInfo.extent.height = layerHeight;
        imageCreateInfo.extent.depth = 1;
        imageCreateInfo.mipLevels = arrayImageInfo->mipLevels;
        imageCreateInfo.arrayLayers = arrayImageInfo->arrayLayers;

        imageCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        //if (MIPMAP_ENABLED && mipmapping) {
        //    imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        //}
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.flags = 0;// VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT;

        imageCreateInfo.queueFamilyIndexCount = 0;
        imageCreateInfo.pQueueFamilyIndices = nullptr;

        Image::CreateImageWithInfo(imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, arrayImageInfo->image, arrayImageInfo->memory);
#if DEBUG_NAMING
        DebugNaming::SetObjectName(arrayImageInfo->image, VK_OBJECT_TYPE_IMAGE, filePath.c_str());
#endif
#if IMAGE_DEBUGGING
        arrayImageInfo->imageName = filePath;
#endif
        Image::CreateImageCommands(*arrayImageInfo, imageCreateInfo, stagingBuffer, false);

        UI_Texture::CreateUIImageView(*arrayImageInfo);
        UI_Texture::CreateUISampler(*arrayImageInfo);

        arrayImageInfo->descriptorImageInfo.sampler = arrayImageInfo->sampler;
        arrayImageInfo->descriptorImageInfo.imageView = arrayImageInfo->imageView;

        std::unique_lock<std::mutex> uniq_lock(imgMgrPtr->imageMutex);
        ImageID ret = imgMgrPtr->currentImageCount;
        imgMgrPtr->imageTrackerIDMap.try_emplace(ret, imageTracker);
        imgMgrPtr->currentImageCount++;
        return ret;
    }

    ImageID Image_Manager::FindByPath(std::string const& path) {
        std::unique_lock<std::mutex> uniq_lock(imgMgrPtr->imageMutex);
        auto foundImage = imgMgrPtr->imageStringToIDMap.find(path);
        if (foundImage == imgMgrPtr->imageStringToIDMap.end()) {
            return IMAGE_INVALID;
        }
        return foundImage->second;
    }

    EWEDescriptorSetLayout* Image_Manager::GetSimpleTextureDSL(VkShaderStageFlags stageFlags) {
        EWEDescriptorSetLayout* simpleTextureDSL;
        std::unique_lock<std::mutex> uniq_lock(imgMgrPtr->imageMutex);
        auto findRet = imgMgrPtr->simpleTextureLayouts.find(stageFlags);
        if (findRet == imgMgrPtr->simpleTextureLayouts.end()) {
            EWEDescriptorSetLayout::Builder builder{};
            builder.AddBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stageFlags);
            simpleTextureDSL = builder.Build();
            imgMgrPtr->simpleTextureLayouts.emplace(stageFlags, simpleTextureDSL);
        }
        else {
            simpleTextureDSL = findRet->second;
        }
        return simpleTextureDSL;
    }

    VkDescriptorSet Image_Manager::CreateSimpleTexture(std::string const& imagePath, bool mipMap, VkShaderStageFlags stageFlags, bool zeroUsageDelete) {
        assert(false && "this needs to be fixed up, it's not waiting for the transfer to graphics transition");

        ImageID imgID = GetCreateImageID(imagePath, mipMap, zeroUsageDelete);

        EWEDescriptorWriter descWriter{ GetSimpleTextureDSL(stageFlags), DescriptorPool_Global };
#if DESCRIPTOR_IMAGE_IMPLICIT_SYNCHRONIZATION
        descWriter.WriteImage(imgID);
#else
        descWriter.WriteImage(0, GetDescriptorImageInfo(imgID));
#endif
        return descWriter.Build();
    }

#if DESCRIPTOR_IMAGE_IMPLICIT_SYNCHRONIZATION
    VkDescriptorSet Image_Manager::CreateSimpleTexture(ImageID imageID, VkShaderStageFlags stageFlags) {
#else
    VkDescriptorSet Image_Manager::CreateSimpleTexture(VkDescriptorImageInfo* imageInfo, VkShaderStageFlags stageFlags) {
#endif

        EWEDescriptorWriter descWriter{ GetSimpleTextureDSL(stageFlags), DescriptorPool_Global };
#if DESCRIPTOR_IMAGE_IMPLICIT_SYNCHRONIZATION
        descWriter.WriteImage(imageID);
#else
        descWriter.WriteImage(0, imageInfo);
#endif
        return descWriter.Build();
    }

    ImageID Image_Manager::GetCreateImageID(std::string const& imagePath, bool mipmap, bool zeroUsageDelete) {
        imgMgrPtr->imageMutex.lock();
        auto findRet = imgMgrPtr->imageStringToIDMap.find(imagePath);
        if (findRet == imgMgrPtr->imageStringToIDMap.end()) {
            imgMgrPtr->imageMutex.unlock();
            return ConstructImageTracker(imagePath, mipmap, zeroUsageDelete);
        }
        else {
            imgMgrPtr->imageMutex.unlock();
            return findRet->second;
        }
    }
    ImageID Image_Manager::GetCreateImageID(std::string const& imagePath, VkSampler sampler, bool mipmap, bool zeroUsageDelete) {
        imgMgrPtr->imageMutex.lock();
        auto findRet = imgMgrPtr->imageStringToIDMap.find(imagePath);
        if (findRet == imgMgrPtr->imageStringToIDMap.end()) {
            imgMgrPtr->imageMutex.unlock();
            return ConstructImageTracker(imagePath, sampler, mipmap, zeroUsageDelete);
        }
        else {
            imgMgrPtr->imageMutex.unlock();
            return findRet->second;
        }
    }
}