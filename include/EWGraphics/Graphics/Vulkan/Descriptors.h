#pragma once

#include "Device.hpp"

// std
#include <memory>
#include <unordered_map>
#include <vector>
#if DESCRIPTOR_TRACING
#include <stacktrace>
#endif

namespace EWE {

    static int64_t activeDescriptors = 0;

    class EWEDescriptorSetLayout {
    public:
        class Builder {
        public:
            Builder() {}

            Builder& AddBinding(VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, uint32_t count = 1);
            Builder& AddGlobalBindingForCompute();
            Builder& AddGlobalBindings();
#if EWE_DEBUG
            EWEDescriptorSetLayout* Build(std::source_location = std::source_location::current());
#else
            EWEDescriptorSetLayout* Build();
#endif
        private:
            std::vector<VkDescriptorSetLayoutBinding> bindings{};
            uint8_t currentBindingCount = 0;
        };

        EWEDescriptorSetLayout(std::vector<VkDescriptorSetLayoutBinding>& bindings);
        ~EWEDescriptorSetLayout();
        EWEDescriptorSetLayout(const EWEDescriptorSetLayout&) = delete;
        EWEDescriptorSetLayout& operator=(const EWEDescriptorSetLayout&) = delete;

        [[nodiscard]] VkDescriptorSetLayout* GetDescriptorSetLayout() { return &descriptorSetLayout; }
        std::vector<VkDescriptorSetLayoutBinding> const& GetBindings() { //read only
            return bindings;
        }
    private:
        VkDescriptorSetLayout descriptorSetLayout;
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        friend class EWEDescriptorWriter;
        friend class EWEDescriptorPool;
    };

    enum DescriptorPool_ID : uint16_t {
        DescriptorPool_Global = 0,
        DescriptorPool_imgui = 1,
        //DescriptorPool_Compute = 1, //i dont even think im using this
    };

    class EWEDescriptorPool {
    public:
        class Builder {
        public:
            Builder() {}

            Builder& AddPoolSize(VkDescriptorType descriptorType, uint32_t count);
            Builder& SetPoolFlags(VkDescriptorPoolCreateFlags flags);
            Builder& SetMaxSets(uint32_t count);
#if EWE_DEBUG
            EWEDescriptorPool* Build(std::source_location srcLoc = std::source_location::current()) const;
#else
            EWEDescriptorPool* Build() const;
#endif

        private:
            std::vector<VkDescriptorPoolSize> poolSizes{};
            uint32_t maxSets = 1000;
            VkDescriptorPoolCreateFlags poolFlags = 0;
        };

        EWEDescriptorPool(uint32_t maxSets, VkDescriptorPoolCreateFlags poolFlags, const std::vector<VkDescriptorPoolSize>& poolSizes);
        EWEDescriptorPool(VkDescriptorPoolCreateInfo& pool_info);
        ~EWEDescriptorPool();
        EWEDescriptorPool(const EWEDescriptorPool&) = delete;
        EWEDescriptorPool& operator=(const EWEDescriptorPool&) = delete;

        static void AllocateDescriptor(DescriptorPool_ID poolID, const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor);
        void AllocateDescriptor(const VkDescriptorSetLayout* descriptorSetLayout, VkDescriptorSet& descriptor);
        void AllocateDescriptor(EWEDescriptorSetLayout* eDSL, VkDescriptorSet& descriptor);

        static void FreeDescriptors(DescriptorPool_ID poolID, EWEDescriptorSetLayout* eDSL, std::vector<VkDescriptorSet>& descriptors);
        void FreeDescriptors(EWEDescriptorSetLayout* eDSL, std::vector<VkDescriptorSet>& descriptors);
        static void FreeDescriptor(DescriptorPool_ID poolID, EWEDescriptorSetLayout* eDSL, const VkDescriptorSet* descriptors);
        void FreeDescriptor(EWEDescriptorSetLayout* eDSL, const VkDescriptorSet* descriptor);

        static void FreeDescriptorWithoutTracker(DescriptorPool_ID poolID, const VkDescriptorSet* descriptors);
        void FreeDescriptorWithoutTracker(const VkDescriptorSet* descriptor);

        //void getPool(); maybe later for imGuiHandler, not rn

        void ResetPool();
        static void BuildGlobalPool();

        static void DestructPools();
        static void DestructPool(DescriptorPool_ID poolID);
        static void AddPool(DescriptorPool_ID poolID, VkDescriptorPoolCreateInfo& pool_info);

        //only using it to create a descriptor set in textoverlay right now
        static VkDescriptorPool GetPool(DescriptorPool_ID poolID);

    private:
        struct DescriptorTracker {
            uint32_t current = 0;
            uint32_t max;
            std::vector<VkDescriptorSet*> descriptors{};

            DescriptorTracker(uint32_t maxCount) : max{ maxCount } {}

            void RemoveDescriptor(uint32_t count);
            bool AddDescriptor(uint32_t count);
        };
#if DESCRIPTOR_TRACING
        struct FakeEntry {
            std::string description;
            std::string source_file;
            std::uint32_t source_line;
            FakeEntry(std::string const& description, std::string const& source_file, uint32_t source_line) : description{ description }, source_file{ source_file }, source_line{ source_line } {}
        };

        std::unordered_map<VkDescriptorSet, std::vector<FakeEntry>> descTracer{};
#endif

        std::unordered_map<VkDescriptorType, DescriptorTracker> trackers;

        VkDescriptorPool descriptorPool;
        std::mutex mutex{};

        void AddDescriptorToTrackers(VkDescriptorType descType, uint32_t count);


        static std::unordered_map<uint16_t, EWEDescriptorPool> pools;
        friend class EWEDescriptorWriter;
    };

    class EWEDescriptorWriter {
    public:
        EWEDescriptorWriter(EWEDescriptorSetLayout* setLayout, EWEDescriptorPool& pool);
        EWEDescriptorWriter(EWEDescriptorSetLayout* setLayout, DescriptorPool_ID poolID);

        EWEDescriptorWriter& WriteBuffer(VkDescriptorBufferInfo* bufferInfo);
        EWEDescriptorWriter& WriteImage(VkDescriptorImageInfo* imgInfo);
        EWEDescriptorWriter& WriteImage(ImageID imageID);
        VkDescriptorSet Build();
        void Overwrite(VkDescriptorSet& set);

    private:
        VkDescriptorSet BuildPrint();
        EWEDescriptorSetLayout* setLayout;
        EWEDescriptorPool& pool;
        std::vector<VkWriteDescriptorSet> writes;
    };

    

}  // namespace EWE