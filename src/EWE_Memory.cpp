#include "EWEngine/Data/EWE_Memory.h"


#if EWE_DEBUG
#include <string>
#include <fstream>
#include <unordered_map>

static constexpr const char* memoryLogPath{ "memoryLog.log" };
bool notFirstMalloc = false;


#endif
#include <cstdlib>
#include <cstring>
#include <vector>
#include <mutex>

#if EWE_DEBUG
std::unordered_map<uint64_t, std::source_location> mallocMap{};
std::mutex mallocMut{};

void UpdateMemoryLogFile() {
	std::ofstream memoryLogFile{};
	if (mallocMap.size() == 0) {
		memoryLogFile.open(memoryLogPath, std::ofstream::out | std::ofstream::trunc);
		memoryLogFile.seekp(std::ios::beg); //i dont think this is necessary
		memoryLogFile << "empty, no mem leaks";
		memoryLogFile.close();
	}
	else {
		std::vector<std::string> memoryLogCopy(mallocMap.size());

		std::string temp;
		for (auto& memPiece : mallocMap) {
			temp.clear();
			temp = "\nfile:{line} - ";
			temp += memPiece.second.file_name();
			temp += ":{";
			temp += std::to_string(memPiece.second.line());
			temp += "}\n\t";
			temp += memPiece.second.function_name();
			temp += '\n';
			memoryLogCopy.push_back(temp);
		}
		memoryLogFile.open(memoryLogPath, std::ofstream::out | std::ofstream::trunc);
		memoryLogFile.seekp(std::ios::beg); //i dont think this is necessary
		for (auto const& logCopy : memoryLogCopy) {
			memoryLogFile << logCopy;
		}
		memoryLogFile.close();
	}

}
#endif

namespace Internal {
#if CALL_TRACING
	void* ewe_alloc(std::size_t element_size, std::size_t element_count, std::source_location srcLoc) {
		void* ptr = malloc(element_count * element_size);
#if EWE_DEBUG
		ewe_alloc_mem_track(ptr, srcLoc);
#endif
		return ptr;
	}
#endif


	void ewe_free(void* ptr) {
#if USING_MALLOC
		free(ptr);
#endif
#if EWE_DEBUG
		ewe_free_mem_track(ptr);
#endif
	}
}//namespace Internal
#if CALL_TRACING
void ewe_alloc_mem_track(void* ptr, std::source_location srcLoc) {

#if EWE_DEBUG
	mallocMut.lock();
	mallocMap.try_emplace(reinterpret_cast<uint64_t>(ptr), srcLoc);
	UpdateMemoryLogFile();
	mallocMut.unlock();
#endif
}
#endif

void ewe_free_mem_track(void* ptr){
#if EWE_DEBUG
	mallocMut.lock();
	auto found = mallocMap.find(reinterpret_cast<uint64_t>(ptr));
	assert((found != mallocMap.end()) && "freeing memory that wasn't allocated");
	mallocMap.erase(found);

	UpdateMemoryLogFile();
	mallocMut.unlock();
#endif
}