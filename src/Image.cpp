#include "EWGraphics/Texture/Image.h"

#ifndef TEXTURE_DIR
#define TEXTURE_DIR "textures/"
#endif

#define MIPMAP_ENABLED true


namespace EWE {


    PixelPeek::PixelPeek(void* data std::string const& path) : pixels{data}
#if DEBUG_NAMING
        : debugName{ path }
#endif
    {
    }



}//namespace EWE