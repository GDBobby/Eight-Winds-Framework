#include "EWGraphics/Vulkan/Descriptors.h"

#include "EWGraphics/Texture/Image_Manager.h"

// std
#include <cassert>
#include <stdexcept>

#define DESCRIPTOR_DEBUGGING true

namespace EWE {

    // *************** Descriptor Set Layout Builder *********************
    DescriptorSetLayout::Builder& DescriptorSetLayout::Builder::AddBinding(VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, uint32_t count) {
        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.binding = bindings.size();
        layoutBinding.descriptorType = descriptorType;
        layoutBinding.descriptorCount = count;
        layoutBinding.stageFlags = stageFlags;
        bindings.push_back(layoutBinding);
        return *this;
    }


    DescriptorSetLayout::Builder& DescriptorSetLayout::Builder::AddGlobalBindingForCompute() {
        VkDescriptorSetLayoutBinding globalBindings{};
        globalBindings.binding = 0;
        globalBindings.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        globalBindings.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        globalBindings.descriptorCount = 1;
        bindings.push_back(globalBindings);
        return *this;
    }

    DescriptorSetLayout::Builder& DescriptorSetLayout::Builder::AddGlobalBindings() {
#if EWE_DEBUG
        assert(bindings.size() == 0);
#endif
        VkDescriptorSetLayoutBinding globalBindings{};
        globalBindings.binding = 0;
        globalBindings.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        globalBindings.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS | VK_SHADER_STAGE_MESH_BIT_EXT;
        globalBindings.descriptorCount = 1;
        bindings.push_back(globalBindings);

        globalBindings.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        globalBindings.binding = 1;
        bindings.push_back(globalBindings);

        return *this;
    }
    DescriptorSetLayout* DescriptorSetLayout::Builder::BuildBindless() {
        auto* ret = Construct<DescriptorSetLayout>(bindings, true);
        ret->BuildVkDSL();
        return ret;
    }
    DescriptorSetLayout* DescriptorSetLayout::Builder::Build() {
        auto* ret = Construct<DescriptorSetLayout>(bindings);
        ret->BuildVkDSL();
        return ret;
    }
    DescriptorSetLayout DescriptorSetLayout::Builder::BuildInPlace() {
        return  DescriptorSetLayout(bindings);
    }

    // *************** Descriptor Set Layout *********************

    void DescriptorSetLayout::BuildVkDSL() {
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
        descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        descriptorSetLayoutInfo.pBindings = bindings.data();

        if (bindless) {
            const VkDescriptorBindingFlagsEXT flags =
                VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT |
                VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT |
                VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT |
                VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT_EXT;

            VkDescriptorSetLayoutBindingFlagsCreateInfoEXT binding_flags{};
            binding_flags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
            binding_flags.pNext = nullptr;
            binding_flags.bindingCount = 1;
            binding_flags.pBindingFlags = &flags;
            descriptorSetLayoutInfo.pNext = &binding_flags;

            descriptorSetLayoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

            EWE_VK(vkCreateDescriptorSetLayout, VK::Object->vkDevice, &descriptorSetLayoutInfo, nullptr, &vkDSL);
        }
        else {
            descriptorSetLayoutInfo.flags = 0;
            descriptorSetLayoutInfo.pNext = nullptr;
            EWE_VK(vkCreateDescriptorSetLayout, VK::Object->vkDevice, &descriptorSetLayoutInfo, nullptr, &vkDSL);
        }
    }

    DescriptorSetLayout::DescriptorSetLayout() : vkDSL{ VK_NULL_HANDLE }, bindings{}, bindless{ false } {}
    DescriptorSetLayout::DescriptorSetLayout(std::vector<VkDescriptorSetLayoutBinding>& bindings, bool bindless)
        : vkDSL{ VK_NULL_HANDLE }, bindings{ std::move(bindings) }, bindless{ bindless }
    {}
    DescriptorSetLayout::DescriptorSetLayout(std::vector<VkDescriptorSetLayoutBinding> const& bindings, bool bindless)
        : vkDSL{ VK_NULL_HANDLE }, bindings{ bindings }, bindless{ bindless }
    {}

    DescriptorSetLayout::~DescriptorSetLayout() {
        if (vkDSL != VK_NULL_HANDLE) {
            EWE_VK(vkDestroyDescriptorSetLayout, VK::Object->vkDevice, vkDSL, nullptr);
        }
#if EWE_DEBUG
        printf("probably have memory leaks currently, address this ASAP \n");
        //the reason i have this print statement (feb 2024)
        //i changed the builder to return a new pointer instead of a unique pointer
        //all deconstruction must be followed by delete. 
        //if this is printed, and it is not preceded by delete, it is a memory leak
        //the print is being put in, instead of immediately addressing the issue because i have multiple other bugs im dealing with currently
        //that all come before deconstruction time
#endif
    }


    DescriptorLayoutPack::DescriptorLayoutPack(std::vector<std::vector<VkDescriptorSetLayoutBinding>> const& sets) {
        setLayouts.reserve(sets.size());
        for (uint8_t i = 0; i < sets.size(); i++) {
            setLayouts.push_back(i, Construct<DescriptorSetLayout>(sets[i]));
        }
    }


    DescriptorLayoutPack::DescriptorLayoutPack(DescriptorLayoutPack&& other) noexcept
        : setLayouts{ std::move(other.setLayouts)}
    {
		other.setLayouts.clear();
    }

    DescriptorLayoutPack::~DescriptorLayoutPack() {
        for (auto& dsl : setLayouts) {
            if (dsl.value) {
                Deconstruct(dsl.value);
            }
        }
        setLayouts.clear();
    }

    VkDescriptorSetLayout* DescriptorLayoutPack::GetDescriptorSetLayout(uint8_t index) { 
        for (auto& set : setLayouts) {
            if (set.key == index) {
                return &set.value->vkDSL;
            }
        }
        EWE_UNREACHABLE;
    }



    // *************** Descriptor Pool Builder *********************

    EWEDescriptorPool::Builder& EWEDescriptorPool::Builder::AddPoolSize(VkDescriptorType descriptorType, uint32_t count) {
        poolSizes.push_back({ descriptorType, count });
        return *this;
    }

    EWEDescriptorPool::Builder& EWEDescriptorPool::Builder::SetPoolFlags(VkDescriptorPoolCreateFlags flags) {
        poolFlags = flags;
        return *this;
    }
    EWEDescriptorPool::Builder& EWEDescriptorPool::Builder::SetMaxSets(uint32_t count) {
        maxSets = count;
        return *this;
    }
    EWEDescriptorPool* EWEDescriptorPool::Builder::Build() const {
        return Construct<EWEDescriptorPool>(maxSets, poolFlags, poolSizes);
    }

    // *************** Descriptor Pool *********************

    std::unordered_map<uint16_t, EWEDescriptorPool> EWEDescriptorPool::pools{};

    EWEDescriptorPool::EWEDescriptorPool(uint32_t maxSets, VkDescriptorPoolCreateFlags poolFlags, const std::vector<VkDescriptorPoolSize>& poolSizes) {

        VkDescriptorPoolCreateInfo descriptorPoolInfo{};
        descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        descriptorPoolInfo.pPoolSizes = poolSizes.data();
        descriptorPoolInfo.maxSets = maxSets;
        descriptorPoolInfo.flags = poolFlags;

        for (auto& poolSize : poolSizes) {
            trackers.emplace(poolSize.type, DescriptorTracker(poolSize.descriptorCount));
        }

        EWE_VK(vkCreateDescriptorPool, VK::Object->vkDevice, &descriptorPoolInfo, nullptr, &descriptorPool);
    }
    EWEDescriptorPool::EWEDescriptorPool(VkDescriptorPoolCreateInfo& pool_info) {
        for (uint32_t i = 0; i < pool_info.poolSizeCount; i++) {
            trackers.emplace(pool_info.pPoolSizes[i].type, DescriptorTracker(pool_info.pPoolSizes[i].descriptorCount));
        }

        EWE_VK(vkCreateDescriptorPool, VK::Object->vkDevice, &pool_info, nullptr, &descriptorPool);
    }

    EWEDescriptorPool::~EWEDescriptorPool() {
        printf("before destroy pool \n");
        for (auto& tracker : trackers) {
			printf("active:max - %d:%d \n", tracker.second.current, tracker.second.max);

		}

        EWE_VK(vkDestroyDescriptorPool, VK::Object->vkDevice, descriptorPool, nullptr);
        printf("after destroy pool \n");
    }
    void EWEDescriptorPool::AllocateDescriptor(DescriptorPool_ID poolID, const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor) {
        pools.at(poolID).AllocateDescriptor(&descriptorSetLayout, descriptor);
    }
    void EWEDescriptorPool::AllocateDescriptor(const VkDescriptorSetLayout* descLayout, VkDescriptorSet& descriptor) {

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.pSetLayouts = descLayout;
        allocInfo.descriptorSetCount = 1;

        mutex.lock();
        activeDescriptors++;
        EWE_VK(vkAllocateDescriptorSets, VK::Object->vkDevice, &allocInfo, &descriptor);
        mutex.unlock();
    }
    void EWEDescriptorPool::AllocateDescriptor(DescriptorSetLayout* eDSL, VkDescriptorSet& descriptor) {

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.pNext = nullptr;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.pSetLayouts = &eDSL->vkDSL;
        allocInfo.descriptorSetCount = 1;

        mutex.lock();
        activeDescriptors++;
        for (int i = 0; i < eDSL->bindings.size(); i++) {
            //if (setLayout->bindings[i].descriptorCount != 1) {
                //printf("\t count:%d\n", setLayout.bindings.at(i).descriptorCount);
            //}
            AddDescriptorToTrackers(eDSL->bindings.at(i).descriptorType, eDSL->bindings[i].descriptorCount);
        }
        EWE_VK(vkAllocateDescriptorSets, VK::Object->vkDevice, &allocInfo, &descriptor);
        mutex.unlock();
    }
    void EWEDescriptorPool::FreeDescriptors(DescriptorPool_ID poolID, DescriptorSetLayout* eDSL, std::vector<VkDescriptorSet>& descriptors) {
        pools.at(poolID).FreeDescriptors(eDSL, descriptors);
        //printf("active descriptors after removal : %d \n", activeDescriptors);
    }
    void EWEDescriptorPool::FreeDescriptors(DescriptorSetLayout* eDSL, std::vector<VkDescriptorSet>& descriptors) {
        mutex.lock();

        for (auto& dt : eDSL->bindings) {
            trackers.at(dt.descriptorType).RemoveDescriptor(static_cast<uint32_t>(descriptors.size()));
        }
#if DESCRIPTOR_TRACING
        for (auto& desc : descriptors) {
            assert(descTracer.contains(desc));
            descTracer.erase(desc);
        }
#endif

        EWE_VK(vkFreeDescriptorSets,
            VK::Object->vkDevice,
            descriptorPool,
            static_cast<uint32_t>(descriptors.size()),
            descriptors.data());

        activeDescriptors -= descriptors.size();
        mutex.unlock();
        //printf("active descriptors after removal : %d \n", activeDescriptors);
    }
    void EWEDescriptorPool::FreeDescriptor(DescriptorPool_ID poolID, DescriptorSetLayout* eDSL, const VkDescriptorSet* descriptor) {
        pools.at(poolID).FreeDescriptor(eDSL, descriptor);
    }
    void EWEDescriptorPool::FreeDescriptor(DescriptorSetLayout* eDSL, const VkDescriptorSet* descriptor) {
        mutex.lock();
        for (auto& dt : eDSL->bindings) {
            trackers.at(dt.descriptorType).RemoveDescriptor(1);
        }
#if DESCRIPTOR_TRACING
        assert(descTracer.erase(*descriptor) > 0);
#endif
        EWE_VK(vkFreeDescriptorSets,
            VK::Object->vkDevice,
            descriptorPool,
            1,
            descriptor
        );
        activeDescriptors--;
        mutex.unlock();
        //printf("active descriptors after removal : %d \n", activeDescriptors);
    }

    void EWEDescriptorPool::FreeDescriptorWithoutTracker(DescriptorPool_ID poolID, const VkDescriptorSet* descriptor) {
        pools.at(poolID).FreeDescriptorWithoutTracker(descriptor);
    }
    void EWEDescriptorPool::FreeDescriptorWithoutTracker(const VkDescriptorSet* descriptor) {
        mutex.lock();
        EWE_VK(vkFreeDescriptorSets,
            VK::Object->vkDevice,
            descriptorPool,
            1,
            descriptor
        );
        activeDescriptors--;
        mutex.unlock();
        //printf("active descriptors after removal : %d \n", activeDescriptors);
    }
    void EWEDescriptorPool::ResetPool() {
        EWE_VK(vkResetDescriptorPool, VK::Object->vkDevice, descriptorPool, 0);
    }
    void EWEDescriptorPool::BuildGlobalPool() {
        uint32_t maxSets = 10000;
        std::vector<VkDescriptorPoolSize> poolSizes{};
        VkDescriptorPoolSize poolSize;
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = 10000;
        poolSizes.emplace_back(poolSize);
        poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSizes.emplace_back(poolSize);
        poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes.emplace_back(poolSize);
        poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        poolSizes.emplace_back(poolSize);
        VkDescriptorPoolCreateFlags poolFlags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT | VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

        EWEDescriptorPool::pools.try_emplace(0, maxSets, poolFlags, poolSizes);
        //EWEDescriptorPool(EWEDevice & eweDevice, uint32_t maxSets,VkDescriptorPoolCreateFlags poolFlags,const std::vector<VkDescriptorPoolSize>&poolSizes);
    }
    void EWEDescriptorPool::AddPool(DescriptorPool_ID poolID, VkDescriptorPoolCreateInfo& pool_info) {
        EWEDescriptorPool::pools.try_emplace(poolID, pool_info);
    }

    void EWEDescriptorPool::DestructPools() {
		pools.clear();
	}
    void EWEDescriptorPool::DestructPool(DescriptorPool_ID poolID) {
#if EWE_DEBUG
        printf("deconstructing pool : %d \n", poolID);
        assert(pools.contains(poolID) && "destructing pool that doesn't exist");
#endif
        pools.erase(poolID);
    }

    bool EWEDescriptorPool::DescriptorTracker::AddDescriptor(uint32_t count) {
        current += count;
        return current >= max;
    }
    void EWEDescriptorPool::DescriptorTracker::RemoveDescriptor(uint32_t count) {
        assert(count <= current);
        current -= count;
    }

    void EWEDescriptorPool::AddDescriptorToTrackers(VkDescriptorType descType, uint32_t count) {
        if (trackers.at(descType).AddDescriptor(count)) {

#if DESCRIPTOR_TRACING

            for (auto& dt : descTracer) {
                for (auto& st : dt.second) {
                    printf("\ndescription : %s\n", st.description.c_str());
                    printf("\tsource file : %s\n", st.source_file.c_str());
                    printf("\tsource line : %d\n", st.source_line);
                }
            }
#endif
            assert(false);
        }
    }
    VkDescriptorPool EWEDescriptorPool::GetPool(DescriptorPool_ID poolID) {
        return pools.at(poolID).descriptorPool;
    }

    // *************** Descriptor Writer *********************

    EWEDescriptorWriter::EWEDescriptorWriter(DescriptorSetLayout* setLayout, EWEDescriptorPool& pool)
        : setLayout{ setLayout }, pool{ pool } {}
    EWEDescriptorWriter::EWEDescriptorWriter(DescriptorSetLayout* setLayout, DescriptorPool_ID poolID) 
        : setLayout{ setLayout }, pool{ EWEDescriptorPool::pools.at(poolID) }
    {}

    EWEDescriptorWriter& EWEDescriptorWriter::WriteBuffer(VkDescriptorBufferInfo* bufferInfo) {
        auto& bindingDescription = setLayout->bindings[writes.size()];

        assert((setLayout->bindings.size() > writes.size()) && "Layout does not contain specified binding");
        assert(bindingDescription.descriptorCount == 1 && "Binding single descriptor info, but binding expects multiple");

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext = nullptr;
        write.descriptorType = bindingDescription.descriptorType;
        write.dstBinding = static_cast<uint32_t>(writes.size());
        write.pBufferInfo = bufferInfo;
        write.descriptorCount = 1;

        writes.push_back(write);
        return *this;
    }
    EWEDescriptorWriter& EWEDescriptorWriter::WriteImage(VkDescriptorImageInfo* imgInfo) {
#if EWE_DEBUG
        assert((setLayout->bindings.size() > writes.size()) && "Layout does not contain specified binding");
#endif
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext = nullptr;
        write.descriptorType = setLayout->bindings[writes.size()].descriptorType;
        write.dstBinding = static_cast<uint32_t>(writes.size());
        write.pImageInfo = imgInfo;
        write.descriptorCount = 1;
        write.dstArrayElement = 0;

        writes.push_back(write);
        return *this;
    }

    EWEDescriptorWriter& EWEDescriptorWriter::WriteImage(ImageID imageID) {
#if EWE_DEBUG
        assert((setLayout->bindings.size() > writes.size()) && "Layout does not contain specified binding");
#endif

        VkDescriptorImageInfo* imgInfo = Image_Manager::GetDescriptorImageInfo(imageID);
#if DESCRIPTOR_IMAGE_IMPLICIT_SYNCHRONIZATION
        auto& bindingDescription = setLayout->bindings[writes.size()];

        assert(bindingDescription.descriptorCount == 1 && "Binding single descriptor info, but binding expects multiple");

        if (bindingDescription.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {

            while (imgInfo->imageLayout != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {

                //microsleep to explicitly give up thread control to other system processes
                std::this_thread::sleep_for(std::chrono::nanoseconds(1));
            }
        }
        else {
            assert(false && "descriptor type not yet supported");
        }
#endif
        return WriteImage(imgInfo);

    }

    VkDescriptorSet EWEDescriptorWriter::Build() {
#if DESCRIPTOR_DEBUGGING
        return BuildPrint();
#else
        pool.allocateDescriptor(setLayout, set);
        Overwrite(set);
        return set;
#endif
    }
    VkDescriptorSet EWEDescriptorWriter::BuildPrint() {
        VkDescriptorSet set;
        pool.AllocateDescriptor(setLayout, set);
#if DESCRIPTOR_TRACING

        auto curr = std::stacktrace::current();
        auto& empRet = pool.descTracer.try_emplace(set, std::vector<EWEDescriptorPool::FakeEntry>()).first->second;
        auto iter = curr.begin();
        uint8_t i = 0;
        for (auto iter = curr.begin() + 2; iter != curr.end(); iter++) {
            empRet.emplace_back(iter->description(), iter->source_file(), iter->source_line());
            i++;
            if (i >= 7) {
                break;
            }
        }
#endif
        //printf("active descriptors after addition : %d \n", activeDescriptors);
        Overwrite(set);
        return set;
    }

    void EWEDescriptorWriter::Overwrite(VkDescriptorSet& set) {
        for (auto& write : writes) {
            write.dstSet = set;
        }

        EWE_VK(vkUpdateDescriptorSets, VK::Object->vkDevice, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }
}  // namespace EWE