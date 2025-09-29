#include "EWGraphics/Vulkan/DebugNaming.h"

#include "EWGraphics/Vulkan/VulkanHeader.h"

#include <cassert>

namespace EWE{
    namespace DebugNaming{
#if DEBUG_NAMING
        PFN_vkQueueBeginDebugUtilsLabelEXT pfnQueueBegin;
        PFN_vkQueueEndDebugUtilsLabelEXT pfnQueueEnd;
        PFN_vkSetDebugUtilsObjectNameEXT pfnSetObjectName;
        PFN_vkCmdBeginDebugUtilsLabelEXT pfnBeginLabel;
        PFN_vkCmdEndDebugUtilsLabelEXT pfnEndLabel;
        bool enabled = false;

        void Initialize(bool extension_enabled){
            if(extension_enabled){

                pfnQueueBegin = reinterpret_cast<PFN_vkQueueBeginDebugUtilsLabelEXT>(vkGetDeviceProcAddr(VK::Object->vkDevice, "vkQueueBeginDebugUtilsLabelEXT"));
                pfnQueueEnd = reinterpret_cast<PFN_vkQueueEndDebugUtilsLabelEXT>(vkGetDeviceProcAddr(VK::Object->vkDevice, "vkQueueEndDebugUtilsLabelEXT"));
                pfnSetObjectName = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetDeviceProcAddr(VK::Object->vkDevice, "vkSetDebugUtilsObjectNameEXT"));
                pfnBeginLabel = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(vkGetDeviceProcAddr(VK::Object->vkDevice, "vkCmdBeginDebugUtilsLabelEXT"));
                pfnEndLabel = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(vkGetDeviceProcAddr(VK::Object->vkDevice, "vkCmdEndDebugUtilsLabelEXT"));

                assert(pfnQueueBegin);
                assert(pfnQueueEnd);
                assert(pfnSetObjectName);
                assert(pfnBeginLabel);
                assert(pfnEndLabel);
                enabled = pfnQueueBegin && pfnQueueEnd && pfnSetObjectName && extension_enabled;
                    
                assert(enabled && "failed to enable DEBUG_MARKER extension");
                
            }
        }
#endif
        void QueueBegin(uint8_t queue, float red, float green, float blue, const char* name) {
#if DEBUG_NAMING
            VkDebugUtilsLabelEXT utilLabel{};
            utilLabel.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
            utilLabel.pNext = nullptr;
            utilLabel.color[0] = red;
            utilLabel.color[1] = green;
            utilLabel.color[2] = blue;
            utilLabel.color[3] = 1.f;
            utilLabel.pLabelName = name;
            VK::Object->queueMutex[queue].lock();
            pfnQueueBegin(VK::Object->queues[queue], &utilLabel);
            VK::Object->queueMutex[queue].unlock();
#endif
        }
        void QueueEnd(uint8_t queue) {
#if DEBUG_NAMING
            VK::Object->queueMutex[queue].lock();
            pfnQueueEnd(VK::Object->queues[queue]);
            VK::Object->queueMutex[queue].unlock();
#endif
        }

        void BeginLabel(const char* name, float red, float green, float blue) {
#if DEBUG_NAMING
            VkDebugUtilsLabelEXT utilLabel{};
            utilLabel.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
            utilLabel.pNext = nullptr;
            utilLabel.color[0] = red;
            utilLabel.color[1] = green;
            utilLabel.color[2] = blue;
            utilLabel.color[3] = 1.f;
            utilLabel.pLabelName = name;
            EWE_VK(pfnBeginLabel, VK::Object->GetFrameBuffer(), &utilLabel);
#endif
        }
        void EndLabel() {
#if DEBUG_NAMING
            EWE_VK(pfnEndLabel, VK::Object->GetFrameBuffer());
#endif
        }

        void Deconstruct(){}

        void SetObjectName(void* object, VkObjectType objectType, const char* name) {
#if DEBUG_NAMING
            // Check for a valid function pointer
            if (enabled) {
                VkDebugUtilsObjectNameInfoEXT nameInfo{};
                nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
                nameInfo.pNext = nullptr;
                nameInfo.objectHandle = reinterpret_cast<uint64_t>(object);
                nameInfo.objectType = objectType;
                nameInfo.pObjectName = name;
                //pfnDebugMarkerSetObjectName(device, &nameInfo);
                pfnSetObjectName(VK::Object->vkDevice, &nameInfo);

            }
#endif
        }

    } //namespace DebugNaming
} //namespace EWE