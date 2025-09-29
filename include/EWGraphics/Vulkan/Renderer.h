#pragma once

#include "EWGraphics/MainWindow.h"
#include "EWGraphics/Vulkan/Device.hpp"
#include "EWGraphics/Vulkan/Swapchain.hpp"

#include <memory>

namespace EWE {
	class EWERenderer {
	private:
		static EWERenderer* instance;

	public:
		static void BindViewportScissor();

		EWERenderer(MainWindow& window);
		~EWERenderer();

		EWERenderer(const EWERenderer&) = delete;
		EWERenderer& operator=(const EWERenderer&) = delete;

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

		bool needToReadjust = false;

		VkPipelineRenderingCreateInfo* getPipelineInfo() {
			return eweSwapChain->GetPipelineInfo();
		}

		void ChangeClearValues(float r, float g, float b, float a) {
			eweSwapChain->ChangeClearValues(r, g, b, a);
		}
		std::unique_ptr<EWESwapChain> eweSwapChain;

	private:
		void RecreateSwapChain();

		bool hasTextOverlayBeenMade = false;
		
		MainWindow& mainWindow;

		uint32_t currentImageIndex;
		bool isFrameStarted{false};

	};
}

