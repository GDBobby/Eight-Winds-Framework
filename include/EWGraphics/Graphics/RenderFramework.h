#pragma once

#include "EWEngine/MainWindow.h"
#include "EWEngine/Graphics/Device.hpp"
#include "EWEngine/Graphics/Renderer.h"
#include "EWEngine/Graphics/Descriptors.h"

#include "EWEngine/Systems/Rendering/Skin/SkinRS.h"

#include "EWEngine/Systems/Rendering/advanced_render_system.h"
//#include "LevelBuilder/LevelBuilder.h"
#include "EWEngine/GUI/UIHandler.h"
//#include "EWEngine/graphicsimGuiHandler.h"
#include "EWEngine/LoadingScreen/LeafSystem.h"
#include "EWEngine/GUI/MenuManager.h"
#include "EWEngine/Systems/PipelineSystem.h"

#include "EWEngine/Graphics/LightBufferObject.h"

#include "EWEngine/Graphics/Texture/Sampler.h"

#include <functional>
#include <memory>
#include <vector>
#include <chrono>



namespace EWE {
	class RenderFramework {
	public:

		//i want to manually control all construciton here
		RenderFramework(std::string windowName);
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

	private:
		bool finishedLoadingScreen = false;
		bool loadingEngine = true;
		double loadingTime = 0.f;


	};
}

