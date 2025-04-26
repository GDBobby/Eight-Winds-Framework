#pragma once

#include "EWGraphics/Vulkan/VulkanHeader.h"
#include "EWGraphics/MainWindow.h"
#include "EWGraphics/Vulkan/SyncHub.h"

// std lib headers

#include <string>
#include <vector>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <unordered_map>

#include <list>
#include <utility>



namespace EWE {


    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    class EWEDevice {
        static EWEDevice* eweDevice; //singleton

    public:

        EWEDevice(MainWindow& window);
        ~EWEDevice();

        // Not copyable or movable
        EWEDevice(const EWEDevice&) = delete;
        EWEDevice& operator=(const EWEDevice&) = delete;
        EWEDevice(EWEDevice&&) = delete;
        EWEDevice& operator=(EWEDevice&&) = delete;

        static EWEDevice* GetEWEDevice() {
#if EWE_DEBUG
            assert(eweDevice && "device is nullptr ??");
#endif
            return eweDevice;
        }

        //VkQueue computeQueue() { return queue[Queue::compute]; }
        //uint32_t getComputeIndex() { return computeIndex; }



        std::string deviceName;

        SwapChainSupportDetails GetSwapChainSupport() { return QuerySwapChainSupport(VK::Object->physicalDevice); }

        VkFormat FindSupportedFormat(std::vector<VkFormat> const& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);


    private:
        //QueueData queueData;
        VkDebugUtilsMessengerEXT debugMessenger;
        MainWindow& window;

        SyncHub* syncHub;

        //DEBUGGING_DEVICE_LOST
        const std::vector<const char*> validationLayers;
        const std::vector<const char*> deviceExtensions;
        

        struct OptionalExtension {
            const char* extension;
            bool enabled{ false };
            OptionalExtension(const char* extension) : extension{ extension } {}
        };
        std::unordered_map<std::string, bool> optionalExtensions;
        void CheckOptionalExtensions();

        void CreateInstance();
        void SetupDebugMessenger();
        void CreateSurface();
        void PickPhysicalDevice();
        void CreateLogicalDevice();

#if USING_VMA
        void CreateVmaAllocator();
#endif

        void CreateCommandPools();


        // helper functions
        bool IsDeviceSuitable(VkPhysicalDevice device);
        std::vector<const char*> GetRequiredExtensions(); //glfw
        void AddOptionalExtensions(std::vector<const char*>& extensions);
        bool CheckValidationLayerSupport();
        void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
        void HasGflwRequiredInstanceExtensions();
        bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
        SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);

#if DEBUGGING_DEVICE_LOST
    public:
        void AddCheckpoint(CommandBuffer cmdBuf, const char* name, VKDEBUG::GFX_vk_checkpoint_type type);
        VKDEBUG::DeviceLostDebugStructure deviceLostDebug{};
#endif

    };

}  // namespace EWE