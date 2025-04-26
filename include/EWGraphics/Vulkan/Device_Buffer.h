#pragma once

#include "EWGraphics/Vulkan/Device.hpp"

namespace EWE {

    class EWEBuffer {
    public:
        EWEBuffer(VkDeviceSize instanceSize, uint32_t instanceCount, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags);

        ~EWEBuffer();
        void Reconstruct(VkDeviceSize instanceSize, uint32_t instanceCount, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags);

        EWEBuffer(const EWEBuffer&) = delete;
        EWEBuffer& operator=(const EWEBuffer&) = delete;

        void Map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        void Unmap();

        void WriteToBufferAligned(void* data, VkDeviceSize size, uint64_t alignmentOffset);
        void WriteToBuffer(void const* data, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        void Flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        VkDescriptorBufferInfo DescriptorInfo(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;
        VkDescriptorBufferInfo* DescriptorInfo(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        void Invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

        void FlushMin(uint64_t offset);
        void FlushIndex(int index);
        VkDescriptorBufferInfo* DescriptorInfoForIndex(int index);
        void InvalidateIndex(int index);

        [[nodiscard]] VkBuffer GetBuffer() const { return buffer_info.buffer; }

        [[nodiscard]] VkBuffer* GetBufferAddress() { return &buffer_info.buffer; }
        
        [[nodiscard]] void* GetMappedMemory() const { return mapped; }
#if DEBUG_NAMING
        void SetName(std::string const& name);
#endif

        //uint32_t getInstanceCount() const { return instanceCount; }
        //VkDeviceSize getInstanceSize() const { return instanceSize; }
        //VkDeviceSize getAlignmentSize() const { return instanceSize; }
        VkBufferUsageFlags GetUsageFlags() const { return usageFlags; }
        VkMemoryPropertyFlags GetMemoryPropertyFlags() const { return memoryPropertyFlags; }
        VkDeviceSize GetBufferSize() const { return bufferSize; }

        //allocated with new, up to the user to delete, or put it in a unique_ptr
#if CALL_TRACING
        static EWEBuffer* CreateAndInitBuffer(void* data, uint64_t dataSize, uint64_t dataCount, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, std::source_location = std::source_location::current());
#else
        static EWEBuffer* CreateAndInitBuffer(void* data, uint64_t dataSize, uint64_t dataCount, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags);
#endif

        static VkDeviceSize GetAlignment(VkDeviceSize instanceSize, VkBufferUsageFlags usageFlags);
    private:
        VkDeviceSize GetAlignment(VkDeviceSize instanceSize);

        void* mapped = nullptr;
        VkDescriptorBufferInfo buffer_info;
         

        VkDeviceSize bufferSize;
        //uint32_t instanceCount;
        VkDeviceSize alignmentSize;
        VkBufferUsageFlags usageFlags;
        VkMemoryPropertyFlags memoryPropertyFlags;
        VkDeviceSize minOffsetAlignment = 1;

#if USING_VMA
        VmaAllocation vmaAlloc{};
#else
        VkDeviceMemory memory = VK_NULL_HANDLE;
#endif
    };

}  // namespace EWE