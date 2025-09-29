#pragma once

#include "EWGraphics/Preprocessor.h"


#include <new>
#include <utility>
#include <cassert>
#include <cstdint>

#define USING_MALLOC false

namespace Internal { //need header access for the templates
    //void* ewe_alloc(std::size_t element_size, std::size_t element_count);
    //void ewe_free(void* ptr);
}//namespace Internal

void ewe_alloc_mem_track(void* ptr);
void ewe_free_mem_track(void* ptr);


/*
template<typename T>
struct ConstructHelper {
    T* ptr;
    template <typename ... Args>
    ConstructHelper(Args&&... args) : 
#if USING_MALLOC
#else
        ptr{new T(std::forward<Args>(args)...)} 
#endif
    {
#if USING_MALLOC
        void* memory = Internal::ewe_alloc(sizeof(T), 1, srcLoc);
        assert(memory);
        ptr = new (memory) T(std::forward<Args>(args)...);
#endif
        ewe_alloc_mem_track(reinterpret_cast<void*>(ptr));
    }
};
*/

template<typename T, typename... Args>
requires (std::is_constructible_v<T, Args...>)
T* Construct(Args&&... args) {
    T* ret = new T(std::forward<Args>(args)...);
    ewe_alloc_mem_track(reinterpret_cast<void*>(ret));
    return ret;
}

template<typename T>
struct ConstructAddrHelper {
    T* ptr;
    template <typename ... Args>
    ConstructAddrHelper(void* address, Args&&... args) :
        ptr{ new(address) T(std::forward<Args>(args)...) }
    {
    }
};

template<typename T>
T* Construct(void* address, ConstructAddrHelper<T> construct
#if CALL_TRACING
    , std::source_location srcLoc = std::source_location::current()) {
    ewe_alloc_mem_track(reinterpret_cast<void*>(construct.ptr), srcLoc);
#else
) {
#endif
    return construct.ptr;
}

template<typename T>
void Deconstruct(T* object) {
    ewe_free_mem_track(object);
#if USING_MALLOC
    object->~T();
    free(object);
#else
    delete object;
#endif
}


/*
template<typename T, typename... Args>
static T** ConstructMultiple(uint32_t count, Args&&... args) {
    void* memory = ewe_alloc(sizeof(T), count);
    if (memory == nullptr) {
		return nullptr;
	}
    uint64_t memLoc = reinterpret_cast<uint64_t>(memory);
    T* ret = new (memory) T(std::forward<Args>(args)...);
    for (uint32_t i = 0; i < count; i++) {
        memcpy(reinterpret_cast<void*>(memLoc + i * sizeof(T)), , sizeof(T));
    }
}
*/

/*
* doesnt compile, probably not possible
template<typename T, typename... Args> 
static T** constructVector(std::vector<Args> args_param) {
    const uint64_t memory_size = sizeof(T);
    void* memory = std::malloc(memory_size * args_param.size());
    if (memory == nullptr) {
        return nullptr;
    }
    T** ret = &memory;
    uint32_t object_iter = 0;
    for (auto const& params : args_param)
    {
        ret[object_iter] = new(T[object_iter]) T(std::forward<Args>(args_params[objectIter])...);
        object_iter++;
    }
}
*/

/*
_THROW1(_STD bad_alloc) {
    void* p;
    while ((p = malloc(size)) == 0)
        if (_callnewh(size) == 0)
        {       // report no memory
            static const std::bad_alloc nomem;
            _RAISE(nomem);
        }

    return (p);
}
*/