
#include <EWEngine/imgui/imGuiHandler.h>

#include <EWGraphics/Vulkan/Pipeline.h>
#include <EWGraphics/Data/ThreadPool.h>

void check_vk_result(VkResult err) {
	if (err == 0) {
		return;
	}
	fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
	if (err < 0) {
		abort();
	}
}

namespace EWE {
	ImGUIHandler::ImGUIHandler(GLFWwindow* window, uint32_t imageCount) {
		//printf("imgui handler constructor \n");

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		ImGui::StyleColorsDark();

		//descriptorPool
		createDescriptorPool();

		ImGui_ImplGlfw_InitForVulkan(window, true);
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = VK::Object->instance;
		init_info.PhysicalDevice = VK::Object->physicalDevice;
		init_info.Device = VK::Object->vkDevice;
		init_info.QueueFamily = VK::Object->queueIndex[Queue::graphics];
		init_info.Queue = VK::Object->queues[Queue::graphics];
		init_info.PipelineCache = nullptr;
		init_info.Allocator = nullptr;
		init_info.MinImageCount = imageCount;
		init_info.ImageCount = imageCount;
		init_info.CheckVkResultFn = check_vk_result;
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		init_info.PipelineRenderingCreateInfo = *EWEPipeline::PipelineConfigInfo::pipelineRenderingInfoStatic;
		

		//if an issue, address this first
		init_info.UseDynamicRendering = true;

		ImGui_ImplVulkan_Init(&init_info);

		ImPlot::CreateContext();
		ImNodes::CreateContext();



		
		//uploadFonts();
		//printf("end of imgui constructor \n");
	}
	ImGUIHandler::~ImGUIHandler() {

		ImNodes::DestroyContext();
		ImPlot::DestroyContext();
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
		EWEDescriptorPool::DestructPool(DescriptorPool_imgui);
		printf("imguihandler deconstructed \n");
	}

	void ImGUIHandler::beginRender() {
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}
	void ImGUIHandler::endRender() {
		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), VK::Object->GetVKCommandBufferDirect());
	}


	void ImGUIHandler::createDescriptorPool() {
		VkDescriptorPoolSize pool_sizes[] = {
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};
		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.pNext = nullptr;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		pool_info.poolSizeCount = static_cast<uint32_t>(IM_ARRAYSIZE(pool_sizes));
		pool_info.maxSets = 0;
		for (uint32_t i = 0; i < pool_info.poolSizeCount; i++) {
			pool_info.maxSets += pool_sizes[i].descriptorCount;
		}
		//pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
		pool_info.pPoolSizes = pool_sizes;
		EWEDescriptorPool::AddPool(DescriptorPool_imgui, pool_info);
	}

	void ImGUIHandler::InitializeRenderGraph() {
		scrollingBuffers.resize(2);
	}
	void ImGUIHandler::AddEngineGraph() {
		auto taskDetails = &ThreadPool::GetThreadTasks();
	}


	void ImGUIHandler::AddRenderData(float x, float y) {
		assert(scrollingBuffers.size() > 0);
		scrollingBuffers[0].AddPoint(x, y);
	}
	void ImGUIHandler::AddLogicData(float x, float y) {
		assert(scrollingBuffers.size() > 1);
		scrollingBuffers[1].AddPoint(x, y);
	}
	void ImGUIHandler::ClearRenderGraphs() {
		scrollingBuffers.clear();
	}

}
