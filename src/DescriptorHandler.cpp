#include "EWEngine/Graphics/DescriptorHandler.h"

#include <stdexcept>

namespace EWE {
    EWEDescriptorSetLayout* globalDSL;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> globalDescSet;

    std::array<EWEBuffer*, MAX_FRAMES_IN_FLIGHT> globalBuffer;
    std::array<EWEBuffer*, MAX_FRAMES_IN_FLIGHT> lightBuffer;

    VkDescriptorBufferInfo* DescriptorHandler::GetCameraDescriptorBufferInfo(uint8_t whichFrame) {
        return globalBuffer[whichFrame]->DescriptorInfo();
    }
    VkDescriptorBufferInfo* DescriptorHandler::GetLightingDescriptorBufferInfo(uint8_t whichFrame) {
        return lightBuffer[whichFrame]->DescriptorInfo();
    }

    void DescriptorHandler::SetCameraBuffers(std::array<EWEBuffer*, MAX_FRAMES_IN_FLIGHT>*& cameraBuffers) {
        cameraBuffers = &globalBuffer;
    }

    void DescriptorHandler::WriteToLightBuffer(LightBufferObject& lbo) {
        lightBuffer[VK::Object->frameIndex]->WriteToBuffer(&lbo);
        lightBuffer[VK::Object->frameIndex]->Flush();
    }

    void DescriptorHandler::Cleanup() {
        Deconstruct(globalDSL);
        for (auto& descriptorSet : globalDescSet) {
            EWEDescriptorPool::FreeDescriptor(DescriptorPool_Global, globalDSL, &descriptorSet);
        }
    }
    namespace DescriptorHandler {
        VkDescriptorSetLayout* GetGlobalDSL() {
            return globalDSL->GetDescriptorSetLayout();
        }

        void InitGlobalDescriptors(LightBufferObject& lbo) {

            globalDSL = EWEDescriptorSetLayout::Builder()
                .AddBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
                .AddBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
                .Build();

            for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                //printf("init ars descriptors, loop : %d \n", i);
                globalBuffer[i] = Construct<EWEBuffer>({ sizeof(GlobalUbo), 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT });
                globalBuffer[i]->Map();
                lightBuffer[i] = EWEBuffer::CreateAndInitBuffer(&lbo, sizeof(LightBufferObject), 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);


                globalDescSet[i] = EWEDescriptorWriter(globalDSL, DescriptorPool_Global)
                    .WriteBuffer(globalBuffer[i]->DescriptorInfo())
                    .WriteBuffer(lightBuffer[i]->DescriptorInfo())
                    .Build();
            }
//#if DEBUG_NAMING
//            DebugNaming::SetObjectName(globalDescSet[0], VK_OBJECT_TYPE_DESCRIPTOR_SET, "global DS[0]");
//            DebugNaming::SetObjectName(globalDescSet[1], VK_OBJECT_TYPE_DESCRIPTOR_SET, "global DS[1]");
//#endif
        }

        void AddCameraDataToDescriptor(EWEDescriptorWriter& descWriter, uint8_t whichFrame) {
            descWriter.WriteBuffer(globalBuffer[whichFrame]->DescriptorInfo());
        }
        void AddGlobalsToDescriptor(EWEDescriptorWriter& descWriter, uint8_t whichFrame) {
            descWriter.WriteBuffer(globalBuffer[whichFrame]->DescriptorInfo());
            descWriter.WriteBuffer(lightBuffer[whichFrame]->DescriptorInfo());
        }

        VkDescriptorSet* GetGlobalDescSet() {
            return &globalDescSet[VK::Object->frameIndex];
        }
    }//namespace Descriptorhandler
}