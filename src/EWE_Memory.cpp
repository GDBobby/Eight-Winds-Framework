#include "EWGraphics/Data/EWE_Memory.h"


#include <unordered_map>
#include <string>
#include <fstream>

static constexpr const char* memoryLogPath{ "memoryLog.log" };
bool notFirstMalloc = false;

#include <cstdlib>
#include <vector>
#include <mutex>

#if CALL_TRACING
#include <stacktrace>
#include <format>
#include <sstream>

//#define STRICT_LOGGING //rewrites every time anything gets added or removed

struct MemoryTrackingStruct {
	std::unordered_map<uint64_t, std::stacktrace> byAddress{};

	std::mutex mut{};

	void UpdateMemoryLogFile();

	~MemoryTrackingStruct() {
		//using raii to make sure this gets written as its destructed. its global scope so it should be destructed after main exits. its possible other things are correctly handled but not destructed before this. would technically be a memory leak, since anythign created with Construct() should be explicitly Destruct()ed

		UpdateMemoryLogFile();
	}

};

MemoryTrackingStruct memoryTracking{};

void MemoryTrackingStruct::UpdateMemoryLogFile() {
	std::ofstream memoryLogFile{};
	if (memoryTracking.byAddress.size() == 0) {
		memoryLogFile.open(memoryLogPath, std::ofstream::out | std::ofstream::trunc);
		memoryLogFile.seekp(std::ios::beg); //i dont think this is necessary
		memoryLogFile << "empty, no mem leaks";
		memoryLogFile.close();
	}
	else {
		std::stringstream memoryLog{};

		for (auto& [key, trace] : memoryTracking.byAddress) {
			memoryLog << "\n--- stacktrace ---\n";
			memoryLog << trace;  // whole stacktrace in one go
			memoryLog << "--- end ---\n";
		}

		memoryLogFile.open(memoryLogPath, std::ofstream::out | std::ofstream::trunc);
		memoryLogFile.seekp(std::ios::beg); //i dont think this is necessary
		memoryLogFile << memoryLog.rdbuf();
		memoryLogFile.close();
	}

}
#endif

namespace Internal {
	/*
	void* ewe_alloc(std::size_t element_size, std::size_t element_count) {
		void* ptr = malloc(element_count * element_size);
#if EWE_DEBUG
		ewe_alloc_mem_track(ptr, srcLoc);
#endif
		return ptr;
	}


	void ewe_free(void* ptr) {
#if USING_MALLOC
		free(ptr);
#endif
#if EWE_DEBUG
		ewe_free_mem_track(ptr);
#endif
	}
	*/
}//namespace Internal
void ewe_alloc_mem_track(void* ptr) {

#if CALL_TRACING
	//printf("need to set up stack trace here\n");
	memoryTracking.mut.lock();

	auto recentCalls = std::stacktrace::current(1, 4);
	memoryTracking.byAddress.try_emplace(reinterpret_cast<uint64_t>(ptr), recentCalls);
#ifdef STRICT_LOGGING
	memoryTracking.UpdateMemoryLogFile();
#endif
	memoryTracking.mut.unlock();
#endif
}

void ewe_free_mem_track(void* ptr){
#if CALL_TRACING
	memoryTracking.mut.lock();
	auto found = memoryTracking.byAddress.find(reinterpret_cast<uint64_t>(ptr));
	assert((found != memoryTracking.byAddress.end()) && "freeing memory that wasn't allocated");
	memoryTracking.byAddress.erase(found);

#ifdef STRICT_LOGGING
	memoryTracking.UpdateMemoryLogFile();
#endif
	memoryTracking.mut.unlock();
#endif
}