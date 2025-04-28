#pragma once

#include "EWGraphics/Preprocessor.h"

#include "vulkan/vulkan.h"

#include <stdint.h>
#include <cassert>
#include <string.h>

#include <LAB/Vector.h>
#include <LAB/Matrix.h>
#include <array>

namespace EWE {
	typedef uint16_t MaterialFlags;
	namespace Material {
		namespace Attributes {
			enum Texture : MaterialFlags {
				Bump = 0, //its important bump comes before anything else because its in the vertex shader
				Albedo, //fragment attributes begin here
				Metal,
				Rough,
				Normal,
				AO,
				SIZE
			};
			enum Other : MaterialFlags {
				Instanced = Texture::SIZE,
				Bones,
#if DEBUGGING_MATERIAL_NORMALS
				GenerateNormals,
#endif
				COUNT //it sucks but i cant use SIZE here because of the naming collision with Texture::SIZE. enum namespaces suck in C++
			};
		}
		namespace Flags {
			enum Texture : MaterialFlags {
				//add albedo here eventually, as 1. then it's possible to create a material without albedo but with metal/rough and others
				Albedo = 1 << Attributes::Texture::Albedo,
				Metal = 1 << Attributes::Texture::Metal,
				Rough = 1 << Attributes::Texture::Rough,
				Normal = 1 << Attributes::Texture::Normal,
				Bump = 1 << Attributes::Texture::Bump,
				AO = 1 << Attributes::Texture::AO,
			};
			enum Other : MaterialFlags {
				Instanced = 1 << Attributes::Instanced,
				Bones = 1 << Attributes::Bones,
#if DEBUGGING_MATERIAL_NORMALS
				GenerateNormals = 1 << Attributes::GenerateNormals,
#endif
			};
		}

		static constexpr uint8_t GetTextureCount(const MaterialFlags flags) {
			const bool hasBumps = flags & Material::Flags::Texture::Bump;
			const bool hasNormal = flags & Material::Flags::Texture::Normal;
			const bool hasRough = flags & Material::Flags::Texture::Rough;
			const bool hasMetal = flags & Material::Flags::Texture::Metal;
			const bool hasAO = flags & Material::Flags::Texture::AO;
			const bool hasAlbedo = flags & Material::Flags::Texture::Albedo;
			//assert(!(hasBones && hasBumps));

			return hasAlbedo + hasNormal + hasRough + hasMetal + hasAO + hasBumps;
		}

		static constexpr uint16_t GetPipeLayoutIndex(const MaterialFlags flags) {
			//converts flags into unique descriptor set layouts
			const bool hasBones = flags & Material::Flags::Other::Bones;
			const bool instanced = flags & Material::Flags::Other::Instanced;
#if DEBUGGING_MATERIAL_NORMALS
			const bool generatingNormals = flags & Material::Flags::Other::GenerateNormals;
#endif

			const uint8_t textureCount = GetTextureCount(flags);
			return textureCount + (Material::Attributes::Texture::SIZE * (hasBones + (2 * instanced)
#if DEBUGGING_MATERIAL_NORMALS
				+ (4 * generatingNormals)));
#else
				));
#endif
		}


	}
	//typedef VkDescriptorSet TextureDesc;

	typedef uint64_t ImageID;
	typedef uint64_t TransformID;
	typedef uint32_t PipelineID;
	typedef uint32_t SkeletonID;
	typedef uint8_t SceneKey;


	struct MaterialBuffer {
		lab::vec3 albedo;
		float rough;
		float metal;
		lab::vec3 p_padding; //no do not use
		//sub surface scattering
		//depth
		//transparency
		//emission
		//specular
		//specular tint
		//sheen
		//clear coat
	};
	struct MaterialInfo {
		MaterialFlags materialFlags{ 0 };
		ImageID imageID;
		MaterialInfo() {}
		MaterialInfo(MaterialFlags flags, ImageID imageID) : materialFlags{ flags }, imageID{ imageID } {}
		bool operator==(MaterialInfo const& other) const {
			return (materialFlags == other.materialFlags) && (imageID == other.imageID);
		}
	};
} //namespace EWE