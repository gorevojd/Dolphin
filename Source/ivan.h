#ifndef IVAN_H
#define IVAN_H

/*
 	TODO(DIMA)
 		build single voxel mesh, that we will send to GPU(CHANGE SHADER!)
		build character atlases for fonts.
		DEBUG UI 		
*/

#include "ivan_platform.h"
#include "ivan_shared.h"
#include "ivan_memory.h"

#include "ivan_intrinsics.h"
#include "ivan_random.h"
#include "ivan_simd.h"
#include "ivan_render.h"
#include "ivan_render_group.h"
#include "ivan_file_formats.h"
#include "ivan_asset.h"
#include "ivan_audio.h"
#include "ivan_particle.h"
#include "ivan_render.h"
#include "ivan_anim.h"

#include "ivan_voxel_world.h"
#include "ivan_voxel_mesh.h"

#include "ivan_debug_interface.h"
#include "ivan_debug.h"

struct task_with_memory{
    bool32 BeingUsed;
    memory_arena Arena;

    temporary_memory Memory;
};

struct particle{
	vec3 P;
	vec3 dP;
	vec3 ddP;
	vec4 Color;
	vec4 dColor;
};

struct hero_bitmap_ids{
    bitmap_id Head;
    bitmap_id Cape;
    bitmap_id Torso;
};

struct camera_transform{
	vec3 P;

	float Yaw;
	float Pitch;
	float Roll;

	vec3 Front;
	vec3 Up;
	vec3 Left;
};

struct game_state{
	memory_arena PermanentArena;

	vec2 PlayerPos;
	real32 HeroFacingDirection;

	audio_state AudioState;
	animator_controller AnimatorController;

	real32 Time;
	vec3 LastMouseP;

	random_series EffectsSeries;
	particle_cache FontainCache;

	camera_transform Camera;
};

struct transient_state{
	game_assets* Assets;
	
	bool32 IsInitialized;
	memory_arena TranArena;
	
	task_with_memory Tasks[4];
	int32_t TasksInUse;
	task_with_memory MeshTasks[4096];
	int32_t MeshTasksInUse;
	task_with_memory ChunkTasks[4096];
	int32_t ChunkTasksInUse;

	loaded_bitmap GroundBitmap;
	platform_work_queue* HighPriorityQueue;
	platform_work_queue* LowPriorityQueue;
	platform_work_queue* VoxelMeshQueue;

	uint32 MainGenerationID;
	
	voxel_chunk_manager* VoxelChunkManager;
};

GLOBAL_VARIABLE platform_api Platform;

INTERNAL_FUNCTION task_with_memory* BeginTaskWithMemory(struct transient_state* TranState);
INTERNAL_FUNCTION task_with_memory* BeginChunkTaskWithMemory(transient_state* TranState);
INTERNAL_FUNCTION task_with_memory* BeginMeshTaskWithMemory(transient_state* TranState);
INTERNAL_FUNCTION void EndTaskWithMemory(task_with_memory* Task);

#endif