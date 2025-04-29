#include "EWGraphics/Vulkan/Renderer.h"

#include <EWGraphics/Vulkan/Descriptors.h>

#include <array>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <iomanip>

namespace EWE {
	void EWERenderer::BindGraphicsPipeline(VkPipeline graphicsPipeline) {
		assert(instance != nullptr);

		EWE_VK(vkCmdBindPipeline, VK::Object->GetFrameBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
		EWE_VK(vkCmdSetViewport, VK::Object->GetFrameBuffer(), 0, 1, &instance->viewport);
		EWE_VK(vkCmdSetScissor, VK::Object->GetFrameBuffer(), 0, 1, &instance->scissor);
	}



	EWERenderer* EWERenderer::instance{ nullptr };

	EWERenderer::EWERenderer(MainWindow& window) : mainWindow{ window } {
		instance = this;
		//printf("EWE renderer constructor \n");
		EWEDescriptorPool::BuildGlobalPool();
		RecreateSwapChain();

	}

	EWERenderer::~EWERenderer() {
		instance = nullptr;
#if DECONSTRUCTION_DEBUG
		printf("beginning EWErender deconstruction \n");
#endif
		
#if DECONSTRUCTION_DEBUG
		printf("end EWErender deconstruciton \n");
#endif
	}

	void EWERenderer::RecreateSwapChain() {
#if EWE_DEBUG
		std::cout << "recreating swap chain" << std::endl;
#endif
		needToReadjust = true;
		auto extent = mainWindow.GetExtent();
		uint64_t refreshCount = 0;
		while (extent.width == 0 || extent.height == 0) {
			extent = mainWindow.GetExtent();
			glfwWaitEvents();
			if (refreshCount > 100) {
				printf("failing to reacquire the window?\n");
			}
		}
		EWE_VK(vkDeviceWaitIdle, VK::Object->vkDevice);

		VkExtent2D vulkanExtent(extent.width, extent.height);
		if (eweSwapChain == nullptr) {
			eweSwapChain = std::make_unique<EWESwapChain>(vulkanExtent, true);
		}
		else {
			std::shared_ptr<EWESwapChain> oldSwapChain = std::move(eweSwapChain);
			eweSwapChain = std::make_unique<EWESwapChain>(vulkanExtent, true, oldSwapChain);

			assert(oldSwapChain->CompareSwapFormats(*eweSwapChain.get()) && "Swap chain image(or depth) format has changed!");
		}


		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(eweSwapChain->GetSwapChainExtent().width);
		viewport.height = static_cast<float>(eweSwapChain->GetSwapChainExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		scissor = { {0, 0}, eweSwapChain->GetSwapChainExtent() };
	}

	bool EWERenderer::BeginFrame() {
		assert(!isFrameStarted && "cannot call begin frame while frame is in progress!");

		//std::cout << "begin frame 1" << std::endl;
		if (eweSwapChain->AcquireNextImage(&currentImageIndex)) {
#if EWE_DEBUG
			std::cout << "out of date KHR " << std::endl;
#endif
			RecreateSwapChain();
			return false;
		}
		//std::cout << "begin frame 3" << std::endl;

		isFrameStarted = true;
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		//std::cout << "begin frame 4" << std::endl;
		
		VK::Object->GetFrameBuffer().Begin();
		//EWE_VK(vkBeginCommandBuffer, VK::Object->GetFrameBuffer(), &beginInfo);
#if DEBUG_NAMING
		DebugNaming::SetObjectName(VK::Object->GetVKCommandBufferDirect(), VK_OBJECT_TYPE_COMMAND_BUFFER, "graphics cmd buffer");
#endif

		return true;
	}
	bool EWERenderer::EndFrame() {
		//printf("end frame :: isFrameStarted : %d \n", isFrameStarted);

		EWE_VK(vkEndCommandBuffer, VK::Object->GetFrameBuffer());
		//printf("after end command buffer \n");
		VkResult vkResult = eweSwapChain->SubmitCommandBuffers(&currentImageIndex);
		if (vkResult == VK_ERROR_OUT_OF_DATE_KHR || vkResult == VK_SUBOPTIMAL_KHR || mainWindow.WasWindowResized()) {
			mainWindow.ResetWindowResizedFlag();
			RecreateSwapChain();
			isFrameStarted = false;
			VK::Object->frameIndex = (VK::Object->frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
			return true;
		}
		EWE_VK_RESULT(vkResult);
		//printf("after submitting command buffer \n");

		isFrameStarted = false;
		VK::Object->frameIndex = (VK::Object->frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;

		return false;
	}

	bool EWERenderer::EndFrameAndWaitForFence() {

		EWE_VK(vkEndCommandBuffer, VK::Object->GetFrameBuffer());
		//printf("after end command buffer \n");
		VkResult vkResult = eweSwapChain->SubmitCommandBuffers(&currentImageIndex);
		if (vkResult == VK_ERROR_OUT_OF_DATE_KHR || vkResult == VK_SUBOPTIMAL_KHR || mainWindow.WasWindowResized()) {
			mainWindow.ResetWindowResizedFlag();
			RecreateSwapChain();
			SyncHub::GetSyncHubInstance()->WaitOnGraphicsFence();
			isFrameStarted = false;
			VK::Object->frameIndex = (VK::Object->frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
			return true;
		}
		EWE_VK_RESULT(vkResult);
		//printf("after submitting command buffer \n");
		SyncHub::GetSyncHubInstance()->WaitOnGraphicsFence();

		isFrameStarted = false;
		VK::Object->frameIndex = (VK::Object->frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;

		return false;
	}
	/**/
	void EWERenderer::BeginSwapChainRender() {
		assert(isFrameStarted && "Can't call beginSwapChainRenderPass if frame is not in progress!");
		/*
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = eweSwapChain->getRenderPass();
		renderPassInfo.framebuffer = eweSwapChain->getFrameBuffer(currentImageIndex);

		renderPassInfo.renderArea.offset = { 0,0 };
		renderPassInfo.renderArea.extent = eweSwapChain->getSwapChainExtent();

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { 0.f, 0.f, 0.f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		*/
#if DEBUG_NAMING
		DebugNaming::QueueBegin(Queue::graphics, 0.75f, 0.1f, 0.f, "Begin Render Pass");
#endif

		VkImageMemoryBarrier image_memory_barrier{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.image = eweSwapChain->GetImage(currentImageIndex),
			.subresourceRange = {
			  .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			  .baseMipLevel = 0,
			  .levelCount = 1,
			  .baseArrayLayer = 0,
			  .layerCount = 1,
			}
		};

		image_memory_barrier.srcQueueFamilyIndex = VK::Object->queueIndex[Queue::present];
		image_memory_barrier.dstQueueFamilyIndex = VK::Object->queueIndex[Queue::graphics];

		EWE_VK(vkCmdPipelineBarrier,
			VK::Object->GetFrameBuffer(),
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,  // srcStageMask
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // dstStageMask
			0,
			0, nullptr,
			0, nullptr,
			1, &image_memory_barrier
		);
		eweSwapChain->BeginRender(currentImageIndex);
	}
	void EWERenderer::EndSwapChainRender() {
		assert(isFrameStarted && "Can't call endSwapChainRenderPass if frame is not in progress!");

		//vkCmdEndRenderPass(commandBuffer);

		//std::cout << "before vkCmdEndRendering : " << std::endl;
		EWE_VK(vkCmdEndRendering, VK::Object->GetFrameBuffer());
		//std::cout << "after vkCmdEndRendering : " << std::endl;
		//vkCmdEndRenderingKHR

		VkImageMemoryBarrier image_memory_barrier{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			.image = eweSwapChain->GetImage(currentImageIndex),
			.subresourceRange = {
			  .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			  .baseMipLevel = 0,
			  .levelCount = 1,
			  .baseArrayLayer = 0,
			  .layerCount = 1,
			}
		};
		image_memory_barrier.srcQueueFamilyIndex = VK::Object->queueIndex[Queue::graphics];
		image_memory_barrier.dstQueueFamilyIndex = VK::Object->queueIndex[Queue::present];
		//printf("end frame :: get currentCommandBuffer \n");

		EWE_VK(vkCmdPipelineBarrier,
			VK::Object->GetFrameBuffer(),
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,  // srcStageMask
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, // dstStageMask
			0,
			0,
			nullptr,
			0,
			nullptr,
			1, // imageMemoryBarrierCount
			&image_memory_barrier // pImageMemoryBarriers
		);
#if DEBUG_NAMING
		DebugNaming::QueueEnd(Queue::graphics);
#endif
	}
}