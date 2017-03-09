#ifndef DOLPHIN_FILE_FORMATS_H
#define DOLPHIN_FILE_FORMATS_H

#include "dolphin_asset_ids.h"

#define DOLPHIN_CODE(a, b, c, d) (((uint32)(a) << 0) | ((uint32)(b) << 8) | ((uint32)(c) << 16) | ((uint32)(d) << 24))

#pragma pack(push, 1)

struct gda_header{
#define GDA_MAGIC_VALUE DOLPHIN_CODE('g', 'd', 'a', ' ')
	uint32 MagickValue; 

#define GDA_VERSION 0
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

struct gda_bitmap{
	uint32 Dimension[2];
	real32 AlignPercentage[2];
};

struct gda_sound{
	uint32 FirstSampleIndex;
	uint32 SampleCount
};

struct gda_tag{
	uint32 ID;
	real32 Value;
};

struct gda_asset{
	uint64 DataOffset;
	uint32 FirstTagIndex;
	uint32 OnePastLastTagIndex;
	union{
		gda_bitmap Bitmap;
		gda_sound Sound;
	}
};

struct gda_asset_type{
	uint32 TypeID;
	uint32 FirstAssetIndex;
	uint32 OnePastLastTagIndex;
};

#pragma pack(pop)

#endif /*DOLPHIN_FILE_FORMATS_H*/