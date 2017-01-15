#ifndef DOLPHIN_H
#define DOLPHIN_H

#include "dolphin_platform.h"

#include "dolphin_intrinsics.h"
#include "dolphin_render_group.h"
#include "dolphin_asset.h"

struct game_state{
	memory_arena PermanentArena;

	gdVec2 PlayerPos;
	int32 HeroFacingDirection;

	real32 Time;
};

struct transient_state{
	
	bool32 IsInitialized;
	memory_arena TranArena;
	
	task_with_memory Tasks[4];

	loaded_bitmap GroundBitmap;
	platform_work_queue* HighPriorityQueue;
	platform_work_queue* LowPriorityQueue;

	game_assets* Assets;
};

GLOBAL_VARIABLE debug_platform_read_entire_file *PlatformReadEntireFile;

#endif