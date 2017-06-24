
/*
	TODO(DIMA):

		1) Least Resently Used algorithm for chunks

		2) Dynamic chunk load from asset file

		3) Frustrum culling for voxel chunks

		4) Think about how to manage multiple chunks...
		Use double linked list??

	NOTE:
		1) My voxel engine assumes that 256 is the maximum height
		of the world you can get. Not actually maximum. It has no
		feature to store chunks in up direction. Only in 
		horizontal and vertical. Height is specified in
		ivan_voxel_shared.h as IVAN_VOXEL_CHUNK_HEIGHT.
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
	int32_t HorizontalIndex;
	int32_t VerticalIndex;

	voxel* Voxels;
	uint32_t VoxelsCount;
};

inline vec3 GetPosForVoxelChunk(voxel_chunk* Chunk){
	vec3 Result;

	Result.x = Chunk->HorizontalIndex * IVAN_VOXEL_CHUNK_WIDTH;
	Result.y = 0.0f;
	Result.z = Chunk->VerticalIndex * IVAN_VOXEL_CHUNK_WIDTH;

	return(Result);
}

/*
	X - Left-right chunk order number
	Y - Top-down chunk order number
	Z - Front-Back chunk order number
*/
void GenerateVoxelChunk(
	memory_arena* Arena, 
	voxel_chunk* Chunk, 
	int32_t HorizontalIndex,
	int32_t VerticalIndex)
{
	float OneOverWidth = 1.0f / (float)IVAN_VOXEL_CHUNK_WIDTH;
	float OneOverHeight = 1.0f / (float)IVAN_VOXEL_CHUNK_HEIGHT;

	temporary_memory TempMem = BeginTemporaryMemory(Arena);

	Chunk->VoxelsCount = IVAN_MAX_VOXELS_IN_CHUNK;
	Chunk->Voxels = (voxel*)PushSize(TempMem.Arena, Chunk->VoxelsCount * sizeof(voxel));

	Chunk->HorizontalIndex = HorizontalIndex;
	Chunk->VerticalIndex = VerticalIndex;

	float StartHeight = 20.0f;

	vec3 ChunkPos = GetPosForVoxelChunk(Chunk);

	//TODO(Dima): Check cache-friendly variations of this loop
	for(int j = 0; j < IVAN_VOXEL_CHUNK_WIDTH; j++){
		for(int i = 0; i < IVAN_VOXEL_CHUNK_WIDTH; i++){
			float RandHeight = stb_perlin_noise3(
				(float)(ChunkPos.x + i) / 16.0f, 
				(float)ChunkPos.y / 16.0f, 
				(float)(ChunkPos.z + j) / 16.0f, 0, 0, 0) * 5.0f + StartHeight;
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

void LoadVoxelChunkFromFile(voxel_chunk* Chunk, int32_t x, int32_t y){

}

void StoreVoxelChunkToFile(voxel_chunk* Chunk){

}

#endif