#include "EWGraphics/Vulkan/Swapchain.hpp"

#include "EWGraphics/Texture/ImageFunctions.h"

// std
#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>

namespace EWE {

    EWESwapChain::EWESwapChain(VkExtent2D extent, bool fullscreen)
        : windowExtent{ extent }, syncHub{SyncHub::GetSyncHubInstance()} {

        Init(fullscreen);
        //deviceRef.receiveImageInFlightFences(&imagesInFlight);
    }
    EWESwapChain::EWESwapChain(VkExtent2D extent, bool fullscreen, std::shared_ptr<EWESwapChain> previous)
        : windowExtent{ extent }, oldSwapChain{ previous }, syncHub{ SyncHub::GetSyncHubInstance() } {
        Init(fullscreen);
        oldSwapChain.reset();
        //deviceRef.receiveImageInFlightFences(&imagesInFlight);
    }
    void EWESwapChain::Init(bool fullScreen) {
       // logFile.open("log.log");
       // logFile << "initializing swap chain \n";
        //printf("init swap chain \n");

        /*
        //structures before creating swapchain, need it in swapchaincreateinfo.pNext
        //the example also has VkPhysicalDeviceSurfaceInfo2KHR but that may be HDR and unrelated
        surfaceFullScreenExclusiveInfoEXT.sType = VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_INFO_EXT;
        surfaceFullScreenExclusiveInfoEXT.pNext = &surfaceFullScreenExclusiveWin32InfoEXT;
        surfaceFullScreenExclusiveInfoEXT.fullScreenExclusive = VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT;

        surfaceFullScreenExclusiveWin32InfoEXT.sType = VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_WIN32_INFO_EXT;
        surfaceFullScreenExclusiveWin32InfoEXT.pNext = nullptr;
        surfaceFullScreenExclusiveWin32InfoEXT.hmonitor = MonitorFromWindow(MainWindow::GetHWND(), MONITOR_DEFAULTTONEAREST);
        FULL SCREEN SHIT */

        CreateSwapChain();
        CreateImageViews();

        //createRenderPass();
        CreateDepthResources();
        InitDynamicStruct();
        //createFramebuffers();
        //createSyncObjects();

        //acquire fullscreen here
        //acquireFullscreen();

        pipeline_rendering_create_info = VkPipelineRenderingCreateInfo{};
        pipeline_rendering_create_info.pNext = nullptr;
        pipeline_rendering_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        pipeline_rendering_create_info.colorAttachmentCount = 1;
        pipeline_rendering_create_info.pColorAttachmentFormats = &swapChainImageFormat;
        pipeline_rendering_create_info.depthAttachmentFormat = swapChainDepthFormat;
    }

    EWESwapChain::~EWESwapChain() {
        //device.removeImageInFlightFences(&imagesInFlight);
        //logFile.close();
        for (auto imageView : swapChainImageViews) {
            EWE_VK(vkDestroyImageView, VK::Object->vkDevice, imageView, nullptr);
        }
        swapChainImageViews.clear();

        if (swapChain != nullptr) {
            EWE_VK(vkDestroySwapchainKHR, VK::Object->vkDevice, swapChain, nullptr);
            swapChain = nullptr;
        }

        for (int i = 0; i < depthImages.size(); i++) {
            EWE_VK(vkDestroyImageView, VK::Object->vkDevice, depthImageViews[i], nullptr);
#if USING_VMA
            vmaDestroyImage(VK::Object->vmaAllocator, depthImages[i], depthImageMemorys[i]);
#else
            EWE_VK(vkDestroyImage, VK::Object->vkDevice, depthImages[i], nullptr);
            EWE_VK(vkFreeMemory, VK::Object->vkDevice, depthImageMemorys[i], nullptr);
#endif
        }
        /*
        for (auto framebuffer : swapChainFramebuffers) {
            vkDestroyFramebuffer(EWEDevice::GetVkDevice(), framebuffer, nullptr);
        }

        vkDestroyRenderPass(EWEDevice::GetVkDevice(), renderPass, nullptr);
        */
        // cleanup synchronization objects
        //for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        //    vkDestroySemaphore(EWEDevice::GetVkDevice(), renderFinishedSemaphores[i], nullptr);
        //    vkDestroySemaphore(EWEDevice::GetVkDevice(), imageAvailableSemaphores[i], nullptr);
        //    vkDestroyFence(EWEDevice::GetVkDevice(), inFlightFences[i], nullptr);
        //}
    }
    bool EWESwapChain::AcquireNextImage(uint32_t* imageIndex) {
       // printf("pre-wait for ANI inflightfences \n");

        //printf("before waiting for in flight fence\n");
        EWE_VK(vkWaitForFences, 
            VK::Object->vkDevice,
            1,
            syncHub->GetFlightFence(),
            VK_TRUE,
            std::numeric_limits<uint64_t>::max()
        );
        //printf("after waiting for fence in ANI \n");
        //printf("before acquiring image\n");
        VkResult result = vkAcquireNextImageKHR(
            VK::Object->vkDevice,
            swapChain,
            std::numeric_limits<uint64_t>::max(),
            syncHub->GetImageAvailableSemaphore(),
            VK_NULL_HANDLE,
            imageIndex
        );
        //printf("after acquiring image\n");
        if (result == VK_ERROR_OUT_OF_DATE_KHR){// || result == VK_SUBOPTIMAL_KHR) {
            return true;
        }
        EWE_VK_RESULT(result);
        return false;
    }

    VkResult EWESwapChain::SubmitCommandBuffers(uint32_t *imageIndex) {

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext = nullptr;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &VK::Object->GetVKCommandBufferDirect();

        syncHub->SubmitGraphics(submitInfo, imageIndex);

        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapChain;
        presentInfo.pImageIndices = imageIndex;
        auto result = syncHub->PresentKHR(presentInfo);
        return result;
    }
    void EWESwapChain::BeginRender(uint8_t imageIndex) {
        //std::cout << "before vkCmdBeginRendering : " << std::endl;
        EWE_VK(vkCmdBeginRendering, VK::Object->GetFrameBuffer(), &dynamicStructs[imageIndex].render_info); //might need to use the frameIndex from renderer, not sure
        //std::cout << "after vkCmdBeginRendering : " << std::endl;
    }

    void EWESwapChain::CreateSwapChain() {
        //logFile << "creating swap chain \n";
        //printf("create swap chain \n");

        EWEDevice* eweDevice = EWEDevice::GetEWEDevice();
        SwapChainSupportDetails swapChainSupport = eweDevice->GetSwapChainSupport();

        VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

        //if max is 0, there is no limit
        if (swapChainSupport.capabilities.maxImageCount > 0 &&
            imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }
        
        syncHub->SetImageCount(imageCount);


        VkSwapchainCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        //createInfo.pNext = &surfaceFullScreenExclusiveInfoEXT;
        createInfo.surface = VK::Object->surface;

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        uint32_t queueData[] = { static_cast<uint32_t>(VK::Object->queueIndex[Queue::graphics]), static_cast<uint32_t>(VK::Object->queueIndex[Queue::present])};

        /*
        if (queueData[0] != queueData[1]) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
        }
        else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 1;      // Optional
        }
        */
        const bool differentFamilies = (queueData[0] != queueData[1]);
        createInfo.imageSharingMode = (VkSharingMode)differentFamilies; //1 is concurrent, 0 is exclusive
        createInfo.queueFamilyIndexCount = 1 + differentFamilies;

        createInfo.pQueueFamilyIndices = &queueData[0];  //if exclusive, only the first element is read from the array

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        createInfo.oldSwapchain = oldSwapChain == nullptr ? VK_NULL_HANDLE : oldSwapChain->swapChain;

        //logFile << "values initialized, vkcreateswapchain now \n";
        EWE_VK(vkCreateSwapchainKHR, VK::Object->vkDevice, &createInfo, nullptr, &swapChain);
        //logFile << "afterr vkswapchain \n";
        // we only specified a minimum number of images in the swap chain, so the implementation is
        // allowed to create a swap chain with more. That's why we'll first query the final number of
        // images with vkGetSwapchainImagesKHR, then resize the container and finally call it again to
        // retrieve the handles.
        EWE_VK(vkGetSwapchainImagesKHR, VK::Object->vkDevice, swapChain, &imageCount, nullptr);
        assert(imageCount > 0 && "failed to get swap chain images\n");

        swapChainImages.resize(imageCount);
        EWE_VK(vkGetSwapchainImagesKHR, VK::Object->vkDevice, swapChain, &imageCount, swapChainImages.data());
        //logFile << "after vkgetswapchain images \n";

        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
     }

    void EWESwapChain::CreateImageViews() {
        swapChainImageViews.resize(swapChainImages.size());
        for (std::size_t i = 0; i < swapChainImages.size(); i++) {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = swapChainImages[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = swapChainImageFormat;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            EWE_VK(vkCreateImageView, VK::Object->vkDevice, &viewInfo, nullptr, &swapChainImageViews[i]);
        }
    }

    
    void EWESwapChain::InitDynamicStruct() {

        dynamicStructs.reserve(swapChainImages.size());
        for (uint8_t i = 0; i < swapChainImages.size(); i++) {
			dynamicStructs.emplace_back(swapChainImageViews[i], depthImageViews[i], swapChainExtent.width, swapChainExtent.height);
		}
    }
    
    /*
    void EWESwapChain::createFramebuffers() {
        swapChainFramebuffers.resize(imageCount());
        for (size_t i = 0; i < imageCount(); i++) {
            std::array<VkImageView, 2> attachments = {swapChainImageViews[i], depthImageViews[i]};

            VkExtent2D swapChainExtent = getSwapChainExtent();
            VkFramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(
                    EWEDevice::GetVkDevice(),
                    &framebufferInfo,
                    nullptr,
                    &swapChainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }
    */

    void EWESwapChain::CreateDepthResources() {
        VkFormat depthFormat = FindDepthFormat();
        swapChainDepthFormat = depthFormat;
        VkExtent2D swapChainExtent = GetSwapChainExtent();

        const std::size_t imageCount = swapChainImages.size();
        depthImages.resize(imageCount);
        depthImageMemorys.resize(imageCount);
        depthImageViews.resize(imageCount);

        for (int i = 0; i < depthImages.size(); i++) {
            VkImageCreateInfo imageCreateInfo{};
            imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
            imageCreateInfo.extent.width = swapChainExtent.width;
            imageCreateInfo.extent.height = swapChainExtent.height;
            imageCreateInfo.extent.depth = 1;
            imageCreateInfo.mipLevels = 1;
            imageCreateInfo.arrayLayers = 1;
            imageCreateInfo.format = depthFormat;
            imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageCreateInfo.flags = 0;

            Image::CreateImageWithInfo(
                imageCreateInfo,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                depthImages[i],
                depthImageMemorys[i]
            );


            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = depthImages[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = depthFormat;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            EWE_VK(vkCreateImageView, VK::Object->vkDevice, &viewInfo, nullptr, &depthImageViews[i]);
        }
    }
    /*
    bool EWESwapChain::acquireFullscreen() {
        if (vkAcquireFullScreenExclusiveModeEXT(EWEDevice::GetVkDevice(), swapChain) != VK_SUCCESS) {
            printf("failed to acquire full screen \n");
            return false;
        }
        return true;
    }
    bool EWESwapChain::releaseFullscreen() {
        if (vkReleaseFullScreenExclusiveModeEXT(EWEDevice::GetVkDevice(), swapChain) != VK_SUCCESS) {
            printf("failed to release full screen \n");
            return false;
        }
        return true;
    }
    */

    VkSurfaceFormatKHR EWESwapChain::ChooseSwapSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR> &availableFormats) {
        for (const auto &availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }
#if EWE_DEBUG
        printf("did not find a suitable swap surface, potentially assert here\n");
#endif
        return availableFormats[0];
    }

    VkPresentModeKHR EWESwapChain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) {
        for (const auto &availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                std::cout << "Present mode: Mailbox" << std::endl;
                return availablePresentMode;
            }
            
        }
#if GPU_LOGGING
        {
            std::ofstream logFile{ GPU_LOG_FILE, std::ios::app };
            logFile << "could not use present mode: mailbox" << std::endl;
            logFile.close();
        }
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_FIFO_KHR) {
                std::cout << "Present mode: V-Sync" << std::endl;
                std::ofstream logFile{ GPU_LOG_FILE, std::ios::app };
                logFile << "Present mode: V-Sync" << std::endl;
                logFile.close();

                return availablePresentMode;
            }

        }
        {
            std::ofstream logFile{ GPU_LOG_FILE, std::ios::app };
            logFile << "could not use present mode: mailbox OR VSYNC" << std::endl;
            logFile << "available present mode count : " << availablePresentModes.size() << std::endl;
            for (const auto& availablePresentMode : availablePresentModes) {
                logFile << "available present mode : " << availablePresentMode << std::endl;
            }
            logFile.close();
        }
#endif



      // for (const auto &availablePresentMode : availablePresentModes) {
      //   if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
      //     std::cout << "Present mode: Immediate" << std::endl;
      //     return availablePresentMode;
      //   }
      // }

        std::cout << "Present mode: V-Sync" << std::endl;
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D EWESwapChain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } 
        else {
            VkExtent2D actualExtent = windowExtent;
            actualExtent.width = std::max(
                capabilities.minImageExtent.width,
                std::min(capabilities.maxImageExtent.width, actualExtent.width)
            );
            actualExtent.height = std::max(
                capabilities.minImageExtent.height,
                std::min(capabilities.maxImageExtent.height, actualExtent.height)
            );

            return actualExtent;
        }
    }

    VkFormat EWESwapChain::FindDepthFormat() {
        return EWEDevice::GetEWEDevice()->FindSupportedFormat(
            {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }


    EWESwapChain::DynamicStructs::DynamicStructs(VkImageView swapImageView, VkImageView depthImageView, uint32_t width, uint32_t height) {

        color_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        color_attachment_info.pNext = NULL;
        color_attachment_info.imageView = swapImageView;
        color_attachment_info.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        color_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment_info.clearValue = { 0.f, 0.f, 0.f, 1.0f };

        depth_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depth_attachment_info.pNext = NULL;
        depth_attachment_info.imageView = depthImageView;
        depth_attachment_info.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        depth_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment_info.clearValue = { 1.0f, 0 };


        render_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        render_info.pNext = nullptr;
        render_info.renderArea = { 0, 0, width, height };
        render_info.layerCount = 1;
        render_info.colorAttachmentCount = 1;
        render_info.pStencilAttachment = nullptr;
        render_info.pColorAttachments = &color_attachment_info;
        render_info.pDepthAttachment = &depth_attachment_info;
    }

}  // namespace EWE
