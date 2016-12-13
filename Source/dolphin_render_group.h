#ifndef DOLPHIN_RENDER_GROUP_H
#define DOLPHIN_RENDER_GROUP_H

#include "dolphin_platform.h"
#include "dolphin_intrinsics.h"

enum bitmap_byte_order{
	ByteOrder_ARGB = 0,
	ByteOrder_RGBA
};

struct loaded_bitmap{
	int32 Width;
	int32 Height;
	int32 BytesPerPixel;
	void* Memory;
};

struct render_group_entity_basis{
	gdVec3 Offset;
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
	gdVec2 Points[16];
};

struct render_entry_clear{
	gdVec4 Color;
};

struct render_entry_rectangle{
	render_group_entity_basis EntityBasis;
	
	gdVec2 Dim;
	gdVec4 Color;
};

struct render_entry_bitmap{
	render_group_entity_basis EntityBasis;

	loaded_bitmap* Bitmap;
	gdVec4 Color;
};

struct render_group{
	real32 MetersToPixels;

	uint32 MaxPushBufferSize;
	uint32 PushBufferSize;
	uint8* PushBufferBase;
};




#endif