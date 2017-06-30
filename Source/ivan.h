#ifndef IVAN_H
#define IVAN_H

#include "ivan_platform.h"
#include "ivan_shared.h"
#include "ivan_memory.h"
#include "ivan_debug.h"

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

#include "ivan_voxel_world.h"
#include "ivan_voxel_mesh.h"

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
	task_with_memory MeshTasks[300];
	task_with_memory ChunkTasks[300];

	loaded_bitmap GroundBitmap;
	platform_work_queue* HighPriorityQueue;
	platform_work_queue* LowPriorityQueue;

	uint32 MainGenerationID;
	
#if 0
	voxel_chunk_header VoxelChunkSentinel;

	voxel_chunk VoxelChunk;
	voxel_chunk_mesh MeshResult;
	voxel_atlas_id VoxelAtlasID;
#else
	voxel_chunk_manager* VoxelChunkManager;
#endif
};

GLOBAL_VARIABLE platform_api Platform;


INTERNAL_FUNCTION task_with_memory* BeginTaskWithMemory(struct transient_state* TranState);
INTERNAL_FUNCTION task_with_memory* BeginChunkTaskWithMemory(transient_state* TranState);
INTERNAL_FUNCTION task_with_memory* BeginMeshTaskWithMemory(transient_state* TranState);
INTERNAL_FUNCTION void EndTaskWithMemory(task_with_memory* Task);

#endif