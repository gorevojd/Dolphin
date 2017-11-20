/*
	TODO(DIMA):
		1) Moving generation of the chunks to the other thread
	
		2) Dynamic chunk load from asset file

		3) Frustrum culling for voxel chunks

	NOTE(DIMA):
		1) My voxel engine assumes that 256 is the maximum height
		of the world you can get. Not actually maximum. It has no
		feature to store chunks in up direction. Only in 
		horizontal and vertical. Height is specified in
		ivan_voxel_shared.h as IVAN_VOXEL_CHUNK_HEIGHT.

		2) You need to guarantee that Manager->ListArena
		will have enough space to contain its elements.
*/

#ifndef IVAN_VOXEL_WORLD_H
#define IVAN_VOXEL_WORLD_H

#define IVAN_VOXEL_WORLD_MULTITHREADED 0
#define IVAN_VOXEL_CONTEXTS_COUNT 16

#include "ivan_voxel_shared.h"

#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"

struct voxel_chunk{
	int32_t HorizontalIndex;
	int32_t VerticalIndex;

	uint8_t* Voxels;
	uint32_t VoxelsCount;

	voxel_chunk* RightChunk;
	voxel_chunk* LeftChunk;
	voxel_chunk* FrontChunk;
	voxel_chunk* BackChunk;
};

enum voxel_chunk_state{
	VoxelChunkState_Unloaded,
	VoxelChunkState_InProcess,
	VoxelChunkState_Generated,
};

enum voxel_mesh_state{
	VoxelMeshState_Unloaded,
	VoxelMeshState_InProcess,
	VoxelMeshState_Generated,
};

struct voxel_chunk_header{
	voxel_chunk_header* Next;
	voxel_chunk_header* Prev;

	struct task_with_memory* ChunkTask;
	struct task_with_memory* MeshTask;

	volatile voxel_chunk_state ChunkState;
	volatile voxel_mesh_state MeshState;

	int32 HorzIndex;
	int32 VertIndex;

	int32_t IsSentinel;

	int32_t FrustumCullingTestResult;

	voxel_chunk* Chunk;
	voxel_chunk_mesh* Mesh;

	ticket_mutex ChangeMutex;
};

struct voxel_table_pair{
	uint64_t Key;
	voxel_chunk_header* Value;
	
	voxel_table_pair* NextPair;
};

struct voxel_chunk_thread_context{
	int32_t IsValid;

	memory_arena ListArena;

	int32_t MinXOffset;
	int32_t MaxXOffset;
	int32_t MinYOffset;
	int32_t MaxYOffset;

	struct voxel_chunk_manager* Manager;

	ticket_mutex ResultMutex;

	voxel_chunk_header* VoxelChunkSentinel;
	voxel_chunk_header* FirstFreeSentinel;
};

struct voxel_chunk_manager{
	ticket_mutex ChangeMutex;
	bool32 LoopCanBeContinued;

	voxel_chunk_header* VoxelChunkSentinel;
	voxel_chunk_header* FirstFreeSentinel;

	/*Table index points to the read table*/
	voxel_table_pair** Tables[2];
	volatile uint32 TableIndex;

	transient_state* TranState;
	random_series RandomSeries;

	memory_arena HashTableArena;
	memory_arena ListArena;

	int32_t CurrHorizontalIndex;
	int32_t CurrVerticalIndex;

	int32_t ChunksViewDistance;
	voxel_atlas_id VoxelAtlasID;

	vec3 CamPos;

	voxel_chunk_thread_context Contexts[IVAN_VOXEL_CONTEXTS_COUNT];
};

#define VOXEL_DLIST_INSERT(Sentinel, Entry)	\
	(Entry)->Prev = Sentinel;			\
	(Entry)->Next = Sentinel->Next;		\
										\
	(Entry)->Prev->Next = Entry;		\
	(Entry)->Next->Prev = Entry;

inline void InsertChunkHeaderAtFront(voxel_chunk_header* Sentinel, voxel_chunk_header* Header){
#if 1
	Header->Prev = Sentinel;
	Header->Next = Sentinel->Next;

	Header->Prev->Next = Header;
	Header->Next->Prev = Header;
#else
	VOXEL_DLIST_INSERT(Sentinel, Header);
#endif
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

	Result = (uint32_t)HorizontalIndex | ((uint64_t)((uint32_t)(VerticalIndex)) << 32);

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

INTERNAL_FUNCTION voxel_chunk* GetChunkAtIndices(
	voxel_table_pair** Table, 
	int32_t HorizontalIndex, 
	int32_t VerticalIndex);

#endif