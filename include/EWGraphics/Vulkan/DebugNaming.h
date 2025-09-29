#pragma once

#include "vulkan/vulkan.h"
#include "EWGraphics/Preprocessor.h"

namespace EWE{
	namespace DebugNaming {
		void QueueBegin(uint8_t queue, float red, float green, float blue, const char* name);
		void QueueEnd(uint8_t queue);

		void BeginLabel(const char* name, float red, float green, float blue);
		void EndLabel();

		void Initialize(bool extension_enabled);
		void Deconstruct();
		void SetObjectName(void* object, VkObjectType objectType, const char* name);
#define SetObjectNameRC(object, objectType, name) SetObjectName(reinterpret_cast<void*>(object), objectType, name)

	} //namespace DebugNaming
} //namespace EWE

