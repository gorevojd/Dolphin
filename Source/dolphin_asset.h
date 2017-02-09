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
	gdVec2 Align;

	loaded_bitmap Head;
	loaded_bitmap Torso;
	loaded_bitmap Cape;
};

enum asset_tag_id{	
	Tag_FacingDirection,

	Tag_Count,
};

enum asset_type_id{
	Asset_None,

	Asset_LastOfUs,
	Asset_StarWars,
	Asset_Witcher,
	Asset_Assassin,
	Asset_Tree,
	Asset_Backdrop,

	Asset_Grass,
	Asset_Tuft,
	Asset_Stone,

    Asset_Head,
    Asset_Cape,
    Asset_Torso,

	Asset_Bloop,
	Asset_Crack,
	Asset_Drop,
	Asset_Glide,
	Asset_Music,
	Asset_Puhp,

	Asset_Count,
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
	gdVec2 AlignPercentage;
};

struct asset_sound_info{
	char* FileName;
};

enum asset_state{
	AssetState_Unloaded,
	AssetState_Queued,
	AssetState_Loaded,
};

struct asset{
	uint32 SlotID;
	uint32 FirstTagIndex;
	uint32 OnePastLastTagIndex;
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

	uint32 BitmapCount;
	asset_bitmap_info* BitmapInfos;
	asset_slot* Bitmaps;

	uint32 SoundCount;
	asset_sound_info* SoundInfos;
	asset_slot* Sounds;

	uint32 AssetCount;
	asset* Assets;

	uint32 TagCount;
	asset_tag* Tags;

	asset_type AssetTypes[Asset_Count];

	uint32 DEBUGUsedBitmapCount;
	uint32 DEBUGUsedSoundCount;
	uint32 DEBUGUsedAssetCount;
	uint32 DEBUGUsedTagCount;
	asset_type *DEBUGAssetType;
	asset *DEBUGAsset;
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

inline loaded_sound* GetSound(game_assets* Assets, sound_id ID){
	loaded_sound* Result = Assets->Sounds[ID.Value].Sound;

	return(Result);
}

inline asset_sound_info* GetSoundInfo(game_assets* Assets, sound_id ID){
	Assert(ID.Value <= Assets->SoundCount);
	asset_sound_info* Result = Assets->SoundInfos + ID.Value;

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