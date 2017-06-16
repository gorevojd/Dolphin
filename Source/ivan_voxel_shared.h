#ifndef IVAN_VOXEL_SHARED_H

#define IVAN_VOXEL_CHUNK_HEIGHT 256
#define IVAN_VOXEL_CHUNK_WIDTH 16
#define IVAN_VOXEL_CHUNK_LAYER_COUNT (IVAN_VOXEL_CHUNK_WIDTH * IVAN_VOXEL_CHUNK_WIDTH)

/*
	i -> left-right
	j -> front-back
	k -> bottom-top
*/
#define IVAN_GET_VOXEL_INDEX(i, j, k)	\
	((k) * IVAN_VOXEL_CHUNK_WIDTH * IVAN_VOXEL_CHUNK_WIDTH + (j) * IVAN_VOXEL_CHUNK_WIDTH + (i))

enum voxel_mat_type{
	VoxelMaterial_None,
	VoxelMaterial_Stone,
	VoxelMaterial_Ground,
	VoxelMaterial_Sand,
	VoxelMaterial_GrassyGround,
	VoxelMaterial_Lava,
	VoxelMaterial_SnowGround,
	VoxelMaterial_Leaves,
	VoxelMaterial_Brick,
	VoxelMaterial_Logs,

	VoxelMaterial_Count,
	VoxelMaterial_CountTest,
};

/*
	T0 - for upper left vertex
	T1 - for upper right vertex
	T2 - for lower right vertex
	T3 - for lower left vertex
*/

union voxel_face_tex_coords_set{
	struct{
		vec2 T0;
		vec2 T1;
		vec2 T2;
		vec2 T3;
	};
	vec2 T[4];
};

enum voxel_texture_face_type{
	IvanVoxelTexture_Top = 1,
	IvanVoxelTexture_Bottom = 2,
	IvanVoxelTexture_TopBottom = 3,
	IvanVoxelTexture_Left = 4,
	IvanVoxelTexture_Right = 8,
	IvanVoxelTexture_Front = 16,
	IvanVoxelTexture_Back = 32,
	IvanVoxelTexture_Side = 60,
	IvanVoxelTexture_All = 63,
};

enum voxel_face_type_index{
	VoxelFaceTypeIndex_Top = 0,
	VoxelFaceTypeIndex_Bottom,
	VoxelFaceTypeIndex_Left,
	VoxelFaceTypeIndex_Right,
	VoxelFaceTypeIndex_Front,
	VoxelFaceTypeIndex_Back,

	VoxelFaceTypeIndex_Count,

	VoxelFaceTypeIndex_All,
	VoxelFaceTypeIndex_Side,
	VoxelFaceTypeIndex_TopBottom,
};

struct voxel_tex_coords_set{
	union {
		struct{
			union{
				struct{
					voxel_face_tex_coords_set Top;
					voxel_face_tex_coords_set Bottom;
				};
				voxel_face_tex_coords_set TopBottom;
			};
			union{
				struct{
					voxel_face_tex_coords_set Left;
					voxel_face_tex_coords_set Right;
					voxel_face_tex_coords_set Front;
					voxel_face_tex_coords_set Back;
				};
				voxel_face_tex_coords_set Side;
			};
		};
		voxel_face_tex_coords_set All;
		voxel_face_tex_coords_set Sets[VoxelFaceTypeIndex_Count];
	};
};

struct voxel_texture_face_atlas{
	voxel_tex_coords_set Materials[VoxelMaterial_Count];
};

#define IVAN_VOXEL_SHARED_H
#endif