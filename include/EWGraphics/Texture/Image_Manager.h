#pragma once

#include <EWGraphics/Data/EngineDataTypes.h>
#include <EWGraphics/Texture/ImageFunctions.h>
#include <EWGraphics/Data/MemoryTypeBucket.h>
#include <EWGraphics/Vulkan/Descriptors.h>


#include <vector>
#include <unordered_map>
#include <string>
#include <functional>
#include <unordered_set>

namespace EWE {

	struct ImageTracker {
		ImageInfo imageInfo;
		uint8_t usageCount;
		bool zeroUsageDelete;
		ImageTracker(std::string const& path, bool mipmap, bool zeroUsageDelete) : usageCount{ 1 }, zeroUsageDelete{ zeroUsageDelete } {
			Image::CreateImage(&imageInfo, path, mipmap);
		}
		ImageTracker(std::string const& path, VkSampler sampler, bool mipmap, bool zeroUsageDelete) : usageCount{ 1 }, zeroUsageDelete{ zeroUsageDelete } {
			imageInfo.sampler = sampler;
			Image::CreateImage(&imageInfo, path, mipmap);
		}
		ImageTracker(ImageInfo& imageInfo, bool zeroUsageDelete) : imageInfo{ imageInfo }, usageCount{ 1 }, zeroUsageDelete{ zeroUsageDelete } {
		}
		ImageTracker(bool zeroUsageDelete = false) : imageInfo{}, usageCount{ 0 }, zeroUsageDelete { zeroUsageDelete} {}
	};

	class Image_Manager {
	private:
		//MemoryTypeBucket<1024> imageTrackerBucket;

	protected:
		std::mutex imageMutex{};

		std::vector<ImageID> sceneIDs; //keeping track so i can remove them later

		std::unordered_map<ImageID, ImageTracker*> imageTrackerIDMap{};
		std::unordered_map<std::string, ImageID> imageStringToIDMap{};
		std::unordered_map<ImageID, MaterialInfo> existingMaterialsByID{};
		std::unordered_map<VkShaderStageFlags, EWEDescriptorSetLayout*> simpleTextureLayouts{};
		
		//ImageInfo* skybox_image;
		//ImageInfo* UI_image;
		ImageID currentImageCount{ 0 };

		friend class Material_Image;

		static Image_Manager* imgMgrPtr;
		static ImageID ConstructImageTracker(std::string const& imagePath, bool mipmap, bool zeroUsageDelete = false);
		static ImageID ConstructImageTracker(std::string const& imagePath, VkSampler sampler, bool mipmap, bool zeroUsageDelete = false);
		static EWEDescriptorSetLayout* GetSimpleTextureDSL(VkShaderStageFlags stageFlags);

	public:
		struct ImageReturn {
			ImageID imgID;
			ImageTracker* imgTracker;
		};
		//this is specifically for CubeImage
		static ImageReturn ConstructEmptyImageTracker(bool zeroUsageDelete = false);
		static ImageID ConstructImageTracker(std::string const& path, ImageInfo& imageInfo, bool zeroUsageDelete = false);

		Image_Manager();

		static ImageID GetCreateImageID(std::string const& imagePath, bool mipmap, bool zeroUsageDelete = false);
		static ImageID GetCreateImageID(std::string const& imagePath, VkSampler sampler, bool mipmap, bool zeroUsageDelete = false);


		static VkDescriptorSet CreateSimpleTexture(std::string const& imagePath, bool mipMap, VkShaderStageFlags stageFlags, bool zeroUsageDelete = false);
#if DESCRIPTOR_IMAGE_IMPLICIT_SYNCHRONIZATION
		static VkDescriptorSet CreateSimpleTexture(ImageID imageID, VkShaderStageFlags stageFlags);
#else
		static VkDescriptorSet CreateSimpleTexture(VkDescriptorImageInfo* imageInfo, VkShaderStageFlags stageFlags);
#endif

		static VkDescriptorImageInfo* GetDescriptorImageInfo(ImageID imgID) {
#if EWE_DEBUG
			assert(imgID != IMAGE_INVALID);
#endif
			std::unique_lock<std::mutex> uniq_lock(imgMgrPtr->imageMutex);
			return imgMgrPtr->imageTrackerIDMap.at(imgID)->imageInfo.GetDescriptorImageInfo();
		}
		static ImageID FindByPath(std::string const& path);

		//void ClearSceneImages();
		//void RemoveMaterialImage(TextureDesc removeID);
		static void RemoveImage(ImageID imgID);
		//static void RemoveMaterialImage(ImageID imgID);
		void Cleanup();

		static Image_Manager* GetImageManagerPtr() { return imgMgrPtr; }
		static ImageID CreateUIImage();
		static ImageID CreateImageArrayFromSingleFile(std::string const& filePath, uint32_t layerWidth, uint32_t layerHeight, uint8_t channelCount);
		static ImageID CreateImageArray(std::vector<PixelPeek> const& pixelPeeks, bool mipmapping);
		//static void AddImageInfo(std::string const& path, ImageInfo& imageTemp, bool global);
	};
}

