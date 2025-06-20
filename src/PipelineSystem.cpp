#include "EWGraphics/PipelineSystem.h"
#if PIPELINE_HOT_RELOAD
#include "EWGraphics/imgui/imgui.h"
#endif

#include "EWGraphics/Texture/Image_Manager.h"

namespace EWE {
	std::unordered_map<PipelineID, PipelineSystem*> PipelineSystem::pipelineSystem{};
#if PIPELINE_HOT_RELOAD
	std::unordered_map<PipelineID, std::string> pipelineNames{};
#endif
#if EWE_DEBUG
	PipelineID PipelineSystem::currentPipe;
#endif

#if PIPELINE_HOT_RELOAD
	void PipelineSystem::Emplace(std::string const& pipeName, PipelineID pipeID, PipelineSystem* pipeSys) {
		assert(!pipelineSystem.contains(pipeID) && "attempting to emplace a pipe with an existing id");
		pipelineSystem.emplace(pipeID, pipeSys);
		pipelineNames.emplace(pipeID, pipeName);
	}
#else
	void PipelineSystem::Emplace(PipelineID pipeID, PipelineSystem* pipeSys) {
		assert(!pipelineSystem.contains(pipeID) && "attempting to emplace a pipe with an existing id");
		pipelineSystem.emplace(pipeID, pipeSys);
	}
#endif

	void PipelineSystem::Destruct() {

		for (auto iter = pipelineSystem.begin(); iter != pipelineSystem.end(); iter++) {
			EWE_VK(vkDestroyPipelineLayout, VK::Object->vkDevice, iter->second->pipeLayout, nullptr);
			Deconstruct(iter->second);
		}

		pipelineSystem.clear();
	}
	void PipelineSystem::DestructAt(PipelineID pipeID) {
#if PIPELINE_HOT_RELOAD
		pipelineNames.erase(pipeID);
#endif


#if EWE_DEBUG
		auto foundPipe = pipelineSystem.find(pipeID);
		assert(foundPipe != pipelineSystem.end() && "destructing invalid pipe \n");
		EWE_VK(vkDestroyPipelineLayout, VK::Object->vkDevice, foundPipe->second->pipeLayout, nullptr);
		Deconstruct(foundPipe->second);
#else
		auto& pipe = pipelineSystem.at(pipeID);
		EWE_VK(vkDestroyPipelineLayout, VK::Object->vkDevice, pipe->pipeLayout, nullptr);
		Deconstruct(pipe);
#endif

	}

	PipelineSystem* PipelineSystem::At(PipelineID pipeID) {
		auto foundPipe = pipelineSystem.find(pipeID);
		assert(foundPipe != pipelineSystem.end() && "searching invalid pipe \n");
		return foundPipe->second;
	}
	
	void PipelineSystem::BindPipeline() {
#if EWE_DEBUG
		currentPipe = myID;
#endif
#if PIPELINE_HOT_RELOAD
		if (pipe->stalePipeline != VK_NULL_HANDLE) {
			printf("rendering with a hot reloaded pipeline\n");
		}
#endif
		pipe->Bind();
		bindedTexture = VK_NULL_HANDLE;
	}
	void PipelineSystem::BindModel(EWEModel* model) {
		bindedModel = model;
		assert(currentPipe == myID && "pipe id mismatch on model bind");
		bindedModel->Bind();
	}
	void PipelineSystem::BindDescriptor(uint8_t descSlot, VkDescriptorSet* descSet) {
		assert(currentPipe == myID && "pipe id mismatch on desc bind");
		EWE_VK(vkCmdBindDescriptorSets, VK::Object->GetFrameBuffer(),
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipeLayout,
			descSlot, 1,
			descSet, 
			0, nullptr
		);
		
	}

	void PipelineSystem::Push(void* push) {
		EWE_VK(vkCmdPushConstants, VK::Object->GetFrameBuffer(), pipeLayout, pushStageFlags, 0, pushSize, push);
	}

	void PipelineSystem::PushAndDraw(void* push) {
		EWE_VK(vkCmdPushConstants, VK::Object->GetFrameBuffer(), pipeLayout, pushStageFlags, 0, pushSize, push);
		
		assert(bindedModel != nullptr && "attempting to draw a model while none is binded");
		assert(currentPipe == myID && "pipe id mismatch on model draw");
		bindedModel->Draw();
	}
	void PipelineSystem::DrawModel() {
		assert(bindedModel != nullptr && "attempting to draw a model while none is binded");
		assert(currentPipe == myID && "pipe id mismatch on model draw");
		bindedModel->Draw();
	}
	void PipelineSystem::DrawInstanced(EWEModel* model) {
		model->BindAndDrawInstance();
	}

#if PIPELINE_HOT_RELOAD
	void PipelineSystem::RenderPipelinesIMGUI() {

		if (ImGui::Begin("Pipeline System")) {

			uint16_t pipeIndex = 0;
			std::string extension{};
			for (auto& pipeName : pipelineNames) {
				if (ImGui::TreeNode(pipeName.second.c_str())) {
					auto* pipe = pipelineSystem.at(pipeName.first);

					if (pipe->pipe->stalePipeline != VK_NULL_HANDLE) {
						pipe->pipe->framesSinceSwap++;
						printf("ticking stale pipeline for destruction - %d : %s\n", pipe->pipe->framesSinceSwap, pipeName.second.c_str());
						if (pipe->pipe->framesSinceSwap > (MAX_FRAMES_IN_FLIGHT * 2 + 1)) {
						//if (pipe->pipe->framesSinceSwap > 200) {
							EWE_VK(vkDestroyPipeline, VK::Object->vkDevice, pipe->pipe->stalePipeline, nullptr);

							printf("destroying stale pipeline, sleeping\n");
							//std::this_thread::sleep_for(std::chrono::seconds(2));
							pipe->pipe->stalePipeline = VK_NULL_HANDLE;
							pipe->pipe->framesSinceSwap = 0;
						}
					}
					else {
						extension = "reload##ps";
						extension += std::to_string(pipeIndex);
						if (ImGui::Button(extension.c_str())) {
							//holdingReloadPipe = pipe->pipe;
							//pipe->pipe = Construct<EWEPipeline>({ holdingReloadPipe->copyStringStruct, holdingReloadPipe->copyConfigInfo });
							//holdingReloadPipe = Construct<EWEPipeline>({ pipe->pipe->copyStringStruct, pipe->pipe->copyConfigInfo });
							pipe->pipe->HotReloadPipeline(false);
						}
						extension = "reload shaders##ps";
						extension += std::to_string(pipeIndex);
						if (ImGui::Button(extension.c_str())) {
							//holdingReloadPipe = pipe->pipe;
							//pipe->pipe = Construct<EWEPipeline>({ holdingReloadPipe->copyStringStruct, holdingReloadPipe->copyConfigInfo });
							//holdingReloadPipe = Construct<EWEPipeline>({ pipe->pipe->copyStringStruct, pipe->pipe->copyConfigInfo });
							pipe->pipe->HotReloadPipeline(true);
						}
						extension = "recompile shaders ( not ready yet) ##ps";
						extension += std::to_string(pipeIndex);
						
					}
					pipe->pipe->shaderModules.RenderIMGUI();
					pipe->pipe->copyConfigInfo.RenderIMGUI();
					ImGui::TreePop();
				}

				pipeIndex++;
			}

		}
		ImGui::End();
	}
#endif

}