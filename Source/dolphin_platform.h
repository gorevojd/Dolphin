#ifndef DOLPHIN_PLATFORM_H
#define DOLPHIN_PLATFORM_H

#include <stdio.h>

#define GD_IMPLEMENTATION
#include "E:/Programming/MyProjects/GD_LIBS/gd.h"

#define GD_MATH_IMPLEMENTATION
#define GD_MATH_STATIC  
#include "E:/Programming/MyProjects/GD_LIBS/gd_math.h"

#define GLOBAL_VARIABLE static
#define LOCAL_PERSIST static
#define INTERNAL_FUNCTION static

#define Assert(Expression) if(!(Expression)){*(int*)0 = 0;}
#define ArrayCount(Array) (sizeof(Array) / sizeof(Array[0]))

#define INVALID_CODE_PATH Assert(!"Invalid code path!")
#define INVALID_DEFAULT_CASE default:{INVALID_CODE_PATH;} break;

#define INTERNAL_BUILD

#if GD_COMPILER_MSVC
#include <intrin.h>
#else  
#include <x86intrin.h>
#endif



/*Some memory and arenas stuff*/
struct memory_arena{
	size_t MemoryUsed;
	void* BaseAddress;
	size_t MemorySize;
	int TempCount;
};

struct temporary_memory{
	memory_arena* Arena;
	size_t Used;
};

struct task_with_memory{
	bool32 BeingUsed;
	memory_arena Arena;

	temporary_memory Memory;
};

inline temporary_memory
BeginTemporaryMemory(memory_arena* Arena){
	temporary_memory Result;

	Result.Arena = Arena;
	Result.Used = Arena->MemoryUsed;
	++Arena->TempCount;

	return(Result);
}

inline void EndTemporaryMemory(temporary_memory TempMem){
	memory_arena* Arena = TempMem.Arena;
	Assert(Arena->MemoryUsed >= TempMem.Used);
	Arena->MemoryUsed = TempMem.Used;
	Assert(Arena->TempCount > 0);
	--Arena->TempCount;
}


inline size_t
GetAlignmentOffset(memory_arena* Arena, size_t Alignment){
	size_t AlignmentOffset = 0;
	size_t ResultPointer = (size_t)Arena->BaseAddress + Arena->MemoryUsed;
	size_t AlignmentMask = Alignment - 1;
	if (ResultPointer & AlignmentMask){
		AlignmentOffset = Alignment - (ResultPointer & AlignmentMask);
	}
	return(AlignmentOffset);
}

#define PushStruct(Arena, Type, ...) (Type*)PushSize_(Arena, sizeof(Type), ## __VA_ARGS__)
#define PushArray(Arena, Count , Type, ...) (Type*)PushSize_(Arena, (Count)*sizeof(Type), ## __VA_ARGS__)
#define PushSize(Arena, Size, ...) PushSize_(Arena, Size, ## __VA_ARGS__)

inline void* PushSize_(memory_arena* Arena, size_t SizeInit, size_t Alignment = 4){
	size_t Size = SizeInit;
	size_t AlignmentOffset = GetAlignmentOffset(Arena, Alignment);
	Size += AlignmentOffset;

	Assert(Arena->MemoryUsed + Size <= Arena->MemorySize);
	void* Result = (uint8*)Arena->BaseAddress + Arena->MemoryUsed + AlignmentOffset;
	Arena->MemoryUsed += Size;

	Assert(Size >= SizeInit);

	return(Result);
}

inline char* 
PushString(memory_arena* Arena, char* Source){
    uint32 BufferSize = 1;
    for(char* It = Source;
        *It;
        It++)
    {
        BufferSize++;
    }

    char* Dest = (char*)PushSize_(Arena, BufferSize);
    for(uint32 CharIndex = 0;
        CharIndex < BufferSize;
        CharIndex++)
    {
        Dest[CharIndex] = Source[CharIndex];
    }

    return(Dest);
}

inline void ZeroSize(void* Memory, size_t Size){
	uint8* Ptr = (uint8*)Memory;
	for (int i = 0; i < Size; i++){
		*Ptr = 0;
	}
}

inline void SubArena(memory_arena* Result, memory_arena* Arena, size_t Size, size_t Alignment = 16){
	Result->MemorySize = Size;
	Result->BaseAddress = (uint8*)PushSize_(Arena, Size, Alignment);
	Result->MemoryUsed = 0;
	Result->TempCount = 0;
}

inline void InitializeMemoryArena(memory_arena* Arena, size_t Size, void* BaseAddress){
	Arena->BaseAddress = BaseAddress;
	Arena->MemoryUsed = 0;
	Arena->MemorySize = Size;
}

extern task_with_memory* BeginTaskWithMemory(struct transient_state* TranState);
extern void EndTaskWithMemory(task_with_memory* Task);


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

    gdVec3 MouseP;
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

#ifdef _MSC_VER
#define BEGIN_TIMED_BLOCK(func_name) uint64 Start_##func_name = __rdtsc();
#define END_TIMED_BLOCK(func_name) DebugGlobalMemory->Counters[DebugCycleCounter_##func_name].CycleCount += __rdtsc() - Start_##func_name;  \
    DebugGlobalMemory->Counters[DebugCycleCounter_##func_name].HitCount++;
#define END_TIMED_BLOCK_COUNTED(func_name, Count) DebugGlobalMemory->Counters[DebugCycleCounter_##func_name].CycleCount += __rdtsc() - Start_##func_name;   \
    DebugGlobalMemory->Counters[DebugCycleCounter_##func_name].HitCount += (Count);
#else
#define BEGIN_TIMED_BLOCK()
#define END_TIMED_BLOCK()
#define END_TIMED_BLOCK_COUNTED()
#endif

struct platform_work_queue;
#define PLATFORM_WORK_QUEUE_CALLBACK(name) void name(platform_work_queue* Queue, void* Data)
typedef PLATFORM_WORK_QUEUE_CALLBACK(platform_work_queue_callback);

typedef void platform_add_entry(platform_work_queue* Queue, platform_work_queue_callback* Callback, void* Data);
typedef void platform_complete_all_work(platform_work_queue* Queue);
extern platform_add_entry* PlatformAddEntry;
extern platform_complete_all_work* PlatformCompleteAllWork;

typedef struct game_memory{
    bool32 IsInitialized;

    uint32 PermanentStorageSize;
    void* PermanentStorage;

    uint32 TransientStorageSize;
    void* TransientStorage;

    platform_work_queue* HighPriorityQueue;
    platform_work_queue* LowPriorityQueue;

    platform_add_entry* PlatformAddEntry;
    platform_complete_all_work* PlatformCompleteAllWork;

    debug_platform_write_entire_file* DEBUGWriteEntireFile;
    debug_platform_read_entire_file* DEBUGReadEntireFile;
    debug_platform_free_file_memory* DEBUGFreeFileMemory;

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
    uint32 Result = _InterlockedCompareExchange((long*)Value, New, Comparand);
    return(Result);
}
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


#endif
