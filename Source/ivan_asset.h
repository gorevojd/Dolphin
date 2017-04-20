#ifndef IVAN_ASSET_H
#define IVAN_ASSET_H

#include "ivan_platform.h"
#include "ivan_intrinsics.h"

struct loaded_sound{
	uint32 SampleCount;
	uint32 ChannelCount;

	int16* Samples[2];
};

struct loaded_font{
	dda_font_glyph* Glyphs;
	float* HorizontalAdvance;
	uint32 BitmapIDOffset;
	uint16* UnicodeMap;
};

enum asset_header_type{
	AssetType_None,
	AssetType_Bitmap,
	AssetType_Sound,
	AssetType_Font,
	AssetType_Model,
};

struct asset_memory_header{
	asset_memory_header* Next;
	asset_memory_header* Prev;

	uint32 AssetType;
	uint32 AssetIndex;
	uint32 TotalSize;
	uint32 GenerationID;

	union{
		loaded_sound Sound;
		loaded_bitmap Bitmap;
		loaded_font Font;
	};
};


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
	int FontBitmapIDOffset;
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

struct game_assets{
	uint32 NextGenerationID;

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

	uint32 OperationLock;

	uint32 InFlightGenerationCount;
	uint32 InFlightGenerations[16];
};

inline void BeginAssetLock(game_assets* Assets){
	for(;;){
		if(AtomicCompareExchangeUInt32(&Assets->OperationLock, 1, 0) == 0){
			break;
		}
	}
}

inline void EndAssetLock(game_assets* Assets){
	GD_COMPLETE_WRITES_BEFORE_FUTURE;
	Assets->OperationLock = 0;
}

inline void InsertAssetHeaderAtFront(game_assets* Assets, asset_memory_header* Header){
    asset_memory_header* Sentinel = &Assets->LoadedAssetSentinel;

    Header->Prev = Sentinel;
    Header->Next = Sentinel->Next;

    Header->Next->Prev = Header;
    Header->Prev->Next = Header;
}

inline void RemoveAssetHeaderFromList(asset_memory_header* Header){
    Header->Prev->Next = Header->Next;
    Header->Next->Prev = Header->Prev;

    Header->Next = Header->Prev = 0;
}

inline asset_memory_header* GetAsset(game_assets* Assets, uint32 ID, uint32 GenerationID){
	Assert(ID <= Assets->AssetCount);
	asset* Asset = Assets->Assets  + ID;

	asset_memory_header* Result = 0;

	BeginAssetLock(Assets);

	if(Asset->State == AssetState_Loaded){
		Result = Asset->Header;
		RemoveAssetHeaderFromList(Result);
		InsertAssetHeaderAtFront(Assets, Result);

		if(Asset->Header->GenerationID < GenerationID){
			Asset->Header->GenerationID = GenerationID;
		}

		GD_COMPLETE_WRITES_BEFORE_FUTURE;
	}

	EndAssetLock(Assets);

	return(Result);
}

inline loaded_bitmap* GetBitmap(game_assets* Assets, bitmap_id ID, uint32 GenerationID){
	
	asset_memory_header* Header = GetAsset(Assets, ID.Value, GenerationID);

	loaded_bitmap* Result = Header ? &Header->Bitmap : 0;

	return(Result);
}

inline dda_bitmap* GetBitmapInfo(game_assets* Assets, bitmap_id ID){
	Assert(ID.Value <= Assets->AssetCount);
	dda_bitmap* Result = &Assets->Assets[ID.Value].DDA.Bitmap;

	return(Result);
}

inline loaded_sound* GetSound(game_assets* Assets, sound_id ID, uint32 GenerationID){
	asset_memory_header* Header = GetAsset(Assets, ID.Value, GenerationID);

	loaded_sound* Result = Header ? &Header->Sound : 0;

	return(Result);
}

inline dda_sound* GetSoundInfo(game_assets* Assets, sound_id ID){
	Assert(ID.Value <= Assets->AssetCount);
	dda_sound* Result = &Assets->Assets[ID.Value].DDA.Sound;

	return(Result);
}

inline loaded_font* GetFont(game_assets* Assets, font_id ID, uint32 GenerationID){
	asset_memory_header* Header = GetAsset(Assets, ID.Value, GenerationID);

	loaded_font* Result = Header ? &Header->Font : 0;

	return(Result);
}

inline dda_font* GetFontInfo(game_assets* Assets, font_id ID){
	Assert(ID.Value <= Assets->AssetCount);
	dda_font* Result = &Assets->Assets[ID.Value].DDA.Font;

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

INTERNAL_FUNCTION void LoadBitmapAsset(game_assets* Assets, bitmap_id ID, bool32 Immediate);
INTERNAL_FUNCTION void LoadSoundAsset(game_assets* Assets, sound_id ID);
INTERNAL_FUNCTION void LoadFontAsset(game_assets* Assets, font_id, bool32 Immediate);


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

inline uint32 BeginGeneration(game_assets* Assets){
	BeginAssetLock(Assets);

	Assert(Assets->InFlightGenerationCount < ArrayCount(Assets->InFlightGenerations));
	uint32 Result = Assets->NextGenerationID++;
	Assets->InFlightGenerations[Assets->InFlightGenerationCount++] = Result;

	EndAssetLock(Assets);

	return(Result);
}

inline void EndGeneration(game_assets* Assets, uint32 GenerationID){
	BeginAssetLock(Assets);

	for(uint32 Index = 0;
		Index < Assets->InFlightGenerationCount;
		Index++)
	{
		if(Assets->InFlightGenerations[Index] == GenerationID){
			Assets->InFlightGenerations[Index] = Assets->InFlightGenerations[--Assets->InFlightGenerationCount];

			break;
		}
	}

	EndAssetLock(Assets);
}

#endif