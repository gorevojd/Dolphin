#ifndef IVAN_FILE_FORMATS_H
#define IVAN_FILE_FORMATS_H

#include "ivan_asset_ids.h"

#define IVAN_CODE(a, b, c, d) (((uint32)(a) << 0) | ((uint32)(b) << 8) | ((uint32)(c) << 16) | ((uint32)(d) << 24))

#pragma pack(push, 1)

struct bitmap_id{
	unsigned int Value;
};

struct sound_id{
	unsigned int Value;
};

struct font_id{
	unsigned int Value;
};

struct voxel_atlas_id{
	unsigned int Value;
};

struct animation_id{
	unsigned int Value;
};

struct mesh_id{
	unsigned int Value;
};

struct dda_header{
#define DDA_MAGIC_VALUE IVAN_CODE('d', 'd', 'a', ' ')
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

enum dda_bitmap_type{
	DDABitmap_Bitmap,
	DDABitmap_VoxelAtlas,
	DDABitmap_FontGlyph,
};

struct dda_bitmap{
	uint32 Dimension[2];
	real32 AlignPercentage[2];
	dda_bitmap_type BitmapType;
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

enum dda_joint_frames_header_type{
	JointFrameHeader_Translation,
	JointFrameHeader_Rotation,
	JointFrameHeader_Scaling,
};

struct dda_joint_frames_header{
	uint8 Type;

	uint32 BytesLength;
};


#define DDA_ANIMATION_MAX_BONE_COUNT 256
struct dda_animation{
	//dda_joint_frames_info JointAnims[DDA_ANIMATION_MAX_BONE_COUNT];
	uint32 JointAnimsCount;
	uint32 TotalFileSize;

    float LengthTime;

    float TicksPerSecond;
};

struct dda_font_glyph{
	uint32 UnicodeCodePoint;
	bitmap_id BitmapID;
};

struct dda_font{
	uint32 OnePastHighestCodepoint;
	uint32 GlyphCount;
	float AscenderHeight;
	float DescenderHeight;
	float ExternalLeading;
};

struct dda_voxel_atlas_texture{
	bitmap_id BitmapID;
};

struct dda_voxel_atlas{	
	uint32 AtlasWidth;
	uint32 OneTextureWidth;
	bitmap_id BitmapID;
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
		dda_font Font;
		dda_voxel_atlas VoxelAtlas;
		dda_animation Animation;
	};
};

struct dda_asset_type{
	uint32 TypeID;
	uint32 FirstAssetIndex;
	uint32 OnePastLastAssetIndex;
};

#pragma pack(pop)

#endif /*IVAN_FILE_FORMATS_H*/