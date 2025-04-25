#include "EWEngine/Data/CommandCallbacks.h"

namespace EWE {



#if SEMAPHORE_TRACKING
    void Semaphore::FinishWaiting(std::source_location srcLoc) {
        assert(waiting && "finished waiting when not waiting");
        waiting = false;
        signaling = false;
        tracking.emplace_back(Tracking::State::FinishWaiting, srcLoc);
    }
    void Semaphore::BeginWaiting(std::source_location srcLoc) {
        assert(!waiting && "attempting to begin wait while waiting");
        waiting = true;
        tracking.emplace_back(Tracking::State::BeginWaiting, srcLoc);
    }
    void Semaphore::BeginSignaling(std::source_location srcLoc) {

        assert(!signaling && "attempting to signal while signaled");
        signaling = true;
        tracking.emplace_back(Tracking::State::BeginSignaling, srcLoc);
    }
#else   
    void Semaphore::FinishWaiting() {
#if EWE_DEBUG
        assert(waiting == true && "finished waiting when not waiting");
#endif
        waiting = false;
        signaling = false; //im not sure if this is good or not. currently its a bit rough to keep track of the graphics single time signaling
    }
    void Semaphore::BeginWaiting() {
#if EWE_DEBUG
        assert(waiting == false && "attempting to begin wait while waiting");
#endif
        waiting = true;
    }
    void Semaphore::BeginSignaling() {
#if EWE_DEBUG
        assert(signaling == false && "attempting to signal while signaled");
#endif
        signaling = true;
    }
#endif

    TransferCommand::TransferCommand(TransferCommand& copySource) : //copy constructor
        commands{ std::move(copySource.commands) },
        stagingBuffers{ std::move(copySource.stagingBuffers) },
        pipeBarriers{ std::move(copySource.pipeBarriers) },
        images{ std::move(copySource.images) },
        semaphore{ copySource.semaphore }
    {
        printf("TransferCommand:: copy constructor\n");

        copySource.semaphore = nullptr;
    }
    TransferCommand& TransferCommand::operator=(TransferCommand& copySource) { //copy assignment
        commands = std::move(copySource.commands);
        stagingBuffers = std::move(copySource.stagingBuffers);
        pipeBarriers = std::move(copySource.pipeBarriers);
        images = std::move(copySource.images);
        semaphore = copySource.semaphore;
        copySource.semaphore = nullptr;
        printf("TransferCommand:: copy constructor\n");

        return *this;
    }

    //TransferCommand& TransferCommand::operator+=(TransferCommand& copySource) {
    //    if (copySource.commands.size() > 0) {
    //        commands.insert(commands.end(), copySource.commands.begin(), copySource.commands.end());
    //        copySource.commands.clear();
    //    }
    //    if (copySource.stagingBuffers.size() > 0) {
    //        stagingBuffers.insert(stagingBuffers.end(), copySource.stagingBuffers.begin(), copySource.stagingBuffers.end());
    //        copySource.stagingBuffers.clear();
    //    }
    //    if (copySource.pipeBarriers.size() > 0) {
    //        pipeBarriers.insert(pipeBarriers.end(), copySource.pipeBarriers.begin(), copySource.pipeBarriers.end());
    //        copySource.pipeBarriers.clear();
    //    }
    //    if (copySource.images.size() > 0) {
    //        images.insert(images.end(), copySource.images.begin(), copySource.images.end());
    //        copySource.images.clear();
    //    }

    //    Semaphore = copySource.Semaphore;
    //    copySource.Semaphore = nullptr;
    //}

    TransferCommand::TransferCommand(TransferCommand&& moveSource) noexcept ://move constructor
        commands{ std::move(moveSource.commands) },
        stagingBuffers{ std::move(moveSource.stagingBuffers) },
        pipeBarriers{ std::move(moveSource.pipeBarriers) },
        images{ std::move(moveSource.images) },
        semaphore{ moveSource.semaphore }

    {

        printf("TransferCommand:: move constructor\n");

        moveSource.semaphore = nullptr;
    }

    TransferCommand& TransferCommand::operator=(TransferCommand&& moveSource) noexcept { //move assignment
        commands = std::move(moveSource.commands);
        stagingBuffers = std::move(moveSource.stagingBuffers);
        pipeBarriers = std::move(moveSource.pipeBarriers);
        images = std::move(moveSource.images);
        semaphore = moveSource.semaphore;
        moveSource.semaphore = nullptr;

        printf("TransferCommand:: move assignment\n");

        return *this;
    }

}//namespace EWE