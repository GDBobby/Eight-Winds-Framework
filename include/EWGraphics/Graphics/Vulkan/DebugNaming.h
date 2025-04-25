#pragma once

#include "vulkan/vulkan.h"
#include "Preprocessor.h"

#if DEBUG_NAMING
namespace EWE{
	namespace DebugNaming {
		void QueueBegin(uint8_t queue, float red, float green, float blue, const char* name);
		void QueueEnd(uint8_t queue);

		void BeginLabel(const char* name, float red, float green, float blue);
		void EndLabel();

		void Initialize(bool extension_enabled);
		void Deconstruct();
		void SetObjectName(void* object, VkObjectType objectType, const char* name);

	} //namespace DebugNaming
} //namespace EWE
#endif

