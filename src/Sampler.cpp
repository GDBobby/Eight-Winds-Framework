#include "EWGraphics/Vulkan/VulkanHeader.h"

#include <vector>
#include <cassert>
#include <string.h>

#define EXPECTED_MAXIMUM_AMOUNT_OF_SAMPLERS 50

#define SAMPLER_DUPLICATION_TRACKING true

#if SAMPLER_DUPLICATION_TRACKING
namespace EWE {


    struct SamplerTracker {
        uint32_t inUseCount = 1;
        uint32_t totalUsed = 0;
        void Add() {
            inUseCount++;
            totalUsed++;

#if EWE_DEBUG
            //printf("sampler count after add : %d \n", inUseCount);
#endif
        }
        bool Remove() {
#if EWE_DEBUG
            assert(inUseCount > 0 && "removing sampler from tracker when none exist");
#endif
            inUseCount--;
            return inUseCount == 0;
        }
    };
    struct SamplerDuplicateTracker {
        VkSamplerCreateInfo samplerInfo;
        VkSampler sampler;
        //putting the tracker here instead of using a unordered_map<SamplerTracker, VkSampler>
        SamplerTracker tracker{};

        SamplerDuplicateTracker(VkSamplerCreateInfo const& samplerInfo) : samplerInfo(samplerInfo) {
            EWE_VK(vkCreateSampler, VK::Object->vkDevice, &samplerInfo, nullptr, &sampler);
        }
    };


    std::vector<SamplerDuplicateTracker> storedSamplers{};
#else
    std::vector<VkSampler> storedSamplers{};
#endif

#if SAMPLER_DUPLICATION_TRACKING
    inline bool BitwiseEqualOperator(VkSamplerCreateInfo const& lhs, VkSamplerCreateInfo const& rhs) {
		return memcmp(&lhs, &rhs, sizeof(VkSamplerCreateInfo)) == 0;
	}

#endif


    namespace Sampler {
        VkSampler GetSampler(VkSamplerCreateInfo const& samplerInfo) {
#if SAMPLER_DUPLICATION_TRACKING
            for (auto& duplicate : storedSamplers) {
                if (BitwiseEqualOperator(duplicate.samplerInfo, samplerInfo)) {
                    duplicate.tracker.Add();
                    return duplicate.sampler;
                }
            }
#if EWE_DEBUG
            assert(storedSamplers.size() < EXPECTED_MAXIMUM_AMOUNT_OF_SAMPLERS && "warning: sampler count is greater than expected maximum amount of samplers");
#endif
            return storedSamplers.emplace_back(samplerInfo).sampler;
#else
            VkSampler sampler;
            EWE_VK(vkCreateSampler, EWEDevice::GetVkDevice(), &samplerInfo, nullptr, &sampler);
            return sampler;
#endif
        }
        void RemoveSampler(VkSampler sampler) {
#if SAMPLER_DUPLICATION_TRACKING
            for (auto iter = storedSamplers.begin(); iter != storedSamplers.end(); iter++) {
                if (iter->sampler == sampler) {
                    if (iter->tracker.Remove()) {
                        EWE_VK(vkDestroySampler, VK::Object->vkDevice, iter->sampler, nullptr);
                        storedSamplers.erase(iter);
                    }
                    return;
                }
            }
#if EWE_DEBUG
            assert(false && "removing a sampler that does not exist");
#endif
#else
            EWE_VK(vkDestroySampler, EWEDevice::GetVkDevice(), sampler, nullptr);
#endif
        }
        void Initialize() {

#if SAMPLER_DUPLICATION_TRACKING
            storedSamplers.reserve(EXPECTED_MAXIMUM_AMOUNT_OF_SAMPLERS);
#endif
        }
        void Deconstruct() {
#if EWE_DEBUG
            //assert(storedSamplers.size() == 0 && "destroying sampler manager with samplers still in use");
#endif

            //if the sampler was already destroyed this will create a validation error
            //if the sampler was not destroyed, it'll be fun tracing the source
            for (auto const& sampler : storedSamplers) {
#if SAMPLER_DUPLICATION_TRACKING
                EWE_VK(vkDestroySampler, VK::Object->vkDevice, sampler.sampler, nullptr);
#else
                EWE_VK(vkDestroySampler, VK::Object->vkDevice, sampler, nullptr);
#endif
            }
        }
    } //namespace Sampler
} //namespace EWE