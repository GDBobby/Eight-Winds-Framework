#pragma once
#include "EWEngine/Graphics/VulkanHeader.h"


namespace EWE {
    namespace Sampler {
        VkSampler GetSampler(VkSamplerCreateInfo const& samplerInfo);
        void RemoveSampler(VkSampler sampler);
        void Initialize();
        void Deconstruct();
    } //namespace Sampler
}
