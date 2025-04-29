#include "EWGraphics/MainWindow.h"

#include <stdexcept>

namespace EWE {
	//HWND MainWindow::hWnd;
	MainWindow* MainWindow::mainWindowPtr = nullptr;

	MainWindow::MainWindow(uint32_t width, uint32_t height, std::string name) : windowName{ name }, screenDimensions{ .width = width, .height = height } {
		//printf("main window constructor \n");
		assert(mainWindowPtr == nullptr);
		mainWindowPtr = this;
		InitWindow();
	}

	MainWindow::~MainWindow() {
		glfwDestroyWindow(window);
		glfwTerminate();
	}
	void MainWindow::InitWindow() {
		glfwInit();

		int count;
		GLFWmonitor** monitors = glfwGetMonitors(&count);
#if EWE_DEBUG
		printf("monitor count : %d \n", count);
#endif
		//for (int i = 0; i < count; i++) {
		//	std::string monName = glfwGetMonitorName(monitors[i]);

		//	const GLFWvidmode* mode = glfwGetVideoMode(monitors[i]);
		//	

		//	printf("monitor name:size - %s:(%.3f:%.3f) \n", monName.c_str(), mode->width, mode->height);
		//}
		mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
		if (screenDimensions.width > mode->width) {
			screenDimensions.width = mode->width;
			//SettingsJSON::saveToJsonFile();
		}
		if (screenDimensions.height > mode->height) {
			screenDimensions.height = mode->height;
			//SettingsJSON::saveToJsonFile();
		}
		//SettingsInfo::ScreenDimensions::FixCommonDimensionsToScreenSize(mode->width, mode->height);

		if ((screenDimensions.width == 0) || (screenDimensions.height == 0)) {
			if (mode != nullptr) {
				screenDimensions.width = mode->width;
				screenDimensions.height = mode->height;
				//printf("primary monitor - %d:%d \n", width, height);
				/*
				SettingsJSON::settingsData.screenDimensions.width = width;
				SettingsJSON::settingsData.screenDimensions.height = height;
				SettingsJSON::tempSettings.screenDimensions.width = width;
				SettingsJSON::tempSettings.screenDimensions.height = height;
				SettingsJSON::saveToJsonFile();
				*/
			}
			else {
				printf("failed to find primary monitor \n");
				screenDimensions.width = 1280;
				screenDimensions.height = 720;
				/*
				SettingsJSON::settingsData.screenDimensions.width = width;
				SettingsJSON::settingsData.screenDimensions.height = height;
				SettingsJSON::tempSettings.screenDimensions.width = width;
				SettingsJSON::tempSettings.screenDimensions.height = height;
				screenDimensions = SettingsJSON::settingsData.screenDimensions;
				*/
			}
		}



		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);


		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		
		//if (SettingsJSON::settingsData.windowMode == SettingsInfo::WT_windowed) {
			glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
		//}
		//else {
		//	glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
		//}

		printf("creating window with width:height  - %d:%d \n", screenDimensions.width, screenDimensions.height);
		window = glfwCreateWindow(screenDimensions.width, screenDimensions.height, windowName.c_str(), nullptr, nullptr);
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, FrameBufferResizeCallback);
		//hWnd = glfwGetWin32Window(window);
	}

	void MainWindow::CreateWindowSurface(VkInstance instance, VkSurfaceKHR* surface, bool gpuLogging) {
		//this->width = width;
		//this->height = height;
		//i think glfwcreatewindow does this, and chatGPT says it does so...
		/*
#if defined (_WIN32)
		VkWin32SurfaceCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		createInfo.hwnd = glfwGetWin32Window(window);
		createInfo.hinstance = GetModuleHandle(nullptr);
		if (vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, surface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create win32 surface");
		}
		return;
#endif
*/
		//glfwSetWindowSize(window, width, height);


		EWE_VK(glfwCreateWindowSurface, instance, window, nullptr, surface);
	}

	void MainWindow::FrameBufferResizeCallback(GLFWwindow* windowPtr, int width, int height) {
		auto mainWindow = reinterpret_cast<MainWindow*>(glfwGetWindowUserPointer(windowPtr));
		mainWindow->frameBufferResized = true;
		const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
		if (width > mode->width) {
			mainWindowPtr->screenDimensions.width = mode->width;
			//SettingsJSON::saveToJsonFile();
		}
		if (height > mode->height) {
			mainWindowPtr->screenDimensions.height = mode->height;
			//SettingsJSON::saveToJsonFile();
		}
		mainWindow->screenDimensions.width = width;
		mainWindow->screenDimensions.height = height;
	}
	/*
	void MainWindow::updateSettings() {
		if (screenDimensions != SettingsJSON::settingsData.screenDimensions) {
			printf("local - SettingsJSON - %u:%u \n", screenDimensions.width, SettingsJSON::settingsData.screenDimensions.width);
			const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
			if (screenDimensions.width > mode->width) {
				screenDimensions.width = mode->width;
				SettingsJSON::saveToJsonFile();
			}
			if (screenDimensions.height > mode->height) {
				screenDimensions.height = mode->height;
				SettingsJSON::saveToJsonFile();
			}
			screenDimensions = SettingsJSON::settingsData.screenDimensions;
			width = SettingsJSON::settingsData.screenDimensions.width;
			height = SettingsJSON::settingsData.screenDimensions.height;

			printf("updating dimensions in mainWindow - %d:%d\n", width, height);
			glfwSetWindowSize(window, width, height);
			frameBufferResized = true;
		}
		if (windowMode != SettingsJSON::settingsData.windowMode) {
			windowMode = SettingsJSON::settingsData.windowMode;
			printf("updating window mode in mainWindow : %d \n", windowMode);
			if (SettingsJSON::settingsData.windowMode == SettingsInfo::WT_windowed) {
				glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_TRUE);
				glfwGetWindowSize(window, &width, &height);
			}
			else {
				glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_FALSE);
				GLFWmonitor* currentMonitor = getCurrentMonitor(window);
				screenDimensions = SettingsJSON::settingsData.screenDimensions;
				width = SettingsJSON::settingsData.screenDimensions.width;
				height = SettingsJSON::settingsData.screenDimensions.height;

				printf("updating dimensions in mainWindow - %d:%d\n", width, height);
				glfwSetWindowSize(window, width, height);

				int xPos, yPos;
				glfwGetMonitorPos(currentMonitor, &xPos, &yPos);
				glfwSetWindowPos(window, xPos, yPos);
			}
			frameBufferResized = true;
		}
	}
	*/
	GLFWmonitor* MainWindow::GetCurrentMonitor(GLFWwindow* window) {
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

}