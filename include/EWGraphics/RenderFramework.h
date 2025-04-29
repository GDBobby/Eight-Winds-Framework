#pragma once

#include "EWGraphics/MainWindow.h"
#include "EWGraphics/Vulkan/Device.hpp"
#include "EWGraphics/Vulkan/Renderer.h"
#include "EWGraphics/Vulkan/Descriptors.h"
//#include "EWEngine/graphicsimGuiHandler.h"
#include "EWGraphics/LeafSystem.h"
#include "EWGraphics/PipelineSystem.h"

#include "EWGraphics/Texture/Image_Manager.h"

#include <functional>
#include <memory>
#include <vector>
#include <chrono>

#define EWF_VERSION "1.0.0.0"

namespace EWE {
	class RenderFramework {
	public:

		//i want to manually control all construciton here
		RenderFramework(uint32_t windowWidth, uint32_t windowHeight, std::string windowName);
		//fourth will be class member construciton without brackets

		~RenderFramework();

		RenderFramework(const RenderFramework&) = delete;
		RenderFramework& operator=(const RenderFramework&) = delete;


		MainWindow mainWindow;
		EWEDevice eweDevice;
		EWERenderer eweRenderer;
		Image_Manager imageManager;

		LeafSystem* leafSystem;


#if BENCHMARKING_GPU
		VkQueryPool queryPool[MAX_FRAMES_IN_FLIGHT] = { VK_NULL_HANDLE, VK_NULL_HANDLE };
		float gpuTicksPerSecond = 0;
#endif


		void LoadingScreen();

		void EndEngineLoadScreen() {
#if EWE_DEBUG
			printf("~~~~ ENDING LOADING SCREEN ~~~ \n");
#endif
#if ONE_SUBMISSION_THREAD_PER_QUEUE
			//dependent on this not being in the graphics thread, or it'll infinitely loop
			SyncHub* syncHub = SyncHub::GetSyncHubInstance();
			while (!TransferCommandManager::Empty() || syncHub->CheckFencesForUsage()) {/*printf("waiting on fences\n");*/ std::this_thread::sleep_for(std::chrono::nanoseconds(1)); }
#endif
			loadingEngine = false;
		}
		bool GetLoadingScreenProgress() {
			return (!finishedLoadingScreen);// || (loadingTime < 2.0);
		}

		std::function<void(uint32_t width, uint32_t height)> OnResizeCallback = nullptr;

	private:
		bool finishedLoadingScreen = false;
		bool loadingEngine = true;
		double loadingTime = 0.f;


	};
}

