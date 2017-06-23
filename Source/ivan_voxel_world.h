
/*
	TODO(DIMA):

		1) Least Resently Used algorithm for chunks

		2) Dynamic chunk load from asset file

		3) Frustrum culling for voxel chunks
*/

#ifndef IVAN_VOXEL_WORLD_H
#define IVAN_VOXEL_WORLD_H

#include "ivan_voxel_shared.h"

#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"


struct voxel{
	//TODO(Dima): Encode this data to 16 or 32 bit unsigned integer
	bool IsAir;
	voxel_mat_type Type;
};


struct voxel_chunk{
	vec3 Pos;

	voxel* Voxels;
	uint32_t VoxelsCount;
};

/*
	X - Left-right chunk order number
	Y - Top-down chunk order number
	Z - Front-Back chunk order number
*/
void GenerateVoxelChunk(memory_arena* Arena, voxel_chunk* Chunk, int32_t X, int32_t Y, int32_t Z){
	float OneOverWidth = 1.0f / (float)IVAN_VOXEL_CHUNK_WIDTH;
	float OneOverHeight = 1.0f / (float)IVAN_VOXEL_CHUNK_HEIGHT;

	temporary_memory TempMem = BeginTemporaryMemory(Arena);

	Chunk->VoxelsCount = IVAN_MAX_VOXELS_IN_CHUNK;
	Chunk->Voxels = (voxel*)PushSize(TempMem.Arena, Chunk->VoxelsCount * sizeof(voxel));

	float PosX = (float)X * (float)IVAN_VOXEL_CHUNK_WIDTH;
	float PosY = (float)Y * (float)IVAN_VOXEL_CHUNK_HEIGHT;
	float PosZ = (float)Z * (float)IVAN_VOXEL_CHUNK_WIDTH;

	Chunk->Pos = Vec3(PosX, PosY, PosZ);

	float StartHeight = 10.0f;

	//TODO(Dima): Check cache-friendly variations of this loop
	for(int j = 0; j < IVAN_VOXEL_CHUNK_WIDTH; j++){
		for(int i = 0; i < IVAN_VOXEL_CHUNK_WIDTH; i++){
			float RandHeight = stb_perlin_noise3(
				(float)(PosX + i) / 16.0f, 
				(float)PosY / 16.0f, 
				(float)(PosZ + j) / 16.0f, 0, 0, 0) * 5.0f + StartHeight;
			uint32_t RandHeightU32 = (uint32_t)(RandHeight + 0.5f);

			//NOTE(Dima): Do not change IsAir sence because of this
			int32_t TempFlag = 0;
			for(int32_t k = 0; k < IVAN_VOXEL_CHUNK_HEIGHT; k++){
				voxel* TempVoxel = &Chunk->Voxels[IVAN_GET_VOXEL_INDEX(i, j, k)];
				if(k == RandHeightU32){
					TempVoxel->IsAir = TempFlag;
					TempVoxel->Type = VoxelMaterial_GrassyGround;
					TempFlag = 1;					
				}
				else{
					TempVoxel->IsAir = TempFlag;
					TempVoxel->Type = VoxelMaterial_Ground;
				}
			}
		}
	}
	//EndTemporaryMemory(TempMem);
}



#endif