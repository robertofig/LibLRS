/* ==========================================================================
   $Arquivo: $
   $Data: $
   $Revisão: $
   $Autoria: $

   $Descrição: $
   ========================================================================== */

#include "liblrs.h"

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#endif

inline void
StartTiming(time_interval* Timing)
{

#ifdef _WIN32
    LARGE_INTEGER ClockNow;
    QueryPerformanceCounter(&ClockNow);
    Timing->T0 = ClockNow.QuadPart;
#endif

}

inline void
EndTiming(time_interval* Timing)
{

#ifdef _WIN32
    LARGE_INTEGER ClockNow, ClockFrequency;
    QueryPerformanceCounter(&ClockNow);
    QueryPerformanceFrequency(&ClockFrequency);

    Timing->T1 = ClockNow.QuadPart;
    Timing->Diff = (float)((Timing->T1 - Timing->T0) * 1000000) / ClockFrequency.QuadPart;
#endif

}

static void*
GetSystemMemory(u64 MemorySize)
{

#ifdef _WIN32
    void* Result = VirtualAlloc(0, MemorySize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    return Result;
#endif

}

static void
FreeSystemMemory(void* Memory)
{

#ifdef _WIN32
    VirtualFree(Memory, 0, MEM_RELEASE);
#endif

}

static void*
GetHeapMemory(u64 MemorySize)
{

#ifdef _WIN32
    HANDLE Heap = GetProcessHeap();
    void* Memory = HeapAlloc(Heap, HEAP_ZERO_MEMORY, MemorySize);
    return Memory;
#endif

}

static void
FreeHeapMemory(void* Memory)
{

#ifdef _WIN32
    HANDLE Heap = GetProcessHeap();
    HeapFree(Heap, 0, Memory);
#endif

}
