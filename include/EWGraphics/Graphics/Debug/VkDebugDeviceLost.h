#pragma once
#include "EWEngine/Graphics/Preprocessor.h"

#if DEBUGGING_DEVICE_LOST
#include "EWEngine/Data/MemoryTypeBucket.h"
#if USING_NVIDIA_AFTERMATH
#include "EWEngine/Graphics/Debug/NSightAftermathGpuCrashTracker.h"
#endif

#include "vulkan/vulkan.h"

#include <array>
#include <vector>
#include <future>
#include <type_traits>

//there's a license in there, not sure if i need to copy it or not
//https://github.com/ConfettiFX/The-Forge/blob/23483a282ddc8a917f8b292b0250dec122eab6a9/Common_3/Graphics/Vulkan/Vulkan.cpp#L741



namespace EWE {
    template<typename F, typename... Args>
    void vkWrap(F&& f, Args&&... args) {
        //using return_type = typename std::invoke_result<F(Args...)>::type;
        
        //constexpr auto retType = typename std::invoke_result<F(Args...)>::type();

        if constexpr (std::is_same_v<std::invoke_result<F(Args...)>::type, void>) {
            printf("void return type\n");
            //std::bind(std::forward<F>(f), std::forward<Args>(args)...)();
            //std::forward<F>(f)(args...);
            f(args);
		}
        else {
            printf("not void return type\n");
			auto ret = std::bind(std::forward<F>(f), std::forward<Args>(args)...)();
            //auto ret = f(args);
		}
        //do something with the return

    }

    namespace VKDEBUG {

        enum class GFX_vk_checkpoint_type : uint8_t {
            //https://feresignum.com/debugging-vk_error_device_lost-with-nvidia-device-diagnostics/
            begin_render_pass,
            end_render_pass,
            push_marker,
            pop_marker,
            draw,
            generi_c
        };

        struct GFX_vk_checkpoint_data {
            GFX_vk_checkpoint_data(const char* name, GFX_vk_checkpoint_type type) :
                type(type)
            {
                strncpy(this->name, name, sizeof(this->name));
                this->name[sizeof(this->name) - 1] = '\0';
            }

            char name[48];
            GFX_vk_checkpoint_type type;
        };



        struct DeviceLostDebugStructure {
            //https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_NV_device_diagnostic_checkpoints.html
            // nvidia extension depends on get_physical_device_properties2 which was promoted to core in 1.1 (currently using 1.3)
            //https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_AMD_buffer_marker.html
            std::vector<const char*> debugExtensions = {
                VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME,
                VK_AMD_BUFFER_MARKER_EXTENSION_NAME
            };
            void InitAMDDebug();
            bool AMDdebug = false;

            bool NVIDIAdebug = false;
            std::vector<VkCheckpointDataNV> checkpoints{};
            void InitNvidiaDebug();

            //std::vector<GFX_vk_checkpoint_data> checkpoints{};

            DeviceLostDebugStructure();
            void Initialize(VkDevice device);

            bool GetVendorDebugExtension(VkPhysicalDevice physDevice);

            static void AddAMDCheckpoint(DeviceLostDebugStructure* thisPtr, CommandBuffer cmdBuf, const char* name, GFX_vk_checkpoint_type type);
            static void AddNvidiaCheckpoint(DeviceLostDebugStructure* thisPtr, CommandBuffer cmdBuf, const char* name, GFX_vk_checkpoint_type type);
            void AddCheckpoint(CommandBuffer cmdBuf, const char* name, GFX_vk_checkpoint_type type);

            void ClearCheckpoints();
        };



        void Initialize(VkDevice vkDevice, VkInstance vkInstance, std::array<VkQueue, 4>& queues, bool deviceFaultEnabledParam, bool nvidiaCheckpointEnabled, bool amdCheckpointEnabled);
        void OnDeviceLost();
    } //namespace VKDEBUG
} //namespace EWE
#endif