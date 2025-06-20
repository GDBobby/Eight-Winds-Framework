#include "EWGraphics/Vulkan/Device.hpp"

#include "EWGraphics/Texture/Sampler.h" //this is only for construction and deconstruction, do not call Sampler directly from device.cpp

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>


// std headers
#include <cstring>
#include <iostream>

#include <unordered_set>
#include <stack>
#include <set>

#define ENGINE_DIR "../"

//my NVIDIA card is chosen before my AMD card.
//on a machine with an AMD card chosen before the NVIDIA card, NVIDIA_TARGET preprocessor is required for nvidia testing
//if you have two discrete amd gpus, and an nvidia gpu, itll randomly select an amd gpu with amd target
#define AMD_TARGET false
#define NVIDIA_TARGET (false && !AMD_TARGET) //not currently setup to correctly
#define INTEGRATED_TARGET (false && ((!NVIDIA_TARGET) && (!AMD_TARGET)))

namespace EWE {
#if DEBUGGING_DEVICE_LOST
    void EWEDevice::AddCheckpoint(CommandBuffer cmdBuf, const char* name, VKDEBUG::GFX_vk_checkpoint_type type) {
        deviceLostDebug.AddCheckpoint(cmdBuf, name, type);
    }
#endif

    EWEDevice* EWEDevice::eweDevice = nullptr;

    const bool enableValidationLayers = true;// EWE_DEBUG;

    // local callback functions
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData)
    {

        switch (messageSeverity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            std::cout << "validation verbose: " << messageType << ":" << pCallbackData->pMessage << '\n' << std::endl;
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            std::cout << "validation info: " << messageType << ":" << pCallbackData->pMessage << '\n' << std::endl;
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: {
            std::string idName = pCallbackData->pMessageIdName;
            std::cout << "validation warning: " << messageType << ":" << pCallbackData->pMessage << '\n' << std::endl;
            break;
        }
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: {
            std::cout << "validation error: " << messageType << ":" << pCallbackData->pMessage << '\n' << std::endl;

#if GPU_LOGGING
            std::ofstream logFile{ GPU_LOG_FILE, std::ios::app };
            logFile << "current frame index - " << VK::Object->frameIndex << std::endl;
            for (uint8_t i = 0; i < VK::Object->renderCommands.size(); i++) {
                while (VK::Object->renderCommands[i].usageTracking.size() > 0) {
                    for (auto& usage : VK::Object->renderCommands[i].usageTracking.front()) {
                        logFile << "cb" << i << " : " << usage.funcName;
                    }
                    VK::Object->renderCommands[i].usageTracking.pop();
                }
            }
            logFile.close();
#endif

            assert(false && "validation layer error");
            break;
        }
        default:
            printf("validation default: %s \n", pCallbackData->pMessage);
            EWE_UNREACHABLE;
            break;

        }
        //throw std::exception("validition layer \n");
        return VK_FALSE;
    }

    void CreateDebugUtilsMessengerEXT(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDebugUtilsMessengerEXT* pDebugMessenger) {
        auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(
            instance,
            "vkCreateDebugUtilsMessengerEXT"
        ));
        if (func != nullptr) {
            EWE_VK(func, instance, pCreateInfo, pAllocator, pDebugMessenger);
        }
        else {
            EWE_VK_RESULT(VK_ERROR_EXTENSION_NOT_PRESENT);
        }
    }

    void DestroyDebugUtilsMessengerEXT(
        VkInstance instance,
        VkDebugUtilsMessengerEXT debugMessenger,
        const VkAllocationCallbacks* pAllocator) {
        auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
        if (func != nullptr) {
            EWE_VK(func, instance, debugMessenger, pAllocator);
        }
    }

    bool FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface_) {

        uint32_t queueFamilyCount = 0;
        EWE_VK(vkGetPhysicalDeviceQueueFamilyProperties, device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        EWE_VK(vkGetPhysicalDeviceQueueFamilyProperties, device, &queueFamilyCount, queueFamilies.data());
#if EWE_DEBUG
        printf("queue family count : %d:%zu \n", queueFamilyCount, queueFamilies.size());
#endif

        //i want a designated graphics/present queue, or throw an error
        //i want a dedicated async compute queue
        //i want a dedicated async transfer queue
        //if i dont get two separate dedicated queues for transfer and compute, attempt to combine those 2
        //otherwise, flop them in the graphics queue

        bool foundDedicatedGraphicsPresent = false;
#if EWE_DEBUG
        for (const auto& queueFamily : queueFamilies) {
            printf("queue properties - %d:%d:%d\n", queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT, queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT, queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT);
        }
#endif
        std::array<bool, Queue::_count> found{ false, false, false, false };
        //fidning graphics/present queue
        int currentIndex = 0;
        for (const auto& queueFamily : queueFamilies) {
            VkBool32 presentSupport = false;
            EWE_VK(vkGetPhysicalDeviceSurfaceSupportKHR, device, currentIndex, surface_, &presentSupport);
            printf("queue present support[%d] : %d\n", currentIndex, presentSupport);
            bool graphicsSupport = queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT;
            bool computeSupport = queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT;
            if ((presentSupport && graphicsSupport && computeSupport) == true) {
                //im pretty sure compute and graphics in a queue is a vulkan requirement, but not 100%
                foundDedicatedGraphicsPresent = true;
                VK::Object->queueIndex[Queue::graphics] = currentIndex;
                VK::Object->queueIndex[Queue::present] = currentIndex;
                found[Queue::graphics] = true;
                found[Queue::present] = true;
                break;
            }
            currentIndex++;
        }
        assert(foundDedicatedGraphicsPresent && "failed to find a graphics/present queue that could also do compute");

        //re-searching for compute and transfer queues
        std::stack<int> dedicatedComputeFamilies{};
        std::stack<int> dedicatedTransferFamilies{};
        std::stack<int> combinedTransferComputeFamilies{};

        currentIndex = 0;
        for (const auto& queueFamily : queueFamilies) {
            if (currentIndex == VK::Object->queueIndex[Queue::graphics]) {
                currentIndex++;
                continue;
            }
            bool computeSupport = queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT;
            bool transferSupport = queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT;
            printf("queue support[%d] - %d:%d \n", currentIndex, computeSupport, transferSupport);
            if (computeSupport && transferSupport) {
                combinedTransferComputeFamilies.push(currentIndex);
            }
            else if (computeSupport) {
                dedicatedComputeFamilies.push(currentIndex);
            }
            else if (transferSupport) {
                dedicatedTransferFamilies.push(currentIndex);
            }
            currentIndex++;
        }
        printf("after the queue family \n");
        if (dedicatedComputeFamilies.size() > 0) {
            VK::Object->queueIndex[Queue::compute] = dedicatedComputeFamilies.top();
            found[Queue::compute] = true;
        }
        if (dedicatedTransferFamilies.size() > 0) {
            VK::Object->queueIndex[Queue::transfer] = dedicatedTransferFamilies.top();
            found[Queue::transfer] = true;
        }
        if (combinedTransferComputeFamilies.size() > 0) {
            if ((!found[Queue::compute]) && (!found[Queue::transfer])) {
                //assert(combinedTransferComputeFamilies.size() >= 2 && "not enough queues for transfer and compute");

                //VK::Object->queueIndex[Queue::compute] = combinedTransferComputeFamilies.top();
                //found[Queue::compute] = true;
                //combinedTransferComputeFamilies.pop();
                
                //need a flag in VK::Object for combined transfer and compute


                VK::Object->queueIndex[Queue::transfer] = combinedTransferComputeFamilies.top();
                found[Queue::transfer] = true;
            }
            else if (!found[Queue::compute]) {
                VK::Object->queueIndex[Queue::compute] = combinedTransferComputeFamilies.top();
                found[Queue::compute] = true;
            }
            else if (!found[Queue::transfer]) {
                VK::Object->queueIndex[Queue::transfer] = combinedTransferComputeFamilies.top();
                found[Queue::transfer] = true;
            }
        }
        //assert(found[Queue::compute] && found[Queue::transfer] && "did not find a dedicated transfer or compute queue");
        //assert(VK::Object->queueIndex[Queue::compute] != VK::Object->queueIndex[Queue::transfer] && "compute queue and transfer q should not be the same");

        if (!found[Queue::compute]) {
            printf("missing dedicated compute queue, potentially fatal crash incoming if this hasn't been resolved yet (if you don't crash it has)\n");
        }
        for (uint8_t i = 0; i < Queue::_count; i++) {
            VK::Object->queueEnabled[i] = found[i];
        }
        
        return found[Queue::graphics] && found[Queue::present];// && found[Queue::transfer];//&& found[Queue::compute];
    }

    // class member functions
    EWEDevice::EWEDevice(MainWindow& window) :
        validationLayers{ "VK_LAYER_KHRONOS_validation",  },
        deviceExtensions{
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            //VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
        },
        window{ window },
        optionalExtensions{
    #if DEBUGGING_DEVICE_LOST
                {VK_EXT_DEVICE_FAULT_EXTENSION_NAME, false},
                //{VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME}
    #endif
            {VK_EXT_MESH_SHADER_EXTENSION_NAME, false},
        }
    { //ewe device entrance
        
        
        //printf("device constructor \n");
        assert(eweDevice == nullptr && "EWEDevice already exists");
        eweDevice = this;
        VK::Object = Construct<VK>({});
#if GPU_LOGGING
        {
            std::ofstream logFile{ GPU_LOG_FILE, std::ofstream::trunc };
            logFile << "initializing log file \n";
            //initialize log file (reset it)

            logFile.close();
        }
#endif
        printf("creating device stuff\n");
        CreateInstance();
        //printf("after creating device instance \n");
        SetupDebugMessenger();
        //printf("after setup debug messenger \n");
        CreateSurface();
        //printf("after creating device surface \n");
        PickPhysicalDevice();
        //printf("after picking physical device \n");
        CreateLogicalDevice();
        //printf("after creating logical device \n");
#if USING_VMA
        CreateVmaAllocator();
#endif

        CreateCommandPools();
        //printf("command pool, transfer CP - %lld:%lld \n", commandPool, transferCommandPool);
        //std::cout << "command pool, transfer CP - " << std::hex << commandPool << ":" << transferCommandPool << std::endl;
        //printf("after creating transfer command pool \n");

        //mainThreadID = std::this_thread::get_id();

        SyncHub::Initialize();
        syncHub = SyncHub::GetSyncHubInstance();
        Sampler::Initialize();

#if DEBUGGING_DEVICE_LOST
        VKDEBUG::Initialize(VK::Object->vkDevice, instance, optionalExtensions.at(VK_EXT_DEVICE_FAULT_EXTENSION_NAME), deviceLostDebug.NVIDIAdebug, deviceLostDebug.AMDdebug);
        deviceLostDebug.Initialize(VK::Object->vkDevice);
#endif
#if DEBUG_NAMING
        DebugNaming::Initialize(enableValidationLayers);
#endif

        /*
        createTextureImage();
        createTextureImageView();
        createTextureSampler();
        */
    }

    EWEDevice::~EWEDevice() {
        Sampler::Deconstruct();
        syncHub->Destroy();
#if DECONSTRUCTION_DEBUG
        printf("beginning EWEdevice deconstruction \n");
#endif
        if (VK::Object->renderCmdPool != VK_NULL_HANDLE) {
            EWE_VK(vkDestroyCommandPool, VK::Object->vkDevice, VK::Object->renderCmdPool, nullptr);
        }
#if USING_VMA
        vmaDestroyAllocator(VK::Object->vmaAllocator);
#endif
        eweDevice = nullptr;
        EWE_VK(vkDestroyDevice, VK::Object->vkDevice, nullptr);

        if (enableValidationLayers) {
            DestroyDebugUtilsMessengerEXT(VK::Object->instance, debugMessenger, nullptr);
        }

        EWE_VK(vkDestroySurfaceKHR, VK::Object->instance, VK::Object->surface, nullptr);
        EWE_VK(vkDestroyInstance, VK::Object->instance, nullptr);

        Deconstruct(VK::Object);

#if DECONSTRUCTION_DEBUG
        printf("end EWEdevice deconstruction \n");
#endif
    }

    void EWEDevice::CreateInstance() {
        if (enableValidationLayers && !CheckValidationLayerSupport()) {

#if GPU_LOGGING
            std::ofstream logFile{ GPU_LOG_FILE, std::ios::app };
            logFile << "validation layers requested, but not available!" << std::endl;
            logFile.close();
#endif
            printf("validation layers not available \n");
            assert(false && "validation layers requested, but not available!");
        }
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Eight Winds";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 1, 1);
        appInfo.pEngineName = "Eight Winds Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 1, 1);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        if (!glfwVulkanSupported()) {
#if GPU_LOGGING
            //printf("opening file? \n");
            std::ofstream logFile{ GPU_LOG_FILE, std::ios::app };
            logFile << "glfw Vulkan is not suppoted! " << std::endl;
            logFile.close();

#endif
        }

        std::vector<const char*> extensions = GetRequiredExtensions();

        //AddOptionalExtensions(extensions);

        //extensions.push_back("VK_KHR_get_physical_Device_properties2");
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
#if 0
        VkValidationFeaturesEXT validationFeatures{};
        std::vector<VkValidationFeatureEnableEXT>  validation_feature_enables = { VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT };
        validationFeatures.enabledValidationFeatureCount = static_cast<uint32_t>(validation_feature_enables.size());
        validationFeatures.pEnabledValidationFeatures = validation_feature_enables.data();
        validationFeatures.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
        validationFeatures.pNext = nullptr;
#endif
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            PopulateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
#if 0
            debugCreateInfo.pNext = &validationFeatures;
#endif
        }
        else {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        EWE_VK(vkCreateInstance, &createInfo, nullptr, &VK::Object->instance);

        HasGflwRequiredInstanceExtensions();
    }

    void EWEDevice::PickPhysicalDevice() {

        uint32_t deviceCount = 16;
        //deviceCount = 2;
        //printf("enumerate devices result : %lld \n", result);
        std::vector<VkPhysicalDevice> devices(deviceCount);

        //score     //device iter in the vector
        std::list<std::pair<uint32_t, uint32_t>> deviceScores{};

        EWE_VK(vkEnumeratePhysicalDevices, VK::Object->instance, &deviceCount, devices.data());
        std::cout << "Device count: " << deviceCount << std::endl;

        //printf("enumerate devices2 result : %u \n", deviceCount);
        if (deviceCount == 0) {
            std::cout << "failed to find GPUs with Vulkan support!" << std::endl;
#if GPU_LOGGING
            //printf("opening file? \n");
            std::ofstream logFile{ GPU_LOG_FILE, std::ios::app };
            logFile << "failed to find device with vulkan support\n" << std::endl;
            logFile.close();

#endif
            assert(false && "failed to find GPUs with Vulkan support!");
        }

        for (uint32_t i = 0; i < deviceCount; i++) {

            EWE_VK(vkGetPhysicalDeviceProperties, devices[i], &VK::Object->properties);

            uint32_t score = 0;
            score += (VK::Object->properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) * 1000;
            score += VK::Object->properties.limits.maxImageDimension2D;
            std::string deviceNameTemp = VK::Object->properties.deviceName;
#if AMD_TARGET
            //printf("found an amd card\n");
            if ((deviceNameTemp.find("AMD") != deviceNameTemp.npos) && (VK::Object->properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)) {
                score = UINT32_MAX;
            }
#elif NVIDIA_TARGET
            if ((deviceNameTemp.find("GeForce") != deviceNameTemp.npos) && (VK::Object->properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)) {
                score = UINT32_MAX;
            }
#elif INTEGRATED_TARGET
            if (VK::Object->properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
                score = UINT32_MAX;
            }
#endif
            //properties.limits.maxFramebufferWidth;

            printf("Device Name:Score %s:%u \n", VK::Object->properties.deviceName, score);
            for (auto iter = deviceScores.begin(); iter != deviceScores.end(); iter++) {

                //big to little
                if (iter->first < score) {
                    deviceScores.insert(iter, { score, i });
                }
            }
            if (deviceScores.size() == 0) {
                deviceScores.push_back({ score, i });
            }

            //std::cout << "minimum alignment " << properties.limits.minUniformBufferOffsetAlignment << std::endl;
            //std::cout << "max sampler anisotropy : " << properties.limits.maxSamplerAnisotropy << std::endl;
        }
        //bigger score == gooder device
        //printf("after getting scores \n");

        for (auto iter = deviceScores.begin(); iter != deviceScores.end(); iter++) {
            if (IsDeviceSuitable(devices[iter->second])) {
                VK::Object->physicalDevice = devices[iter->second];
                break;
            }
            else {
                printf("device unsuitable \n");
            }
        }
        //printf("before physical device null handle \n");
        if (VK::Object->physicalDevice == VK_NULL_HANDLE) {
            printf("failed to find a suitable GPU! \n");
#if GPU_LOGGING
            //printf("opening file? \n");
            std::ofstream logFile{ GPU_LOG_FILE, std::ios::app };
            logFile << "Failed to find a suitable GPU! " << std::endl;
            logFile.close();
#endif
            assert(false && "failed to find a suitable GPU!");
        }
#if DEBUGGING_DEVICE_LOST

#endif
        CheckOptionalExtensions();

        //printf("before get physical device properties \n");
        EWE_VK(vkGetPhysicalDeviceProperties, VK::Object->physicalDevice, &VK::Object->properties);
#if EWE_DEBUG
        std::cout << "Physical Device: " << VK::Object->properties.deviceName << std::endl;
        deviceName = VK::Object->properties.deviceName;
        std::cout << "max ubo, storage : " << VK::Object->properties.limits.maxUniformBufferRange << ":" << VK::Object->properties.limits.maxStorageBufferRange << std::endl;
        std::cout << "minimum alignment " << VK::Object->properties.limits.minUniformBufferOffsetAlignment << std::endl;
        std::cout << "max sampler anisotropy : " << VK::Object->properties.limits.maxSamplerAnisotropy << std::endl;
#endif

#if GPU_LOGGING
        //printf("opening file? \n");
        std::ofstream logFile{ GPU_LOG_FILE, std::ios::app };
        logFile << "Device Name: " << VK::Object->properties.deviceName << std::endl;
        logFile.close();

        //printf("remaining memory : %d \n", GetMemoryRemaining());

#endif

        //printf("max draw vertex count : %d \n", properties.limits.maxDrawVertexCount);
    }

    void EWEDevice::CreateLogicalDevice() {

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::vector<std::vector<float>> queuePriorities{};

        VK::Object->queueIndex[Queue::present] = VK::Object->queueIndex[Queue::graphics];

        queuePriorities.emplace_back().push_back(1.f);
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = VK::Object->queueIndex[Queue::graphics];
        queueCreateInfo.queueCount = 1;
        queueCreateInfos.push_back(queueCreateInfo);

        if (VK::Object->queueEnabled[Queue::compute]) {
            queuePriorities.emplace_back().push_back(0.9f);
            queueCreateInfo.queueFamilyIndex = VK::Object->queueIndex[Queue::compute];
            queueCreateInfos.push_back(queueCreateInfo);
        }
        if (VK::Object->queueEnabled[Queue::transfer]) {
            queuePriorities.emplace_back().push_back(0.8f);
            queueCreateInfo.queueFamilyIndex = VK::Object->queueIndex[Queue::transfer];
            queueCreateInfos.push_back(queueCreateInfo);
        }
        //not currently doing a separate queue for present. its currently combined with graphics
        //not sure how much wrok it'll be to fix that, i'll come back to it later


        for (int i = 0; i < queueCreateInfos.size(); i++) {
            queueCreateInfos[i].pQueuePriorities = queuePriorities[i].data();
        }

        //VkPhysicalDeviceMeshShaderFeaturesNV nvMeshStruct{};
        //nvMeshStruct.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV;
        //nvMeshStruct.pNext = nullptr;
        //nvMeshStruct.taskShader = VK_TRUE;
        //nvMeshStruct.meshShader = VK_TRUE;

        
        VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeatures{};
        meshShaderFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
        meshShaderFeatures.meshShader = VK_TRUE;
        meshShaderFeatures.taskShader = VK_TRUE;
        //if (optionalExtensions.at(VK_NV_MESH_SHADER_EXTENSION_NAME)) {
        //    meshShaderFeatures.pNext = &nvMeshStruct;
        //}
        //else {
            meshShaderFeatures.pNext = nullptr;
        //}
        //meshShaderFeatures.meshShaderQueries = VK_TRUE;

        //VkPhysicalDeviceMeshShaderPropertiesEXT
        

        VkPhysicalDeviceFeatures2 deviceFeatures2{};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        if (optionalExtensions.at(VK_EXT_MESH_SHADER_EXTENSION_NAME)) {
            deviceFeatures2.pNext = &meshShaderFeatures;
        }
        else {
            deviceFeatures2.pNext = nullptr;
        }
        //deviceFeatures2.pNext = nullptr; //disables mesh extension
        deviceFeatures2.features.samplerAnisotropy = VK_TRUE;
        deviceFeatures2.features.geometryShader = VK_TRUE;
        deviceFeatures2.features.wideLines = VK_TRUE;
        deviceFeatures2.features.tessellationShader = VK_TRUE;
#if EWE_DEBUG
        deviceFeatures2.features.fillModeNonSolid = VK_TRUE;
#endif


        VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering_feature{};
        dynamic_rendering_feature.pNext = &deviceFeatures2;
        dynamic_rendering_feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
        dynamic_rendering_feature.dynamicRendering = VK_TRUE;

        VkPhysicalDeviceSynchronization2FeaturesKHR synchronization_2_feature{};
        synchronization_2_feature.pNext = &dynamic_rendering_feature;
        synchronization_2_feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR;

#if USING_NVIDIA_AFTERMATH
        VkDeviceDiagnosticsConfigCreateInfoNV nvDiagCreateInfo{};
        if (deviceLostDebug.NVIDIAdebug) {
            nvDiagCreateInfo.pNext = nullptr;
            nvDiagCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_DIAGNOSTICS_CONFIG_CREATE_INFO_NV;
            nvDiagCreateInfo.flags =
                VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_RESOURCE_TRACKING_BIT_NV |
                VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_AUTOMATIC_CHECKPOINTS_BIT_NV |
                VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_SHADER_DEBUG_INFO_BIT_NV;

            dynamic_rendering_feature.pNext = &nvDiagCreateInfo;
        }
#endif

        VkDeviceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pNext = &synchronization_2_feature;

        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

        createInfo.pEnabledFeatures = nullptr;

        /*
        printf("printing active device extensions \n");
        for (int i = 0; i < deviceExtensions.size(); i++) {
            printf("\t %d : %s \n", i, deviceExtensions[i]);
        }
        */
#if DEBUGGING_DEVICE_LOST
        deviceLostDebug.GetVendorDebugExtension(VK::Object->physicalDevice);
        std::vector<const char*> debugDeviceExtensions{ deviceExtensions };
        std::copy(deviceLostDebug.debugExtensions.begin(), deviceLostDebug.debugExtensions.end(), std::back_inserter(debugDeviceExtensions));

        //temp debug check
        printf("\ntotal extensions activated\n");
        for (uint32_t i = 0; i < debugDeviceExtensions.size(); i++) {
            printf("\t%s\n", debugDeviceExtensions[i]);
        }
        printf("\n");

        createInfo.enabledExtensionCount = static_cast<uint32_t>(debugDeviceExtensions.size());
        createInfo.ppEnabledExtensionNames = debugDeviceExtensions.data();
#else

        std::vector<const char*> totalDeviceExtensions{ deviceExtensions };
        for (auto const& option : optionalExtensions) {
            if (option.second) {
                totalDeviceExtensions.push_back(option.first.c_str());
            }
        }

        createInfo.enabledExtensionCount = static_cast<uint32_t>(totalDeviceExtensions.size());
        createInfo.ppEnabledExtensionNames = totalDeviceExtensions.data();
#endif



        // might not really be necessary anymore because device specific validation layers
        // have been deprecated
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else {
            createInfo.enabledLayerCount = 0;
        }
        EWE_VK(vkCreateDevice, VK::Object->physicalDevice, &createInfo, nullptr, &VK::Object->vkDevice);


        if (optionalExtensions.at(VK_EXT_MESH_SHADER_EXTENSION_NAME)) {
            VK::Object->meshShaderProperties = Construct<VkPhysicalDeviceMeshShaderPropertiesEXT>({});
            VK::Object->meshShaderProperties->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_EXT;
            VK::Object->meshShaderProperties->pNext = nullptr;
            VkPhysicalDeviceProperties2 testDeviceFeatures2{};
            testDeviceFeatures2.pNext = VK::Object->meshShaderProperties;
            testDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            EWE_VK(vkGetPhysicalDeviceProperties2, VK::Object->physicalDevice, &testDeviceFeatures2);
        }

        VK::CmdDrawMeshTasksEXT = reinterpret_cast<PFN_vkCmdDrawMeshTasksEXT>(vkGetDeviceProcAddr(VK::Object->vkDevice, "vkCmdDrawMeshTasksEXT"));
#if EWE_DEBUG
        std::cout << "getting device queues \n";
        std::cout << "\t graphics family:queue index - " << VK::Object->queueIndex[Queue::graphics] << std::endl;
        std::cout << "\t present family:queue index - " << VK::Object->queueIndex[Queue::present] << std::endl;
        std::cout << "\t compute family:queue index - " << VK::Object->queueIndex[Queue::compute] << std::endl;
        std::cout << "\t transfer family:queue index - " << VK::Object->queueIndex[Queue::transfer] << std::endl;
#endif
        //printf("before graphics queue \n");
        EWE_VK(vkGetDeviceQueue, VK::Object->vkDevice, VK::Object->queueIndex[Queue::graphics], 0, &VK::Object->queues[Queue::graphics]);
        //printf("after graphics queue \n");
        if (VK::Object->queueIndex[Queue::graphics] != VK::Object->queueIndex[Queue::present]) {
            EWE_VK(vkGetDeviceQueue, VK::Object->vkDevice, VK::Object->queueIndex[Queue::present], 0, &VK::Object->queues[Queue::present]);
        }
        else {
            VK::Object->queues[Queue::present] = VK::Object->queues[Queue::graphics];
        }
        if (VK::Object->queueEnabled[Queue::compute]) {
            EWE_VK(vkGetDeviceQueue, VK::Object->vkDevice, VK::Object->queueIndex[Queue::compute], 0, &VK::Object->queues[Queue::compute]);
        }
        if (VK::Object->queueEnabled[Queue::transfer]) {
            EWE_VK(vkGetDeviceQueue, VK::Object->vkDevice, VK::Object->queueIndex[Queue::transfer], 0, &VK::Object->queues[Queue::transfer]);
        }
#if EWE_DEBUG
        printf("after transfer qeuue \n");
#endif
    }
#if USING_VMA
    void EWEDevice::CreateVmaAllocator() {
        VmaAllocatorCreateInfo allocatorCreateInfo{};
        allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
        allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;
        allocatorCreateInfo.physicalDevice = VK::Object->physicalDevice;
        allocatorCreateInfo.device = VK::Object->vkDevice;
        allocatorCreateInfo.instance = VK::Object->instance;
        allocatorCreateInfo.pVulkanFunctions = nullptr;
        VkResult result = vmaCreateAllocator(&allocatorCreateInfo, &VK::Object->vmaAllocator);
        EWE_VK_RESULT(result);
    }
#endif

    void EWEDevice::CreateCommandPools() {
        {
            VkCommandPoolCreateInfo poolInfo = {};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.queueFamilyIndex = VK::Object->queueIndex[Queue::graphics];
            poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

            EWE_VK(vkCreateCommandPool, VK::Object->vkDevice, &poolInfo, nullptr, &VK::Object->renderCmdPool);
#if DEBUG_NAMING
            DebugNaming::SetObjectName(reinterpret_cast<void*>(VK::Object->renderCmdPool), VK_OBJECT_TYPE_COMMAND_POOL, "render cmd pool");
#endif
        }
    }
    void EWEDevice::CreateSurface() { window.CreateWindowSurface(VK::Object->instance, &VK::Object->surface, GPU_LOGGING); }

    bool EWEDevice::IsDeviceSuitable(VkPhysicalDevice device) {
        bool queuesComplete = FindQueueFamilies(device, VK::Object->surface);

        bool extensionsSupported = CheckDeviceExtensionSupport(device);


        bool swapChainAdequate = false;
        if (extensionsSupported) {
            SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        VkPhysicalDeviceFeatures supportedFeatures;
        EWE_VK(vkGetPhysicalDeviceFeatures, device, &supportedFeatures);

        return queuesComplete && extensionsSupported && swapChainAdequate &&
            supportedFeatures.samplerAnisotropy;
    }

    void EWEDevice::PopulateDebugMessengerCreateInfo(
        VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData = nullptr;  // Optional
    }

    void EWEDevice::SetupDebugMessenger() {
        if (!enableValidationLayers) { return; }
        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        PopulateDebugMessengerCreateInfo(createInfo);
        CreateDebugUtilsMessengerEXT(VK::Object->instance, &createInfo, nullptr, &debugMessenger);
    }

    bool EWEDevice::CheckValidationLayerSupport() {
        uint32_t layerCount;
        EWE_VK(vkEnumerateInstanceLayerProperties, &layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        EWE_VK(vkEnumerateInstanceLayerProperties, &layerCount, availableLayers.data());

        for (const char* layerName : validationLayers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }

        return true;
    }

    std::vector<const char*> EWEDevice::GetRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
#if GPU_LOGGING
        if (glfwExtensions == nullptr) {
            std::ofstream logFile{ GPU_LOG_FILE, std::ios::app };
            logFile << "Failed to get required extensions! " << std::endl;
            logFile.close();
        }
#endif

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
        //extensions.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
        //extensions.push_back(VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME);

        //extension.push_back()
        //extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    void EWEDevice::AddOptionalExtensions(std::vector<const char*>& extensions) {
        if (optionalExtensions.size() == 0) {
            return;
        }
        //uint32_t extensionCount;

        //EWE_VK(vkEnumerateDeviceExtensionProperties, VK::Object->physicalDevice, nullptr, &extensionCount, nullptr);

        //std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        //EWE_VK(vkEnumerateDeviceExtensionProperties,
        //    VK::Object->physicalDevice,
        //    nullptr,
        //    &extensionCount,
        //    availableExtensions.data()
        //);
        //for (auto& optional : optionalExtensions) {
        //    optional.second = false;
        //}

        //for (auto& extension : availableExtensions) {
        //    //printf("device extension name available : %s \n", extension.extensionName);
        //    auto optionalExtension = optionalExtensions.find(extension.extensionName);
        //    if (optionalExtension != optionalExtensions.end()) {
        //        optionalExtension->second = true;
        //    }
        //}

        for (auto const& optional : optionalExtensions) {
            if (optional.second) {
                extensions.push_back(optional.first.c_str());
            }
        }

    }

    void EWEDevice::HasGflwRequiredInstanceExtensions() {
        uint32_t extensionCount = 0;
        EWE_VK(vkEnumerateInstanceExtensionProperties, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensions(extensionCount);
        EWE_VK(vkEnumerateInstanceExtensionProperties, nullptr, &extensionCount, extensions.data());

        //std::cout << "available extensions:" << std::endl;
        std::unordered_set<std::string> available;
        for (const auto& extension : extensions) {
            //std::cout << "\t" << extension.extensionName << std::endl;
            available.insert(extension.extensionName);
        }

        //std::cout << "required extensions:" << std::endl;
        auto requiredExtensions = GetRequiredExtensions();
        for (const auto& required : requiredExtensions) {
            //std::cout << "\t" << required << std::endl;
            if (available.find(required) == available.end()) {
#if GPU_LOGGING
                std::ofstream logFile{ GPU_LOG_FILE, std::ios::app };
                logFile << "Extension is not available! : " << required << std::endl;
                logFile.close();
#endif
                assert(false && "Missing required glfw extension");
            }
        }
    }

    bool EWEDevice::CheckDeviceExtensionSupport(VkPhysicalDevice device) {
        uint32_t extensionCount;
        EWE_VK(vkEnumerateDeviceExtensionProperties, device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        EWE_VK(vkEnumerateDeviceExtensionProperties,
            device,
            nullptr,
            &extensionCount,
            availableExtensions.data()
        );

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto& extension : availableExtensions) {
            printf("device extension name available : %s \n", extension.extensionName);
            requiredExtensions.erase(extension.extensionName);
        }


        for (auto iter = requiredExtensions.begin(); iter != requiredExtensions.end(); iter++) {
#if GPU_LOGGING
            std::ofstream logFile{ GPU_LOG_FILE, std::ios::app };
            logFile << "REQUIRED EXTENSIONOS NOT SUPPORTED : " << *iter << std::endl;
            logFile.close();
#endif
            printf("REQUIRED EXTENSIONOS NOT SUPPORTED : %s \n", iter->c_str());
        }

        return requiredExtensions.empty();
    }
    void EWEDevice::CheckOptionalExtensions() {
        if (optionalExtensions.size() == 0) {
            return;
        }
        uint32_t extensionCount;

        EWE_VK(vkEnumerateDeviceExtensionProperties, VK::Object->physicalDevice, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        EWE_VK(vkEnumerateDeviceExtensionProperties,
            VK::Object->physicalDevice,
            nullptr,
            &extensionCount,
            availableExtensions.data()
        );

        for (auto& extension : availableExtensions) {
            //printf("device extension name available : %s \n", extension.extensionName);
            auto optionalExtension = optionalExtensions.find(extension.extensionName);
            if (optionalExtension != optionalExtensions.end()) {
                optionalExtension->second = true;
            }
        }


        for (const auto& optional : optionalExtensions) {
            if (!optional.second) {
                printf("optional extensions not supported : %s \n", optional.first.c_str());
            }
        }
    }


    SwapChainSupportDetails EWEDevice::QuerySwapChainSupport(VkPhysicalDevice device) {
        SwapChainSupportDetails details;
        EWE_VK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR, device, VK::Object->surface, &details.capabilities);

        uint32_t formatCount;
        EWE_VK(vkGetPhysicalDeviceSurfaceFormatsKHR, device, VK::Object->surface, &formatCount, nullptr);

        if (formatCount != 0) {
            details.formats.resize(formatCount);
            EWE_VK(vkGetPhysicalDeviceSurfaceFormatsKHR, device, VK::Object->surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;
        EWE_VK(vkGetPhysicalDeviceSurfacePresentModesKHR, device, VK::Object->surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            EWE_VK(vkGetPhysicalDeviceSurfacePresentModesKHR, device, VK::Object->surface, &presentModeCount, details.presentModes.data());
        }
        return details;
    }

    VkFormat EWEDevice::FindSupportedFormat(
        std::vector<VkFormat> const& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
        for (VkFormat format : candidates) {
            VkFormatProperties props;
            EWE_VK(vkGetPhysicalDeviceFormatProperties, VK::Object->physicalDevice, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }
#if GPU_LOGGING
        {
            std::ofstream file{ GPU_LOG_FILE };
            file << "failed to find supported format \n";
        }
#endif
        assert(false && "failed to find supported format");
        return VkFormat{}; //error silencing
    }
}  // namespace EWE