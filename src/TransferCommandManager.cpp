#include "EWEngine/Graphics/TransferCommandManager.h"


#include <cassert>
#include <mutex>
#include <iterator>

namespace EWE {

#if ONE_SUBMISSION_THREAD_PER_QUEUE
	namespace TransferCommandManager {
		std::mutex callbackMutex{};
		TransferCommand commandCallbacks;


		bool Empty(){
#if EWE_DEBUG
			std::lock_guard<std::mutex> lock(callbackMutex);
			if (commandCallbacks.commands.size() == 0) {
				assert(commandCallbacks.images.size() == 0);
				assert(commandCallbacks.pipeBarriers.size() == 0);
				assert(commandCallbacks.stagingBuffers.size() == 0);
			}
#endif
			return commandCallbacks.commands.size() == 0;
		}

		TransferCommand PrepareSubmit(){
			std::lock_guard<std::mutex> guard{ callbackMutex };
			return commandCallbacks;
		}


		void AddCommand(CommandBuffer& cmdBuf) {
			EWE_VK(vkEndCommandBuffer, cmdBuf);
			callbackMutex.lock();

			commandCallbacks.commands.push_back(&cmdBuf);
		}
		void AddPropertyToCommand(StagingBuffer* stagingBuffer) {
			commandCallbacks.stagingBuffers.push_back(stagingBuffer);
		}
		void AddPropertyToCommand(PipelineBarrier& pipeBarrier) {
			commandCallbacks.pipeBarriers.push_back(std::move(pipeBarrier));
		}
		void AddPropertyToCommand(ImageInfo* imageInfo) {
			commandCallbacks.images.push_back(imageInfo);
		}
		void FinalizeCommand() {
			callbackMutex.unlock();
		}

	} //namespace TransferCommandManager
#endif
} //namespace EWE