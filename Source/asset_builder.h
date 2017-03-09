#ifndef ASSET_BUILDER_H

#include <stdio.h>
#include <stdlib.h>

#include "dolphin_asset_ids.h"
#include "dolphin_file_formats.h"
#include "dolphin_intrinsics.h"
#include "dolphin_render_math.h"

struct bitmap_id{
	unsigned int Value;
};

struct sound_id{
	unsigned int Value;
};

#if 0
enum asset_type{
	AssetType_Sound,
	AssetType_Bitmap,
	AssetType_Font,
	AssetType_FontGlyph,
};

struct loaded_font;
struct asset_source_font{
	loaded_font* Font;
};

struct asset_source_font_glyph{
	loaded_font* Font;
	unsigned int Codepoint;
};

struct asset_source_bitmap{
	char* FileName;
};

struct asset_source_sound{
	char* FileName;
	unsigned int FirstSampleIndex;
};

struct asset_source{
	asset_type Type;
	union{
		asset_source_bitmap Bitmap;
		asset_source_sound Sound;
		asset_source_font Font;
		asset_source_font_glyph Glyph;
	};
};
#endif

struct asset_bitmap_info{
	char* FileName;
	float AlignPercentage;
};

struct asset_sound_info{
	char* FileName;
	unsigned int FirstSampleIndex;
	unsigned int SampleCount;
	sound_id NextIDToPlay;
};

struct asset{
	unsigned long long DataOffset;
	unsigned int FirstTagIndex;
	unsigned int OnePastLastTagIndex;
	union{
		asset_bitmap_info Bitmap;
		asset_sound_info Sound;
	};
};

#define DOLPHIN_NUMBER_OF_ASSETS 4096

struct game_assets{
	unsigned int TagCount;
	dfa_tag Tags[DOLPHIN_NUMBER_OF_ASSETS];

	unsigned int AssetTypeCount;
	dfa_asset_type AssetTypes[Asset_Count];

	unsigned int AssetCount;
	asset Assets[DOLPHIN_NUMBER_OF_ASSETS];

	dfa_asset_type* DEBUGAssetType;
	asset* DEBUGAsset;
};

#define ASSET_BUILDER_H
#endif