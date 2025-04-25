#include "EWEngine/Graphics/EWE_Object.h"
#include "EWEngine/Systems/Rendering/Skin/SkinRS.h"
#include "EWEngine/Graphics/Texture/Material_Textures.h"

namespace EWE {
    void AddMaterialInfo(MaterialInfo matInfo, std::vector<MaterialInfo>& materialInfo) {
        for (auto& mat : materialInfo) {
            if (mat == matInfo) {
                return;
            }
        }
        materialInfo.push_back(matInfo);
    }


#if DEBUG_NAMING
    void EweObject::AddDebugNames(std::string const& name){
        for(auto& mesh : meshes){
            mesh->SetDebugNames(name);
        }
    }
#endif
    EweObject::EweObject(std::string objectPath, bool globalTextures) {

        ImportData tempData = ImportData::LoadData(objectPath);
        TextureMapping textureTracker;
        LoadTextures(objectPath, tempData.nameExport, textureTracker, globalTextures);

        AddToRigidRenderingSystem(tempData, textureTracker, 1, false);

#if DEBUG_NAMING
        AddDebugNames(objectPath);
#endif
    }
    EweObject::EweObject(std::string objectPath, bool globalTextures, uint32_t instanceCount, bool computedTransforms) {

        instanced = true;
        ImportData tempData = ImportData::LoadData(objectPath);
        TextureMapping textureTracker;
        LoadTextures(objectPath, tempData.nameExport, textureTracker, globalTextures);
       
        AddToRigidRenderingSystem(tempData, textureTracker, instanceCount, computedTransforms);

#if DEBUG_NAMING
        AddDebugNames(objectPath);
#endif
    }
    EweObject::EweObject(std::string objectPath, bool globalTextures, SkeletonID ownerID, SkeletonID myID) : mySkinID{ myID } {
#if EWE_DEBUG
        printf("weapon object construction : objectPath - %s\n", objectPath.c_str());
#endif
        ImportData tempData = ImportData::LoadData(objectPath);
        TextureMapping textureTracker;
        LoadTextures(objectPath, tempData.nameExport, textureTracker, globalTextures);

        //addToRigidRenderingSystem(device, tempData, textureTracker);
        AddToSkinHandler(tempData, textureTracker, ownerID, 1);
#if DEBUG_NAMING
        AddDebugNames(objectPath);
#endif
    }
    EweObject::EweObject(std::string objectPath, bool globalTextures, SkeletonID ownerID, SkeletonID myID, uint32_t instanceCount) : mySkinID{myID} {
#if EWE_DEBUG
        printf("weapon object construction : objectPath - %s\n", objectPath.c_str());
#endif
        instanced = true;
        ImportData tempData = ImportData::LoadData(objectPath);
        TextureMapping textureTracker;
        LoadTextures(objectPath, tempData.nameExport, textureTracker, globalTextures);

        //addToRigidRenderingSystem(device, tempData, textureTracker);
        AddToSkinHandler(tempData, textureTracker, ownerID, instanceCount);
#if DEBUG_NAMING
        AddDebugNames(objectPath);
#endif
    }
    EweObject::~EweObject() {

        //printf("before removing textures \n");
        //this needs to be improved, wtf happened here
        if (instanced) {
            for (auto& mesh : meshes) {
                RigidRenderingSystem::RemoveInstancedMaterialObject(mesh);
            }
        }
        else {
            for (auto& mesh : meshes) {
                RigidRenderingSystem::RemoveByTransform(&transform);
            }
        }
        for (auto& mesh : meshes) {
            Deconstruct(mesh);
        }
        //printf("after removing textures \n");
    }
    void EweObject::AddToRigidRenderingSystem(ImportData const& tempData, TextureMapping& textureTracker, uint32_t instanceCount, bool computedTransforms) {

        //Actor_Type actorType = (Actor_Type)(1 + isKatana);
        /*
        for (int i = 0; i < tempData.meshExport.meshes.size(); i++) {
            meshes.push_back(EWEModel::createMesh(device, tempData.meshExport.meshes[i].first, tempData.meshExport.meshes[i].second));
            //printf("meshes flag : %d \n", textureTracker.meshNames[i].first + 128);
            materialInstance->addMaterialObject(textureTracker.meshNames[i].first + 128, actorType, nullptr, meshes.back().get(), textureTracker.meshNames[i].second, &drawable);
        }
        for (int i = 0; i < tempData.meshNTExport.meshesNT.size(); i++) {
            meshes.push_back(EWEModel::createMesh(device, tempData.meshNTExport.meshesNT[i].first, tempData.meshNTExport.meshesNT[i].second));
            //printf("meshesNT flag : %d \n", textureTracker.meshNTNames[i].first + 128);
            materialInstance->addMaterialObject(textureTracker.meshNTNames[i].first + 128, actorType, nullptr, meshes.back().get(), textureTracker.meshNTNames[i].second, &drawable);
        }
        */

        meshes.reserve(tempData.meshSimpleExport.meshes.size() + tempData.meshNTSimpleExport.meshes.size()); //a mesh should not have both simple and simpleNT

        auto const& meshSimple = tempData.meshSimpleExport.meshes;
        for (int i = 0; i < meshSimple.size(); i++) {
            meshes.emplace_back(Construct<EWEModel>({ meshSimple[i].vertices.data(), meshSimple[i].vertices.size(), tempData.meshSimpleExport.vertex_size, meshSimple[i].indices}));
            if (instanceCount > 1) {
                textureTracker.meshSimpleNames[i].materialFlags |= Material::Flags::Other::Instanced;
                RigidRenderingSystem::AddInstancedMaterialObject(textureTracker.meshSimpleNames[i], meshes.back(), instanceCount, computedTransforms);
            }
            else {
                assert(false && "need to rewrite this to support the new material buffer");
                RigidRenderingSystem::AddMaterialObject(textureTracker.meshSimpleNames[i], &transform, meshes.back(), &drawable, nullptr);
            }
        }
        auto const& meshNTSimple = tempData.meshNTSimpleExport.meshes;
        for (int i = 0; i < meshNTSimple.size(); i++) {
            meshes.emplace_back(Construct<EWEModel>({ meshNTSimple[i].vertices.data(), meshNTSimple[i].vertices.size(), tempData.meshNTSimpleExport.vertex_size, meshNTSimple[i].indices}));

            if (instanceCount > 1) {
                textureTracker.meshNTSimpleNames[i].materialFlags |= Material::Flags::Other::Instanced;
                RigidRenderingSystem::AddInstancedMaterialObject(textureTracker.meshNTSimpleNames[i], meshes.back(), instanceCount, computedTransforms);
            }
            else {
                assert(false && "need to rewrite this to support the new material buffer");
                RigidRenderingSystem::AddMaterialObject(textureTracker.meshNTSimpleNames[i], &transform, meshes.back(), &drawable, nullptr);
            }
        }

        std::size_t nameSum = tempData.meshExport.meshes.size() + tempData.meshNTExport.meshes.size() + tempData.meshSimpleExport.meshes.size() + tempData.meshNTSimpleExport.meshes.size();

        assert(nameSum == meshes.size() && "failed to match mesh to name");
    }

    void EweObject::AddToSkinHandler(ImportData& tempData, TextureMapping& textureTracker, SkeletonID skeletonOwner, uint32_t instanceCount) {
        if ((tempData.meshNTSimpleExport.meshes.size() > 0) || (tempData.meshSimpleExport.meshes.size() > 0)) {
            printf("weapon can not have simple meshes \n");
            assert(false && "object can not have both simple meshes");
        }
        assert(instanceCount == 1 && "instancing not set up here yet, should be simple but im rushing");

        if (tempData.meshExport.meshes.size() > 0) {
            meshes.reserve(tempData.meshExport.meshes.size());
            auto const& mesh = tempData.meshExport.meshes;
            for (uint16_t i = 0; i < tempData.meshExport.meshes.size(); i++) {
                meshes.push_back(Construct<EWEModel>({ mesh[i].vertices.data(), mesh[i].vertices.size(), tempData.meshExport.vertex_size, mesh[i].indices}));
                textureTracker.meshNames[i].materialFlags |= Material::Flags::Other::Bones;
                SkinRenderSystem::AddWeapon(textureTracker.meshNames[i], meshes[i], mySkinID, skeletonOwner);
            }
        }
        else if (tempData.meshNTExport.meshes.size() > 0) {
            meshes.reserve(tempData.meshNTExport.meshes.size());
            auto const& meshNT = tempData.meshNTExport.meshes;
            
            for (uint16_t i = 0; i < tempData.meshNTExport.meshes.size(); i++) {
                meshes.push_back(Construct<EWEModel>({ meshNT[i].vertices.data(), meshNT[i].vertices.size(), tempData.meshNTExport.vertex_size, tempData.meshNTExport.meshes[i].indices}));
                textureTracker.meshNames[i].materialFlags |= Material::Flags::Other::Bones;
                SkinRenderSystem::AddWeapon(textureTracker.meshNTNames[i], meshes[i], mySkinID, skeletonOwner);
            }
        }
        else {
            assert(false && "invalid weapon type");
        }
       
    }

    void EweObject::LoadTextures(std::string objectPath, ImportData::NameExportData& importData, TextureMapping& textureTracker, bool globalTextures) {
        //TEXTURES
        //this should be put in a separate function but im too lazy rn
        //printf("before loading ewe textures \n");


        for (int i = 0; i < importData.meshNames.size(); i++) {
            importData.meshNames[i] = importData.meshNames[i].substr(0, importData.meshNames[i].find_first_of("."));
            if (importData.meshNames[i].find("lethear") != std::string::npos) {
                for (int j = 0; j < importData.meshNames.size(); j++) {
                    if (j == i) { continue; }
                    if (importData.meshNames[j].find("leather") != std::string::npos) {
                        if (j >= i) {
                            printf("leather farther back than lethear 2 ?  \n");
                        }
                        else {
                            textureTracker.meshNames.push_back(textureTracker.meshNames[j]);
                            AddMaterialInfo(textureTracker.meshNames[j], ownedTextures);
                            break;
                        }
                    }
                }
                continue;
            }
            std::string finalDir = objectPath;
            finalDir += "/" + importData.meshNames[i];
            const MaterialInfo matInfo = Material_Image::CreateMaterialImage(finalDir, true, globalTextures);
            //printf("normal map texture? - return pair.first, &8 - %d;%d \n", returnPair.first, returnPair.first & 8);

            textureTracker.meshNames.push_back(matInfo);
            AddMaterialInfo(matInfo, ownedTextures);
            
        }
        //printf("after mesh texutres \n");
        for (int i = 0; i < importData.meshNTNames.size(); i++) {
            importData.meshNTNames[i] = importData.meshNTNames[i].substr(0, importData.meshNTNames[i].find_first_of("."));
            std::string finalDir = objectPath;
            finalDir += "/" + importData.meshNTNames[i];
            const MaterialInfo matInfo = Material_Image::CreateMaterialImage(finalDir, true, globalTextures);
            //printf("no normal map texture? - return pair.first, &8 - %d;%d \n", returnPair.first, returnPair.first & 8);

            textureTracker.meshNTNames.push_back(matInfo);
            AddMaterialInfo(matInfo, ownedTextures);
            
        }
        //printf("after mesh nt texutres \n");


        for (int i = 0; i < importData.meshSimpleNames.size(); i++) {
            importData.meshSimpleNames[i] = importData.meshSimpleNames[i].substr(0, importData.meshSimpleNames[i].find_first_of("."));
            std::string finalDir = objectPath;
            finalDir += "/" + importData.meshSimpleNames[i];
            //printf("simple names final Dir : %s \n", finalDir.c_str());
            const MaterialInfo matInfo = Material_Image::CreateMaterialImage(finalDir, true, globalTextures);
            //printf("no normal map texture? - return pair.first, &8 - %d;%d \n", returnPair.first, returnPair.first & 8);

            textureTracker.meshSimpleNames.push_back(matInfo);
            AddMaterialInfo(matInfo, ownedTextures);
            
        }

        for (int i = 0; i < importData.meshNTSimpleNames.size(); i++) {
            importData.meshNTSimpleNames[i] = importData.meshNTSimpleNames[i].substr(0, importData.meshNTSimpleNames[i].find_first_of("."));
            std::string finalDir = objectPath;
            finalDir += "/" + importData.meshNTSimpleNames[i];
            const MaterialInfo matInfo = Material_Image::CreateMaterialImage(finalDir, true, globalTextures);
            //printf("no normal map texture? - return pair.first, &8 - %d;%d \n", returnPair.first, returnPair.first & 8);

            textureTracker.meshNTSimpleNames.push_back(matInfo);
            AddMaterialInfo(matInfo, ownedTextures);
        }
    }
}//namespace EWE