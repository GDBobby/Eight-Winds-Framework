#pragma once

#include <cstdint>
 //on linux, _DEBUG needs to be defined explicitly
 //_DEBUG is already defined in Visual Studio while in DEBUG mode
 //so im just replacing _DEBUG with EWE_DEBUG, supports release with debug too

#if _WIN32
    #ifdef _DEBUG
        #define EWE_DEBUG true
    #else
        #define EWE_DEBUG false
    #endif
#else //linux
    //need to manually define it
    #define EWE_DEBUG true
#endif

#define CALL_TRACING (true && EWE_DEBUG)
#if CALL_TRACING
#include <source_location>
#endif

#define DEBUGGING_DEVICE_LOST false
#define USING_NVIDIA_AFTERMATH (true && DEBUGGING_DEVICE_LOST)

#define GPU_LOGGING true
#define DECONSTRUCTION_DEBUG (true && EWE_DEBUG)

#define DEBUGGING_PIPELINES (false && EWE_DEBUG)
#define DEBUGGING_MATERIAL_PIPE (false && EWE_DEBUG)

#define DEBUG_NAMING (true && EWE_DEBUG)

#define RENDER_DEBUG false

#define USING_VMA true
#define DEBUGGING_MEMORY_WITH_VMA (USING_VMA && false)

#define SEMAPHORE_TRACKING (true && DEBUG_NAMING && EWE_DEBUG)

//descriptor tracing requires C++23 and <stacktrace> stacktrace is not supported in clang20 (afaik)
#define DESCRIPTOR_TRACING (false && EWE_DEBUG)

#define COMMAND_BUFFER_TRACING (true && EWE_DEBUG)
#define DEBUGGING_FENCES (false && EWE_DEBUG)

#define ONE_SUBMISSION_THREAD_PER_QUEUE false

#define IMAGE_DEBUGGING (true && EWE_DEBUG)

#ifndef DESCRIPTOR_IMAGE_IMPLICIT_SYNCHRONIZATION
#define DESCRIPTOR_IMAGE_IMPLICIT_SYNCHRONIZATION true
#endif

#define MIPMAP_ENABLED true

#define THREAD_NAMING (true && EWE_DEBUG)
#define DEBUGGING_MATERIAL_NORMALS (false && EWE_DEBUG)

#define PIPELINE_HOT_RELOAD false

#if EWE_DEBUG
    #define EWE_UNREACHABLE assert(false)
#else
    #ifdef _MSC_VER
        #define EWE_UNREACHABLE __assume(false)
    #elif defined(__GNUC__) || defined(__clang__)
        #define EWE_UNREACHABLE __builtin_unreachable()
    #else
        #define EWE_UNREACHABLE throw std::runtime_error("unreachable code")
    #endif
#endif


#if CALL_TRACING
#define SRC_HEADER_FIRST_PARAM std::source_location srcLoc = std::source_location::current()
#define SRC_HEADER_PARAM , SRC_HEADER_FIRST_PARAM
#define SRC_FIRST_PARAM std::source_location srcLoc
#define SRC_PARAM , SRC_FIRST_PARAM
#define SRC_PASS , srcLoc
#else
#define SRC_HEADER_FIRST_PARAM
#define SRC_HEADER_PARAM
#define SRC_PARAM
#define SRC_FIRST_PARAM
#define SRC_PASS
#endif