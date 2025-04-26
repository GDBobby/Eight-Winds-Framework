#include "EWGraphics/RenderFramework.h"

#include "EWGraphics/Vulkan/Device_Buffer.h"


namespace EWE {


	inline void printmat4(lab::mat4& theMatrix, const std::string& matrixName) {
		printf("matrix values : %s\n", matrixName.c_str());
		for (uint8_t i = 0; i < 4; i++) {
			printf("\t%.3f:%.3f:%.3f:%.3f\n", theMatrix.columns[i].x, theMatrix.columns[i].y, theMatrix.columns[i].z, theMatrix.columns[i].w);
		}

	}


	RenderFramework::RenderFramework(std::string windowName) :
		//first, any members not mentioned here with brackets will be initialized
		//second, any memberss in this section will be initialized

		mainWindow{ windowName },
		eweDevice{ mainWindow },
		eweRenderer{ mainWindow },
		//imguiHandler{ mainWindow.getGLFWwindow(), MAX_FRAMES_IN_FLIGHT, eweRenderer.getSwapChainRenderPass() },
		imageManager{ }
	{
#if EWE_DEBUG
		printf("after finishing construction of engine\n");
#endif
		EWEPipeline::PipelineConfigInfo::pipelineRenderingInfoStatic = eweRenderer.getPipelineInfo();
#if EWE_DEBUG
		printf("eight winds constructor, ENGINE_VERSION: %s \n", EWF_VERSION);
#endif

		leafSystem = Construct<LeafSystem>({});

#if EWE_DEBUG
		printf("end of RenderFramework constructor \n");
#endif
	}

	RenderFramework::~RenderFramework() {


		PipelineSystem::Destruct();
		Deconstruct(leafSystem);
#if DECONSTRUCTION_DEBUG
		printf("beginning of RenderFramework deconstructor \n");
#endif

		imageManager.Cleanup();

#if DECONSTRUCTION_DEBUG
		printf("end of RenderFramework deconstructor \n");
#endif
	}


	void RenderFramework::LoadingScreen() {
#if EWE_DEBUG
		printf("BEGINNING LEAF RENDER ~~~~~~~~~~~~~~~~ \n");
#endif

		//setup a temporary camera here
		
		SyncHub* syncHub = SyncHub::GetSyncHubInstance();
#if EWE_DEBUG
		printf("before init leaf data on GPU\n");
#endif
		leafSystem->InitData();
#if EWE_DEBUG
		printf("after init leaf data\n");
#endif

		double renderThreadTime = 0.0;
		const double renderTimeCheck = 1000.0 / 60.0;
		auto startThreadTime = std::chrono::high_resolution_clock::now();
		auto endThreadTime = startThreadTime;
		//printf("starting loading thread loop \n");
		while (loadingEngine || (loadingTime < 2000.0)) {

			endThreadTime = std::chrono::high_resolution_clock::now();
			renderThreadTime += std::chrono::duration<double, std::chrono::milliseconds::period>(endThreadTime - startThreadTime).count();
			startThreadTime = endThreadTime;

			if (renderThreadTime > renderTimeCheck) {
				loadingTime += renderTimeCheck;
				//printf("rendering loading thread start??? \n");
				syncHub->RunGraphicsCallbacks();

				if (eweRenderer.BeginFrame()) {

					eweRenderer.BeginSwapChainRender();
					leafSystem->FallCalculation(static_cast<float>(renderThreadTime / 1000.0));

					leafSystem->Render();
					//uiHandler.drawMenuMain(commandBuffer);
					eweRenderer.EndSwapChainRender();
					if (eweRenderer.EndFrame()) {
						VkExtent2D swapExtent = eweRenderer.GetExtent();

						if (OnResizeCallback != nullptr) {
							OnResizeCallback(swapExtent.width, swapExtent.height);
						}

						//camera.SetPerspectiveProjection(lab::DegreesToRadians(70.0f), static_cast<float>(swapExtent.width) / static_cast<float>(swapExtent.height), 0.1f, 10000.0f);
						
					}
				}
				else {

					VkExtent2D swapExtent = eweRenderer.GetExtent();
					if (OnResizeCallback != nullptr) {
						OnResizeCallback(swapExtent.width, swapExtent.height);
					}
				}

				renderThreadTime = 0.0;
				//printf("end rendering thread \n");
			}
			//printf("end of render thread loop \n");
		}
		finishedLoadingScreen = true;
#if EWE_DEBUG
		printf(" ~~~~ END OF LOADING SCREEN FUNCTION \n");
#endif
	}
}