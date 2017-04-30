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

struct task_with_memory{
    bool32 BeingUsed;
    memory_arena Arena;
    bool32 DependsOnGameMode;

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

struct game_state{
	memory_arena PermanentArena;

	vec2 PlayerPos;
	real32 HeroFacingDirection;

	loaded_sound TestSound;
	uint32 TestSampleIndex;

	audio_state AudioState;

	real32 Time;

	random_series EffectsSeries;
	particle_cache FontainCache;

	particle Particles[256];
	uint32 NextParticle;
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

GLOBAL_VARIABLE platform_api Platform;


INTERNAL_FUNCTION task_with_memory* BeginTaskWithMemory(struct transient_state* TranState, bool32 DependsOnGameMode);
INTERNAL_FUNCTION void EndTaskWithMemory(task_with_memory* Task);

#endif