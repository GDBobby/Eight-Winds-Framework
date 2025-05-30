#include "EWGraphics/Model/Model.h"


// std
#include <cassert>
#include <unordered_map>
#include <iostream>

#ifndef ENGINE_DIR
#define ENGINE_DIR "models/"
#endif

namespace EWE {
    EWEModel::EWEModel(void const* verticesData, const std::size_t vertexCount, const std::size_t sizeOfVertex, std::vector<uint32_t> const& indices) {
        assert(vertexCount >= 3 && "vertex count must be at least 3");
        VertexBuffers(static_cast<uint32_t>(vertexCount), static_cast<uint32_t>(sizeOfVertex), verticesData);
        CreateIndexBuffers(indices);
    }
    EWEModel::EWEModel(void const* verticesData, const std::size_t vertexCount, const std::size_t sizeOfVertex) {
        assert(vertexCount >= 3 && "vertex count must be at least 3");
        VertexBuffers(static_cast<uint32_t>(vertexCount), static_cast<uint32_t>(sizeOfVertex), verticesData);
    }
    
    inline void CopyModelBuffer(StagingBuffer* stagingBuffer, VkBuffer dstBuffer, const VkDeviceSize bufferSize) {
        SyncHub* syncHub = SyncHub::GetSyncHubInstance();
        CommandBuffer& cmdBuf = syncHub->BeginSingleTimeCommand();
        VK::CopyBuffer(cmdBuf, stagingBuffer->buffer, dstBuffer, bufferSize);


        if (VK::Object->CheckMainThread() || (!VK::Object->queueEnabled[Queue::transfer])) {
            GraphicsCommand gCommand{};
            gCommand.command = &cmdBuf;
            gCommand.stagingBuffer = stagingBuffer;
            syncHub->EndSingleTimeCommandGraphics(gCommand);
        }
        else {
            //transitioning from transfer to compute not supported currently
            TransferCommand command{};
            command.commands.push_back(&cmdBuf);
            command.stagingBuffers.push_back(stagingBuffer);
            syncHub->EndSingleTimeCommandTransfer(command);
        }
    }

    void EWEModel::AddInstancing(const uint32_t instanceCount, const uint32_t instanceSize, void const* data) {
        VkDeviceSize bufferSize = instanceSize * instanceCount;
        this->instanceCount = instanceCount;

        uint64_t alignmentSize = EWEBuffer::GetAlignment(instanceSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT) * instanceCount;

#if USING_VMA
        StagingBuffer* stagingBuffer = Construct<StagingBuffer>({ alignmentSize, data });
#else
        StagingBuffer* stagingBuffer = Construct<StagingBuffer>({ alignmentSize, data });
#endif

        instanceBuffer = Construct<EWEBuffer>({
            instanceSize,
            instanceCount,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT 
        });
        
        CopyModelBuffer(stagingBuffer, instanceBuffer->GetBuffer(), bufferSize);
    }
    /*
    void EWEModel::updateInstancing(uint32_t instanceCount, uint32_t instanceSize, void* data, uint8_t instanceIndex, CommandBuffer cmdBuf) {
        assert(instanceIndex < instanceBuffer.size());
        VkDeviceSize bufferSize = instanceSize * instanceCount;
        this->instanceCount = instanceCount;
        EWEBuffer stagingBuffer{
            eweDevice,
            instanceSize,
            instanceCount,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        };

        stagingBuffer.map();
        stagingBuffer.writeToBuffer(data);

        instanceBuffer[instanceIndex] = std::make_unique<EWEBuffer>(
            eweDevice,
            instanceSize,
            instanceCount,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        eweDevice.copySecondaryBuffer(stagingBuffer.getBuffer(), instanceBuffer[instanceIndex]->getBuffer(), bufferSize, cmdBuf);
    }
    */

    void EWEModel::VertexBuffers(uint32_t vertexCount, uint32_t vertexSize, void const* data){
        VkDeviceSize bufferSize = vertexSize * vertexCount;
        this->vertexCount = vertexCount;

#if USING_VMA
        StagingBuffer* stagingBuffer = Construct<StagingBuffer>({ bufferSize, data });
#else
        StagingBuffer* stagingBuffer = Construct<StagingBuffer>({ bufferSize, data });
#endif
#if DEBUGGING_MEMORY_WITH_VMA
        vertexBuffer = Construct<EWEBuffer>({
            vertexSize,
            vertexCount,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        });
#else
        vertexBuffer = Construct<EWEBuffer>({
            vertexSize,
            vertexCount,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        });
#endif

        CopyModelBuffer(stagingBuffer, vertexBuffer->GetBuffer(), bufferSize);
    }

    void EWEModel::CreateIndexBuffer(const void* indexData, uint32_t indexCount){
        const uint32_t indexSize = sizeof(uint32_t);
        this->indexCount = indexCount;
        hasIndexBuffer = true;

        VkDeviceSize bufferSize = indexSize * indexCount;
#if USING_VMA
        StagingBuffer* stagingBuffer = Construct<StagingBuffer>({ bufferSize, indexData });
#else
        StagingBuffer* stagingBuffer = Construct<StagingBuffer>({ bufferSize, indexData });
#endif
#if DEBUGGING_MEMORY_WITH_VMA
        indexBuffer = Construct<EWEBuffer>({
            indexSize,
            indexCount,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        });
#else
        indexBuffer = Construct<EWEBuffer>({
            indexSize,
            indexCount,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        });
#endif
        CopyModelBuffer(stagingBuffer, indexBuffer->GetBuffer(), bufferSize);
    }

    void EWEModel::CreateIndexBuffers(std::vector<uint32_t> const& indices){
        indexCount = static_cast<uint32_t>(indices.size());
        CreateIndexBuffer(static_cast<const void*>(indices.data()), indexCount);
    }


    void EWEModel::Draw() {
#if EWE_DEBUG
        assert(hasIndexBuffer);
#endif
        EWE_VK(vkCmdDrawIndexed, VK::Object->GetFrameBuffer(), indexCount, 1, 0, 0, 0);
    }
    void EWEModel::DrawNoIndex() {
        EWE_VK(vkCmdDraw, VK::Object->GetFrameBuffer(), vertexCount, 1, 0, 0);
    }

    void EWEModel::Bind() {
        VkDeviceSize offset = 0;
        EWE_VK(vkCmdBindVertexBuffers, VK::Object->GetFrameBuffer(), 0, 1, vertexBuffer->GetBufferAddress(), &offset);
#if EWE_DEBUG
        assert(hasIndexBuffer);
#endif
        EWE_VK(vkCmdBindIndexBuffer, VK::Object->GetFrameBuffer(), indexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
    }

    void EWEModel::BindNoIndex() {
        VkDeviceSize offset = 0;
        EWE_VK(vkCmdBindVertexBuffers, VK::Object->GetFrameBuffer(), 0, 1, vertexBuffer->GetBufferAddress(), &offset);
    }

    void EWEModel::BindAndDraw() {
        VkDeviceSize offset = 0;
        EWE_VK(vkCmdBindVertexBuffers, VK::Object->GetFrameBuffer(), 0, 1, vertexBuffer->GetBufferAddress(), &offset);

#if EWE_DEBUG
        assert(hasIndexBuffer);
#endif
        EWE_VK(vkCmdBindIndexBuffer, VK::Object->GetFrameBuffer(), indexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
        EWE_VK(vkCmdDrawIndexed, VK::Object->GetFrameBuffer(), indexCount, 1, 0, 0, 0);
        
    }
    void EWEModel::BindAndDrawNoIndex() {
        VkDeviceSize offset = 0;
        EWE_VK(vkCmdBindVertexBuffers, VK::Object->GetFrameBuffer(), 0, 1, vertexBuffer->GetBufferAddress(), &offset);
        EWE_VK(vkCmdDraw, VK::Object->GetFrameBuffer(), vertexCount, 1, 0, 0);
    }

    void EWEModel::BindAndDrawInstance(uint32_t instanceCount) {
        VkDeviceSize offset = 0;
        EWE_VK(vkCmdBindVertexBuffers, VK::Object->GetFrameBuffer(), 0, 1, vertexBuffer->GetBufferAddress(), &offset);
        EWE_VK(vkCmdBindIndexBuffer, VK::Object->GetFrameBuffer(), indexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
        EWE_VK(vkCmdDrawIndexed, VK::Object->GetFrameBuffer(), indexCount, instanceCount, 0, 0, 0);
    }
    void EWEModel::BindAndDrawInstance() {
        VkBuffer buffers[2] = { vertexBuffer->GetBuffer(), instanceBuffer->GetBuffer()};
        VkDeviceSize offsets[2] = { 0, 0 };
        EWE_VK(vkCmdBindVertexBuffers, VK::Object->GetFrameBuffer(), 0, 2, buffers, offsets);
        EWE_VK(vkCmdBindIndexBuffer, VK::Object->GetFrameBuffer(), indexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
        EWE_VK(vkCmdDrawIndexed, VK::Object->GetFrameBuffer(), indexCount, instanceCount, 0, 0, 0);
    }
    void EWEModel::BindAndDrawInstanceNoIndex() {
        VkBuffer buffers[2] = { vertexBuffer->GetBuffer(), instanceBuffer->GetBuffer() };
        VkDeviceSize offsets[2] = { 0, 0 };
        EWE_VK(vkCmdBindVertexBuffers, VK::Object->GetFrameBuffer(), 0, 2, buffers, offsets);
        EWE_VK(vkCmdDraw, VK::Object->GetFrameBuffer(), vertexCount, instanceCount, 0, 0);
    }


    void EWEModel::BindAndDrawInstanceNoBuffer(int instanceCount) {
        VkDeviceSize offset = 0;
        EWE_VK(vkCmdBindVertexBuffers, VK::Object->GetFrameBuffer(), 0, 1, vertexBuffer->GetBufferAddress(), &offset);

#if EWE_DEBUG
        assert(hasIndexBuffer);
#endif
        EWE_VK(vkCmdBindIndexBuffer, VK::Object->GetFrameBuffer(), indexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
        EWE_VK(vkCmdDrawIndexed, VK::Object->GetFrameBuffer(), indexCount, instanceCount, 0, 0, 0);
    }
    void EWEModel::BindAndDrawInstanceNoBufferNoIndex(int instanceCount) {
        VkDeviceSize offset = 0;
        EWE_VK(vkCmdBindVertexBuffers, VK::Object->GetFrameBuffer(), 0, 1, vertexBuffer->GetBufferAddress(), &offset);

        EWE_VK(vkCmdDraw, VK::Object->GetFrameBuffer(), vertexCount, instanceCount, 0, 0);
        
    }

#if DEBUG_NAMING
    void EWEModel::SetDebugNames(std::string const& name){
        std::string comboName{"vertex:"};
        comboName += name;
        DebugNaming::SetObjectName(vertexBuffer->GetBuffer(), VK_OBJECT_TYPE_BUFFER, comboName.c_str());
        if(hasIndexBuffer){
            comboName = "index:";
            comboName += name;
            DebugNaming::SetObjectName(indexBuffer->GetBuffer(), VK_OBJECT_TYPE_BUFFER, comboName.c_str());
        }
        if(hasInstanceBuffer){
           comboName = "instance:";
           comboName += name;   
           DebugNaming::SetObjectName(instanceBuffer->GetBuffer(), VK_OBJECT_TYPE_BUFFER, comboName.c_str());
        }
    }
#endif


    /*
    EWEModel* EWEModel::LoadGrassField() {
        printf("beginning load grass field \n");
        std::ifstream grassFile{ "..//models//grassField.gs" };
        if (!grassFile.is_open()) {
            printf("failed to open grass field file \n");
            return nullptr;
        }
        uint32_t vertexCount = 2500 * 13;
        uint32_t vertexSize = 12;

        std::vector<bobvec3> vertexBuffer(vertexCount);
        grassFile.seekg(0);
        grassFile.read(reinterpret_cast<char*>(&vertexBuffer[0]), vertexCount * vertexSize);

        uint32_t indexCount = 2500 * 42;
        std::vector<uint32_t> indexBuffer(indexCount);

        grassFile.seekg(vertexCount * vertexSize);
        grassFile.read(reinterpret_cast<char*>(&indexBuffer[0]), indexCount * 4);
        grassFile.close();
        printf("successfuly read file \n");
        //printf("size of vertices and indices in grass field  - %d : %d \n", vertices.size(), indices.size());
        return std::make_unique<EWEModel>(device, vertexCount, 12, vertexBuffer.data(), indexBuffer.data(), indexCount);
    }
    */
}  // namespace EWE
