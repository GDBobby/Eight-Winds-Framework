#include "EWEngine/Graphics/VkDebugDeviceLost.h"

#if DEBUGGING_DEVICE_LOST
#include <cstdio>
#include <cassert>
#include <set>
#include <string>
#include <chrono>

//there's a license in there, not sure if i need to copy it or not
//https://github.com/ConfettiFX/The-Forge/blob/23483a282ddc8a917f8b292b0250dec122eab6a9/Common_3/Graphics/Vulkan/Vulkan.cpp#L741
//im pretty much copy and pasting with minor adjustments

PFN_vkGetDeviceFaultInfoEXT vkGetDeviceFaultInfoEXTX;
PFN_vkGetQueueCheckpointDataNV vkGetQueueCheckpointDataNVX;
PFN_vkCmdSetCheckpointNV vkCmdSetCheckpointNVX;
PFN_vkCmdWriteBufferMarkerAMD vkCmdWriteBufferMarkerAMDX;


namespace EWE {
	namespace VKDEBUG {
		VkDevice device;
		VkInstance instance;
		GpuCrashTracker::MarkerMap* markerMap;
		GpuCrashTracker* gpuCrashTracker;

		DeviceLostDebugStructure::DeviceLostDebugStructure() {
			printf("initializing gpuCrashTracker\n");
			markerMap = new GpuCrashTracker::MarkerMap();
			gpuCrashTracker = new GpuCrashTracker(*markerMap);
			gpuCrashTracker->Initialize();
		}
		void DeviceLostDebugStructure::Initialize(VkDevice vkDevice) {
			device = vkDevice;
		}

		bool DeviceLostDebugStructure::GetVendorDebugExtension(VkPhysicalDevice physDevice) {
			//double checking device extensions, which isn't great, shouldnt be completely awful tho
			uint32_t extensionCount;
			VkResult result = vkEnumerateDeviceExtensionProperties(physDevice, nullptr, &extensionCount, nullptr);
			if (result != VK_SUCCESS) {
				printf("VK_ERROR : %s(%d) : %s - %d \n", __FILE__, __LINE__, __FUNCTION__, result);
			}

			std::vector<VkExtensionProperties> availableExtensions(extensionCount);
			result = vkEnumerateDeviceExtensionProperties(
				physDevice,
				nullptr,
				&extensionCount,
				availableExtensions.data()
			);
			if (result != VK_SUCCESS) {
				printf("VK_ERROR : %s(%d) : %s - %d \n", __FILE__, __LINE__, __FUNCTION__, result);
			}

			std::set<std::string> requestedExtensions(debugExtensions.begin(), debugExtensions.end());

			printf("available extensions: \n");
			for (const auto& extension : availableExtensions) {
				printf("\t%s\n", extension.extensionName);
				requestedExtensions.erase(extension.extensionName);
			}
			const uint32_t inSize = debugExtensions.size();
			//remainign extensions, not supported
			for (uint8_t i = 0; i < debugExtensions.size(); i++) {
				const std::string tempString{ debugExtensions[i] };

				if (requestedExtensions.find(tempString) != requestedExtensions.end()) {
					debugExtensions.erase(debugExtensions.begin() + i);
					i--;
				}
			}

			for (const auto& extension : debugExtensions) {
				if (strcmp(extension, VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME) == 0) {
					NVIDIAdebug = true;
					printf("using nvidia debug\n");
					InitNvidiaDebug();
					break;
				}
				else if (strcmp(extension, VK_AMD_BUFFER_MARKER_EXTENSION_NAME) == 0) {
					AMDdebug = true;
					printf("using amd debug\n");
					InitAMDDebug();
					break;
				}
			}


			return debugExtensions.size() != inSize;
		}

		//function ptr
		void (*checkpointPtr)(DeviceLostDebugStructure*, VkCommandBuffer, const char*, GFX_vk_checkpoint_type);


		void DeviceLostDebugStructure::InitNvidiaDebug() {
			vkGetQueueCheckpointDataNVX = reinterpret_cast<PFN_vkGetQueueCheckpointDataNV>(vkGetDeviceProcAddr(device, "vkGetQueueCheckpointDataNV"));
			vkCmdSetCheckpointNVX = reinterpret_cast<PFN_vkCmdSetCheckpointNV>(vkGetDeviceProcAddr(device, "vkCmdSetCheckpointNV"));
			//assert(vkCmdSetCheckpointNVX != nullptr);
			//assert(vkGetQueueCheckpointDataNVX != nullptr);

			checkpoints.reserve(50);

			checkpointPtr = &DeviceLostDebugStructure::AddNvidiaCheckpoint;
		}
		void DeviceLostDebugStructure::InitAMDDebug() {
			vkCmdWriteBufferMarkerAMDX = reinterpret_cast<PFN_vkCmdWriteBufferMarkerAMD>(vkGetDeviceProcAddr(device, "vkCmdWriteBufferMarkerAMDX"));
			//assert(vkCmdWriteBufferMarkerAMDX != nullptr);

			checkpointPtr = &DeviceLostDebugStructure::AddAMDCheckpoint;
		}

		void DeviceLostDebugStructure::AddNvidiaCheckpoint(DeviceLostDebugStructure* thisPtr, CommandBuffer cmdBuf, const char* name, GFX_vk_checkpoint_type type) {
			VkCheckpointDataNV baseCopy{};
			baseCopy.sType = VK_STRUCTURE_TYPE_CHECKPOINT_DATA_NV;
			baseCopy.pNext = nullptr;
			if (thisPtr->checkpoints.size() > 0) {
				baseCopy.pCheckpointMarker = &thisPtr->checkpoints.back();
			}
			else {
				baseCopy.pCheckpointMarker = nullptr;
			}
			baseCopy.stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
			thisPtr->checkpoints.push_back(baseCopy);

			assert(thisPtr->checkpoints.size() < 50 && "checkpoints is getting oversized, missed a clear?");
			vkCmdSetCheckpointNVX(cmdBuf, &thisPtr->checkpoints.back());
		}
		void DeviceLostDebugStructure::AddAMDCheckpoint(DeviceLostDebugStructure* thisPtr, CommandBuffer cmdBuf, const char* name, GFX_vk_checkpoint_type type) {
			printf("not implemented\n");
			/*
			from what I saw,
				forge uses aftermath for device lost
				o3de uses aftermath
				nabla does nothing
				ogre does nothing, or uses aftermath, i cant remember

			point being, i havent seen anybody debug AMD device lost

			*/



			//i want it to write when i tell it to :(
			//vkCmdWriteBufferMarkerAMD(cmdBuf, VK_PIPELINE_STAGE_NONE, buffer, offset, marker);

			//vkCmdWriteBufferMarkerAMD(cmdBuf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, buffer, 0, static_cast<uint32_t>(type));
		}

		void DeviceLostDebugStructure::AddCheckpoint(CommandBuffer cmdBuf, const char* name, GFX_vk_checkpoint_type type) {

			printf("\n\n *** ADDING CHECKPOINT *** \n\n");
			//checkpointPtr(this, cmdBuf, name, type);
		}
		void DeviceLostDebugStructure::ClearCheckpoints() {
			checkpoints.clear();
		}

		bool deviceFaultEnabled = false;
		bool nvidiaCheckpoint = false;
		bool amdCheckpoint = false;
		std::array<VkQueue, 4> queues;

		void Initialize(VkDevice vkDevice, VkInstance vkInstance, std::array<VkQueue, 4>& queuesParam, bool deviceFaultEnabledParam, bool nvidiaCheckpointEnabled, bool amdCheckpointEnabled) {
			device = vkDevice;
			instance = vkInstance;

			deviceFaultEnabled = deviceFaultEnabledParam;
			nvidiaCheckpoint = nvidiaCheckpointEnabled;
			amdCheckpoint = amdCheckpointEnabled;

			queues = queuesParam;
			if (deviceFaultEnabled) {
				//func =				   (PFN_vkGetDeviceFaultInfoEXT)vkGetDeviceProcAddr(device, "vkGetDeviceFaultInfoEXT");
				vkGetDeviceFaultInfoEXTX = reinterpret_cast<PFN_vkGetDeviceFaultInfoEXT>(vkGetDeviceProcAddr(device, "vkGetDeviceFaultInfoEXT"));
				if (vkGetDeviceFaultInfoEXTX == nullptr) {
					deviceFaultEnabled = false;
				}
				//assert(vkGetDeviceFaultInfoEXTX != nullptr && "vkGetDeviceFaultInfoEXT proc addr was nullptr");

			}
			if (nvidiaCheckpointEnabled) {
				vkGetQueueCheckpointDataNVX = reinterpret_cast<PFN_vkGetQueueCheckpointDataNV>(vkGetDeviceProcAddr(device, "vkGetQueueCheckpointDataNV"));
				vkCmdSetCheckpointNVX = reinterpret_cast<PFN_vkCmdSetCheckpointNV>(vkGetDeviceProcAddr(device, "vkCmdSetCheckpointNV"));
				assert(vkCmdSetCheckpointNVX != nullptr);
				assert(vkGetQueueCheckpointDataNVX != nullptr);
			}
		}
		void NSightActivate() {
			printf("activating nsight aftermath??\n");
			// Device lost notification is asynchronous to the NVIDIA display
			// driver's GPU crash handling. Give the Nsight Aftermath GPU crash dump
			// thread some time to do its work before terminating the process.
			auto tdrTerminationTimeout = std::chrono::seconds(3);
			auto tStart = std::chrono::steady_clock::now();
			auto tElapsed = std::chrono::milliseconds::zero();

			GFSDK_Aftermath_CrashDump_Status status = GFSDK_Aftermath_CrashDump_Status_Unknown;
			AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GetCrashDumpStatus(&status));

			while (status != GFSDK_Aftermath_CrashDump_Status_CollectingDataFailed &&
				status != GFSDK_Aftermath_CrashDump_Status_Finished &&
				tElapsed < tdrTerminationTimeout)
			{
				// Sleep 50ms and poll the status again until timeout or Aftermath finished processing the crash dump.
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
				AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GetCrashDumpStatus(&status));

				auto tEnd = std::chrono::steady_clock::now();
				tElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(tEnd - tStart);
			}

			if (status != GFSDK_Aftermath_CrashDump_Status_Finished) {
				std::stringstream err_msg;
				err_msg << "Unexpected crash dump status: " << status;
				assert(false && err_msg.str().c_str());
			}

			// Terminate on failure
			//handled in the calling function
		}


		void OnDeviceLost() {
			if (deviceFaultEnabled) {


				VkDeviceFaultCountsEXT faultCounts = { VK_STRUCTURE_TYPE_DEVICE_FAULT_COUNTS_EXT };
				printf("immediately before device lost function\n");

				VkResult vkres = vkGetDeviceFaultInfoEXTX(device, &faultCounts, nullptr);
				assert(vkres == VK_SUCCESS && "failed to get device fault info");

				VkDeviceFaultInfoEXT faultInfo = { VK_STRUCTURE_TYPE_DEVICE_FAULT_INFO_EXT };

				faultInfo.pVendorInfos = nullptr;
				if (faultCounts.vendorInfoCount > 0) {
					faultInfo.pVendorInfos = new VkDeviceFaultVendorInfoEXT[faultCounts.vendorInfoCount];
				}

				faultInfo.pVendorInfos = nullptr;
				if (faultCounts.addressInfoCount > 0) {
					faultInfo.pAddressInfos = new VkDeviceFaultAddressInfoEXT[faultCounts.addressInfoCount];
				}

				faultCounts.vendorBinarySize = 0;
				vkres = vkGetDeviceFaultInfoEXTX(device, &faultCounts, &faultInfo);
				assert(vkres == VK_SUCCESS && "failed to get device fault info");

				printf("** Report from VK_EXT_device_fault ** \n");
				printf("Description : %s\n", faultInfo.description);
				printf("Vendor Infos : \n");
				for (uint32_t i = 0; i < faultCounts.vendorInfoCount; i++) {
					const VkDeviceFaultVendorInfoEXT* vendorInfo = &faultInfo.pVendorInfos[i];
					printf("\nInfo[%u]\n", i);
					printf("\tDescription: %s\n", vendorInfo->description);
					printf("\tFault code : %zu\n", (size_t)vendorInfo->vendorFaultCode);
					printf("\tFault Data : %zu\n", (size_t)vendorInfo->vendorFaultData);
				}

				static constexpr const char* addressTypeNames[] = {
					"NONE",
					"READ_INVALID",
					"WRITE_INVALID",
					"EXECUTE_INVALID",
					"INSTRUCTION_POINTER_UNKNOWN",
					"INSTRUCTION_POINTER_INVALID",
					"INSTRUCTION_POINTER_FAULT",
				};
				printf("\nAddress Infos :\n");
				for (uint32_t i = 0; i < faultCounts.addressInfoCount; i++) {
					const VkDeviceFaultAddressInfoEXT* addrInfo = &faultInfo.pAddressInfos[i];
					const VkDeviceAddress lower = (addrInfo->reportedAddress & ~(addrInfo->addressPrecision - 1));
					const VkDeviceAddress upper = (addrInfo->reportedAddress | (addrInfo->addressPrecision - 1));
					printf("\nInfo[%u]\n", i);
					printf("\tType : %s\n", addressTypeNames[addrInfo->addressType]);
					printf("\tReported Address : %zu\n", (size_t)addrInfo->reportedAddress);
					printf("\tLower Address : %zu\n", (size_t)lower);
					printf("\tUpper Address : %zu\n", (size_t)upper);
					printf("\tPrecision : %zu\n", (size_t)addrInfo->addressPrecision);
				}

				if (faultCounts.vendorInfoCount > 0) {
					delete[] faultInfo.pVendorInfos;
				}
				faultInfo.pVendorInfos = nullptr;

				if (faultCounts.addressInfoCount > 0) {
					delete[] faultInfo.pAddressInfos;
				}
			}
			else {
				printf("device fault extension not supported\n");
			}
			printf("before nvidiaa checkpoint branch\n");
			if (nvidiaCheckpoint) {
				printf("finna get into nvidia checkpoitns\n");

				//go into aftermath here
				NSightActivate();

				/*
				for (uint8_t i = 0; i < 4; i++) {
					uint32_t checkpointCount;

					vkGetQueueCheckpointDataNVX(queues[i], &checkpointCount, NULL);

					printf("[%u] checkpoint found on queue[%d]\n", checkpointCount, i);
					if (checkpointCount > 0) {
						std::vector<VkCheckpointDataNV> checkpointData(checkpointCount);
						//VkQueue queue, uint32_t* pCheckpointDataCount, VkCheckpointDataNV* pCheckpointData
						vkGetQueueCheckpointDataNVX(queues[i], &checkpointCount, checkpointData.data());
						for (auto const& checkpoint : checkpointData) {
							auto* cpData = reinterpret_cast<GFX_vk_checkpoint_data*>(checkpoint.pCheckpointMarker);
							printf("checkpoint {type:name} - %d:%s\n", cpData->type, cpData->name);
						}
					}
				}
				*/
				printf("end of nvidia checkpoint\n");
			}
			else if (amdCheckpoint) {

			}

			assert(false && "end of device lost debug");
		}
	} //namespace VKDEBUG
}//namespace EWE
#endif