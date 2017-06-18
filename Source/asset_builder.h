#ifndef ASSET_BUILDER_H

#include <stdio.h>
#include <stdlib.h>

#include "ivan_platform.h"
#include "ivan_math.h"
#include "ivan_render_math.h"
#include "ivan_intrinsics.h"
#include "ivan_file_formats.h"

#include "ivan_voxel_shared.h"

#define USE_FONTS_FROM_WINDOWS 0

#if USE_FONTS_FROM_WINDOWS
#include <Windows.h>
#define MAX_FONT_WIDTH 1024
#define MAX_FONT_HEIGHT 1024

GLOBAL_VARIABLE VOID* GlobalFontBits;
GLOBAL_VARIABLE HDC GlobalFontDeviceContext;
#else
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#endif


#define ONE_PAST_MAX_FONT_CODEPOINT (0x10FFFF + 1)

enum asset_type{
	AssetType_Sound,
	AssetType_Bitmap,
	AssetType_Model,
	AssetType_Font,
	AssetType_FontGlyph,
	AssetType_VoxelAtlas,
	AssetType_VoxelAtlasTexture,
};

struct loaded_font{

#if USE_FONTS_FROM_WINDOWS
	HFONT Win32Handle;
	TEXTMETRIC TextMetric;
#else
	void* FileContents;
	stbtt_fontinfo FontInfo;
	float Scale;
	float AscenderHeight;
	float DescenderHeight;
	float ExternalLeading;
#endif
	float LineAdvance;

	dda_font_glyph* Glyphs;
	float* HorizontalAdvance;

	uint32 MinCodePoint;
	uint32 MaxCodePoint;

	uint32 MaxGlyphCount;
	uint32 GlyphCount;

	uint32* GlyphIndexFromCodePoint;
	uint32 OnePastHighestCodepoint;
};

struct loaded_voxel_atlas{
	dda_voxel_atlas_texture AtlasTexture;
	voxel_tex_coords_set Materials[VoxelMaterial_Count];

	char* TextureFileName;

	uint32 TextureCount;
	uint32 MaxTextureCount;

	uint32 AtlasWidth;
	uint32 OneTextureWidth;
};

struct asset_source_font{
	loaded_font* Font;
};

struct asset_source_font_glyph{
	loaded_font* Font;
	uint32 Codepoint;
};

struct asset_source_bitmap{
	char* FileName;
};

struct asset_source_sound{
	char* FileName;
	uint32 FirstSampleIndex;
};

struct asset_source_model{
	char* FileName;
};

struct asset_source_voxel_atlas{
	loaded_voxel_atlas* Atlas;
};

struct asset_source_voxel_atlas_texture{
	char* FileName;

	loaded_voxel_atlas* Atlas;
};

struct asset_source{
	asset_type Type;
	union{
		asset_source_bitmap Bitmap;
		asset_source_sound Sound;
		asset_source_font Font;
		asset_source_font_glyph Glyph;
		asset_source_model Model;
		asset_source_voxel_atlas VoxelAtlas;
		asset_source_voxel_atlas_texture VoxelAtlasTexture;
	};
};

#define IVAN_NUMBER_OF_ASSETS_TO_ALLOC 4096

struct game_assets{
	unsigned int TagCount;
	dda_tag Tags[IVAN_NUMBER_OF_ASSETS_TO_ALLOC];

	unsigned int AssetTypeCount;
	dda_asset_type AssetTypes[Asset_Count];

	unsigned int AssetCount;
	asset_source AssetSources[IVAN_NUMBER_OF_ASSETS_TO_ALLOC];
	dda_asset Assets[IVAN_NUMBER_OF_ASSETS_TO_ALLOC];

	dda_asset_type* DEBUGAssetType;
	unsigned int AssetIndex;
};



#define ASSET_BUILDER_H
#endif