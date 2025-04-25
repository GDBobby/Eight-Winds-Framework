#include "EWEngine/Systems/SyncHub.h"
#include "EWEngine/Graphics/Texture/ImageFunctions.h"

#include <future>
#include <cassert>

namespace EWE {
	SyncHub* SyncHub::syncHubSingleton{ nullptr };

	SyncHub::SyncHub() :
		qSyncPool{ 32 }, 
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
		syncHubSingleton = Construct<SyncHub>({});

#if ONE_SUBMISSION_THREAD_PER_QUEUE
		syncHubSingleton->transferSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		syncHubSingleton->transferSubmitInfo.pNext = nullptr;
#endif

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

#if ONE_SUBMISSION_THREAD_PER_QUEUE
	void SyncHub::RunGraphicsCallbacks() {
		qSyncPool.CheckFencesForCallbacks();
#if DEBUG_NAMING
		DebugNaming::QueueBegin(VK::Object->queues[Queue::graphics], 1.f, 0.f, 0.f, "graphics callbacks");
#endif
		qSyncPool.graphicsAsyncMut.lock();
		if (qSyncPool.graphicsSTCGroup.size() == 0) {
			qSyncPool.graphicsAsyncMut.unlock();
			return;
		}

		std::vector<Semaphore*> semaphores{};
		std::vector<CommandBuffer*> submittedAsyncBuffers{};
		std::vector<ImageInfo*> imageInfos{};
		for (int i = 0; i < qSyncPool.graphicsSTCGroup.size(); i++) {
			submittedAsyncBuffers.push_back(qSyncPool.graphicsSTCGroup[i].cmdBuf);
			if (qSyncPool.graphicsSTCGroup[i].semaphores.size() > 0) {
				semaphores.insert(semaphores.end(), qSyncPool.graphicsSTCGroup[i].semaphores.begin(), qSyncPool.graphicsSTCGroup[i].semaphores.end());
				imageInfos.insert(imageInfos.end(), qSyncPool.graphicsSTCGroup[i].imageInfos.begin(), qSyncPool.graphicsSTCGroup[i].imageInfos.end());
			}
		}
		qSyncPool.graphicsSTCGroup.clear();
		qSyncPool.graphicsAsyncMut.unlock();

		std::vector<VkSemaphore> waitSems{};
		std::vector<VkPipelineStageFlags> waitStages{};
		for (int i = 0; i < semaphores.size(); i++) {
			waitSems.push_back(semaphores[i]->vkSemaphore);
			waitStages.push_back(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
		}

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.commandBufferCount = submittedAsyncBuffers.size();
		assert(submitInfo.commandBufferCount != 0 && "had graphics callbacks but don't have any command buffers");

		std::vector<VkCommandBuffer> cmds{};
		cmds.reserve(submittedAsyncBuffers.size());
		for (auto& command : submittedAsyncBuffers) {
#if COMMAND_BUFFER_TRACING
			cmds.push_back(command->cmdBuf);
#else
			cmds.push_back(*command);
#endif
		}
		submitInfo.pCommandBuffers = cmds.data();

		submitInfo.waitSemaphoreCount = waitSems.size();
		submitInfo.pWaitSemaphores = waitSems.data();

		Semaphore* signalSemaphore = qSyncPool.GetSemaphoreForSignaling();

		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &signalSemaphore->vkSemaphore;
		submitInfo.pWaitDstStageMask = waitStages.data();
		
		GraphicsFence& graphicsFence = qSyncPool.GetGraphicsFence();
		graphicsFence.fence.waitSemaphores = std::move(semaphores);
		graphicsFence.imageInfos = std::move(imageInfos);
		EWE_VK(vkQueueSubmit, VK::Object->queues[Queue::graphics], 1, &submitInfo, graphicsFence.fence.vkFence);

		/*
		* this is bad, but currently I'm just going to use VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT regardless
		* ideally, i could take the data from the object being uploaded
		** for example, if an image is only used in the fragment stage i could use VK_PIPELINE_STAGE_FRAGMENT_BIT,
		** or for a vertex/index/instancing buffer I could use VK_PIPELINE_STAGE_VERTEX_BIT,
		** or for a barrier I could use VK_PIPELINE_STAGE_TRANSFER_BIT
		* but I believe this would require a major overhaul (for the major overhaul I haven't even gotten working yet)
		*/
		renderSyncData.AddWaitSemaphore(signalSemaphore, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
		graphicsFence.commands = submittedAsyncBuffers;
#if DEBUGGING_FENCES
		graphicsFence.fence.log.push_back("setting graphics fence to submitted");
#endif
		graphicsFence.fence.submitted = true;
#if DEBUG_NAMING
		DebugNaming::QueueEnd(VK::Object->queues[Queue::graphics]);
#endif
	}
#else
	void SyncHub::RunGraphicsCallbacks() {
		qSyncPool.CheckFencesForCallbacks();
	}
#endif
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


#if ONE_SUBMISSION_THREAD_PER_QUEUE
	void SyncHub::EndSingleTimeCommandTransfer() {

		/*
		* i suspect its possible for transfer commands to hang without being submitted
		*havent had the issue yet
		* potential solution is to have a dedicated thread just run submittransferbuffers on a loop
		* second potential solution is to have the RunGraphicsCallback function check for available commands, then spawn a thread that has a singular run thru of SubmitTransferBuffers every time commands exist, and SubmitTransferBuffers isn't currently active.

		* example of hang, SubmitTransferBuffers locks the TCM mutex with ::Empty
		* this function attempts to lock the TCM mutex with ::FinalizeCommand
		* SubmitTransferBuffer attempts to exit, but lags a little bit
		* this function completes ::FinalizeCommand and moves to try_lock, and fails to acquire lock, exits
		* SubmitTransferBuffers exits, and unlocks the transferSubmissionMut, with 1 command submitted to TCM

		* Might be exceptionally rare for this function to attempt the transferSubmissionMut lock before SubmitTransferBuffer
		* I'll worry about it when it becomes an issue, or I'll just fix it at some point later idk
		*/
		TransferCommandManager::FinalizeCommand();
		if(transferSubmissionMut.try_lock()){
				SubmitTransferBuffers();
				transferSubmissionMut.unlock();
		}
	}
#else
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
#endif
		
#if ONE_SUBMISSION_THREAD_PER_QUEUE
	void SyncHub::SubmitTransferBuffers() {
		assert(!TransferCommandManager::Empty());
		while (!TransferCommandManager::Empty()) {

			TransferFence& transferFence = qSyncPool.GetTransferFence();
			transferFence.callbacks = TransferCommandManager::PrepareSubmit();

			transferSubmitInfo.commandBufferCount = static_cast<uint32_t>(transferFence.callbacks.commands.size());
			std::vector<VkCommandBuffer> cmds{};
			cmds.reserve(transferFence.callbacks.commands.size());

			for (auto& command : transferFence.callbacks.commands) {
#if COMMAND_BUFFER_TRACING
				cmds.push_back(command->cmdBuf);
#else
				cmds.push_back(*command);
#endif
			}
			transferSubmitInfo.pCommandBuffers = cmds.data();

			std::vector<VkSemaphore> sigSems{};
			if ((transferFence.callbacks.images.size() > 0) || (transferFence.callbacks.pipeBarriers.size() > 0)) {
				transferFence.signalSemaphoreForGraphics = qSyncPool.GetSemaphoreForSignaling();
				PipelineBarrier::SimplifyVector(transferFence.callbacks.pipeBarriers);
				transferFence.callbacks.semaphore = transferFence.signalSemaphoreForGraphics;
				transferSubmitInfo.signalSemaphoreCount = 1;
				transferSubmitInfo.pSignalSemaphores = &transferFence.signalSemaphoreForGraphics->vkSemaphore;
			}
			else {
				transferSubmitInfo.pSignalSemaphores = nullptr;
				transferSubmitInfo.signalSemaphoreCount = 0;
			}
			transferSubmitInfo.pWaitSemaphores = nullptr;
			transferSubmitInfo.waitSemaphoreCount = 0;

			VK::Object->queueMutex[Queue::transfer].lock();
			EWE_VK(vkQueueSubmit, VK::Object->queues[Queue::transfer], 1, &transferSubmitInfo, transferFence.fence.vkFence);
			VK::Object->queueMutex[Queue::transfer].unlock();

#if DEBUGGING_FENCES
			transferFence.fence.log.push_back("setting transfer fence to submitted");
#endif
			transferFence.fence.submitted = true;
		}
	}
#endif

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

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &renderSyncData.renderFinishedSemaphore[VK::Object->frameIndex];
		std::unique_lock<std::mutex> queueLock{VK::Object->queueMutex[Queue::graphics]};
		return vkQueuePresentKHR(VK::Object->queues[Queue::present], &presentInfo);
	}
}