#pragma once

#include "Device.hpp"

//#if !defined (_WIN32)
#include "EWEngine/Graphics/VulkanHeader.h"
//#endif

// std lib headers
#include <memory>
#include <string>
#include <vector>

#include <format>
#include <iostream>
//#include <fstream>

namespace EWE {

class EWESwapChain {
    public:

        EWESwapChain(VkExtent2D windowExtent, bool fullscreen);
        EWESwapChain(VkExtent2D windowExtent, bool fullscreen, std::shared_ptr<EWESwapChain> previous);
        ~EWESwapChain();

        EWESwapChain(const EWESwapChain &) = delete;
        EWESwapChain &operator=(const EWESwapChain &) = delete;
        /*
        VkFramebuffer getFrameBuffer(int index) { return swapChainFramebuffers[index]; }
        std::vector<VkFramebuffer> getFrameBuffers() { return swapChainFramebuffers; }
        VkRenderPass getRenderPass() { return renderPass; }
        */
        VkImageView GetImageView(int index) { return swapChainImageViews[index]; }
        size_t ImageCount() { return swapChainImages.size(); }
        VkFormat GetSwapChainImageFormat() { return swapChainImageFormat; }
        VkExtent2D GetSwapChainExtent() { return swapChainExtent; }
        uint32_t Width() { return swapChainExtent.width; }
        uint32_t Height() { return swapChainExtent.height; }

        float ExtentAspectRatio() {
            return static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height);
        }
        VkExtent2D GetExtent() { return swapChainExtent; }
        VkFormat FindDepthFormat();

        //return true if recreating swap chain is necessary
        bool AcquireNextImage(uint32_t *imageIndex);
        VkResult SubmitCommandBuffers(uint32_t *imageIndex);

        bool CompareSwapFormats(const EWESwapChain& swapChain) const {
            return swapChain.swapChainDepthFormat == swapChainDepthFormat && swapChain.swapChainImageFormat == swapChainImageFormat;
        }

        //VkPipelineRenderingCreateInfo const& pipeRenderInfo
        VkPipelineRenderingCreateInfo* GetPipelineInfo() {
            return &pipeline_rendering_create_info;
        }
        VkImage GetImage(uint8_t imageIndex) {
            return swapChainImages[imageIndex];
        }
        void BeginRender(uint8_t imageIndex);

        void ChangeClearValues(float r, float g, float b, float a) {
            for(auto& dynStr : dynamicStructs){
                dynStr.color_attachment_info.clearValue.color.float32[0] = r;
                dynStr.color_attachment_info.clearValue.color.float32[1] = g;
                dynStr.color_attachment_info.clearValue.color.float32[2] = b;
                dynStr.color_attachment_info.clearValue.color.float32[3] = a;
            }
        }

 private:
    void Init(bool fullScreen);
    void CreateSwapChain();
    void CreateImageViews();
    void CreateDepthResources();
    void InitDynamicStruct();
    //void createFramebuffers();
    //void createSyncObjects();
    //bool acquireFullscreen(); FULL SCREEN SHIT
    //bool releaseFullscreen();

    // Helper functions
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

    VkFormat swapChainImageFormat{};
    VkFormat swapChainDepthFormat{};
    VkExtent2D swapChainExtent{};

    //std::vector<VkFramebuffer> swapChainFramebuffers;
    //VkPipelineRenderingCreateInfo const& pipeRenderInfo;

    struct DynamicStructs {
        VkRenderingInfo render_info{};
        VkRenderingAttachmentInfo color_attachment_info{};
        VkRenderingAttachmentInfo depth_attachment_info{};

        DynamicStructs(VkImageView swapImageView, VkImageView depthImageView, uint32_t width, uint32_t height);
    };
    std::vector<DynamicStructs> dynamicStructs{};

    std::vector<VkImage> depthImages{};
#if USING_VMA
    std::vector<VmaAllocation> depthImageMemorys{};
#else
    std::vector<VkDeviceMemory> depthImageMemorys{};
#endif
    std::vector<VkImageView> depthImageViews{};
    std::vector<VkImage> swapChainImages{};
    std::vector<VkImageView> swapChainImageViews{};

    VkExtent2D windowExtent{};

    VkSwapchainKHR swapChain{};
    std::shared_ptr<EWESwapChain> oldSwapChain{};


    SyncHub* syncHub;


    //VkSurfaceFullScreenExclusiveWin32InfoEXT surfaceFullScreenExclusiveWin32InfoEXT{};
    //VkSurfaceFullScreenExclusiveInfoEXT surfaceFullScreenExclusiveInfoEXT{};


    VkPipelineRenderingCreateInfo pipeline_rendering_create_info{};
};

}  // namespace EWE
