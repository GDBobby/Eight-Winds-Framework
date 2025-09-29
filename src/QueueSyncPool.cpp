#include "EWGraphics/Vulkan/QueueSyncPool.h"

#include "EWGraphics/Data/EWE_Memory.h"
#include "EWGraphics/Data/ThreadPool.h"
#include "EWGraphics/Texture/ImageFunctions.h"

#include <sstream>

namespace EWE {

    bool Fence::CheckReturn(uint64_t time) {
        if (!submitted) {
#if DEBUGGING_FENCES
            log.push_back("checked return, not submitted");
#endif
            return false;
        }
#if DEBUGGING_FENCES
        log.push_back("checked return, submitted");
#endif

#if EWE_DEBUG
        if(vkFence == VK_NULL_HANDLE){
            printf("invalid fence?\n");
        }
#endif
        VkResult ret = vkWaitForFences(VK::Object->vkDevice, 1, &vkFence, true, time);
        if (ret == VK_SUCCESS) {
            EWE_VK(vkResetFences, VK::Object->vkDevice, 1, &vkFence);
            //its up to the calling function to unlock the mutex
            for (auto& waitSem : waitSemaphores) {
                waitSem->FinishWaiting();
            }
            waitSemaphores.clear();
            //makes more sense to clear the submitted flag here, rather than on acquire
            submitted = false;
            return true;
        }
        else if (ret == VK_TIMEOUT) {
            //its up to the calling function to unlock the mutex
            return false;
        }
        else {
            //its up to the calling function to unlock the mutex
            EWE_VK_RESULT(ret);
            return false; //error silencing, this should not be reached
        }
    }
    void GraphicsFence::CheckReturn(uint64_t time) {
        if (fence.CheckReturn(time)) {

#if DEBUGGING_FENCES
            fence.log.push_back("checked return, continuing in graphics fence");
#endif
            assert(gCommand.command != nullptr);
            gCommand.command->Reset();
            gCommand.command = nullptr;

            if (gCommand.stagingBuffer != nullptr) {
                gCommand.stagingBuffer->Free();
                Deconstruct(gCommand.stagingBuffer);
                gCommand.stagingBuffer = nullptr;
            }
            if (gCommand.imageInfo != nullptr) {
                //assert(gCommand.imageInfo->descriptorImageInfo.imageLayout != gCommand.imageInfo->destinationImageLayout); //i dont remember why i put this in
                gCommand.imageInfo->descriptorImageInfo.imageLayout = gCommand.imageInfo->destinationImageLayout;
                gCommand.imageInfo = nullptr;
            }
#if DEBUGGING_FENCES
            fence.log.push_back("allowing graphics fence to be reobtained");
#endif
            //signalSemaphore->FinishSignaling();
            fence.inUse = false;
        }
    }


    RenderSyncData::RenderSyncData() {
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.pNext = nullptr;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VkSemaphoreCreateInfo semInfo{};
        semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semInfo.pNext = nullptr;
        for (uint8_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            EWE_VK(vkCreateFence, VK::Object->vkDevice, &fenceInfo, nullptr, &inFlight[i]);
            EWE_VK(vkCreateSemaphore, VK::Object->vkDevice, &semInfo, nullptr, &imageAvailableSemaphore[i]);
            EWE_VK(vkCreateSemaphore, VK::Object->vkDevice, &semInfo, nullptr, &renderFinishedSemaphore[i]);
#if DEBUG_NAMING
            std::string objName{};
            objName = "in flight " + std::to_string(i);
            DebugNaming::SetObjectName(inFlight[i], VK_OBJECT_TYPE_FENCE, objName.c_str());
            objName = "image available " + std::to_string(i);
            DebugNaming::SetObjectName(imageAvailableSemaphore[i], VK_OBJECT_TYPE_SEMAPHORE, objName.c_str());
            objName = "render finished " + std::to_string(i);
            DebugNaming::SetObjectName(renderFinishedSemaphore[i], VK_OBJECT_TYPE_SEMAPHORE, objName.c_str());
#endif
        }
    }
    RenderSyncData::~RenderSyncData() {
        for (uint8_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            EWE_VK(vkDestroyFence, VK::Object->vkDevice, inFlight[i], nullptr);
            EWE_VK(vkDestroySemaphore, VK::Object->vkDevice, imageAvailableSemaphore[i], nullptr);
            EWE_VK(vkDestroySemaphore, VK::Object->vkDevice, renderFinishedSemaphore[i], nullptr);
        }
    }
    void RenderSyncData::AddWaitSemaphore(Semaphore* semaphore, VkPipelineStageFlags waitDstStageMask) {
        waitMutex.lock();
        semaphore->BeginWaiting();
        waitData.semaphores.push_back(semaphore);
        waitData.waitDstMask.push_back(waitDstStageMask);
        waitMutex.unlock();
    }
    void RenderSyncData::AddSignalSemaphore(Semaphore* semaphore) {
        signalMutex.lock();
        signalSemaphores.push_back(semaphore);
        signalMutex.unlock();
    }
    void RenderSyncData::SetWaitData(VkSubmitInfo& submitInfo) {
        waitMutex.lock();
        for (auto& waitSem : previousWait[VK::Object->frameIndex].semaphores) {
            waitSem->FinishWaiting();
        }
        previousWait[VK::Object->frameIndex].semaphores.clear();
        previousWait[VK::Object->frameIndex].waitDstMask.clear();
        previousWait[VK::Object->frameIndex].semaphoreData.clear();
        previousWait[VK::Object->frameIndex] = waitData; //i think this makes the above clears repetitive, but im not sure
        waitData.semaphores.clear();
        waitData.waitDstMask.clear();
        waitData.semaphoreData.clear();

        assert(previousWait[VK::Object->frameIndex].semaphores.size() == previousWait[VK::Object->frameIndex].waitDstMask.size());

        for (auto& waitSem : previousWait[VK::Object->frameIndex].semaphores) {
            previousWait[VK::Object->frameIndex].semaphoreData.push_back(waitSem->vkSemaphore);
        }
        previousWait[VK::Object->frameIndex].semaphoreData.push_back(imageAvailableSemaphore[VK::Object->frameIndex]);
        previousWait[VK::Object->frameIndex].waitDstMask.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

        submitInfo.pWaitDstStageMask = previousWait[VK::Object->frameIndex].waitDstMask.data();
        submitInfo.waitSemaphoreCount = static_cast<uint32_t>(previousWait[VK::Object->frameIndex].semaphoreData.size());
        submitInfo.pWaitSemaphores = previousWait[VK::Object->frameIndex].semaphoreData.data();

        waitMutex.unlock();
    }
    std::vector<VkSemaphore> RenderSyncData::GetSignalData() {
        signalMutex.lock();
        //for (auto& sigSem : previousSignals[VK::Object->frameIndex]) {
            //sigSem->FinishSignaling();
        //}
        previousSignals[VK::Object->frameIndex].clear();
        previousSignals[VK::Object->frameIndex] = signalSemaphores;
        signalSemaphores.clear();

        std::vector<VkSemaphore> ret{};
        ret.reserve(previousSignals[VK::Object->frameIndex].size() + 1);
        for (auto& sigSem : previousSignals[VK::Object->frameIndex]) {
            ret.push_back(sigSem->vkSemaphore);
        }
        ret.push_back(renderFinishedSemaphore[VK::Object->frameIndex]);
        signalMutex.unlock();
        return ret;
    }


    thread_local ThreadedSingleTimeCommands* QueueSyncPool::threadSTC;

    QueueSyncPool::QueueSyncPool(uint16_t size) :
        size{ size },
        fences{size},
        mainThreadGraphicsFences{size},
        mainThreadGraphicsCmdBufs{ size },
        semaphores{ static_cast<std::size_t>(size * 2)},
        threadedSTCs{
            ThreadPool::GetKVContainerWithThreadIDKeys<ThreadedSingleTimeCommands>()
        }
        //cmdBufs{}
    {
        //assert(size <= 64 && "this isn't optimized very well, don't use big size"); //big size probably also isn't necessary

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.flags = 0;
        fenceInfo.pNext = nullptr;
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        for (uint16_t i = 0; i < size; i++) {
            EWE_VK(vkCreateFence, VK::Object->vkDevice, &fenceInfo, nullptr, &fences[i].vkFence);
            EWE_VK(vkCreateFence, VK::Object->vkDevice, &fenceInfo, nullptr, &mainThreadGraphicsFences[i].fence.vkFence);
        }

        VkSemaphoreCreateInfo semInfo{};
        semInfo.flags = 0;
        semInfo.pNext = nullptr;
        semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        for (uint16_t i = 0; i < size * 2; i++) {
            EWE_VK(vkCreateSemaphore, VK::Object->vkDevice, &semInfo, nullptr, &semaphores[i].vkSemaphore);
#if DEBUG_NAMING
            std::string name = "QueueSyncPool semaphore[" + std::to_string(i) + ']';
            DebugNaming::SetObjectName(semaphores[i].vkSemaphore, VK_OBJECT_TYPE_SEMAPHORE, name.c_str());
#endif
        }
        std::vector<VkCommandBuffer> cmdBufVector{};
        //im assuming the input cmdBuf doesn't matter, and it's overwritten without being read
        //if there's a bug, set the resize default to VK_NULL_HANDLE

        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        cmdBufVector.resize(size); 
        VkCommandBufferAllocateInfo cmdBufAllocInfo{};
        cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufAllocInfo.pNext = nullptr;
        cmdBufAllocInfo.commandBufferCount = static_cast<uint32_t>(cmdBufVector.size());
        cmdBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        poolInfo.queueFamilyIndex = VK::Object->queueIndex[Queue::graphics];
        EWE_VK(vkCreateCommandPool, VK::Object->vkDevice, &poolInfo, nullptr, &mainThreadSTCGraphicsPool);
        cmdBufAllocInfo.commandPool = mainThreadSTCGraphicsPool;

        EWE_VK(vkAllocateCommandBuffers, VK::Object->vkDevice, &cmdBufAllocInfo, cmdBufVector.data());
        std::sort(cmdBufVector.begin(), cmdBufVector.end());
        for (int i = 0; i < size; i++) {
            mainThreadGraphicsCmdBufs[i] = cmdBufVector[i];
        }

        for (auto& stc  : threadedSTCs) {
            auto& buf = stc.value;
            for (uint8_t queue = 0; queue < Queue::_count; queue++) {
                if (VK::Object->queueEnabled[queue]) {
                    poolInfo.queueFamilyIndex = VK::Object->queueIndex[queue];
                    EWE_VK(vkCreateCommandPool, VK::Object->vkDevice, &poolInfo, nullptr, &buf.commandPools[queue]);
#if DEBUG_NAMING
                    std::ostringstream poolName{};
                    poolName << stc.key;
                    poolName << " : " << queue;
                    DebugNaming::SetObjectName(buf.commandPools[queue], VK_OBJECT_TYPE_COMMAND_POOL, "graphics STG cmd pool");
#endif

                    buf.cmdBufs[queue].resize(size);
                    cmdBufAllocInfo.commandPool = buf.commandPools[queue];
                    EWE_VK(vkAllocateCommandBuffers, VK::Object->vkDevice, &cmdBufAllocInfo, cmdBufVector.data());
                    std::sort(cmdBufVector.begin(), cmdBufVector.end());
                    for (int i = 0; i < size; i++) {
                        buf.cmdBufs[queue][i] = cmdBufVector[i];
                    }
                }
            }
        }
    }
    QueueSyncPool::~QueueSyncPool() {
        for (uint16_t i = 0; i < size; i++) {
            EWE_VK(vkDestroyFence, VK::Object->vkDevice, fences[i].vkFence, nullptr);
            EWE_VK(vkDestroyFence, VK::Object->vkDevice, mainThreadGraphicsFences[i].fence.vkFence, nullptr);
            EWE_VK(vkDestroySemaphore, VK::Object->vkDevice, semaphores[i].vkSemaphore, nullptr);
            EWE_VK(vkDestroySemaphore, VK::Object->vkDevice, semaphores[i + size].vkSemaphore, nullptr);
        }

        semaphores.clear();

        std::vector<VkCommandBuffer> rawCmdBufs(size);

        for (auto& stc : threadedSTCs) {
            for (uint8_t queue = 0; queue < Queue::_count; queue++) {
                if (stc.value.commandPools[queue] != VK_NULL_HANDLE) {
                    for (uint16_t i = 0; i < size; i++) {
                        rawCmdBufs[i] = stc.value.cmdBufs[queue][i].cmdBuf;
                    }
                    EWE_VK(vkFreeCommandBuffers, VK::Object->vkDevice, stc.value.commandPools[queue], size, rawCmdBufs.data());
                    stc.value.cmdBufs[queue].clear();
                    EWE_VK(vkDestroyCommandPool, VK::Object->vkDevice, stc.value.commandPools[queue], nullptr);
                }
            }
        }

        for (uint16_t i = 0; i < size; i++) {
            rawCmdBufs[i] = mainThreadGraphicsCmdBufs[i].cmdBuf;
            EWE_VK(vkFreeCommandBuffers, VK::Object->vkDevice, mainThreadSTCGraphicsPool, size, rawCmdBufs.data());
            EWE_VK(vkDestroyCommandPool, VK::Object->vkDevice, mainThreadSTCGraphicsPool, nullptr);
        }
    }
#if 0//SEMAPHORE_TRACKING

#else
    Semaphore& QueueSyncPool::GetSemaphore(VkSemaphore semaphore) {
        for (uint16_t i = 0; i < size * 2; i++) {
            if (semaphores[i].vkSemaphore == semaphore) {
                return semaphores[i];
            }
        }
        assert(false && "failed to find SemaphoreData");
        return semaphores[0]; //DO NOT return this, error silencing
        //only way for this to not return an error is if the return type is changed to pointer and nullptr is returned if not found, or std::conditional which im not a fan of
    }
#endif

    Semaphore* QueueSyncPool::GetSemaphoreForSignaling() {
        
        std::unique_lock<std::mutex> semLock(semAcqMut);
        while (true) {
            for (uint16_t i = 0; i < size * 2; i++) {
                if (semaphores[i].Idle()) {
#if SEMAPHORE_TRACKING
                    printf("need to set up stacktrace here\n");
#endif
                    semaphores[i].BeginSignaling();
                    return &semaphores[i];
                }
            }
            //potentially add a resizing function here
            //assert(false && "no semaphore available, if waiting for a semaphore to become available instead of crashing is acceptable, comment this line");
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
    }

    CommandBuffer& QueueSyncPool::GetCmdBufSingleTime(Queue::Enum queue) {
        if (std::this_thread::get_id() == VK::Object->mainThreadID) {
            assert(queue == Queue::graphics);
            while (true) {

                for (auto& cmdBuf : mainThreadGraphicsCmdBufs) {
                    if (!cmdBuf.inUse) {
                        cmdBuf.BeginSingleTime();
                        return cmdBuf;
                    }
                }
                //potentially add a resizing function here
                //assert(false && "no available command buffer when requested, if waiting for a cmdbuf to become available instead of crashing is acceptable, comment this line");
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        }

        if (threadSTC == nullptr) {
            threadSTC = &threadedSTCs.GetValue(std::this_thread::get_id());
        }


        while (true) {

            for (auto& cmdBuf : threadSTC->cmdBufs[queue]) {
                if (!cmdBuf.inUse) {
                    cmdBuf.BeginSingleTime();
                    return cmdBuf;
                }
            }
            //potentially add a resizing function here
            //assert(false && "no available command buffer when requested, if waiting for a cmdbuf to become available instead of crashing is acceptable, comment this line");
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
    }

    bool QueueSyncPool::CheckFencesForUsage() {
        for (auto& fence : mainThreadGraphicsFences) {
            if (fence.fence.inUse) {
                return true;
            }
        }
        return false;
    }

    void QueueSyncPool::CheckFencesForCallbacks() {
        assert(std::this_thread::get_id() == VK::Object->mainThreadID);

        for (uint16_t i = 0; i < size; i++) {
            if (mainThreadGraphicsFences[i].fence.inUse) {
#if DEBUGGING_FENCES
                graphicsFences[i].fence.log.push_back("beginning graphics fence check return");
#endif
                mainThreadGraphicsFences[i].CheckReturn(0);
            }
        }
    }

    Fence& QueueSyncPool::GetFence() {
        std::unique_lock<std::mutex> transferFenceLock(fenceAcqMut);
        while (true) {
            for (uint16_t i = 0; i < size; i++) {
                if (!fences[i].inUse) {
                    fences[i].inUse = true;
    #if DEBUGGING_FENCES
                    transferFences[i].fence.log.push_back("set transfer fence to in-use");
    #endif
                    return fences[i];
                }
            }
            //potentially add a resizing function here
            //assert(false && "no available fence when requested, if waiting for a fence to become available instead of crashing is acceptable, comment this line");
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
    }
    GraphicsFence& QueueSyncPool::GetMainThreadGraphicsFence() {
        assert(std::this_thread::get_id() == VK::Object->mainThreadID);
        while (true) {
            for (auto& fence : mainThreadGraphicsFences) {
                
                if (!fence.fence.inUse) {
                    fence.fence.inUse = true;
#if DEBUGGING_FENCES
                    fence.fence.log.push_back("set transfer fence to in-use");
#endif
                    return fence;
                }
            }
            //potentially add a resizing function here
            //assert(false && "no available fence when requested, if waiting for a fence to become available instead of crashing is acceptable, comment this line");
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
    }

}//namespace EWE