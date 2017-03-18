#ifndef IVAN_H
#define IVAN_H

#include "ivan_platform.h"

#include "ivan_intrinsics.h"
#include "ivan_render_group.h"
#include "ivan_file_formats.h"
#include "ivan_asset.h"
#include "ivan_random.h"
#include "ivan_audio.h"

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

	uint32 NextParticle;
	particle Particles[256];
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

#endif