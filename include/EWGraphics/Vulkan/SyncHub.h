#pragma once

#include "EWGraphics/Vulkan/QueueSyncPool.h"
#include "EWGraphics/Data/EWE_Memory.h"
#include "EWGraphics/Vulkan/TransferCommandManager.h"

#include <mutex>
#include <condition_variable>
#include <vector>

#include <array>


namespace EWE {


	class SyncHub {
	private:
		friend class EWEDevice;

		static SyncHub* syncHubSingleton;

		QueueSyncPool qSyncPool;


#if ONE_SUBMISSION_THREAD_PER_QUEUE
		VkSubmitInfo transferSubmitInfo{};
		std::mutex transferSubmissionMut{};
#endif

		RenderSyncData renderSyncData;

		std::vector<VkFence> imagesInFlight{};


		bool transferring = false;
	public:		
		static SyncHub* GetSyncHubInstance() {
			return syncHubSingleton;
			
		}
		SyncHub();
#if EWE_DEBUG
		~SyncHub();
#endif

		//only class this from EWEDevice
		static void Initialize();
		void SetImageCount(uint32_t imageCount) {
			imagesInFlight.resize(imageCount, VK_NULL_HANDLE);
		}
		void Destroy();

		VkFence* GetFlightFence() {
#if EWE_DEBUG
			if(renderSyncData.inFlight[VK::Object->frameIndex] == VK_NULL_HANDLE){
				printf("invalid in flight fence\n");
			}
#endif
			return &renderSyncData.inFlight[VK::Object->frameIndex];
		}
		VkSemaphore GetImageAvailableSemaphore() {
			return renderSyncData.imageAvailableSemaphore[VK::Object->frameIndex];
		}

		void SubmitGraphics(VkSubmitInfo& submitInfo, uint32_t* imageIndex);
		VkResult PresentKHR(VkPresentInfoKHR& presentInfo);

		//this needs to be called from the graphics thread
		//these have a fence with themselves
		void EndSingleTimeCommandGraphics(GraphicsCommand& graphicsCommand);

#if ONE_SUBMISSION_THREAD_PER_QUEUE
		void EndSingleTimeCommandTransfer();
#else
		void EndSingleTimeCommandTransfer(TransferCommand& transferCommand);
#endif

		CommandBuffer& BeginSingleTimeCommand();
		CommandBuffer& BeginSingleTimeCommandGraphics();
		CommandBuffer& BeginSingleTimeCommandTransfer();

		void WaitOnGraphicsFence() {
			EWE_VK(vkWaitForFences, VK::Object->vkDevice, 1, &renderSyncData.inFlight[VK::Object->frameIndex], VK_TRUE, UINT64_MAX);
		}

		void RunGraphicsCallbacks();
#if ONE_SUBMISSION_THREAD_PER_QUEUE
		bool CheckFencesForUsage() {
			return qSyncPool.CheckFencesForUsage();
		}
#endif

#if ONE_SUBMISSION_THREAD_PER_QUEUE
		void SubmitTransferBuffers();
#endif
	private:

		void CreateBuffers();
		bool transferSubmissionThreadActive = false;
	};
}