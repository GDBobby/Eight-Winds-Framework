#pragma once
#include "EWGraphics/Texture/Image_Manager.h"


namespace EWE {
	class Material_Image {
	private:

	public:
		static MaterialInfo CreateMaterialImage(std::string texPath, bool mipmapping, bool global);
	protected:

	};
}