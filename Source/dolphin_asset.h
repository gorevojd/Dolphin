#ifndef DOLPHIN_ASSET_H
#define DOLPHIN_ASSET_H

#include "dolphin_platform.h"
#include "dolphin_intrinsics.h"


struct hero_bitmaps{
	gdVec2 Align;

	loaded_bitmap Head;
	loaded_bitmap Torso;
	loaded_bitmap Cape;
};

enum asset_type_id{
	Asset_None,

	Asset_LastOfUs,
	Asset_StarWars,
	Asset_Witcher,
	Asset_Assassin,
	Asset_Tree,
	Asset_Backdrop,

	Asset_Count,
};

struct asset_type{
	uint32 FirstAssetIndex;
	uint32 OnePastLastAssetIndex;
};

enum asset_state{
	AssetState_Unloaded,
	AssetState_Queued,
	AssetState_Loaded,
};

struct asset{
	uint32 SlotID;
};

struct asset_slot{
	asset_state State;
	loaded_bitmap* Bitmap;
};

struct game_assets{
	memory_arena Arena;
	struct transient_state* TranState;

	uint32 BitmapCount;
	asset_slot* Bitmaps;

	uint32 SoundCount;
	asset_slot* Sounds;

	uint32 AssetCount;
	asset* Assets;

	hero_bitmaps HeroBitmaps[4];

	loaded_bitmap Grasses[2];
	loaded_bitmap Stones[4];
	loaded_bitmap Tufts[3];

	asset_type AssetTypes[Asset_Count];
};

struct bitmap_id{
	uint32 Value;
};

struct sound_id{
	uint32 Value;
};

inline loaded_bitmap* GetBitmap(game_assets* Assets, bitmap_id ID){
	loaded_bitmap* Result = Assets->Bitmaps[ID.Value].Bitmap;
	return(Result);
}

INTERNAL_FUNCTION void LoadBitmapAsset(game_assets* Assets, bitmap_id ID);
INTERNAL_FUNCTION void LoadSoundAsset(game_assets* Assets, sound_id ID);

#endif