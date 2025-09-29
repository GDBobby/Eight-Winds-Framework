#pragma once

#include "EWGraphics/Data/KeyValueContainer.h"

#include "EWGraphics/Vulkan/VulkanHeader.h"
#include "EWGraphics/Vulkan/PipelineBarrier.h"
#include "EWGraphics/Vulkan/CommandCallbacks.h"

#include <cassert>
#include <thread>
#include <mutex>
#include <array>
#include <vector>


/*
* std::this_thread::sleep_for(std::chrono::microseconds(1));
    this is a small amount of time, 
    and most likely 
    it'll allow the OS to take over the thread and do other operations on it, 
    then return control of this thead to our program.
    It's not 100% for the OS to take over, but if it does, the thread will potentially sleep for longer (up to multiple milliseconds)

    it is possible to spin the thread until a certain amount of time has passed
    BUT if immediate control of a new fence/semaphore is required,
    it's recommended throw an error if the requested data is not available, and increase pool size as needed
*/

namespace EWE{
    struct Fence {
        VkFence vkFence{ VK_NULL_HANDLE };
        bool inUse{ false };
        bool submitted{ false };
        std::vector<Semaphore*> waitSemaphores{};
#if DEBUGGING_FENCES
        std::vector<std::string> log{};
#endif

        //its up to the calling function to unlock the mutex
        bool CheckReturn(uint64_t time);
    };

    struct GraphicsFence {
        Fence fence{};
        std::mutex mut{};
        GraphicsCommand gCommand{};
        Semaphore* signalSemaphore{ nullptr };
        void CheckReturn(uint64_t time);
    };

    //not thread safe

    //each semaphore and fence could track which queue it's being used in, but I don't think that's the best approach

    struct WaitData {
        std::vector<Semaphore*> semaphores{};
        std::vector<VkPipelineStageFlags> waitDstMask{};

        //for submission, the lifetime needs to be controlled here
        //theres probably a better way to control lifetime but i cant think of it right now
        //main concern is repeatedly reallocating memory, which this might avoid depending on how the vector is handled behind the scenes
        //an alternative solution is using an array, with a max size of say 16, and using a counter to keep track of the active count, rather than vectors
        std::vector<VkSemaphore> semaphoreData{}; 
    };


    struct RenderSyncData {
    private:
        std::mutex waitMutex{};
        std::mutex signalMutex{};
        WaitData previousWait[MAX_FRAMES_IN_FLIGHT]{};
        std::vector<Semaphore*> previousSignals[MAX_FRAMES_IN_FLIGHT]{};
    public:
        VkFence inFlight[MAX_FRAMES_IN_FLIGHT] = { VK_NULL_HANDLE, VK_NULL_HANDLE };
        VkSemaphore imageAvailableSemaphore[MAX_FRAMES_IN_FLIGHT] = { VK_NULL_HANDLE, VK_NULL_HANDLE };
        VkSemaphore renderFinishedSemaphore[MAX_FRAMES_IN_FLIGHT] = { VK_NULL_HANDLE, VK_NULL_HANDLE };
        WaitData waitData{};
        std::vector<Semaphore*> signalSemaphores{};

        RenderSyncData();
        ~RenderSyncData();
        void AddWaitSemaphore(Semaphore* semaphore, VkPipelineStageFlags waitDstStageMask);
        void AddSignalSemaphore(Semaphore* semaphore);
        void SetWaitData(VkSubmitInfo& submitInfo);
        std::vector<VkSemaphore> GetSignalData();
    };

    class QueueSyncPool{
    private:
        const uint16_t size;

        std::vector<Semaphore> semaphores;
        std::mutex semAcqMut{};


        KeyValueContainer<std::thread::id, ThreadedSingleTimeCommands> threadedSTCs;
        static thread_local ThreadedSingleTimeCommands* threadSTC;
        //std::mutex threadedSTCMutex{};

        std::vector<Fence> fences;
        std::mutex fenceAcqMut{};
        std::vector<GraphicsFence> mainThreadGraphicsFences;
        VkCommandPool mainThreadSTCGraphicsPool{ VK_NULL_HANDLE };
        std::vector<CommandBuffer> mainThreadGraphicsCmdBufs;

    public:
        QueueSyncPool(uint16_t size);

        ~QueueSyncPool();
        Semaphore& GetSemaphore(VkSemaphore semaphore);


        void SemaphoreBeginSignaling(VkSemaphore semaphore) {
            GetSemaphore(semaphore).BeginSignaling();
        }
        void SemaphoreBeginWaiting(VkSemaphore semaphore) {
            GetSemaphore(semaphore).BeginWaiting();
        }
        void SemaphoreFinishedWaiting(VkSemaphore semaphore) {
            GetSemaphore(semaphore).FinishWaiting();
        }
        
        Semaphore* GetSemaphoreForSignaling();

        CommandBuffer& GetCmdBufSingleTime(Queue::Enum queue);
        bool CheckFencesForUsage();
        void CheckFencesForCallbacks();
        Fence& GetFence();
        GraphicsFence& GetMainThreadGraphicsFence();
    };
} //namespace EWE