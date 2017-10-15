#ifndef IVAN_PLATFORM_H
#define IVAN_PLATFORM_H

#define IVAN_INTERNAL 0


#include <stdio.h>

#ifndef IVAN_BIT
#define IVAN_BIT(value) (1 << (value))
#endif

#ifndef IVAN_LERP
#define IVAN_LERP(a, b, t) ((a) + ((b) - (a)) * (t))
#endif

#ifndef IVAN_MIN
#define IVAN_MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef IVAN_MAX
#define IVAN_MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef IVAN_CLAMP
#define IVAN_CLAMP(value, lower, upper) (IVAN_MIN(IVAN_MAX(value, lower), upper))
#endif

#ifndef IVAN_CLAMP01
#define IVAN_CLAMP01(value) (IVAN_CLAMP(value, 0, 1))
#endif

#ifndef IVAN_SQUARE
#define IVAN_SQUARE(value) ((value) * (value))
#endif

#ifndef IVAN_CUBE
#define IVAN_CUBE(value) ((value) * (value) * (value))
#endif

#ifndef IVAN_ABS
#define IVAN_ABS(value) ((value) >= 0 ? (value) : -(value))
#endif

#ifndef IVAN_ARRAY_COUNT
#define IVAN_ARRAY_COUNT(Array) (sizeof(Array) / sizeof(Array[0]))
#endif

#ifndef IVAN_KILOBYTES
#define IVAN_KILOBYTES(Value) (Value * 1024)
#define IVAN_MEGABYTES(Value) (IVAN_KILOBYTES(1024) * Value)
#define IVAN_GIGABYTES(Value) (IVAN_MEGABYTES(1024) * Value)
#define IVAN_TERABYTES(Value) (IVAN_GIGABYTES(1024) * Value)
#endif

#ifdef __cplusplus
#define IVAN_EXTERN extern "C"
#else
#define IVAN_EXTERN extern
#endif

#ifdef _WIN32
#define IVAN_DLL_EXPORT IVAN_EXTERN __declspec(dllexport)
#define IVAN_DLL_IMPORT IVAN_EXTERN __declspec(dllimport)
#else
#define IVAN_DLL_EXPORT IVAN_EXTERN __attribute__((visibility("default")))
#define IVAN_DLL_IMPORT IVAN_EXTERN
#endif

#if defined(_WIN64) || defined(__x86_64__) || defined(__64BIT__) || defined(__powerpc64__) || defined(__ppc64__)
#ifndef IVAN_ARCH_64_BIT
#define IVAN_ARCH_64_BIT 1
#endif
#else
#ifndef IVAN_ARCH_32_BIT
#define IVAN_ARCH_32_BIT 1
#endif
#endif
#if defined(_WIN32) || defined(_WIN64)
#ifndef IVAN_SYSTEM_WINDOWS
#define IVAN_SYSTEM_WINDOWS 1
#endif
#elif defined(__APPLE__) && defined(__MACH__)
#ifndef IVAN_SYSTEM_OSX
#define IVAN_SYSTEM_OSX 1
#endif
#elif defined(__unix__)
#ifndef IVAN_SYSTEM_UNIX
#define IVAN_SYSTEM_UNIX 1
#endif
#ifdef __linux__
#ifndef IVAN_SYSTEM_LINUX
#define IVAN_SYSTEM_LINUX 1
#endif
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
#ifndef IVAN_SYSTEM_FREEBSD
#define IVAN_SYSTEM_FREEBSD 1
#endif
#else
#error this UNIX OS is not supported
#endif
#else
#error This operating system is not supported
#endif


#ifdef _MSC_VER
#define IVAN_COMPILER_MSVC 1
#elif defined(__GNUC__)
#define IVAN_COMPILER_GCC 1
#elif defined(__clang__)
#define IVAN_COMPILER_CLANG 1
#else
#error This compiler is not supported
#endif


#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)
#ifndef IVAN_CPU_X86
#define IVAN_CPU_X86 1
#endif
#ifndef IVAN_CACHE_LINE_SIZE
#define IVAN_CACHE_LINE_SIZE 64
#endif
#elif defined(_M_PPC) || defined(__powerpc__) || defined(__powerpc64__)
#ifndef IVAN_CPU_PPC
#define IVAN_CPU_PPC 1
#endif
#ifndef IVAN_CACHE_LINE_SIZE
#define IVAN_CACHE_LINE_SIZE 128
#endif
#elif defined(__arm__)
#ifndef IVAN_CPU_ARM
#define IVAN_CPU_ARM 1
#endif
#ifndef IVAN_CACHE_LINE_SIZE
#define IVAN_CACHE_LINE_SIZE 64
#endif
#elif defined(__MIPSEL__) || defined(__mips_isa_rev)
#ifndef IVAN_CPU_MIPS
#define IVAN_CPU_MIPS 1
#endif
#ifndef IVAN_CACHE_LINE_SIZE
#define IVAN_CACHE_LINE_SIZE 64
#endif
#else
#error Unknown CPU type
#endif


#if defined(_WIN32) && !defined(__MINGW32__)
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#ifndef IVAN_TYPES_DEFINED
#define IVAN_TYPES_DEFINED

#ifdef IVAN_COMPILER_MSVC
#if _MSC_VER < 1300
typedef unsigned char uint8;
typedef signed char int8;
typedef unsigned short uint16;
typedef signed short int16;
typedef unsigned int uint32;
typedef signed int int32;
#else
typedef unsigned __int8 uint8;
typedef signed __int8 int8;
typedef unsigned __int16 uint16;
typedef signed __int16 int16;
typedef unsigned __int32 uint32;
typedef signed __int32 int32;
#endif
typedef unsigned __int64 uint64;
typedef signed __int64 int64;
#else
#include <stdint.h>
typedef uint8_t uint8;
typedef int8_t int8;
typedef uint16_t uint16;
typedef int16_t int16;
typedef uint32_t uint32;
typedef int32_t int32;
typedef uint64_t uint64;
typedef int64_t int64;
#endif
#endif

typedef float real32;
typedef double real64;

#include <stdio.h>
typedef uintptr_t uintptr;
typedef intptr_t  intptr;

typedef int32 bool32;

#define F32_MIN 1.175494351e-38F  
#define F32_MAX 3.402823466e+38F

#define Real32Maximum (F32_MAX)
#define Real32Minimum -(F32_MAX)

#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

#define GLOBAL_VARIABLE static
#define LOCAL_PERSIST static
#define INTERNAL_FUNCTION static

#define Assert(Expression) if(!(Expression)){*(int*)0 = 0;}
#define ArrayCount(Array) (sizeof(Array) / sizeof(Array[0]))

#define Align4(Value) ((Value + 3) & ~3)
#define Align8(Value) ((Value + 7) & ~7)
#define Align16(Value) ((Value + 15) & ~15)

#define INVALID_CODE_PATH Assert(!"Invalid code path!")
#define INVALID_DEFAULT_CASE default:{INVALID_CODE_PATH;} break;

typedef size_t memory_index;

#if IVAN_COMPILER_MSVC
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

#include "ivan_math.h"

#define UINT32_FROM_POINTER(Pointer) ((uint32)(memory_index)(Pointer))
#define POINTER_FROM_UINT32(type, Value) (type *)((memory_index)Value)

#define OffsetOf(type, Member) (uintptr_t)&(((type *)0)->Member)

/*Platform stuff*/
inline uint32
SafeTruncateUInt64(uint64 Value){
    Assert(Value <= 0xFFFFFFFF);
    uint32 Result = (uint32)Value;
    return(Result);
}

inline real32 SafeRatioN(real32 Numerator, real32 Divisor, real32 N){
    real32 Result = N;

    if(Divisor != 0.0f){
        Result = Numerator / Divisor;
    }

    return(Result);
}

inline real32 SafeRatio1(real32 Numerator, real32 Divisor){
    real32 Result = SafeRatioN(Numerator, Divisor, 1.0f);
    return(Result);
}

inline real32 SafeRatio0(real32 Numerator, real32 Divisor){
    real32 Result = SafeRatioN(Numerator, Divisor, 0.0f);
    return(Result);
}

typedef struct game_offscreen_buffer{
    void* Memory;
    int32 Width;
    int32 Height;
}game_offscreen_buffer;

typedef struct game_render_commands{
    uint32 Width;
    uint32 Height;

    uint32 MaxPushBufferSize;
    uint8* PushBufferBase;
    uint8* PushBufferDataAt;

    uint32 PushBufferElementCount;
} game_render_commands;

inline game_render_commands DefaultRenderCommands(
    uint32 MaxPushBufferSize,
    uint8* PushBufferBase,
    uint32 Width,
    uint32 Height)
{
    game_render_commands Commands;

    Commands.Width = Width;
    Commands.Height = Height;

    Commands.MaxPushBufferSize = MaxPushBufferSize;
    Commands.PushBufferBase = PushBufferBase;
    Commands.PushBufferDataAt = PushBufferBase;

    Commands.PushBufferElementCount = 0;

    return(Commands);
}

struct game_sound_output_buffer{
    int SamplesPerSecond;
    int SampleCount;
    int16* Samples;
};

struct debug_read_file_result{
    void* Contents;
    uint32 ContentsSize;
};
#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(char* FileName)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool32 name(char* FileName, void* Contents, uint32 ContentsSize)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(void* Memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

struct game_button_state{
    int HalfTransitionCount;
    bool32 EndedDown;
};

typedef struct platform_file_handle{
    bool32 NoErrors;
} platform_file_handle;

typedef struct platform_file_group{
    uint32 FileCount;
} platform_file_group;

#define PLATFORM_GET_ALL_FILES_OF_TYPE_BEGIN(name) platform_file_group* name(char* Type)
typedef PLATFORM_GET_ALL_FILES_OF_TYPE_BEGIN(platform_get_all_files_of_type_begin);

#define PLATFORM_GET_ALL_FILES_OF_TYPE_END(name) void name(platform_file_group* FileGroup)
typedef PLATFORM_GET_ALL_FILES_OF_TYPE_END(platform_get_all_files_of_type_end);

#define PLATFORM_OPEN_FILE(name) platform_file_handle *name(platform_file_group* FileGroup, uint32 FileIndex)
typedef PLATFORM_OPEN_FILE(platform_open_file);

#define PLATFORM_READ_DATA_FROM_FILE(name) void name(platform_file_handle* Source, uint64 Offset, uint64 Size, void* Dest)
typedef PLATFORM_READ_DATA_FROM_FILE(platform_read_data_from_file);

#define PLATFORM_FILE_ERROR(name) void name(platform_file_handle* Handle, char* Message)
typedef PLATFORM_FILE_ERROR(platform_file_error);

#define PlatformNoFileErrors(Handle) ((Handle)->NoErrors)

struct platform_work_queue;
#define PLATFORM_WORK_QUEUE_CALLBACK(name) void name(platform_work_queue* Queue, void* Data)
typedef PLATFORM_WORK_QUEUE_CALLBACK(platform_work_queue_callback);

#define PLATFORM_ADD_ENTRY(name) void name(platform_work_queue* Queue, platform_work_queue_callback* Callback , void* Data)
typedef PLATFORM_ADD_ENTRY(platform_add_entry);

#define PLATFORM_COMPLETE_ALL_WORK(name) void name(platform_work_queue* Queue)
typedef PLATFORM_COMPLETE_ALL_WORK(platform_complete_all_work);

#define PLATFORM_ALLOCATE_MEMORY(name) void* name(size_t Size)
typedef PLATFORM_ALLOCATE_MEMORY(platform_allocate_memory);

#define PLATFORM_DEALLOCATE_MEMORY(name) void name(void* Memory)
typedef PLATFORM_DEALLOCATE_MEMORY(platform_deallocate_memory);

struct ticket_mutex{
    uint64 volatile Ticket;
    uint64 volatile Serving;
};

struct platform_texture_op_queue{
    ticket_mutex Mutex;
    struct texture_op* First;
    texture_op* Last;
    texture_op* FirstFree;
};

typedef struct platform_api{
    platform_add_entry* AddEntry;
    platform_complete_all_work* CompleteAllWork;

    platform_get_all_files_of_type_begin* GetAllFilesOfTypeBegin;
    platform_get_all_files_of_type_end* GetAllFilesOfTypeEnd;
    platform_open_file* OpenNextFile;
    platform_read_data_from_file* ReadDataFromFile;
    platform_file_error* FileError;

    debug_platform_write_entire_file* DEBUGWriteEntireFile;
    debug_platform_read_entire_file* DEBUGReadEntireFile;
    debug_platform_free_file_memory* DEBUGFreeFileMemory;

    platform_allocate_memory* AllocateMemory;
    platform_deallocate_memory* DeallocateMemory;
} platform_api;

struct game_controller_input{
    bool32 IsAnalog;
    bool32 IsConnected;

    real32 StickAverageX;
    real32 StickAverageY;

    union{
        game_button_state Buttons[21];
        struct{
            game_button_state MoveUp;
            game_button_state MoveDown;
            game_button_state MoveLeft;
            game_button_state MoveRight;

            game_button_state ActionUp;
            game_button_state ActionDown;
            game_button_state ActionLeft;
            game_button_state ActionRight;

            game_button_state LeftShoulder;
            game_button_state RightShoulder;

            game_button_state Back;
            game_button_state Start;
            game_button_state Esc;

            game_button_state Ctrl;
            game_button_state Shift;
            game_button_state Space;

            game_button_state Home;
            game_button_state End;
            game_button_state Backspace;
            game_button_state Tab;
            game_button_state F4;

            //ADD BUTTONS ABOVE THIS LINE
            game_button_state Terminator;
        };
    };
};

enum game_input_mouse_button{
    PlatformMouseButton_Left,
    PlatformMouseButton_Middle,
    PlatformMouseButton_Right,
    PlatformMouseButton_Extended0,
    PlatformMouseButton_Extended1,

    PlatformMouseButton_Count,
};

struct game_input{
    //1 KEYBOARD, 4 GAMEPADS
    game_controller_input Controllers[5];

    real32 DeltaTime;

    bool32 QuitRequested;

    game_button_state MouseButtons[PlatformMouseButton_Count];
    vec3 MouseP;
    vec3 DeltaMouseP;
    bool32 CapturingMouse;

    bool32 ShiftDown;
    bool32 AltDown;
    bool32 ControlDown;
};

inline game_controller_input* GetController(game_input* Input, int ControllerIndex){
    Assert(ControllerIndex < ArrayCount(Input->Controllers));
    game_controller_input* Result = &Input->Controllers[ControllerIndex];
    return(Result);
}

inline bool32 WasPressed(game_button_state State){
    bool32 Result = ((State.HalfTransitionCount > 1) || ((State.HalfTransitionCount == 1) &&(State.EndedDown)));

    return(Result);
}

inline bool32 IsDown(game_button_state State){
    bool32 Result = (State.EndedDown);

    return(Result);
}

typedef struct game_memory{
    bool32 IsInitialized;

    uint64 PermanentStorageSize;
    void* PermanentStorage;

    uint64 TransientStorageSize;
    void* TransientStorage;

    uint64 DebugStorageSize;
    void* DebugStorage;

    struct debug_table* DebugTable;

    platform_work_queue* HighPriorityQueue;
    platform_work_queue* LowPriorityQueue;
    platform_work_queue* VoxelMeshQueue;

    platform_texture_op_queue TextureOpQueue;

    bool32 ExecutableReloaded;
    platform_api PlatformAPI;
} game_memory;

extern game_memory* DebugGlobalMemory;

#if IVAN_COMPILER_MSVC
#define IVAN_COMPLETE_WRITES_BEFORE_FUTURE _WriteBarrier();
#define IVAN_COMPLETE_READS_BEFORE_FUTURE _ReadBarrier();
#elif IVAN_COMPILER_GCC
#define IVAN_COMPLETE_WRITES_BEFORE_FUTURE asm volatile("" ::: "memory");
#define IVAN_COMPLETE_READS_BEFORE_FUTURE asm volatile("" ::: "memory");
#endif

#if IVAN_COMPILER_MSVC
inline uint32 AtomicCompareExchangeU32(uint32 volatile *Value, uint32 New, uint32 Comparand){
    uint32 Result = _InterlockedCompareExchange((long volatile*)Value, New, Comparand);
    return(Result);
}

//NOTE(Dima): Returns the original value
inline uint64 AtomicAddU64(uint64 volatile* Value, uint64 Addend){
    uint64 Result = _InterlockedExchangeAdd64((__int64*)Value, Addend);
    return(Result);
}

inline uint64 AtomicExchangeU64(uint64 volatile* Value, uint64 New){
    uint64 Result = _InterlockedExchange64((__int64*)Value, New);
    return(Result);
}

inline uint32 GetThreadID(void){
    uint8* ThreadLocalStorage = (uint8*)__readgsqword(0x30);
    uint32 ThreadID = *(uint32*)(ThreadLocalStorage + 0x48);

    return(ThreadID);
}

#elif IVAN_COMPILER_GCC
inline uint32 AtomicCompareExchangeU32(uint32 volatile *Value, uint32 New, uint32 Comparand){
    uint32 Result = __sync_val_compare_and_swap(Value, Comparand, New);
    return(Result);
}

inline uint64 = AtomicExchangeU64(uint64 volatile* Value, uint64 New){
    uint64 Result = __sync_lock_test_and_set(Value, New);

    return(Result);
}

inline uint64 AtomicAddU64(uint64 volatile* Value, uint64 Addend){
    uint64 Result = __sync_fetch_and_add(Value, Addend);

    return(Result);
}

inline uint32 GetThreadID(){
    uint32 ThreadID;
#if defined(__APPLE__) && defined(__x86_64__)
    asm("mov %%gs:0x00,%0" : "=r"(ThreadID));
#elif defined(__i386__)
    asm("mov %%gs:0x08,%0" : "=r"(ThreadID));
#elif defined(__x86_64__)
    asm("mov %%fs:0x10,%0" : "=r"(ThreadID));
#else
#error Unsupported architecture
#endif
}
#endif

#define GAME_UPDATE_AND_RENDER(name) void name(game_memory* Memory, game_input* Input, game_offscreen_buffer* Buffer, game_render_commands* RenderCommands)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

#define GAME_GET_SOUND_SAMPLES(name) void name(game_memory* Memory, game_sound_output_buffer* SoundOutput)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);

#define DEBUG_GAME_FRAME_END(name) void name(game_memory *Memory, game_input *Input, game_render_commands *RenderCommands)
typedef DEBUG_GAME_FRAME_END(debug_game_frame_end);

inline void BeginTicketMutex(ticket_mutex* Mutex){
    uint64 Ticket = AtomicAddU64(&Mutex->Ticket, 1);
    while(Ticket != Mutex->Serving){_mm_pause();}
}

inline void EndTicketMutex(ticket_mutex* Mutex){
    AtomicAddU64(&Mutex->Serving, 1);
}

#endif
