#include "EWGraphics/Vulkan/VulkanHeader.h"

#if CALL_TRACING
#if _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <DbgHelp.h>
#pragma comment(lib, "Dbghelp.lib")
#endif
#endif


#if USING_VMA
#define VMA_IMPLEMENTATION
#if WIN32
#define WIN32_LEAN_AND_MEAN
#endif
#include "EWGraphics/Vulkan/vk_mem_alloc.h"
#endif
#if CALL_TRACING
void EWE_VK_RESULT(VkResult vkResult, const std::source_location& sourceLocation) {
#if DEBUGGING_DEVICE_LOST                                                                                        
    if (vkResult == VK_ERROR_DEVICE_LOST) { EWE::VKDEBUG::OnDeviceLost(); }
    else
#endif
    if (vkResult != VK_SUCCESS) {
        printf("VK_ERROR : %s(%d) : %s - %d \n", sourceLocation.file_name(), sourceLocation.line(), sourceLocation.function_name(), vkResult);
        std::ofstream logFile{};
        logFile.open(GPU_LOG_FILE, std::ios::app);
        assert(logFile.is_open() && "Failed to open log file");
        logFile << "VK_ERROR : " << sourceLocation.file_name() << '(' << sourceLocation.line() << ") : " << sourceLocation.function_name() << " : VkResult(" << vkResult << ")\n";
        
        logFile << "current frame index - " << EWE::VK::Object->frameIndex << std::endl;
        for (uint8_t i = 0; i < EWE::VK::Object->renderCommands.size(); i++) {

            while (EWE::VK::Object->renderCommands[i].usageTracking.size() > 0) {
                for (auto& usage : EWE::VK::Object->renderCommands[i].usageTracking.front()) {
                    logFile << "cb(" << +i  << ")(" << EWE::VK::Object->renderCommands[i].usageTracking.size() << ") : " << usage.funcName << '\n';
                }
                EWE::VK::Object->renderCommands[i].usageTracking.pop();
            }
        }
        logFile.close();
        assert(vkResult == VK_SUCCESS && "VK_ERROR");
    }
}
#else
void EWE_VK_RESULT(VkResult vkResult) {
#if DEBUGGING_DEVICE_LOST                                                                                        
    if (vkResult == VK_ERROR_DEVICE_LOST) { EWE::VKDEBUG::OnDeviceLost(); }
    else
#endif
        if (vkResult != VK_SUCCESS) {
            printf("VK_ERROR : %d \n", vkResult);
            std::ofstream logFile{};
            logFile.open(GPU_LOG_FILE, std::ios::app);
            assert(logFile.is_open() && "Failed to open log file");
            logFile << "VK_ERROR : " << vkResult << "\n";
            logFile.close();
            assert(vkResult == VK_SUCCESS && "VK_ERROR");
        }
}
#endif


namespace EWE {
    VK* VK::Object{ nullptr };
    PFN_vkCmdDrawMeshTasksEXT VK::CmdDrawMeshTasksEXT = VK_NULL_HANDLE;


    uint32_t FindMemoryType(uint32_t typeFilter, const VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        EWE_VK(vkGetPhysicalDeviceMemoryProperties, VK::Object->physicalDevice, &memProperties);

        /*
        printf("memory heap count : %d \n", memProperties.memoryHeapCount);
        printf("memory type count : %d \n", memProperties.memoryTypeCount);

        for (uint32_t i = 0; i < memProperties.memoryHeapCount; i++) {
            printf("heap[%d] size : %llu \n", i, memProperties.memoryHeaps->size);
            printf("heap flags : %d \n", memProperties.memoryHeaps->flags);
        }
        */

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            //printf("memory type[%d] heap index : %d \n", i, memProperties.memoryTypes->heapIndex);
            if ((typeFilter & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        assert(false && "find memory type unsupported");
        return UINT32_MAX;
    }


#if USING_VMA
    StagingBuffer::StagingBuffer(VkDeviceSize size, const void* data) {
        VkBufferCreateInfo bufferCreateInfo{};
        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.size = size;
        bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationInfo vmaAllocInfo{};
        VmaAllocationCreateInfo vmaAllocCreateInfo{};
        vmaAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
        vmaAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_MAPPED_BIT;
        EWE_VK(vmaCreateBuffer, VK::Object->vmaAllocator, &bufferCreateInfo, &vmaAllocCreateInfo, &buffer, &vmaAlloc, &vmaAllocInfo);

        Stage(data, size);
    }
    StagingBuffer::StagingBuffer(VkDeviceSize size) {
        VkBufferCreateInfo bufferCreateInfo{};
        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.size = size;
        bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationInfo vmaAllocInfo{};
        VmaAllocationCreateInfo vmaAllocCreateInfo{};
        vmaAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
        vmaAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_MAPPED_BIT;
        EWE_VK(vmaCreateBuffer, VK::Object->vmaAllocator, &bufferCreateInfo, &vmaAllocCreateInfo, &buffer, &vmaAlloc, &vmaAllocInfo);
    }
#else
    StagingBuffer::StagingBuffer(VkDeviceSize size, const void* data) : bufferSize{size} {
        VkBufferCreateInfo bufferCreateInfo{};
        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.pNext = nullptr;
        bufferCreateInfo.size = size;
        bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        EWE_VK(vkCreateBuffer, VK::Object->vkDevice, &bufferCreateInfo, nullptr, &buffer);

        VkMemoryRequirements memRequirements;
        EWE_VK(vkGetBufferMemoryRequirements, VK::Object->vkDevice, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.pNext = nullptr;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        EWE_VK(vkAllocateMemory, VK::Object->vkDevice, &allocInfo, nullptr, &memory);

        EWE_VK(vkBindBufferMemory, VK::Object->vkDevice, buffer, memory, 0);

        Stage(data, size);
    }
    StagingBuffer::StagingBuffer(VkDeviceSize size) : bufferSize{size} {
        VkBufferCreateInfo bufferCreateInfo{};
        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.pNext = nullptr;
        bufferCreateInfo.size = size;
        bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        EWE_VK(vkCreateBuffer, VK::Object->vkDevice, &bufferCreateInfo, nullptr, &buffer);

        VkMemoryRequirements memRequirements;
        EWE_VK(vkGetBufferMemoryRequirements, VK::Object->vkDevice, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.pNext = nullptr;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        EWE_VK(vkAllocateMemory, VK::Object->vkDevice, &allocInfo, nullptr, &memory);

        EWE_VK(vkBindBufferMemory, VK::Object->vkDevice, buffer, memory, 0);
    }
#endif

#if USING_VMA
    void StagingBuffer::Free() {
        if (buffer == VK_NULL_HANDLE) {
            return;
        }
        EWE_VK(vmaDestroyBuffer, VK::Object->vmaAllocator, buffer, vmaAlloc);
#else
    void StagingBuffer::Free() {
        if (buffer != VK_NULL_HANDLE) {
            EWE_VK(vkDestroyBuffer, VK::Object->vkDevice, buffer, nullptr);
        }
        if (memory != VK_NULL_HANDLE) {
            EWE_VK(vkFreeMemory, VK::Object->vkDevice, memory, nullptr);
        }
#endif
    }

#if USING_VMA
    void StagingBuffer::Free() const {
        if (buffer == VK_NULL_HANDLE) {
            return;
        }
        EWE_VK(vmaDestroyBuffer, VK::Object->vmaAllocator, buffer, vmaAlloc);
#else
    void StagingBuffer::Free() const {
        if (buffer != VK_NULL_HANDLE) {
            EWE_VK(vkDestroyBuffer, VK::Object->vkDevice, buffer, nullptr);
        }
        if (memory != VK_NULL_HANDLE) {
            EWE_VK(vkFreeMemory, VK::Object->vkDevice, memory, nullptr);
        }
#endif
    }
#if USING_VMA
    void StagingBuffer::Stage(const void* data, uint64_t bufferSize) {
        void* stagingData;

        EWE_VK(vmaMapMemory, VK::Object->vmaAllocator, vmaAlloc, &stagingData);
        memcpy(stagingData, data, bufferSize);
        EWE_VK(vmaUnmapMemory, VK::Object->vmaAllocator, vmaAlloc);
    }
    void StagingBuffer::Map(void*& data) {
        EWE_VK(vmaMapMemory, VK::Object->vmaAllocator, vmaAlloc, &data);
    }
    void StagingBuffer::Unmap() {
        EWE_VK(vmaUnmapMemory, VK::Object->vmaAllocator, vmaAlloc);
    }
#else
    void StagingBuffer::Stage(const void* data, VkDeviceSize bufferSize) {
        void* stagingData;
        EWE_VK(vkMapMemory, VK::Object->vkDevice, memory, 0, bufferSize, 0, &stagingData);
        memcpy(stagingData, data, bufferSize);
        EWE_VK(vkUnmapMemory, VK::Object->vkDevice, memory);
    }

    void StagingBuffer::Map(void*& data) {
        EWE_VK(vkMapMemory, VK::Object->vkDevice, memory, 0, bufferSize, 0, &data);
    }
    void StagingBuffer::Unmap() {
        EWE_VK(vkUnmapMemory, VK::Object->vkDevice, memory);
    }
#endif




    void CommandBuffer::Reset() {
        //VkCommandBufferResetFlags flags = VkCommandBufferResetFlagBits::VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT;
        VkCommandBufferResetFlags flags = 0;
        EWE_VK(vkResetCommandBuffer, *this, flags);
        inUse = false;
    }

#if COMMAND_BUFFER_TRACING
    namespace PLEASE {
        std::string GetFuncName(void* funcPtr) {

#if _WIN32
            static bool initialized = false;
            if (!initialized) {
                SymInitialize(GetCurrentProcess(), nullptr, TRUE);
                initialized = true;
            }

            DWORD64 address = reinterpret_cast<DWORD64>(funcPtr);

            char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
            SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(buffer);
            symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
            symbol->MaxNameLen = MAX_SYM_NAME;

            if (SymFromAddr(GetCurrentProcess(), address, nullptr, symbol)) {
                return std::string(symbol->Name);
            }

            return "<unknown symbol>";
#endif
        }
    }
#endif
    void CommandBuffer::Begin() {
        inUse = true;
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.pNext = nullptr;
#if CALL_TRACING
        if (usageTracking.size() > 2) {
            usageTracking.pop();
        }
        usageTracking.push({});
#endif
        EWE_VK(vkBeginCommandBuffer, *this, &beginInfo);
    }
    void CommandBuffer::BeginSingleTime() {
        inUse = true;
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.pNext = nullptr;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
#if CALL_TRACING
        if (usageTracking.size() > 2) {
            usageTracking.pop();
        }
        usageTracking.push({});
#endif
        EWE_VK(vkBeginCommandBuffer, *this, &beginInfo);
    }
    void VK::CopyBuffer(CommandBuffer& cmdBuf, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        //printf("COPY SECONDARY BUFFER, thread ID: %d \n", std::this_thread::get_id());
        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;  // Optional
        copyRegion.dstOffset = 0;  // Optional
        copyRegion.size = size;
        EWE_VK(vkCmdCopyBuffer, cmdBuf, srcBuffer, dstBuffer, 1, &copyRegion);
    }


} //namespaceEWE