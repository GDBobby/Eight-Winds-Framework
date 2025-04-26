#pragma once

#include "EWGraphics/Texture/Image.h"

namespace EWE {
	namespace UI_Texture {
		void CreateUIImage(ImageInfo& uiImageInfo, std::vector<PixelPeek> const& pixelPeek, bool mipmapping);

		void CreateUIImageView(ImageInfo& uiImageInfo);
		void CreateUISampler(ImageInfo& uiImageInfo);
	}//namespace UI_Texture
} //namespace EWE

