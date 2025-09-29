#include "EWGraphics/Vulkan/SyncHub.h"
#include "EWGraphics/Texture/ImageFunctions.h"

#include <future>
#include <cassert>

namespace EWE {
	SyncHub* SyncHub::syncHubSingleton{ nullptr };

	SyncHub::SyncHub() :
		qSyncPool{ 255 }, 
		renderSyncData{}
	{
#if EWE_DEBUG
		printf("CONSTRUCTING SYNCHUB\n");
#endif
	}
#if EWE_DEBUG
	SyncHub::~SyncHub() {
#if DECONSTRUCTION_DEBUG
		printf("DECONSTRUCTING SYNCHUB\n");
#endif
	}
#endif

	void SyncHub::Initialize() {
		syncHubSingleton = Construct<SyncHub>();
		syncHubSingleton->CreateBuffers();
	}
	void SyncHub::Destroy() {
		assert(syncHubSingleton != nullptr);
#if DECONSTRUCTION_DEBUG
		printf("beginniing synchub destroy \n");
#endif
		std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> cmdBufs{
			VK::Object->renderCommands[0].cmdBuf,
			VK::Object->renderCommands[1].cmdBuf
		};
		for (uint8_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			EWE_VK(vkFreeCommandBuffers, VK::Object->vkDevice, VK::Object->renderCmdPool, MAX_FRAMES_IN_FLIGHT, cmdBufs.data());
		}

		Deconstruct(syncHubSingleton);
		syncHubSingleton = nullptr;

#if DECONSTRUCTION_DEBUG
		printf("end synchub destroy \n");
#endif
	}

	void SyncHub::CreateBuffers() {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.pNext = nullptr;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = VK::Object->renderCmdPool;
		allocInfo.commandBufferCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> tempCmdBuf{};
		EWE_VK(vkAllocateCommandBuffers, VK::Object->vkDevice, &allocInfo, &tempCmdBuf[0]);
		VK::Object->renderCommands[0].cmdBuf = tempCmdBuf[0];
		VK::Object->renderCommands[1].cmdBuf = tempCmdBuf[1];
	}

	CommandBuffer& SyncHub::BeginSingleTimeCommandGraphics() {
#if DEBUG_NAMING
		CommandBuffer& ret = qSyncPool.GetCmdBufSingleTime(Queue::graphics);
		DebugNaming::SetObjectName(ret.cmdBuf, VK_OBJECT_TYPE_COMMAND_BUFFER, "single time cmd buf[graphics]");
		return ret;
#else
		return qSyncPool.GetCmdBufSingleTime(Queue::graphics);
#endif
	}
	CommandBuffer& SyncHub::BeginSingleTimeCommandTransfer() {
		assert(VK::Object->queueEnabled[Queue::transfer]);
#if DEBUG_NAMING
		CommandBuffer& ret = qSyncPool.GetCmdBufSingleTime(Queue::transfer);
		DebugNaming::SetObjectName(ret.cmdBuf, VK_OBJECT_TYPE_COMMAND_BUFFER, "single time cmd buf[transfer]");
		return ret;
#else
		return qSyncPool.GetCmdBufSingleTime(Queue::transfer);
#endif
	}

	CommandBuffer& SyncHub::BeginSingleTimeCommand() {
		if (VK::Object->CheckMainThread()) {
			return BeginSingleTimeCommandGraphics();
		}
		else if (!VK::Object->queueEnabled[Queue::transfer]) {
			return BeginSingleTimeCommandGraphics();
		}
		else {
			return BeginSingleTimeCommandTransfer();
		}
	}


	void SyncHub::RunGraphicsCallbacks() {
		qSyncPool.CheckFencesForCallbacks();
	}
	void SyncHub::EndSingleTimeCommandGraphics(GraphicsCommand& graphicsCommand) {

		EWE_VK(vkEndCommandBuffer, *graphicsCommand.command);

		if (std::this_thread::get_id() == VK::Object->mainThreadID) {

			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.pNext = nullptr;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &graphicsCommand.command->cmdBuf;
			submitInfo.signalSemaphoreCount = 1;
			Semaphore* semaphore = qSyncPool.GetSemaphoreForSignaling();
			submitInfo.pSignalSemaphores = &semaphore->vkSemaphore;

			GraphicsFence& fence = qSyncPool.GetMainThreadGraphicsFence();
			fence.signalSemaphore = semaphore;
			fence.gCommand = graphicsCommand;

			VK::Object->queueMutex[Queue::graphics].lock();
			EWE_VK(vkQueueSubmit, VK::Object->queues[Queue::graphics], 1, &submitInfo, fence.fence.vkFence);
			renderSyncData.AddWaitSemaphore(semaphore, graphicsCommand.waitStage);
			VK::Object->queueMutex[Queue::graphics].unlock();
			fence.fence.submitted = true;
		}
		else {
			VkSubmitInfo graphicsSubmitInfo{};
			graphicsSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			graphicsSubmitInfo.pNext = nullptr;
			graphicsSubmitInfo.commandBufferCount = 1;
			graphicsSubmitInfo.pCommandBuffers = &graphicsCommand.command->cmdBuf;

			graphicsSubmitInfo.waitSemaphoreCount = 0;
			graphicsSubmitInfo.pWaitSemaphores = nullptr;

			Semaphore* graphicsSemaphore = qSyncPool.GetSemaphoreForSignaling();
			graphicsSubmitInfo.signalSemaphoreCount = 1;
			graphicsSubmitInfo.pSignalSemaphores = &graphicsSemaphore->vkSemaphore;

			VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
			graphicsSubmitInfo.pWaitDstStageMask = &waitStage;

			Fence& graphicsFence = qSyncPool.GetFence();
			VK::Object->queueMutex[Queue::graphics].lock();
			EWE_VK(vkQueueSubmit, VK::Object->queues[Queue::graphics], 1, &graphicsSubmitInfo, graphicsFence.vkFence);
			renderSyncData.AddWaitSemaphore(graphicsSemaphore, waitStage);
			VK::Object->queueMutex[Queue::graphics].unlock();

			graphicsFence.submitted = true;

			while (!graphicsFence.CheckReturn(0)) {
				std::this_thread::sleep_for(std::chrono::nanoseconds(1));
			}
			if (graphicsCommand.stagingBuffer != nullptr) {
				graphicsCommand.stagingBuffer->Free();
				Deconstruct(graphicsCommand.stagingBuffer);
			}
			if (graphicsCommand.imageInfo != nullptr) {
				graphicsCommand.imageInfo->descriptorImageInfo.imageLayout = graphicsCommand.imageInfo->destinationImageLayout;
			}
			graphicsCommand.command->Reset();
			graphicsFence.submitted = false;
			graphicsFence.inUse = false;
			
		}
	}

	void SyncHub::EndSingleTimeCommandTransfer(TransferCommand& transferCommand) {
		assert(VK::Object->queueEnabled[Queue::transfer]);


		Fence& transferFence = qSyncPool.GetFence();
		VkSubmitInfo transferSubmitInfo{};
		transferSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		transferSubmitInfo.pNext = nullptr;
		transferSubmitInfo.commandBufferCount = 1;
#if 0//GROUPING_SUBMITS
		std::vector<VkCommandBuffer> cmdBufs(transferCommand.commands.size());
		for (uint32_t i = 0; i < cmdBufs.size(); i++) {
			EWE_VK(vkEndCommandBuffer, *transferCommand.commands[i]);
			cmdBufs[i] = transferCommand.commands[i]->cmdBuf
		}
		transferSubmitInfo.pCommandBuffers = cmdBufs.data();
#else
		EWE_VK(vkEndCommandBuffer, *transferCommand.commands[0]);
		assert(transferCommand.commands.size() == 1);
		transferSubmitInfo.pCommandBuffers = &transferCommand.commands[0]->cmdBuf;
#endif

		Semaphore* transferSemaphore = nullptr; 

		if ((transferCommand.images.size() > 0) || (transferCommand.pipeBarriers.size() > 0)) {
			transferSemaphore = qSyncPool.GetSemaphoreForSignaling();
			PipelineBarrier::SimplifyVector(transferCommand.pipeBarriers);
			transferSubmitInfo.signalSemaphoreCount = 1;
			transferSubmitInfo.pSignalSemaphores = &transferSemaphore->vkSemaphore;
		}
		else {
			transferSubmitInfo.pSignalSemaphores = nullptr;
			transferSubmitInfo.signalSemaphoreCount = 0;
		}

		VK::Object->queueMutex[Queue::transfer].lock();
		EWE_VK(vkQueueSubmit, VK::Object->queues[Queue::transfer], 1, &transferSubmitInfo, transferFence.vkFence);
		VK::Object->queueMutex[Queue::transfer].unlock();

#if DEBUGGING_FENCES
		transferFence.fence.log.push_back("setting transfer fence to submitted");
#endif
		transferFence.submitted = true;

		if (transferSemaphore == nullptr) {
			while (!transferFence.CheckReturn(0)) {
				std::this_thread::sleep_for(std::chrono::nanoseconds(1));
			}
			for (auto& sb : transferCommand.stagingBuffers) {
				sb->Free();
				Deconstruct(sb);
			}
			for (auto& cmd : transferCommand.commands) {
				cmd->Reset();
			}
			transferFence.inUse = false;
		}
		else {
			CommandBuffer& graphicsCmdBuf = qSyncPool.GetCmdBufSingleTime(Queue::graphics);
			std::vector<ImageInfo*> genMipImages{};
			for (auto& img : transferCommand.images) {
				if (img->descriptorImageInfo.imageLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
					genMipImages.push_back(img);
				}
			}
			if (genMipImages.size() > 1) {
				Image::GenerateMipMapsForMultipleImagesTransferQueue(graphicsCmdBuf, genMipImages);
			}
			else if (genMipImages.size() == 1) {
				Image::GenerateMipmaps(graphicsCmdBuf, genMipImages[0], Queue::transfer);
			}

			for (auto& barrier : transferCommand.pipeBarriers) {
				barrier.Submit(graphicsCmdBuf);
			}
			EWE_VK(vkEndCommandBuffer, graphicsCmdBuf);

			VkSubmitInfo graphicsSubmitInfo{};
			graphicsSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			graphicsSubmitInfo.pNext = nullptr;
			graphicsSubmitInfo.commandBufferCount = 1;
			graphicsSubmitInfo.pCommandBuffers = &graphicsCmdBuf.cmdBuf;

			graphicsSubmitInfo.waitSemaphoreCount = 1;
			transferSemaphore->BeginWaiting();
			graphicsSubmitInfo.pWaitSemaphores = &transferSemaphore->vkSemaphore;

			Semaphore* graphicsSemaphore = qSyncPool.GetSemaphoreForSignaling();
			graphicsSubmitInfo.signalSemaphoreCount = 1;
			graphicsSubmitInfo.pSignalSemaphores = &graphicsSemaphore->vkSemaphore;

			VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
			graphicsSubmitInfo.pWaitDstStageMask = &waitStage;

			Fence& graphicsFence = qSyncPool.GetFence();
			VK::Object->queueMutex[Queue::graphics].lock();
			EWE_VK(vkQueueSubmit, VK::Object->queues[Queue::graphics], 1, &graphicsSubmitInfo, graphicsFence.vkFence);
			renderSyncData.AddWaitSemaphore(graphicsSemaphore, waitStage);
			VK::Object->queueMutex[Queue::graphics].unlock();

			graphicsFence.submitted = true;
#if DEBUGGING_FENCES
			graphicsFence.fence.log.push_back("setting graphics fence to submitted");
#endif


			while (!transferFence.CheckReturn(0)) {
				std::this_thread::sleep_for(std::chrono::nanoseconds(1));
			}
			for (auto& sb : transferCommand.stagingBuffers) {
				sb->Free();
				Deconstruct(sb);
			}
			for (auto& cmd : transferCommand.commands) {
				cmd->Reset();
			}
			transferFence.inUse = false;
			while (!graphicsFence.CheckReturn(0)) {
				std::this_thread::sleep_for(std::chrono::nanoseconds(1));
			}
			transferSemaphore->FinishWaiting();
			graphicsCmdBuf.Reset();
			for (auto& image : transferCommand.images) {
				image->descriptorImageInfo.imageLayout = image->destinationImageLayout;
			}
			graphicsFence.inUse = false;
			graphicsFence.submitted = false;
		}

	}

	void SyncHub::SubmitGraphics(VkSubmitInfo& submitInfo, uint32_t* imageIndex) {

		if (imagesInFlight[*imageIndex] != VK_NULL_HANDLE) {
			EWE_VK(vkWaitForFences, VK::Object->vkDevice, 1, &imagesInFlight[*imageIndex], VK_TRUE, UINT64_MAX);
		}
		imagesInFlight[*imageIndex] = renderSyncData.inFlight[VK::Object->frameIndex];

		renderSyncData.SetWaitData(submitInfo);

		std::vector<VkSemaphore> signalSemaphores = renderSyncData.GetSignalData();
		submitInfo.pSignalSemaphores = signalSemaphores.data();
		submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());

		EWE_VK(vkResetFences, VK::Object->vkDevice, 1, &renderSyncData.inFlight[VK::Object->frameIndex]);

		VK::Object->queueMutex[Queue::graphics].lock();
		EWE_VK(vkQueueSubmit, VK::Object->queues[Queue::graphics], 1, &submitInfo, renderSyncData.inFlight[VK::Object->frameIndex]);
		VK::Object->queueMutex[Queue::graphics].unlock();
	}

	VkResult SyncHub::PresentKHR(VkPresentInfoKHR& presentInfo) {

		VK::Object->totalFrameCount++;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &renderSyncData.renderFinishedSemaphore[VK::Object->frameIndex];
		std::unique_lock<std::mutex> queueLock{VK::Object->queueMutex[Queue::graphics]};
		return vkQueuePresentKHR(VK::Object->queues[Queue::present], &presentInfo);
	}
}