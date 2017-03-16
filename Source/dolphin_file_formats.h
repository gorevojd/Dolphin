#ifndef DOLPHIN_FILE_FORMATS_H
#define DOLPHIN_FILE_FORMATS_H

#include "dolphin_asset_ids.h"

#define DOLPHIN_CODE(a, b, c, d) (((uint32)(a) << 0) | ((uint32)(b) << 8) | ((uint32)(c) << 16) | ((uint32)(d) << 24))

#pragma pack(push, 1)

struct bitmap_id{
	unsigned int Value;
};

struct sound_id{
	unsigned int Value;
};

struct dda_header{
#define DDA_MAGIC_VALUE DOLPHIN_CODE('d', 'd', 'a', ' ')
	uint32 MagicValue; 

#define DDA_VERSION 0
	uint32 Version;

	uint32 TagCount;
	uint32 AssetTypeCount;
	uint32 AssetCount;

	/*Offset to tag array in file*/
	uint64 TagOffset;
	/*Offset to asset type array in file*/
	uint64 AssetTypeOffset;
	/*Offset to asset array in file*/
	uint64 AssetOffset;
};

struct dda_bitmap{
	uint32 Dimension[2];
	real32 AlignPercentage[2];
};

enum dda_sound_chain{
	DDASoundChain_None,
	DDASoundChain_Loop,
	DDASoundChain_Advance,
};

struct dda_sound{
	uint32 SampleCount;
	uint32 ChannelCount;
	dda_sound_chain Chain;
};

struct dda_tag{
	uint32 ID;
	real32 Value;
};

struct dda_asset{
	uint64 DataOffset;
	uint32 FirstTagIndex;
	uint32 OnePastLastTagIndex;
	union{
		dda_bitmap Bitmap;
		dda_sound Sound;
	};
};

struct dda_asset_type{
	uint32 TypeID;
	uint32 FirstAssetIndex;
	uint32 OnePastLastAssetIndex;
};

#pragma pack(pop)

#endif /*DOLPHIN_FILE_FORMATS_H*/