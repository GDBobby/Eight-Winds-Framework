#include "EWGraphics/Vulkan/Device_Buffer.h"

// std
#include <cassert>
#include <cstring>

namespace EWE {

    /**
     * Returns the minimum instance size required to be compatible with devices minOffsetAlignment
     *
     * @param instanceSize The size of an instance
     * @param minOffsetAlignment The minimum required alignment, in bytes, for the offset member (eg
     * minUniformBufferOffsetAlignment)
     *
     * @return VkResult of the buffer mapping call
     */

    VkDeviceSize EWEBuffer::GetAlignment(VkDeviceSize instanceSize, VkBufferUsageFlags usageFlags) {
        VkDeviceSize minOffsetAlignment = 1;
        if (((usageFlags & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) == VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) ||
            ((usageFlags & VK_BUFFER_USAGE_INDEX_BUFFER_BIT) == VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
            ) {
            minOffsetAlignment = 1;
        }
        else if (((usageFlags & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) == VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)) {
            minOffsetAlignment = VK::Object->properties.limits.minUniformBufferOffsetAlignment;
        }
        else if (((usageFlags & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) == VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)) {
            minOffsetAlignment = VK::Object->properties.limits.minStorageBufferOffsetAlignment;
        }

        if (minOffsetAlignment > 0) {
            //printf("get alignment size : %zu \n", (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1));
            return (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
        }
        return instanceSize;
    }
    VkDeviceSize EWEBuffer::GetAlignment(VkDeviceSize instanceSize) {

        if (((usageFlags & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) == VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) ||
            ((usageFlags & VK_BUFFER_USAGE_INDEX_BUFFER_BIT) == VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
            ) {
            minOffsetAlignment = 1;
        }
        else if (((usageFlags & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) == VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)) {
            minOffsetAlignment = VK::Object->properties.limits.minUniformBufferOffsetAlignment;
            //printf("uniform buffer alignment : %zu\n", minOffsetAlignment);
        }
        else if (((usageFlags & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) == VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)) {
            minOffsetAlignment = VK::Object->properties.limits.minStorageBufferOffsetAlignment;
        }

        if (minOffsetAlignment > 0) {
            //printf("get alignment size : %zu \n", (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1));
            return (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
        }
        return instanceSize;
    }

    EWEBuffer::EWEBuffer(VkDeviceSize instanceSize, uint32_t instanceCount, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags)
        : usageFlags{usageFlags}, memoryPropertyFlags{memoryPropertyFlags} {

        alignmentSize = GetAlignment(instanceSize);
        bufferSize = alignmentSize * instanceCount;
        //printf("buffer size : %zu\n", bufferSize);
#if USING_VMA
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = usageFlags;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VmaAllocationInfo vmaAllocInfo{};
        VmaAllocationCreateInfo vmaAllocCreateInfo{};
        vmaAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
        vmaAllocCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT;
#if DEBUGGING_MEMORY_WITH_VMA
        vmaAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_MAPPED_BIT;
#else
        switch (memoryPropertyFlags) {
        case VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT: {
            vmaAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
            break;
        }
        case VK_MEMORY_PROPERTY_HOST_CACHED_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT: {
            vmaAllocCreateInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                VMA_ALLOCATION_CREATE_MAPPED_BIT;
            break;
        }
        case VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT: {
            vmaAllocCreateInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                VMA_ALLOCATION_CREATE_MAPPED_BIT;
            break;
        }
        case VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT: {
            vmaAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            break;
        }
        }
#endif
        EWE_VK(vmaCreateBuffer, VK::Object->vmaAllocator, &bufferInfo, &vmaAllocCreateInfo, &buffer_info.buffer, &vmaAlloc, nullptr);
#else
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = usageFlags;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        EWE_VK(vkCreateBuffer, VK::Object->vkDevice, &bufferInfo, nullptr, &buffer_info.buffer);

        VkMemoryRequirements memRequirements;
        EWE_VK(vkGetBufferMemoryRequirements, VK::Object->vkDevice, buffer_info.buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, memoryPropertyFlags);

        EWE_VK(vkAllocateMemory, VK::Object->vkDevice, &allocInfo, nullptr, &memory);

        EWE_VK(vkBindBufferMemory, VK::Object->vkDevice, buffer_info.buffer, memory, 0);
#endif
    }

    EWEBuffer::~EWEBuffer() {
        Unmap();
#if USING_VMA
        vmaDestroyBuffer(VK::Object->vmaAllocator, buffer_info.buffer, vmaAlloc);
#else
        EWE_VK(vkDestroyBuffer, VK::Object->vkDevice, buffer_info.buffer, nullptr);
        EWE_VK(vkFreeMemory, VK::Object->vkDevice, memory, nullptr);
#endif
    }
    void EWEBuffer::Reconstruct(VkDeviceSize instanceSize, uint32_t instanceCount, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags) {
        Unmap();
#if USING_VMA
        vmaDestroyBuffer(VK::Object->vmaAllocator, buffer_info.buffer, vmaAlloc);
#else
        EWE_VK(vkDestroyBuffer, VK::Object->vkDevice, buffer_info.buffer, nullptr);
        EWE_VK(vkFreeMemory, VK::Object->vkDevice, memory, nullptr);
#endif

        this->usageFlags = usageFlags;
        this->memoryPropertyFlags = memoryPropertyFlags;

        alignmentSize = GetAlignment(instanceSize);
        bufferSize = alignmentSize * instanceCount;
#if USING_VMA
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = usageFlags;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VmaAllocationInfo vmaAllocInfo{};
        VmaAllocationCreateInfo vmaAllocCreateInfo{};
        vmaAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
#if DEBUGGING_MEMORY_WITH_VMA
        vmaAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_MAPPED_BIT;
#endif
        EWE_VK(vmaCreateBuffer, VK::Object->vmaAllocator, &bufferInfo, &vmaAllocCreateInfo, &buffer_info.buffer, &vmaAlloc, nullptr);
#else
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = usageFlags;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        EWE_VK(vkCreateBuffer, VK::Object->vkDevice, &bufferInfo, nullptr, &buffer_info.buffer);

        VkMemoryRequirements memRequirements;
        EWE_VK(vkGetBufferMemoryRequirements, VK::Object->vkDevice, buffer_info.buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, memoryPropertyFlags);

        EWE_VK(vkAllocateMemory, VK::Object->vkDevice, &allocInfo, nullptr, &memory);

        EWE_VK(vkBindBufferMemory, VK::Object->vkDevice, buffer_info.buffer, memory, 0);
#endif
        Map();
    }


    /**
     * Map a memory range of this buffer. If successful, mapped points to the specified buffer range.
     *
     * @param size (Optional) Size of the memory range to map. Pass VK_WHOLE_SIZE to map the complete
     * buffer range.
     * @param offset (Optional) Byte offset from beginning
     *
     * @return VkResult of the buffer mapping call
     */
    void EWEBuffer::Map(VkDeviceSize size, VkDeviceSize offset) {
#if USING_VMA
        EWE_VK(vmaMapMemory, VK::Object->vmaAllocator, vmaAlloc, &mapped);
#else
#if EWE_DEBUG
        assert(buffer_info.buffer && memory && "Called map on buffer before create");
#endif
        EWE_VK(vkMapMemory, VK::Object->vkDevice, memory, offset, size, 0, &mapped);
#endif
//#endif
    }

    /**
     * Unmap a mapped memory range
     *
     * @note Does not return a result as vkUnmapMemory can't fail
     */
    void EWEBuffer::Unmap() {
        if (mapped) {
#if USING_VMA
            EWE_VK(vmaUnmapMemory, VK::Object->vmaAllocator, vmaAlloc);
#else
            EWE_VK(vkUnmapMemory, VK::Object->vkDevice, memory);
#endif
            mapped = nullptr;
        }
    }

    /**
     * Copies the specified data to the mapped buffer. Default value writes whole buffer range
     *
     * @param data Pointer to the data to copy
     * @param size (Optional) Size of the data to copy. Pass VK_WHOLE_SIZE to flush the complete buffer
     * range.
     * @param offset (Optional) Byte offset from beginning of mapped region
     *
     */
    void EWEBuffer::WriteToBufferAligned(void* data, VkDeviceSize size, uint64_t alignmentOffset) {
        assert(mapped && "Cannot copy to unmapped buffer");

        char* memOffset = (char*)mapped;
        const uint64_t offset = alignmentOffset * alignmentSize;
#if EWE_DEBUG
        if ((offset + size) > bufferSize) {
            printf("overflow error in buffer - %zu:%zu \n", offset + size, bufferSize);
            assert(false && "buffer overflow");
        }
#endif
        memOffset += offset;
        memcpy(memOffset, data, size);

        
    }

    void EWEBuffer::WriteToBuffer(void const* data, VkDeviceSize size, VkDeviceSize offset) {
#if EWE_DEBUG
        assert(mapped && "Cannot copy to unmapped buffer");
#endif

        if (size == VK_WHOLE_SIZE) {
            memcpy(mapped, data, bufferSize);
        }
        else {
#if EWE_DEBUG
            if ((offset + size) > bufferSize) {
                printf("overflow error in buffer - %zu:%zu \n", offset+size, bufferSize);
                assert(false && "buffer overflow");
            }
#endif

            char* memOffset = (char*)mapped;
            memOffset += offset;
            memcpy(memOffset, data, size);
        }
    }

    /**
     * Flush a memory range of the buffer to make it visible to the device
     *
     * @note Only required for non-coherent memory
     *
     * @param size (Optional) Size of the memory range to flush. Pass VK_WHOLE_SIZE to flush the
     * complete buffer range.
     * @param offset (Optional) Byte offset from beginning
     *
     * @return VkResult of the flush call
     */
    void EWEBuffer::Flush(VkDeviceSize size, VkDeviceSize offset) {
#if USING_VMA
        EWE_VK(vmaFlushAllocation, VK::Object->vmaAllocator, vmaAlloc, offset, size);
#else
        VkMappedMemoryRange mappedRange = {};
        mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mappedRange.memory = memory;
        mappedRange.offset = offset;
        mappedRange.size = size;
        EWE_VK(vkFlushMappedMemoryRanges, VK::Object->vkDevice, 1, &mappedRange);
#endif
    }
    void EWEBuffer::FlushMin(uint64_t offset) {
        VkDeviceSize trueOffset = offset - (offset % minOffsetAlignment);
#if USING_VMA
        EWE_VK(vmaFlushAllocation, VK::Object->vmaAllocator, vmaAlloc, trueOffset, minOffsetAlignment);
#else
        
        VkMappedMemoryRange mappedRange = {};
        mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mappedRange.memory = memory;
        mappedRange.offset = trueOffset;
        mappedRange.size = minOffsetAlignment;
#if EWE_DEBUG
        printf("flushing minimal : %zu \n", minOffsetAlignment);
#endif
        EWE_VK(vkFlushMappedMemoryRanges, VK::Object->vkDevice, 1, &mappedRange);
#endif
    }

    /**
     * Invalidate a memory range of the buffer to make it visible to the host
     *
     * @note Only required for non-coherent memory
     *
     * @param size (Optional) Size of the memory range to invalidate. Pass VK_WHOLE_SIZE to invalidate
     * the complete buffer range.
     * @param offset (Optional) Byte offset from beginning
     *
     * @return VkResult of the invalidate call
     */
    void EWEBuffer::Invalidate(VkDeviceSize size, VkDeviceSize offset) {
#if USING_VMA
        EWE_VK(vmaInvalidateAllocation, VK::Object->vmaAllocator, vmaAlloc, offset, size);
#else
        VkMappedMemoryRange mappedRange = {};
        mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mappedRange.memory = memory;
        mappedRange.offset = offset;
        mappedRange.size = size;
        EWE_VK(vkInvalidateMappedMemoryRanges, VK::Object->vkDevice, 1, &mappedRange);
#endif
    }

    /**
     * Create a buffer info descriptor
     *
     * @param size (Optional) Size of the memory range of the descriptor
     * @param offset (Optional) Byte offset from beginning
     *
     * @return VkDescriptorBufferInfo of specified offset and range
     */
    VkDescriptorBufferInfo EWEBuffer::DescriptorInfo(VkDeviceSize size, VkDeviceSize offset) const {
        VkDescriptorBufferInfo ret = buffer_info;
        ret.offset = offset;
        ret.range = size;
        return ret;
    }

    VkDescriptorBufferInfo* EWEBuffer::DescriptorInfo(VkDeviceSize size, VkDeviceSize offset) {
        buffer_info.offset = offset;
        buffer_info.range = size;
        return &buffer_info;
        //return &VkDescriptorBufferInfo{ buffer, offset, size, };
    }

    /**
     *  Flush the memory range at index * alignmentSize of the buffer to make it visible to the device
     *
     * @param index Used in offset calculation
     *
     */
    void EWEBuffer::FlushIndex(int index) { Flush(alignmentSize, index * alignmentSize); }

    /**
     * Create a buffer info descriptor
     *
     * @param index Specifies the region given by index * alignmentSize
     *
     * @return VkDescriptorBufferInfo for instance at index
     */
    VkDescriptorBufferInfo* EWEBuffer::DescriptorInfoForIndex(int index) {
        return DescriptorInfo(alignmentSize, index * alignmentSize);
    }

    /**
     * Invalidate a memory range of the buffer to make it visible to the host
     *
     * @note Only required for non-coherent memory
     *
     * @param index Specifies the region to invalidate: index * alignmentSize
     *
     * @return VkResult of the invalidate call
     */
    void EWEBuffer::InvalidateIndex(int index) {
        Invalidate(alignmentSize, index * alignmentSize);
    }

#if CALL_TRACING
    EWEBuffer* EWEBuffer::CreateAndInitBuffer(void* data, uint64_t dataSize, uint64_t dataCount, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, std::source_location srcLoc) {
        EWEBuffer* retBuffer = Construct<EWEBuffer>({ dataSize * dataCount, 1, usageFlags, memoryPropertyFlags }, srcLoc);

        retBuffer->Map();
        retBuffer->WriteToBuffer(data, dataSize * dataCount);
        retBuffer->Flush();
        return retBuffer;
    }
#else
    EWEBuffer* EWEBuffer::CreateAndInitBuffer(void* data, uint64_t dataSize, uint64_t dataCount, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags) {
        EWEBuffer* retBuffer = Construct<EWEBuffer>({dataSize * dataCount, 1, usageFlags, memoryPropertyFlags });
        
        retBuffer->Map();
        retBuffer->WriteToBuffer(data, dataSize * dataCount);
        retBuffer->Flush();
        return retBuffer;
    }
#endif

#if DEBUG_NAMING
    void EWEBuffer::SetName(std::string const& name) {
        std::string bufferName = name;
        bufferName += ":buffer";
        DebugNaming::SetObjectNameRC(buffer_info.buffer, VK_OBJECT_TYPE_BUFFER, bufferName.c_str());
        std::string memoryName = name;
        memoryName += ":memory";
#if USING_VMA
        vmaSetAllocationName(VK::Object->vmaAllocator, vmaAlloc, memoryName.c_str());
        
#else
        DebugNaming::SetObjectName(memory, VK_OBJECT_TYPE_DEVICE_MEMORY, memoryName.c_str());
#endif
    }
#endif
}  // namespace EWE