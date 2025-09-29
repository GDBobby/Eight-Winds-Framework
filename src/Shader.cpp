#include "EWGraphics/Vulkan/Shader.h"


#include "EWGraphics/Data/magic_enum.hpp"
#include "spirvcross/spirv_reflect.hpp"
#if PIPELINE_HOT_RELOAD
#include "EWGraphics/imgui/imgui.h"


#endif
#include <algorithm>

#include <unordered_map>
#include <cassert>
#define SANITY_CHECK true

#if SANITY_CHECK
#include <string.h>
#endif

#define SPV_REFLECT(func, ...) {auto result = func(__VA_ARGS__); if (result != SPV_REFLECT_RESULT_SUCCESS) {printf("failed to read reflected shader data - %s - %s\n", #func, magic_enum::enum_name(result).data());}}

namespace EWE {

	std::vector<char> ReadShaderFile(std::string_view filepath) {

		//const std::string enginePath = filepath;

		std::ifstream shaderFile{};
		shaderFile.open(filepath.data(), std::ios::binary);
		if (!shaderFile.is_open()) {
			printf("failed ot open shader file : %s\n", filepath.data());
			assert(shaderFile.is_open() && "failed to open shader");
		}
		shaderFile.seekg(0, std::ios::end);
		const std::size_t fileSize = static_cast<std::size_t>(shaderFile.tellg());
		assert(fileSize > 0 && "shader is empty");

		shaderFile.seekg(0, std::ios::beg);
		std::vector<char> returnVec(fileSize);
		shaderFile.read(returnVec.data(), fileSize);
		shaderFile.close();
		return returnVec;
	}


	void AddBinding(spirv_cross::Compiler const& compiler, DescriptorLayoutPack* layoutPack, spirv_cross::Resource const& res, VkDescriptorType descType, VkShaderStageFlagBits stageFlag) {

		const uint32_t setIndex = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
		if(!layoutPack->setLayouts.Contains(setIndex)) {
			layoutPack->setLayouts.push_back(setIndex, Construct<DescriptorSetLayout>());
		}
		uint32_t descCount = 1;
		auto const& type = compiler.get_type(res.type_id);
		if (!type.array.empty() && type.array[0] != 0) {
			descCount = type.array[0];
		}
		compiler.get_name(res.id);
		

		layoutPack->setLayouts.at(setIndex).value->bindings.push_back(
			VkDescriptorSetLayoutBinding{
				.binding = compiler.get_decoration(res.id, spv::DecorationBinding),
				.descriptorType = descType,
				.descriptorCount = descCount,
				.stageFlags = VK_SHADER_STAGE_ALL,//bad but going with it for now
				.pImmutableSamplers = nullptr,
			}
		);
	}


	DescriptorLayoutPack* CreateDescriptorLayoutPack(spirv_cross::Compiler const& compiler, VkShaderStageFlagBits stageFlag) {
		auto const& resources = compiler.get_shader_resources();

		DescriptorLayoutPack* ret = Construct<DescriptorLayoutPack>();

#define AddBindingType(vec, descType) for(auto& binding : vec) {AddBinding(compiler, ret, binding, descType, stageFlag);}

		AddBindingType(resources.uniform_buffers, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		AddBindingType(resources.storage_buffers, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		AddBindingType(resources.sampled_images, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		AddBindingType(resources.storage_images, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		AddBindingType(resources.separate_samplers, VK_DESCRIPTOR_TYPE_SAMPLER);
		AddBindingType(resources.separate_images, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);

		for (auto& set : ret->setLayouts) {
			std::sort(set.value->bindings.begin(), set.value->bindings.end(),
				[](VkDescriptorSetLayoutBinding const& a, VkDescriptorSetLayoutBinding const& b) {
					return a.binding < b.binding;
				}
			);
		}


		return ret;
	}
	

	VkFormat SPIRTypeToVkFormat(const spirv_cross::SPIRType& type)
	{
		switch (type.basetype)	{
		case spirv_cross::SPIRType::BaseType::Float:
			switch (type.vecsize) {
				case 1: return VK_FORMAT_R32_SFLOAT;
				case 2: return VK_FORMAT_R32G32_SFLOAT;
				case 3: return VK_FORMAT_R32G32B32_SFLOAT;
				case 4: return VK_FORMAT_R32G32B32A32_SFLOAT;
			}
			break;
		case spirv_cross::SPIRType::BaseType::Int:
			switch (type.vecsize) {
				case 1: return VK_FORMAT_R32_SINT;
				case 2: return VK_FORMAT_R32G32_SINT;
				case 3: return VK_FORMAT_R32G32B32_SINT;
				case 4: return VK_FORMAT_R32G32B32A32_SINT;
			}
			break;
		case spirv_cross::SPIRType::BaseType::UInt:
			switch (type.vecsize) {
				case 1: return VK_FORMAT_R32_UINT;
				case 2: return VK_FORMAT_R32G32_UINT;
				case 3: return VK_FORMAT_R32G32B32_UINT;
				case 4: return VK_FORMAT_R32G32B32A32_UINT;
			}
			break;
		default: break;
		}
		EWE_UNREACHABLE;
		return VK_FORMAT_UNDEFINED;
	}

	void InterpretInputAttributes(spirv_cross::Compiler const& compiler, std::vector<VkVertexInputAttributeDescription>& vk_attributes) {

		auto const& stage_inputs = compiler.get_shader_resources().stage_inputs;
		std::vector<std::pair<uint8_t, spirv_cross::SPIRType>> input_types{};
		for (auto const& input : stage_inputs) {
			VkVertexInputAttributeDescription& backDesc = vk_attributes.emplace_back();
			backDesc.location = compiler.get_decoration(input.id, spv::DecorationLocation);
			backDesc.binding = 0;
			backDesc.format = SPIRTypeToVkFormat(compiler.get_type(input.type_id)); //this throws if its a struct
			backDesc.offset = 0;

			input_types.emplace_back(backDesc.location, compiler.get_type(input.type_id));
		}    
		std::sort(vk_attributes.begin(), vk_attributes.end(),
			[](const VkVertexInputAttributeDescription& a,
				const VkVertexInputAttributeDescription& b) {
					return a.location < b.location;
			}
		);
		std::sort(input_types.begin(), input_types.end(),
			[](const std::pair<uint8_t, spirv_cross::SPIRType>& a,
				const std::pair<uint8_t, spirv_cross::SPIRType>& b) {
					return a.first < b.first;
			}
		);

		uint32_t offset = 0;
		for (uint8_t i = 0; i < vk_attributes.size(); i++) {
			vk_attributes[i].offset = offset;
			offset += input_types[i].second.width / 8 * input_types[i].second.vecsize; //width is in bits /=8 for bytes
		}
	}

	bool Shader::ValidateVertexInputAttributes(std::vector<VkVertexInputAttributeDescription> const& cpu_side) const {
		if (cpu_side.size() != vertexInputAttributes.size()) {
			return false;
		}
		for (uint8_t i = 0; i < vertexInputAttributes.size(); i++) {
			bool mismatched = false;
			mismatched |= cpu_side[i].binding != vertexInputAttributes[i].binding;
			mismatched |= cpu_side[i].format != vertexInputAttributes[i].format;
			mismatched |= cpu_side[i].location != vertexInputAttributes[i].location;
			mismatched |= cpu_side[i].offset != vertexInputAttributes[i].offset;
			if (mismatched) {
				return false;
			}
		}
		return true;
	}

	void InterpretPushConstants(spirv_cross::Compiler const& compiler, spirv_cross::SmallVector<spirv_cross::Resource> const& pushResource, VkPushConstantRange& pushRange) {
		if (pushResource.size() == 0) {
			return;
		}
		//only supporting 1 push range rn
		auto& reflectedPush = pushResource[0];
		const auto& pushReflectedType = compiler.get_type(pushResource[0].base_type_id);
		pushRange.size = static_cast<uint32_t>(compiler.get_declared_struct_size(pushReflectedType));
		if (pushRange.size > 0) {
			//pushRange.offset = pushReflectedType.member_types[0];
		}
		//else {
		pushRange.offset = 0;
		//}
	}

	void ProcessEntry(spirv_cross::Compiler const& compiler, spirv_cross::SpecializationConstant const& sc, std::vector<Shader::SpecializationEntry>& specConstants) {
		if (!sc.id) return;
		auto& entry = specConstants.emplace_back();
		auto const& constant = compiler.get_constant(sc.id);
#if PIPELINE_HOT_RELOAD
		entry.name = compiler.get_name(sc.id);
		auto const& name = entry.name;
#else
		auto const& name = compiler.get_name(sc.id);
#endif
		if (name == "") {
			specConstants.pop_back();
			return;
		}

		auto const& type = compiler.get_type(constant.constant_type);
		entry.elementCount = type.array.size();
		if (entry.elementCount == 0) {
			entry.elementCount = 1;
		}
		entry.constantID = sc.constant_id;
		switch (type.basetype) {
			case spirv_cross::SPIRType::Boolean:
				entry.type = Shader::ST_BOOL;
				break;
			case spirv_cross::SPIRType::Int:
				entry.type = Shader::ST_INT;
				break;
			case spirv_cross::SPIRType::UInt:
				entry.type = Shader::ST_UINT;
				break;
			case spirv_cross::SPIRType::Float:
				entry.type = Shader::ST_FLOAT;
				break;
			default:
				EWE_UNREACHABLE;
				entry.type = Shader::ST_COUNT;
				break;
		}

		auto const& val = constant.scalar();
		memcpy(entry.value, &val, sizeof(float) * entry.elementCount); //the element size is always 4
	}

	void InterpretSpecializationConstants(spirv_cross::Compiler const& compiler, std::vector<Shader::SpecializationEntry>& specConstants) {
		auto spec_constants = compiler.get_specialization_constants();
		//potentially do some kind of sorting after figuring out which 1s are the local group size constants
		//spirv_cross::SpecializationConstant x;
		//spirv_cross::SpecializationConstant y;
		//spirv_cross::SpecializationConstant z;
		//compiler.get_work_group_size_specialization_constants(x, y, z);
		//ProcessEntry(compiler, x, specConstants);
		//ProcessEntry(compiler, y, specConstants);
		//ProcessEntry(compiler, z, specConstants);


		for (auto& sc : spec_constants) {
			ProcessEntry(compiler, sc, specConstants);
		}
	}
	void Shader::ReadReflection(const std::size_t dataSize, const void* data) {

		spirv_cross::Compiler compiler(reinterpret_cast<const uint32_t*>(data), dataSize / sizeof(uint32_t));
		auto entryPoints = compiler.get_entry_points_and_stages();
		for (auto& entryPoint : entryPoints) {
			std::string name = entryPoint.name;
			switch (entryPoint.execution_model)	{
				case spv::ExecutionModelVertex:   shaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT; break;
				case spv::ExecutionModelFragment: shaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT; break;
				case spv::ExecutionModelGLCompute:shaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT; break;
				case spv::ExecutionModelTaskEXT:  shaderStageCreateInfo.stage = VK_SHADER_STAGE_TASK_BIT_EXT; break;
				case spv::ExecutionModelMeshEXT:  shaderStageCreateInfo.stage = VK_SHADER_STAGE_MESH_BIT_EXT; break;
				default: EWE_UNREACHABLE; break;
			}
		}
		auto resources = compiler.get_shader_resources();
		InterpretPushConstants(compiler, resources.push_constant_buffers, pushRange);
		pushRange.stageFlags = shaderStageCreateInfo.stage;
		//InterpretInputAttributes(compiler, vertexInputAttributes);

		shaderStageCreateInfo.pName = "main";
		shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageCreateInfo.pNext = nullptr;
		shaderStageCreateInfo.pSpecializationInfo = nullptr;
		shaderStageCreateInfo.flags = 0;
		
		descriptorSets = CreateDescriptorLayoutPack(compiler, shaderStageCreateInfo.stage);

		defaultSpecConstants.clear();
		InterpretSpecializationConstants(compiler, defaultSpecConstants);
	

	}

	void Shader::CompileModule(const std::size_t dataSize, const void* data) {
		VkShaderModuleCreateInfo shaderCreateInfo{};
		shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderCreateInfo.pNext = nullptr;
		shaderCreateInfo.codeSize = dataSize;
		shaderCreateInfo.pCode = reinterpret_cast<const uint32_t*>(data);
		shaderCreateInfo.flags = 0;
		EWE_VK(vkCreateShaderModule, VK::Object->vkDevice, &shaderCreateInfo, nullptr, &shaderStageCreateInfo.module);

#if DEBUG_NAMING
		DebugNaming::SetObjectName(shaderStageCreateInfo.module, VK_OBJECT_TYPE_SHADER_MODULE, filepath.data());
		//for (auto& dsl : reflectedData.descriptorSets->setLayouts) {
		//	std::string debug_name = std::string(fileLocation.data()) + std::string(" - dsl#") + std::to_string(dsl.first);
		//	DebugNaming::SetObjectName(dsl.second->vkDSL, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, debug_name.c_str());
		//}
#endif
	}


	Shader::Shader(std::string_view fileLocation) : filepath{ fileLocation.data() }, descriptorSets{ nullptr } {
		std::vector<char> shaderData = ReadShaderFile(fileLocation);
		ReadReflection(shaderData.size(), shaderData.data());
		CompileModule(shaderData.size(), shaderData.data());
	}
	Shader::Shader(std::string_view fileLocation, const std::size_t dataSize, const void* data) : filepath{ fileLocation.data() }, descriptorSets{ nullptr } {
		ReadReflection(dataSize, data);
		CompileModule(dataSize, data);
	}


	Shader::Shader() : filepath{} {} //idk how to initialize shaderstageinfo yet
	Shader::~Shader() {
		if (shaderStageCreateInfo.module != VK_NULL_HANDLE) {
			EWE_VK(vkDestroyShaderModule, VK::Object->vkDevice, shaderStageCreateInfo.module, nullptr);
		}
		for (auto& staleModule : staleModules) {
			if (staleModule != VK_NULL_HANDLE) {
				EWE_VK(vkDestroyShaderModule, VK::Object->vkDevice, staleModule, nullptr);
			}
		}
		staleModules.clear();
		if (descriptorSets) {
			Deconstruct(descriptorSets);
		}
	}

	void Shader::HotReload() {
		staleModules.push_back(shaderStageCreateInfo.module);
		shaderStageCreateInfo.module = VK_NULL_HANDLE;
		if (descriptorSets != nullptr) {
			Deconstruct(descriptorSets);
		}
		descriptorSets = nullptr;
		std::vector<char> shaderData = ReadShaderFile(filepath);
		CompileModule(shaderData.size(), shaderData.data());
		ReadReflection(shaderData.size(), shaderData.data());
	}

	void Shader::DrawImgui() {

	}

	Shader::VkSpecInfo_RAII::VkSpecInfo_RAII(std::vector<Shader::SpecializationEntry> const& entries) 
		: mapEntries(entries.size())
	{
		specInfo.dataSize = 0;
		for (uint8_t i = 0; i < mapEntries.size(); i++) {
			VkSpecializationMapEntry& mapEntry = mapEntries[i];
			mapEntry.constantID = entries[i].constantID;
			mapEntry.offset = specInfo.dataSize;
			mapEntry.size = entries[i].elementCount * sizeof(float);
			specInfo.dataSize += mapEntry.size;
		}

		specInfo.mapEntryCount = static_cast<uint32_t>(mapEntries.size());
		specInfo.pMapEntries = mapEntries.data();
		memPtr = reinterpret_cast<uint64_t>(malloc(specInfo.dataSize));

		std::size_t offset = 0;
		for (uint8_t i = 0; i < mapEntries.size(); i++) {
			const std::size_t localSize = sizeof(float) * entries[i].elementCount;
			memcpy(reinterpret_cast<void*>(memPtr + offset), entries[i].value, localSize);
			offset += localSize;
		}
		specInfo.pData = reinterpret_cast<void*>(memPtr);

	}

	Shader::VkSpecInfo_RAII::VkSpecInfo_RAII(Shader::VkSpecInfo_RAII&& move) noexcept
		: specInfo{ move.specInfo },
		memPtr{ move.memPtr },
		mapEntries{ std::move(move.mapEntries) }
	{
		move.memPtr = 0;
	}

	Shader::VkSpecInfo_RAII::VkSpecInfo_RAII(VkSpecInfo_RAII const& copy)
		: specInfo{ copy.specInfo },
		mapEntries{copy.mapEntries.begin(), copy.mapEntries.end() } //this is a copy
	{
		memPtr = reinterpret_cast<uint64_t>(malloc(specInfo.dataSize));
		specInfo.mapEntryCount = static_cast<uint32_t>(mapEntries.size());
		specInfo.pMapEntries = mapEntries.data();
		specInfo.pData = reinterpret_cast<void*>(memPtr);
		memcpy(reinterpret_cast<void*>(memPtr), copy.specInfo.pData, specInfo.dataSize);
	}

	Shader::VkSpecInfo_RAII::~VkSpecInfo_RAII() {
		if (memPtr != 0) {
			free(reinterpret_cast<void*>(memPtr));
		}
	}

	struct ShaderModuleTracker {
		Shader* shader;
		
		int16_t usageCount;
		template<class... Args>
		requires (std::is_constructible_v<Shader, Args...>)
		explicit ShaderModuleTracker(std::in_place_t, Args&&... args) : shader(Construct<Shader>(std::forward<Args>(args)...)), usageCount(1) {}
		explicit ShaderModuleTracker(Shader* shader) : shader{ shader }, usageCount{ 1 } {}
	};
	struct StringHash {
		using is_transparent = void;
		std::size_t operator()(std::string_view sv) const noexcept {
			return std::hash<std::string_view>{}(sv);
		}
	};

	struct StringEqual {
		using is_transparent = void;
		bool operator()(std::string_view a, std::string_view b) const noexcept {
			return a == b;
		}
	};

	//this hash isnt working correctly
	std::unordered_map<std::string, ShaderModuleTracker, StringHash, StringEqual> shaderModuleMap;
	std::mutex shaderMapMutex;


	Shader* GetShaderIfExist(std::string const& path) {
		auto modFind = shaderModuleMap.find(path);
		if (modFind == shaderModuleMap.end()) {
			return nullptr;
		}
		else {
			std::unique_lock<std::mutex> uniqLock{ shaderMapMutex };
			modFind->second.usageCount++;
			return modFind->second.shader;
		}
	}

	Shader* GetShader(std::string_view filepath) {
		auto modFind = shaderModuleMap.find(filepath);
		if (modFind == shaderModuleMap.end()) {
			auto empRet = shaderModuleMap.emplace(filepath, Construct<Shader>(filepath));
			assert(empRet.second);
			return empRet.first->second.shader;
		}
		else {
			shaderMapMutex.lock();
			modFind->second.usageCount++;
			return modFind->second.shader;
		}
	}
	Shader* CreateShader(std::string_view filepath, const std::size_t dataSize, const void* data) {
		assert(shaderModuleMap.find(filepath) == shaderModuleMap.end());

		auto empRet = shaderModuleMap.emplace(filepath, Construct<Shader>(filepath, dataSize, data));
		assert(empRet.second);
		return empRet.first->second.shader;
	}

	void DestroyShader(Shader& shader) {
		if (shader.shaderStageCreateInfo.module != VK_NULL_HANDLE) {
			shaderMapMutex.lock();
			
			auto findRet = shaderModuleMap.find(shader.filepath);
			if (findRet == shaderModuleMap.end()) {
#if EWE_DEBUG
				printf("trying to delete a shader that's not in the shader moduel map\n");
#endif
			}
			else {
				findRet->second.usageCount--;
				if (findRet->second.usageCount <= 0) {
					EWE_VK(vkDestroyShaderModule, VK::Object->vkDevice, shader.shaderStageCreateInfo.module, nullptr);
					shaderModuleMap.erase(findRet);
				}
			}

			shaderMapMutex.unlock();
#if PIPELINE_HOT_RELOAD
			return;
#endif
			EWE_UNREACHABLE;
		}
	}

	void DestroyAllShaders() {
		shaderMapMutex.lock();
		for (auto iter = shaderModuleMap.begin(); iter != shaderModuleMap.end(); iter++) {
			EWE_VK(vkDestroyShaderModule, VK::Object->vkDevice, iter->second.shader->shaderStageCreateInfo.module, nullptr);
		}
		shaderModuleMap.clear();
		shaderMapMutex.unlock();
	}




//	void ShaderGroup::Validate() const {
//#if EWE_DEBUG
//		bool hasNormalPipeline = false;
//		hasNormalPipeline |= shaderData[ShaderStage::Vertex].filepath.size() > 0;
//		hasNormalPipeline |= shaderData[ShaderStage::Geometry].filepath.size() > 0;
//		hasNormalPipeline |= shaderData[ShaderStage::tessControl].filepath.size() > 0;
//		hasNormalPipeline |= shaderData[ShaderStage::tessEval].filepath.size() > 0;
//
//		bool hasMeshPipeline = false;
//		hasMeshPipeline |= shaderData[ShaderStage::task].filepath.size() > 0;
//		hasMeshPipeline |= shaderData[ShaderStage::mesh].filepath.size() > 0;
//
//		bool hasRaytracingPipelines = false;
//
//		assert((hasMeshPipeline + hasNormalPipeline + hasRaytracingPipelines) == 1);
//		for (uint8_t i = 0; i < ShaderStage::COUNT; i++) {
//			if ((shaderData[i].shader != VK_NULL_HANDLE) && (shaderData[i].filepath.size() == 0)) {
//				printf("this shader won't be tracked\n");
//			}
//		}
//#endif
//	}

	//void ShaderGroup::RenderIMGUI() {
	//	static int currentImguiIndex = 0;
	//	if (imguiIndex < 0) {
	//		imguiIndex = currentImguiIndex;
	//		currentImguiIndex++;
	//	}

	//	std::string extension = "##p";
	//	extension += std::to_string(imguiIndex);

	//	std::string treeName = "shaderStrings";
	//	treeName += extension;

	//	const auto str_callback = [](ImGuiInputTextCallbackData* data) -> int {
	//		if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
	//			auto* str = static_cast<std::string*>(data->UserData);
	//			str->resize(data->BufTextLen);
	//			data->Buf = str->data();
	//		}
	//		return 0;
	//		};


	//	if (ImGui::TreeNode(treeName.c_str())) {
	//		for (uint8_t i = 0; i < ShaderStage::COUNT; i++) {
	//			ImGui::InputText(magic_enum::enum_name(static_cast<ShaderStage::Bits>(i)).data(), shaderData[i].filepath.data(), shaderData[i].filepath.capacity() + 1, ImGuiInputTextFlags_CallbackResize, str_callback, &shaderData[i].filepath);
	//		}

	//		ImGui::TreePop();
	//	}
	//}


}