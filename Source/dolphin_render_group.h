#ifndef DOLPHIN_RENDER_GROUP_H
#define DOLPHIN_RENDER_GROUP_H

#include "dolphin_platform.h"
#include "dolphin_intrinsics.h"
#include "dolphin_render_math.h"

struct loaded_bitmap{
	void* Memory;
	int32 Width;
	int32 Height;

	real32 WidthOverHeight;
	gdVec2 AlignPercentage;
};

enum render_group_entry_type{
	RenderGroupEntry_render_entry_clear = 1,
	RenderGroupEntry_render_entry_rectangle,
	RenderGroupEntry_render_entry_bitmap,
	RenderGroupEntry_render_entry_coordinate_system,
};

struct render_group_entry_header{
	render_group_entry_type Type;
};

struct render_entry_coordinate_system{
	gdVec2 Origin;
	gdVec2 XAxis;
	gdVec2 YAxis;
	gdVec4 Color;
	loaded_bitmap* Texture;
};

struct render_entry_clear{
	gdVec4 Color;
};

struct render_entry_rectangle{
	gdVec2 Dim;
	gdVec2 P;
	gdVec4 Color;
};

struct render_entry_bitmap{
	loaded_bitmap* Bitmap;
	gdVec2 P;
	gdVec4 Color;
	gdVec2 Size;
};

struct bitmap_dimension{
	gdVec2 Size;
	gdVec2 Align;
	gdVec3 P;
};

struct render_group{
	real32 MetersToPixels;

	struct game_assets* Assets;

	uint32 MaxPushBufferSize;
	uint32 PushBufferSize;
	uint8* PushBufferBase;
};



#endif