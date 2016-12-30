#ifndef DOLPHIN_H
#define DOLPHIN_H

#include "dolphin_platform.h"
#include "dolphin_intrinsics.h"
#include "dolphin_render_group.h"
#include "dolphin_asset.h"

#define GD_MATH_IMPLEMENTATION
#define GD_MATH_STATIC
#include "E:/Programming/MyProjects/GD_LIBS/gd_math.h"


struct hero_bitmaps{

	gdVec2 Align;

	loaded_bitmap Head;
	loaded_bitmap Torso;
	loaded_bitmap Cape;
};

enum game_assets_id{
	GAI_LastOfUs,
	GAI_Tree,
	GAI_Backdrop,

	GAI_Count
};

struct game_assets{
	hero_bitmaps HeroBitmaps[4];

	loaded_bitmap Grasses[2];
	loaded_bitmap Stones[4];
	loaded_bitmap Tufts[3];

	loaded_bitmap Bitmaps[GAI_Count];
};

inline loaded_bitmap* GetBitmap(game_assets* Assets, game_assets_id Id){
	loaded_bitmap* Result = Assets->Bitmaps + Id;
	return(Result);
}


struct game_state{
	memory_arena PermanentArena;

	game_assets Assets;

	gdVec2 PlayerPos;
	int32 HeroFacingDirection;

	real32 Time;
};

struct transient_state{
	memory_arena TranArena;
	bool32 IsInitialized;
	
	loaded_bitmap GroundBitmap;
};

#endif