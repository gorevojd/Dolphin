#ifndef IVAN_PLATFORM_H
#define IVAN_PLATFORM_H

#include <stdio.h>

#define GD_IMPLEMENTATION
#include "E:/Programming/MyProjects/GD_LIBS/gd.h"

#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"


#define GLOBAL_VARIABLE static
#define LOCAL_PERSIST static
#define INTERNAL_FUNCTION static

#define Assert(Expression) if(!(Expression)){*(int*)0 = 0;}
#define ArrayCount(Array) (sizeof(Array) / sizeof(Array[0]))

#define INVALID_CODE_PATH Assert(!"Invalid code path!")
#define INVALID_DEFAULT_CASE default:{INVALID_CODE_PATH;} break;

#define INTERNAL_BUILD

typedef size_t memory_index;

#if GD_COMPILER_MSVC
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

struct game_input{
    //1 KEYBOARD, 4 GAMEPADS
    game_controller_input Controllers[5];

    real32 DeltaTime;

    vec3 MouseP;
    game_button_state MouseButtons[5];
};

inline game_controller_input* GetController(game_input* Input, int ControllerIndex){
    Assert(ControllerIndex < ArrayCount(Input->Controllers));
    game_controller_input* Result = &Input->Controllers[ControllerIndex];
    return(Result);
}

enum{
    DebugCycleCounter_GameUpdateAndRender,
    DebugCycleCounter_RenderRectangleQuickly,
    DebugCycleCounter_RenderRectangleQuicklyCounted,
    DebugCycleCounter_RenderRectangleSlowly,
    DebugCycleCounter_RenderRectangleSlowlyCounted,
    DebugCycleCounter_RenderRectangle,

    DebugCycleCounter_Count
};

struct debug_cycle_counter{
    uint32 CycleCount;
    uint32 HitCount;
};


//extern platform_add_entry* PlatformAddEntry;
//extern platform_complete_all_work* PlatformCompleteAllWork;

typedef struct game_memory{
    bool32 IsInitialized;

    uint32 PermanentStorageSize;
    void* PermanentStorage;

    uint32 TransientStorageSize;
    void* TransientStorage;

    platform_work_queue* HighPriorityQueue;
    platform_work_queue* LowPriorityQueue;

    platform_texture_op_queue TextureOpQueue;

    platform_api PlatformAPI;

#ifdef INTERNAL_BUILD
    debug_cycle_counter Counters[DebugCycleCounter_Count];
#endif
} game_memory;
extern game_memory* DebugGlobalMemory;

#if GD_COMPILER_MSVC
#define GD_COMPLETE_WRITES_BEFORE_FUTURE _WriteBarrier();
#define GD_COMPLETE_READS_BEFORE_FUTURE _ReadBarrier();
#elif GD_COMPILER_GCC
#define GD_COMPLETE_WRITES_BEFORE_FUTURE asm volatile("" ::: "memory");
#define GD_COMPLETE_READS_BEFORE_FUTURE asm volatile("" ::: "memory");
#endif

#if GD_COMPILER_MSVC
inline uint32 AtomicCompareExchangeUInt32(uint32 volatile *Value, uint32 New, uint32 Comparand){
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

inline 

#elif GD_COMPILER_GCC
inline uint32 AtomicCompareExchangeUInt32(uint32 volatile *Value, uint32 New, uint32 Comparand){
    uint32 Result = __sync_val_compare_and_swap(Value, Comparand, New);
    return(Result);
}
#endif

#define GAME_UPDATE_AND_RENDER(name) void name(game_memory* Memory, game_input* Input, game_offscreen_buffer* Buffer, game_sound_output_buffer* SoundOutput)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);
GAME_UPDATE_AND_RENDER(GameUpdateAndRenderStub){

}

#define GAME_GET_SOUND_SAMPLES(name) void name(game_memory* Memory, game_sound_output_buffer* SoundOutput)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);
GAME_GET_SOUND_SAMPLES(GameGetSoundSamplesStub){

}

inline void BeginTicketMutex(ticket_mutex* Mutex){
    uint64 Ticket = AtomicAddU64(&Mutex->Ticket, 1);
    while(Ticket != Mutex->Serving){_mm_pause();}
}

inline void EndTicketMutex(ticket_mutex* Mutex){
    AtomicAddU64(&Mutex->Serving, 1);
}

#endif
