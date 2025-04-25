#include "EWEngine/Graphics/Preprocessor.h"
#if USING_VMA
#include <cstdio>

#define VMA_IMPLEMENTATION
#define VMA_DEBUG_INITIALIZE_ALLOCATIONS 1
#define VMA_DEBUG_MARGIN 16
#define VMA_DEBUG_DETECT_CORRUPTION 1
#include "EWEngine/Graphics/vk_mem_alloc.h"
#endif