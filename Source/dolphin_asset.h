#ifndef DOLPHIN_ASSET_H
#define DOLPHIN_ASSET_H

#include "dolphin_platform.h"
#include "dolphin_intrinsics.h"

struct loaded_sound{
	uint32 SampleCount;
	uint32 ChannelCount;

	int16* Samples[2];
};

struct hero_bitmaps{
	vec2 Align;

	loaded_bitmap Head;
	loaded_bitmap Torso;
	loaded_bitmap Cape;
};

struct asset_type{
	uint32 FirstAssetIndex;
	uint32 OnePastLastAssetIndex;
};

struct asset_vector{
	real32 Data[Tag_Count];
};

struct asset_bitmap_info{
	char* FileName;
	vec2 AlignPercentage;
};

struct asset_sound_info{
	char* FileName;
	uint32 FirstSampleIndex;
	uint32 SampleCount;
	sound_id NextIDToPlay;
};

enum asset_state{
	AssetState_Unloaded,
	AssetState_Queued,
	AssetState_Loaded,
};

struct asset{
	uint32 FirstTagIndex;
	uint32 OnePastLastTagIndex;

	union{
		asset_bitmap_info Bitmap;
		asset_sound_info Sound;
	};
};

struct asset_slot{
	asset_state State;
	union{
		loaded_bitmap* Bitmap;
		loaded_sound* Sound;
	};
};

struct asset_tag{
	uint32 ID;
	real32 Value;
};

struct game_assets{
	memory_arena Arena;
	struct transient_state* TranState;

	real32 TagRange[Tag_Count];

	uint32 TagCount;
	dda_tag* Tags;
	
	uint32 AssetCount;
	dda_asset* Assets;
	asset_slot* Slots;

	asset_type AssetTypes[Asset_Count];

	uint8* DDAContents;

	uint32 DEBUGUsedBitmapCount;
	uint32 DEBUGUsedSoundCount;
	uint32 DEBUGUsedAssetCount;
	uint32 DEBUGUsedTagCount;
	asset_type *DEBUGAssetType;
	asset *DEBUGAsset;
};


inline loaded_bitmap* GetBitmap(game_assets* Assets, bitmap_id ID){
	loaded_bitmap* Result = Assets->Slots[ID.Value].Bitmap;
	
	return(Result);
}

inline loaded_sound* GetSound(game_assets* Assets, sound_id ID){
	loaded_sound* Result = Assets->Slots[ID.Value].Sound;

	return(Result);
}

inline dda_sound* GetSoundInfo(game_assets* Assets, sound_id ID){
	Assert(ID.Value <= Assets->AssetCount);
	dda_sound* Result = &Assets->Assets[ID.Value].Sound;

	return(Result);
}

inline bool32 IsValid(bitmap_id ID){
	bool32 Result = (ID.Value != 0);
	return(Result);
}

inline bool32 IsValid(sound_id ID){
	bool32 Result = (ID.Value != 0);
	return(Result);
}

INTERNAL_FUNCTION void LoadBitmapAsset(game_assets* Assets, bitmap_id ID);
INTERNAL_FUNCTION void LoadSoundAsset(game_assets* Assets, sound_id ID);

#endif