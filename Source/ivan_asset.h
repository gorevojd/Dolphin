#ifndef IVAN_ASSET_H
#define IVAN_ASSET_H

#include "ivan_platform.h"
#include "ivan_intrinsics.h"

struct loaded_sound{
	uint32 SampleCount;
	uint32 ChannelCount;

	int16* Samples[2];
};

struct asset_memory_header{
	asset_memory_header* Next;
	asset_memory_header* Prev;

	uint32 AssetIndex;
	uint32 TotalSize;

	union{
		loaded_sound Sound;
		loaded_bitmap Bitmap;
	};
};

enum asset_memory_block_flags{
	AssetMemory_Used = 0x1,
};
struct asset_memory_block{
	asset_memory_block* Prev;
	asset_memory_block* Next;
	uint64 Flags;
	size_t Size;
};

/*struct hero_bitmaps{
	vec2 Align;

	loaded_bitmap Head;
	loaded_bitmap Torso;
	loaded_bitmap Cape;
};
*/
struct asset{
	uint32 State;
	asset_memory_header* Header;

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

	AssetState_StateMask = 0xFFF,
	AssetState_Lock = 0x10000,
};

struct asset_file{
	platform_file_handle* Handle;

	dda_header Header;
	dda_asset_type* AssetTypeArray;

	uint32 TagBase;
};

struct game_assets{
	struct transient_state* TranState;

	asset_memory_block MemorySentinel;
	asset_memory_header LoadedAssetSentinel;

	real32 TagRange[Tag_Count];

	uint32 TagCount;
	dda_tag* Tags;
	
	uint32 AssetCount;
	asset* Assets;

	asset_type AssetTypes[Asset_Count];

	uint32 FileCount;
	asset_file* Files;

	uint8* DDAContents;
};

inline bool32 IsLocked(asset* Asset){
	bool32 Result = (Asset->State & AssetState_Lock);
	return(Result);
}

inline uint32 GetState(asset* Asset){
	uint32 Result = Asset->State & AssetState_StateMask;
	return(Result);
}

extern void MoveHeaderToFront(game_assets* Assets, asset* Asset);
inline loaded_bitmap* GetBitmap(game_assets* Assets, bitmap_id ID){
	Assert(ID.Value <= Assets->AssetCount);
	asset* Asset = Assets->Assets + ID.Value;

	loaded_bitmap* Result = 0;
	if(GetState(Asset) >= AssetState_Loaded){
		GD_COMPLETE_READS_BEFORE_FUTURE;
		Result = &Asset->Header->Bitmap;
		MoveHeaderToFront(Assets, Asset);
	}

	return(Result);
}

inline loaded_sound* GetSound(game_assets* Assets, sound_id ID){
	Assert(ID.Value <= Assets->AssetCount);
	asset* Asset = Assets->Assets + ID.Value;

	loaded_sound* Result = 0;
	if(GetState(Asset) >= AssetState_Loaded){
		GD_COMPLETE_READS_BEFORE_FUTURE;
		Result = &Asset->Header->Sound;
		MoveHeaderToFront(Assets, Asset);
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

INTERNAL_FUNCTION void LoadBitmapAsset(game_assets* Assets, bitmap_id ID, bool32 Locked);
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