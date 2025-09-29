#pragma once

#include "EWGraphics/PipelineSystem.h"

namespace EWE {
	struct ComputePipeline : public Pipeline {
		ComputePipeline(PipelineID pipeID, PipeLayout* layout);
		ComputePipeline(PipelineID pipeID, PipeLayout* layout, std::vector<Shader::SpecializationEntry> const& specInfo);

		void CreateVkPipeline();
#if PIPELINE_HOT_RELOAD
		void HotReload(bool layoutReload = true) override final;
#endif
	};
}