#pragma once

#include "EWEngine/MainWindow.h"
#include "EWEngine/Graphics/Device.hpp"
#include "EWEngine/Graphics/Swapchain.hpp"
#include "EWEngine/Graphics/TextOverlay.h"

#include <cassert>
#include <memory>
#include <vector>

namespace EWE {
	class EWERenderer {
	private:
		static EWERenderer* instance;

	public:

		static void BindGraphicsPipeline(VkPipeline graphicsPipeline);

		EWERenderer(MainWindow& window);
		~EWERenderer();

		EWERenderer(const EWERenderer&) = delete;
		EWERenderer& operator=(const EWERenderer&) = delete;

		//VkRenderPass getSwapChainRenderPass() const { return eweSwapChain->getRenderPass(); }
		float GetAspectRatio() const { return eweSwapChain->ExtentAspectRatio(); }

		VkExtent2D GetExtent() { 
			needToReadjust = false;
			return eweSwapChain->GetExtent(); 
		}

		bool IsFrameInProgresss() const { return isFrameStarted; }

		bool BeginFrame();
		bool EndFrame();
		bool EndFrameAndWaitForFence();

		void BeginSwapChainRender();
		void EndSwapChainRender();
		TextOverlay* MakeTextOverlay() {
			//assert(!loadingState && "text overlay being made in loading screen renderer?");
			assert(!hasTextOverlayBeenMade && "textoverlay has already been made?");
			hasTextOverlayBeenMade = true;
			printf("CREATING TEXT OVERLAY\n");

			return Construct<TextOverlay>({static_cast<float>(eweSwapChain->Width()), static_cast<float>(eweSwapChain->Height()), *eweSwapChain->GetPipelineInfo()});
		}


		bool needToReadjust = false;

		VkPipelineRenderingCreateInfo* getPipelineInfo() {
			return eweSwapChain->GetPipelineInfo();
		}

		void ChangeClearValues(float r, float g, float b, float a) {
			eweSwapChain->ChangeClearValues(r, g, b, a);
		}

	private:
		VkViewport viewport{};
		VkRect2D scissor{};

		
		//void createCommandBuffers();
		//void freeCommandBuffers();

		void RecreateSwapChain();

		bool hasTextOverlayBeenMade = false;
		
		MainWindow& mainWindow;
		std::unique_ptr<EWESwapChain> eweSwapChain;

		uint32_t currentImageIndex;
		bool isFrameStarted{false};

	};
}

