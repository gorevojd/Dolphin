#ifndef DOLPHIN_PLATFORM_H
#define DOLPHIN_PLATFORM_H

#include <stdio.h>
#define GD_IMPLEMENTATION
#include "E:/Programming/MyProjects/GDA/gd.h"

#define	GD_MATH_IMPLEMENTATION
#define GD_MATH_STATIC	
#include "E:/Programming/MyProjects/GDA/gd_math.h"

#define GLOBAL_VARIABLE static
#define LOCAL_PERSIST static
#define INTERNAL_FUNCTION static

#define Assert(Expression) if(!(Expression)){*(int*)0 = 0;}
#define ArrayCount(Array) (sizeof(Array) / sizeof(Array[0]))

#define INVALID_CODE_PATH Assert(!"Invalid code path!")
#define INVALID_DEFAULT_CASE default:{INVALID_CODE_PATH;} break;

#define INTERNAL_BUILD


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

#define PushStruct(Arena, Type) (Type*)PushSize_(Arena, sizeof(Type))
#define PushArray(Arena, Count , Type) (Type*)PushSize_(Arena, (Count)*sizeof(Type))
#define PushSize(Arena, Size) PushSize_(Arena, Size)

inline void* PushSize_(memory_arena* Arena, size_t Size){
	Assert(Arena->MemoryUsed + Size <= Arena->MemorySize);
	void* Result = (uint8*)Arena->BaseAddress + Arena->MemoryUsed;
	Arena->MemoryUsed += Size;
	return(Result);
}

inline void ZeroSize(void* Memory, size_t Size){
	uint8* Ptr = (uint8*)Memory;
	for (int i = 0; i < Size; i++){
		*Ptr = 0;
	}
}

inline void InitializeMemoryArena(memory_arena* Arena, size_t Size, void* BaseAddress){
	Arena->BaseAddress = BaseAddress;
	Arena->MemoryUsed = 0;
	Arena->MemorySize = Size;
}

inline uint32
SafeTruncateUInt64(uint64 Value){
	Assert(Value <= 0xFFFFFFFF);
	uint32 Result = (uint32)Value;
	return(Result);
}

typedef struct thread_context{
	int Placeholder;
} thread_context;

typedef struct game_offscreen_buffer{
	int32 Width;
	int32 Height;
	int32 BytesPerPixel;
	void* Memory;
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
#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(thread_context* Thread, char* FileName)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool32 name(thread_context* Thread, const char* FileName, void* Contents, uint32 ContentsSize)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(thread_context* Thread, void* Memory)
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

	DebugCycleCounter_Count
};

struct debug_cycle_counter{
	uint32 CycleCount;
	uint32 HitCount;
};

extern struct game_memory* DebugGlobalMemory;
#ifdef _MSC_VER
#define BEGIN_TIMED_BLOCK(func_name) uint64 Start_##func_name = __rdtsc();
#define END_TIMED_BLOCK(func_name) DebugGlobalMemory->Counters[DebugCycleCounter_##func_name].CycleCount += __rdtsc() - Start_##func_name;	\
	DebugGlobalMemory->Counters[DebugCycleCounter_##func_name].HitCount++;
#define END_TIMED_BLOCK_COUNTED(func_name, Count) DebugGlobalMemory->Counters[DebugCycleCounter_##func_name].CycleCount += __rdtsc() - Start_##func_name;	\
	DebugGlobalMemory->Counters[DebugCycleCounter_##func_name].HitCount += (Count);
#else
#define BEGIN_TIMED_BLOCK()
#define END_TIMED_BLOCK()
#define END_TIMED_BLOCK_COUNTED()
#endif

typedef struct game_memory{
	bool32 IsInitialized;

	uint32 MemoryBlockSize;
	void* MemoryBlock;

	uint32 TempMemoryBlockSize;
	void* TempMemoryBlock;


	debug_platform_write_entire_file* DEBUGWriteEntireFile;
	debug_platform_read_entire_file* DEBUGReadEntireFile;
	debug_platform_free_file_memory* DEBUGFreeFileMemory;

#ifdef INTERNAL_BUILD
	debug_cycle_counter Counters[DebugCycleCounter_Count];
#endif
} game_memory;


#define GAME_UPDATE_AND_RENDER(name) void name(thread_context* Thread, game_memory* Memory, game_input* Input, game_offscreen_buffer* Buffer, game_sound_output_buffer* SoundOutput)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);
GAME_UPDATE_AND_RENDER(GameUpdateAndRender);



#endif