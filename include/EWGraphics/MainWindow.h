#pragma once
/* COME BACK TO FULLSCREEN LATER ITS TIME TO LET IT REST
#if defined(_WIN32)
#define NOMINMAX
#define VK_USE_PLATFORM_WIN32_KHR
#include <Windows.h>
#include <vulkan/vulkan.h>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_win32.h>
#endif
*/
#include "EWGraphics/Vulkan/VulkanHeader.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
/*
#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"

#endif
*/

#include <string>
namespace EWE {
	class MainWindow {
	public:
		MainWindow(int w, int h, std::string name);
		MainWindow(std::string name);
		~MainWindow();

		MainWindow(const MainWindow&) = delete;
		MainWindow &operator=(const MainWindow&) = delete;

		inline bool shouldClose() { return glfwWindowShouldClose(window); }
		inline void closeWindow() { glfwSetWindowShouldClose(window, GLFW_TRUE); }
		VkExtent2D getExtent() { return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) }; }
		bool wasWindowResized() { return frameBufferResized; }
		void resetWindowResizedFlag() { frameBufferResized = false; }
		GLFWwindow* getGLFWwindow() const { return window; }

		void createWindowSurface(VkInstance instance, VkSurfaceKHR* surface, bool gpuLogging);
		/*
#if defined (_WIN32)
		static HWND GetHWND() { return hWnd; };
#endif
*/
		void updateSettings();

	private:
		static void frameBufferResizeCallback(GLFWwindow* window, int width, int height);

		void initWindow();
		int width;
		int height;
		bool frameBufferResized = false;
		struct ScreenDimensions {
			uint32_t width;
			uint32_t height;
		};
		ScreenDimensions screenDimensions;
		enum WindowMode {
			WM_windowed,
			WM_borderless,
			WM_fullscreen,

			WT_size,
		};
		WindowMode windowMode;

		std::string windowName;
		GLFWwindow* window;





		GLFWmonitor* getCurrentMonitor(GLFWwindow* window) {
			int nmonitors, i;
			int wx, wy, ww, wh;
			int mx, my, mw, mh;
			int overlap, bestoverlap;
			GLFWmonitor* bestmonitor;
			GLFWmonitor** monitors;
			const GLFWvidmode* mode;

			bestoverlap = 0;
			bestmonitor = NULL;

			glfwGetWindowPos(window, &wx, &wy);
			glfwGetWindowSize(window, &ww, &wh);
			monitors = glfwGetMonitors(&nmonitors);

			for (i = 0; i < nmonitors; i++) {
				mode = glfwGetVideoMode(monitors[i]);
				glfwGetMonitorPos(monitors[i], &mx, &my);
				mw = mode->width;
				mh = mode->height;

				overlap = std::max(0, std::min(wx + ww, mx + mw) - std::max(wx, mx)) *
					std::max(0, std::min(wy + wh, my + mh) - std::max(wy, my));

				if (bestoverlap < overlap) {
					bestoverlap = overlap;
					bestmonitor = monitors[i];
				}
			}

			return bestmonitor;
		}
	};



}