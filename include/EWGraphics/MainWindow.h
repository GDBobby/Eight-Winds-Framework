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
		MainWindow(uint32_t width, uint32_t height, std::string name);
		~MainWindow();

		MainWindow(const MainWindow&) = delete;
		MainWindow &operator=(const MainWindow&) = delete;

		struct ScreenDimensions {
			uint32_t width;
			uint32_t height;
		};

		inline bool ShouldClose() { return glfwWindowShouldClose(window); }
		inline void CloseWindow() { glfwSetWindowShouldClose(window, GLFW_TRUE); }
		ScreenDimensions GetExtent() { return screenDimensions; }
		bool WasWindowResized() { return frameBufferResized; }
		void ResetWindowResizedFlag() { frameBufferResized = false; }
		GLFWwindow* GetGLFWwindow() const { return window; }

		void CreateWindowSurface(VkInstance instance, VkSurfaceKHR* surface, bool gpuLogging);
		/*
#if defined (_WIN32)
		static HWND GetHWND() { return hWnd; };
#endif
*/
		//void UpdateSettings();
		const GLFWvidmode* GetWindowData() {
			return mode;
		}
	private:
		static MainWindow* mainWindowPtr;
		static void FrameBufferResizeCallback(GLFWwindow* window, int width, int height);

		void InitWindow();
		bool frameBufferResized = false;
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

		GLFWmonitor* GetCurrentMonitor(GLFWwindow* window);
		const GLFWvidmode* mode = nullptr;
	};



}