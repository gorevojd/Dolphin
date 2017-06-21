#ifndef IVAN_RENDER_GROUP_H
#define IVAN_RENDER_GROUP_H

#include "ivan_platform.h"
#include "ivan_intrinsics.h"
#include "ivan_render_math.h"

struct loaded_bitmap{
	void* Memory;
	int32 Width;
	int32 Height;

	real32 WidthOverHeight;
	vec2 AlignPercentage;

	void* TextureHandle;
};

enum render_group_entry_type{
	RenderGroupEntry_render_entry_clear = 1,
	RenderGroupEntry_render_entry_rectangle,
	RenderGroupEntry_render_entry_bitmap,
	RenderGroupEntry_render_entry_coordinate_system,
	RenderGroupEntry_render_entry_voxel_mesh,
};

struct render_group_entry_header{
	render_group_entry_type Type;
};

struct push_buffer_result{
	render_group_entry_header* Header;
};

struct render_entry_coordinate_system{
	vec2 Origin;
	vec2 XAxis;
	vec2 YAxis;
	vec4 Color;
	loaded_bitmap* Texture;
};

struct render_entry_clear{
	vec4 Color;
};

struct render_entry_rectangle{
	vec2 Dim;
	vec2 P;
	vec4 Color;
};

struct render_entry_bitmap{
	loaded_bitmap* Bitmap;
	vec2 P;
	vec4 Color;
	vec2 Size;
};

struct voxel_chunk_mesh;
struct render_entry_voxel_mesh{
	voxel_chunk_mesh* Mesh;

	loaded_bitmap* Bitmap;

	vec3 P;
};

struct bitmap_dimension{
	vec2 Size;
	vec2 Align;
	vec3 P;
};

struct render_group_transform{
	bool32 Orthographic;

	real32 MetersToPixels;
	vec2 ScreenCenter;

	real32 FocalLength;
	real32 DistanceAboveTarget;

	vec3 OffsetP;
	real32 Scale;
};

struct render_group{
	struct game_assets* Assets;
	real32 GlobalAlphaChannel;

	vec2 MonitorHalfDimInMeters;
	bool32 InsideRender;
	render_group_transform Transform;

	game_render_commands* Commands;
	uint32 GenerationID;
};

#endif