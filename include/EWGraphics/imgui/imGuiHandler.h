
#pragma once

#include "EWGraphics/imgui/imgui.h"
#include "EWGraphics/imgui/backends/imgui_impl_glfw.h"
#include "EWGraphics/imgui/backends/imgui_impl_vulkan.h"
#include "EWGraphics/imgui/implot.h"
#include "EWGraphics/imgui/imnodes.h"

#include "EWGraphics/Data/EngineDataTypes.h"
#include "EWGraphics/Vulkan/Device.hpp"
#include "EWGraphics/Vulkan/Descriptors.h"



#include <stdexcept>

//#define IMGUI_UNLIMITED_FRAME_RATE
#if EWE_DEBUG
#define IMGUI_VULKAN_DEBUG_REPORT
#endif


#ifdef IMGUI_VULKAN_DEBUG_REPORT
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_report(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData) {
	(void)flags; (void)object; (void)location; (void)messageCode; (void)pUserData; (void)pLayerPrefix; // Unused arguments
	fprintf(stderr, "[vulkan] Debug report from ObjectType: %i\nMessage: %s\n\n", objectType, pMessage);
	return VK_FALSE;
}
#endif // IMGUI_VULKAN_DEBUG_REPORT

namespace EWE {
	class ImGUIHandler {
	public:
		ImGUIHandler(GLFWwindow* window, uint32_t imageCount);
        ~ImGUIHandler();

		void beginRender();
		void endRender();

		void rebuild() {
			//ImGui_ImplVulkanH_CreateWindow(device.getInstance(), device.getPhysicalDevice(), EWEDevice::GetVkDevice(), &g_MainWindowData, g_QueueFamily, nullptr, g_SwapChainResizeWidth, g_SwapChainResizeHeight, g_MinImageCount);
		}


		void AddEngineGraph();
		bool logicThreadEnabled = false;
		void AddRenderData(float x, float y);
		void AddLogicData(float x, float y);

		void ChangeCurrentVRAM(VkDeviceSize memoryUsed);
		void InitializeRenderGraph();

		float currentTime{ 0.f };

		void ClearRenderGraphs();

	private:

		struct RollingBufferEWE {
			float Span;
			std::vector<lab::vec2> data;
			RollingBufferEWE() {
				Span = 10.0f;
				data.reserve(2000);
			}
			void AddPoint(float x, float y) {
				float xmod = fmodf(x, Span);
				if (!data.empty() && (xmod < data.back().x)) {
					data.clear();
				}
				data.emplace_back(xmod, y);
			}
		};
		struct ScrollingBufferEWE {
			int MaxSize;
			int Offset;
			float currentTime = 0.f;

			std::vector<lab::vec2> data;
			ScrollingBufferEWE(int max_size = 2000) {
				MaxSize = max_size;
				Offset = 0;
				data.reserve(MaxSize);
			}
			void AddPoint(float x, float y) {
				if (data.size() < MaxSize) {
					data.emplace_back(x, y);
				}
				else {
					data[Offset].x = x;
					data[Offset].y = y;
					Offset = (Offset + 1) % MaxSize;
				}
			}
			void Erase() {
				if (data.size() > 0) {
					data.clear();
					Offset = 0;
				}
			}
		};

		//std::vector<RollingBufferEWE> rollingBuffers{};
		std::vector<ScrollingBufferEWE> scrollingBuffers{};
		

		//float tempFloat{0.f};
		void createDescriptorPool();

	};
}
