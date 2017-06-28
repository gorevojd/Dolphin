
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

enum voxel_chunk_state{
	VoxelChunkState_Unloaded,
	VoxelChunkState_InProcess,
	VoxelChunkState_Generated,
};

struct voxel_chunk_header{
	voxel_chunk_header* Next;
	voxel_chunk_header* Prev;

	struct task_with_memory* Task;

	voxel_chunk_state State;

	int32_t IsSentinel;

	int32_t TempNumber;

	voxel_chunk* Chunk;
	voxel_chunk_mesh* Mesh;
};

struct voxel_table_pair{
	uint64_t Key;
	voxel_chunk_header* Value;
	
	voxel_table_pair* NextPair;
};


struct voxel_chunk_manager{
	voxel_chunk_header* VoxelChunkSentinel;
	ticket_mutex Mutex;

	transient_state* TranState;

	memory_arena HashTableArena;
	memory_arena ListArena;

	int32_t CurrHorizontalIndex;
	int32_t CurrVerticalIndex;

	int32_t TempLstCount;

	int32_t ChunksViewDistance;
	voxel_atlas_id VoxelAtlasID;
};

inline void InsertChunkHeaderAtFront(voxel_chunk_manager* Manager, voxel_chunk_header* Header){
	Header->Prev = Manager->VoxelChunkSentinel;
	Header->Next = Manager->VoxelChunkSentinel->Next;

	Header->Prev->Next = Header;
	Header->Next->Prev = Header;
}

inline uint32_t VoxelHashFunc(uint64_t Key){
	uint8_t* Bytes = (uint8_t*)&Key;

	uint32_t Result = 0;

	int Index = 0;
	for(Index; Index < 8; Index++){
		Result = (Result * 1664525) + (uint8_t)(Bytes[Index]) + 1013904223;
	}

	return(Result);
}

inline uint64_t GenerateKeyForChunkIndices(int32_t HorizontalIndex, int32_t VerticalIndex){
	uint64_t Result;

#if 0
	Result = (HorizontalIndex & 0xFFFFFFFF) | (((uint64_t)(VerticalIndex) << 32) & 0xFFFFFFFF00000000);
#else
	Result = (uint32_t)HorizontalIndex | ((uint64_t)((uint32_t)(VerticalIndex)) << 32);
#endif

	return(Result);
}

inline void GetCurrChunkIndexBasedOnCamera(vec3 CamPos, int32_t* OutX, int32_t* OutY){
	int32_t ResX;
	int32_t CamPosX = (int32_t)(CamPos.x);
	if(CamPos.x >= 0.0f){
		ResX = CamPosX / IVAN_VOXEL_CHUNK_WIDTH;
	}
	else{
		ResX = (CamPosX / IVAN_VOXEL_CHUNK_WIDTH) - 1;
	}

	int32_t ResY;
	int32_t CamPosY = (int32_t)CamPos.z;
	if(CamPos.z >= 0.0f){
		ResY = CamPosY / IVAN_VOXEL_CHUNK_WIDTH;
	}
	else{
		ResY = (CamPosY / IVAN_VOXEL_CHUNK_WIDTH) - 1;
	}

	*OutX = ResX;
	*OutY = ResY;
}

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
INTERNAL_FUNCTION void GenerateVoxelChunk(
	memory_arena* Arena, voxel_chunk* Chunk, 
	int32_t HorizontalIndex, int32_t VerticalIndex);

#endif