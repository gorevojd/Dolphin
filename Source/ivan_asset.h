#ifndef IVAN_ASSET_H
#define IVAN_ASSET_H

#include "ivan_platform.h"
#include "ivan_intrinsics.h"

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

struct asset{
	dda_asset DDA;
	uint32 FileIndex;
};

struct asset_type{
	uint32 FirstAssetIndex;
	uint32 OnePastLastAssetIndex;
};

struct asset_vector{
	real32 Data[Tag_Count];
};

enum asset_state{
	AssetState_Unloaded,
	AssetState_Queued,
	AssetState_Loaded,
};

struct asset_slot{
	asset_state State;
	union{
		loaded_bitmap* Bitmap;
		loaded_sound* Sound;
	};
};

struct asset_file{
	platform_file_handle* Handle;

	dda_header Header;
	dda_asset_type* AssetTypeArray;

	uint32 TagBase;
};

struct game_assets{
	memory_arena Arena;
	struct transient_state* TranState;

	real32 TagRange[Tag_Count];

	uint32 TagCount;
	dda_tag* Tags;
	
	uint32 AssetCount;
	asset* Assets;
	asset_slot* Slots;

	asset_type AssetTypes[Asset_Count];

	uint32 FileCount;
	asset_file* Files;

	uint8* DDAContents;
};


inline loaded_bitmap* GetBitmap(game_assets* Assets, bitmap_id ID){
	Assert(ID.Value <= Assets->AssetCount);
	asset_slot* Slot = Assets->Slots + ID.Value;

	loaded_bitmap* Result = 0;
	if(Slot->State >= AssetState_Loaded){
		GD_COMPLETE_READS_BEFORE_FUTURE;
		Result = Slot->Bitmap;
	}

	return(Result);
}

inline loaded_sound* GetSound(game_assets* Assets, sound_id ID){
	Assert(ID.Value <= Assets->AssetCount);
	asset_slot* Slot = Assets->Slots + ID.Value;

	loaded_sound* Result = 0;
	if(Slot->State >= AssetState_Loaded){
		GD_COMPLETE_READS_BEFORE_FUTURE;
		Result = Slot->Sound;
	}

	return(Result);
}

inline dda_sound* GetSoundInfo(game_assets* Assets, sound_id ID){
	Assert(ID.Value <= Assets->AssetCount);
	dda_sound* Result = &Assets->Assets[ID.Value].DDA.Sound;

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

inline sound_id GetNextSoundInChain(game_assets* Assets, sound_id ID){
	sound_id Result = {};

	dda_sound* Info = GetSoundInfo(Assets, ID);
	switch(Info->Chain){
		case(DDASoundChain_None):{

		}break;
		case(DDASoundChain_Loop):{
			Result = ID;
		}break;
		case(DDASoundChain_Advance):{
			Result.Value = ID.Value + 1;
		}break;
		default:{

		}break;
	}

	return(Result);
}

#endif