#pragma once

#include "EWEngine/Data/EWE_Import.h"
#include "EWEngine/Systems/Rendering/Rigid/RigidRS.h"

#include <LAB/Transform.h>

#include <unordered_set>

//need instancing here

namespace EWE {

	class EweObject {
	public:

        EweObject(const EweObject& other) = delete;
        EweObject(EweObject&& other) = default;
        EweObject& operator=(const EweObject&& other) = delete;
        EweObject& operator=(const EweObject& other) = delete;


        EweObject(std::string objectPath, bool globalTextures);
        EweObject(std::string objectPath, bool globalTextures, uint32_t instanceCount, bool computedTransforms);
        EweObject(std::string objectPath, bool globalTextures, SkeletonID ownerID, SkeletonID myID);
        EweObject(std::string objectPath, bool globalTextures, SkeletonID ownerID, SkeletonID myID, uint32_t instanceCount);
        ~EweObject();

		lab::Transform<float, 3> transform{};
        std::vector<EWEModel*> meshes{};
        bool drawable = true;
        std::vector<MaterialInfo> ownedTextures{};



        //void deTexturize();
        uint32_t GetSkeletonID() {
            return mySkinID;
        }
	private:
        bool instanced = false; //purely for deconstruction branching

        struct TextureMapping {
            std::vector<MaterialInfo> meshNames;
            std::vector<MaterialInfo> meshNTNames;
            std::vector<MaterialInfo> meshSimpleNames;
            std::vector<MaterialInfo> meshNTSimpleNames;
        };

        uint32_t mySkinID = 0;

        void AddToRigidRenderingSystem(ImportData const& tempData, TextureMapping& textureTracker, uint32_t instanceCount, bool computedTransforms);
        void AddToSkinHandler(ImportData& tempData, TextureMapping& textureTracker, uint32_t skeletonOwner, uint32_t instanceCount);

        void LoadTextures(std::string objectPath, ImportData::NameExportData& importData, TextureMapping& textureTracker, bool globalTextures);
	
#if DEBUG_NAMING
        void AddDebugNames(std::string const& name);
#endif
    };
}