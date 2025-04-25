#pragma once
#define DRAWING_POINTS false

#include "EWEngine/Graphics/Descriptors.h"
#include "EWEngine/Graphics/Device_Buffer.h"
#include "EWEngine/Graphics/LightBufferObject.h"

#include <unordered_map>


namespace EWE {

    namespace DescriptorHandler {

        void Cleanup();
        //VkDescriptorSetLayout* GetGlobalDSL();
        void InitGlobalDescriptors(LightBufferObject& lbo);
        //VkDescriptorSet* GetGlobalDescSet();

        void SetCameraBuffers(std::array<EWEBuffer*, MAX_FRAMES_IN_FLIGHT>*& cameraBuffers);

        void WriteToLightBuffer(LightBufferObject& lbo);
        void AddCameraDataToDescriptor(EWEDescriptorWriter& descWriter, uint8_t whichFrame);
        void AddGlobalsToDescriptor(EWEDescriptorWriter& descWriter, uint8_t whichFrame);

        VkDescriptorBufferInfo* GetCameraDescriptorBufferInfo(uint8_t whichFrame);
        VkDescriptorBufferInfo* GetLightingDescriptorBufferInfo(uint8_t whichFrame);

        VkDescriptorSet* GetGlobalDescSet();

        /*
        namespace Template_Helper {
            void AddInfo(EWEDescriptorSetLayout* eDSL, EWEDescriptorWriter& descWriter, uint8_t whichFrame, uint8_t& whichBinding, std::array<VkDescriptorBufferInfo*, MAX_FRAMES_IN_FLIGHT>& arg) {
                descWriter.WriteBuffer(whichBinding, arg[whichFrame]);
                whichBinding++;
            }
            void AddInfo(EWEDescriptorSetLayout* eDSL, EWEDescriptorWriter& descWriter, uint8_t whichFrame, uint8_t& whichBinding, std::array<VkDescriptorBufferInfo*, MAX_FRAMES_IN_FLIGHT>&& arg) {
                descWriter.WriteBuffer(whichBinding, std::forward<VkDescriptorBufferInfo*>(arg[whichFrame]));
                whichBinding++;
            }
            void AddInfo(EWEDescriptorSetLayout* eDSL, EWEDescriptorWriter& descWriter, uint8_t whichFrame, uint8_t& whichBinding, std::array<VkDescriptorImageInfo*, MAX_FRAMES_IN_FLIGHT>& arg) {
                descWriter.WriteImage(whichBinding, arg[whichFrame]);
                whichBinding++;
            }
            void AddInfo(EWEDescriptorSetLayout* eDSL, EWEDescriptorWriter& descWriter, uint8_t whichFrame, uint8_t& whichBinding, std::array<VkDescriptorImageInfo*, MAX_FRAMES_IN_FLIGHT>&& arg) {
                descWriter.WriteImage(whichBinding, std::forward<VkDescriptorImageInfo*>(arg[whichFrame]));
                whichBinding++;
            }
            void AddInfo(EWEDescriptorSetLayout* eDSL, EWEDescriptorWriter& descWriter, uint8_t whichFrame, uint8_t& whichBinding, VkDescriptorBufferInfo*& arg) {
                descWriter.WriteBuffer(whichBinding, arg);
                whichBinding++;
            }
            void AddInfo(EWEDescriptorSetLayout* eDSL, EWEDescriptorWriter& descWriter, uint8_t whichFrame, uint8_t& whichBinding, VkDescriptorBufferInfo*&& arg) {
                descWriter.WriteBuffer(whichBinding, std::forward<VkDescriptorBufferInfo*>(arg));
                whichBinding++;
            }
            void AddInfo(EWEDescriptorSetLayout* eDSL, EWEDescriptorWriter& descWriter, uint8_t whichFrame, uint8_t& whichBinding, VkDescriptorImageInfo*& arg) {
                descWriter.WriteImage(whichBinding, arg);
                whichBinding++;
            }
            void AddInfo(EWEDescriptorSetLayout* eDSL, EWEDescriptorWriter& descWriter, uint8_t whichFrame, uint8_t& whichBinding, VkDescriptorImageInfo*&& arg) {
                descWriter.WriteImage(whichBinding, std::forward< VkDescriptorImageInfo*>(arg));
                whichBinding++;
            }
        }

        namespace Template_Helper {
            template<typename Arg, typename = std::enable_if_t<
                std::is_same_v<std::decay_t<Arg>, VkDescriptorBufferInfo*> ||
                std::is_same_v<std::decay_t<Arg>, VkDescriptorImageInfo*>
            >
            >
            void AddInfo(EWEDescriptorSetLayout * eDSL, EWEDescriptorWriter & descWriter, uint8_t whichFrame, uint8_t & whichBinding, std::array<Arg, MAX_FRAMES_IN_FLIGHT> const& arg) {
                if constexpr (std::is_same_v<std::decay_t<Arg>, VkDescriptorBufferInfo*>) {
                    descWriter.WriteBuffer(whichBinding, std::forward<Arg>(arg[whichFrame]));
                }
                else if constexpr (std::is_same_v<std::decay_t<Arg>, VkDescriptorImageInfo*>) {
                    descWriter.WriteImage(whichBinding, std::forward<Arg>(arg[whichFrame]));
                }
                else {
                    static_assert(false);
                }
            }

            template<typename Arg,
                typename = std::enable_if_t<
                std::is_same_v<std::decay_t<Arg>, VkDescriptorBufferInfo*> ||
                std::is_same_v<std::decay_t<Arg>, VkDescriptorImageInfo*>
                >
            >
            void AddInfo(EWEDescriptorSetLayout* eDSL, EWEDescriptorWriter& descWriter, uint8_t whichFrame, uint8_t& whichBinding, Arg&& arg) {
                if constexpr (std::is_same_v<std::decay_t<Arg>, VkDescriptorBufferInfo*>) {
                    descWriter.WriteBuffer(whichBinding, std::forward<Arg>(arg));
                }
                else if constexpr (std::is_same_v<std::decay_t<Arg>, VkDescriptorImageInfo*>) {
                    descWriter.WriteImage(whichBinding, std::forward<Arg>(arg));
                }
                else {
                    static_assert(false);
                }
            }
        }

        template<typename T, typename... Args>
            requires(
                std::is_same_v<std::decay_t<T>, std::array<VkDescriptorBufferInfo*, MAX_FRAMES_IN_FLIGHT>>
                || std::is_same_v<std::decay_t<T>, std::array<VkDescriptorImageInfo*, MAX_FRAMES_IN_FLIGHT>>
                || std::is_same_v<std::decay_t<T>, VkDescriptorBufferInfo*>
                || std::is_same_v<std::decay_t<T>, VkDescriptorImageInfo*>
            )
        [[nodiscard]] std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> CreateDescriptorSets(EWEDescriptorSetLayout* eDSL, T&& first, Args const&... args) {
            std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> ret;
            for (uint8_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                EWEDescriptorWriter descWriter(eDSL, DescriptorPool_Global);

                
                uint8_t whichBinding = 0;
                Template_Helper::AddInfo(eDSL, descWriter, i, whichBinding, std::forward<T>(first));

                if constexpr (sizeof...(args) > 0) {
                    (Template_Helper::AddInfo(eDSL, descWriter, i, whichBinding, std::forward<Args>(args)), ...);
                }
                ret[i] = descWriter.Build();
            }
            return ret;
        }

        template<typename T, typename... Args>
            requires(
                std::is_same_v<std::decay_t<T>, std::array<VkDescriptorBufferInfo*, MAX_FRAMES_IN_FLIGHT>>
                || std::is_same_v<std::decay_t<T>, std::array<VkDescriptorImageInfo*, MAX_FRAMES_IN_FLIGHT>>
                || std::is_same_v<std::decay_t<T>, VkDescriptorBufferInfo*>
                || std::is_same_v<std::decay_t<T>, VkDescriptorImageInfo*>
            )
        [[nodiscard]] std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> CreateDescriptorSetsWithGlobals(EWEDescriptorSetLayout* eDSL, T&& first, Args&&... args) {
            std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> ret;
            for (uint8_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                EWEDescriptorWriter descWriter(eDSL, DescriptorPool_Global);
                AddGlobalsToDescriptor(descWriter, i);

                uint8_t whichBinding = 2;
                Template_Helper::AddInfo(eDSL, descWriter, i, whichBinding, std::forward<T>(first));
                if constexpr (sizeof...(args) > 0) {
                    (Template_Helper::AddInfo(eDSL, descWriter, i, whichBinding, std::forward<Args>(args)), ...);
                }
                ret[i] = descWriter.Build();
            }
            return ret;
        }
        */
    };
}