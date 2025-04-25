#pragma once

#include "EWEngine/Graphics/VulkanHeader.h"

namespace EWE {


	struct PixelPeek {
		void* pixels{ nullptr };
		int width;
		int height;
		int channels;

#if DEBUG_NAMING
		std::string debugName{};
#endif

		PixelPeek() {}
		PixelPeek(std::string const& path);
		//dont currrently care about desired channels, but maybe one day
	};
	struct ImageInfo {
		VkImage image;
#if USING_VMA
		VmaAllocation memory;
#else
		VkDeviceMemory memory;
#endif
		VkImageView imageView;
		VkSampler sampler{ VK_NULL_HANDLE };
		uint8_t mipLevels{ 1 };
		uint16_t arrayLayers{ 1 };
		VkDescriptorImageInfo descriptorImageInfo;
		uint32_t width;
		uint32_t height;

		VkImageLayout destinationImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
#if IMAGE_DEBUGGING
		std::string imageName{};
#endif


	public:
		VkDescriptorImageInfo* GetDescriptorImageInfo() {
			return &descriptorImageInfo;
		}

		ImageInfo() {}
	};
}
