#ifdef STADAFX_ENABLED
#include <string>
#include <vector>

#include "vulkan/vulkan.h"
#include "EWGraphics/Vulkan/vk_mem_alloc.h"

#include "EWGraphics/Data/EngineDataTypes.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "EWGraphics/imgui/imgui.h"
#include "EWGraphics/imgui/implot.h"
#include "EWGraphics/imgui/imnodes.h"
#include "EWGraphics/imgui/imguihandler.h"

#include "EWGraphics/Model/Basic_Model.h"
#include "EWGraphics/Model/Model.h"

#include "EWGraphics/resources/common.h"
#include "EWGraphics/resources/LeafMesh.h"
#include "EWGraphics/resources/LeafNames.h"
#include "EWGraphics/resources/LeafTex.h"
#include "EWGraphics/resources/LoadingFrag.h"
#include "EWGraphics/resources/LoadingVert.h"

#include "EWGraphics/Vulkan/Device_Buffer.h"
#include "EWGraphics/MainWindow.h"

//heavy on templates from here?
#include "EWGraphics/Data/ThreadPool.h"

#endif