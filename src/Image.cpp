#include "EWGraphics/Texture/Image.h"

#include <stb/stb_image.h>
#include <filesystem>

#ifndef TEXTURE_DIR
#define TEXTURE_DIR "textures/"
#endif

#define MIPMAP_ENABLED true


namespace EWE {


    PixelPeek::PixelPeek(std::string const& path)
#if DEBUG_NAMING
        : debugName{ path }
#endif
    {
#if EWE_DEBUG
        if(!std::filesystem::exists(path)){
            printf("image path doesn't exist - %s\n", path.c_str());
            assert(false);
        }
#endif
        //pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
#if EWE_DEBUG
        if(!pixels || (width * height <= 0)){
            printf("peel peek error : %s - %zu : %d\n", path.c_str(), reinterpret_cast<std::size_t>(pixels), width * height);
        }
        assert(pixels && ((width * height) > 0));
#endif
    }



}//namespace EWE