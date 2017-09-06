/*
	IMPORTANT:
		I'm about optimizing some things. I will use
		16 bits per position(9 bist per y, and 5 and 5
		for x, z), 3 bits per normal index, 8 bits per 
		texture index, 2 bits for face index type(upper left, 
		lower right and etc.)

	TODO(DIMA): 
		Think about how to allocate memory for mesh

	IDEAS ABOUT OPTIMIZATION:
		1) Send per-vertex index of normal in normal array. 
		Extract it in the shader and assign. It will cost 3
		bits per vertex instead of 12 bytes = 96 bits.

		2) Send index of the texture in voxel atlas to shader
		and extract it there. MAYBE. Almost 10 - 16 bits will
		be enough instead of 8 bytes = 64 bits.

		3) We don't need to send floats per tex coord. 
		We can send index of texture in voxel atlas.
		1 byte instead of 8.
		
		4) We don't actually need to send 12 bytes of 
		vertex position data. We only can send lets say
		7 bits per x, 9 bits per y, 7 bits per z.
		If we want to say that we will never change 
		chunk default metrics, than we should store 
		4 bits per x and z and 8 bits per y. So after
		building mesh we can apply model transform
		matrix to transpose our chunk to needed place.
*/

#ifndef IVAN_VOXEL_MESH_H
#define IVAN_VOXEL_MESH_H

#include "ivan_voxel_shared.h"

enum voxel_texture_vert_type{
	VoxelTextureVertType_UpLeft,
	VoxelTextureVertType_UpRight,
	VoxelTextureVertType_DownRight,
	VoxelTextureVertType_DownLeft,
};

typedef struct voxel_mesh_generation_queue{
	voxel_mesh_generation_queue* Next;

	int32_t IsSentinel;
	uint32_t EntryCount;

	vec3 VoxelPos;
	uint8_t TextureIndexInAtlas;
} voxel_mesh_generation_queue;

typedef struct voxel_mesh_generation_context{
	uint32_t MemoryRequired;

	voxel_mesh_generation_queue LeftQueue;
	voxel_mesh_generation_queue RightQueue;
	voxel_mesh_generation_queue TopQueue;
	voxel_mesh_generation_queue BottomQueue;
	voxel_mesh_generation_queue FrontQueue;
	voxel_mesh_generation_queue BackQueue;

	uint32_t GenerationID;
	game_assets* Assets;
} voxel_mesh_generation_context;

struct voxel_mesh_size{
	uint32_t TriangleCount;
	uint32_t TotalByteSize;
};

struct voxel_chunk_mesh{
	void* MeshHandle;

	ivan_vertex_type* PUVN;

	uint32_t VerticesCount;
	uint32_t ActiveVertexIndex;
};

typedef void do_face_work_prot(voxel_chunk_mesh* Mesh, vec3 Pos, uint8_t TextureIndexInAtlas);

inline uint32_t GetEncodedVertexData(
	vec3 Pos, 
	uint8_t TexIndex,
	voxel_texture_vert_type TexVertType,
	uint8_t NormIndex)
{
	uint32_t Result = 0;

	uint32_t EncP = 
		(((uint32_t)(Pos.x) & 31) << 27) | 
		(((uint32_t)(Pos.y) & 511) << 18) | 
		(((uint32_t)(Pos.z) & 31) << 13);

	uint32_t EncN = ((uint32_t)(NormIndex) & 7) << 10;
	uint32_t EncT = ((uint32_t)(TexIndex) << 2) | ((uint32_t)(TexVertType) & 3);

	Result = EncP | EncN | EncT;

	return(Result);
}

inline void PushVoxelMeshChunkFace(
	voxel_chunk_mesh* Mesh,
	vec3 P0, 
	vec3 P1,
	vec3 P2,
	vec3 P3,
	uint8_t TextureIndex,
	uint8_t NormalIndex)
{
	uint32_t Index = Mesh->ActiveVertexIndex;

	uint32_t Value0 = GetEncodedVertexData(P0, TextureIndex, VoxelTextureVertType_UpLeft, NormalIndex);
	uint32_t Value1 = GetEncodedVertexData(P1, TextureIndex, VoxelTextureVertType_UpRight, NormalIndex);
	uint32_t Value2 = GetEncodedVertexData(P2, TextureIndex, VoxelTextureVertType_DownRight, NormalIndex);
	uint32_t Value3 = GetEncodedVertexData(P3, TextureIndex, VoxelTextureVertType_DownLeft, NormalIndex);

	Mesh->PUVN[Index] = Value0;
	Mesh->PUVN[Index + 1] = Value1;
	Mesh->PUVN[Index + 2] = Value2;

	Mesh->PUVN[Index + 3] = Value0;
	Mesh->PUVN[Index + 4] = Value2;
	Mesh->PUVN[Index + 5] = Value3;

	Mesh->ActiveVertexIndex += 6;
}

inline void DoFaceWorkAtFront(
	voxel_chunk_mesh* Mesh,
	vec3 Pos,
	uint8_t TextureIndexInAtlas)
{
	PushVoxelMeshChunkFace(
		Mesh,
		Vec3(Pos.x - 0.5f, Pos.y + 0.5f, Pos.z - 0.5f),
		Vec3(Pos.x + 0.5f, Pos.y + 0.5f, Pos.z - 0.5f),
		Vec3(Pos.x + 0.5f, Pos.y - 0.5f, Pos.z - 0.5f),
		Vec3(Pos.x - 0.5f, Pos.y - 0.5f, Pos.z - 0.5f),
		TextureIndexInAtlas,
		VoxelNormalIndex_Front);
}

inline void DoFaceWorkAtBack(
	voxel_chunk_mesh* Mesh,
	vec3 Pos,
	uint8_t TextureIndexInAtlas)
{
	PushVoxelMeshChunkFace(
		Mesh,
		Vec3(Pos.x + 0.5f, Pos.y + 0.5f, Pos.z + 0.5f),
		Vec3(Pos.x - 0.5f, Pos.y + 0.5f, Pos.z + 0.5f),
		Vec3(Pos.x - 0.5f, Pos.y - 0.5f, Pos.z + 0.5f),
		Vec3(Pos.x + 0.5f, Pos.y - 0.5f, Pos.z + 0.5f),
		TextureIndexInAtlas,
		VoxelNormalIndex_Back);
}

inline void DoFaceWorkAtLeft(
	voxel_chunk_mesh* Mesh,
	vec3 Pos,
	uint8_t TextureIndexInAtlas)
{
	PushVoxelMeshChunkFace(
		Mesh,
		Vec3(Pos.x - 0.5f, Pos.y + 0.5f, Pos.z - 0.5f),
		Vec3(Pos.x - 0.5f, Pos.y + 0.5f, Pos.z + 0.5f),
		Vec3(Pos.x - 0.5f, Pos.y - 0.5f, Pos.z + 0.5f),
		Vec3(Pos.x - 0.5f, Pos.y - 0.5f, Pos.z - 0.5f),
		TextureIndexInAtlas,
		VoxelNormalIndex_Left);	
}

inline void DoFaceWorkAtRight(
	voxel_chunk_mesh* Mesh,
	vec3 Pos,
	uint8_t TextureIndexInAtlas)
{
	PushVoxelMeshChunkFace(
		Mesh,
		Vec3(Pos.x + 0.5f, Pos.y + 0.5f, Pos.z + 0.5f),
		Vec3(Pos.x + 0.5f, Pos.y + 0.5f, Pos.z - 0.5f),
		Vec3(Pos.x + 0.5f, Pos.y - 0.5f, Pos.z - 0.5f),
		Vec3(Pos.x + 0.5f, Pos.y - 0.5f, Pos.z + 0.5f),
		TextureIndexInAtlas,
		VoxelNormalIndex_Right);
}

inline void DoFaceWorkAtTop(
	voxel_chunk_mesh* Mesh,
	vec3 Pos,
	uint8_t TextureIndexInAtlas)
{
	PushVoxelMeshChunkFace(
		Mesh,
		Vec3(Pos.x - 0.5f, Pos.y + 0.5f, Pos.z - 0.5f),
		Vec3(Pos.x + 0.5f, Pos.y + 0.5f, Pos.z - 0.5f),
		Vec3(Pos.x + 0.5f, Pos.y + 0.5f, Pos.z + 0.5f),
		Vec3(Pos.x - 0.5f, Pos.y + 0.5f, Pos.z + 0.5f),
		TextureIndexInAtlas,
		VoxelNormalIndex_Up);	
}

inline void DoFaceWorkAtBottom(
	voxel_chunk_mesh* Mesh,
	vec3 Pos,
	uint8_t TextureIndexInAtlas)
{
	PushVoxelMeshChunkFace(
		Mesh,
		Vec3(Pos.x - 0.5f, Pos.y - 0.5f, Pos.z + 0.5f),
		Vec3(Pos.x + 0.5f, Pos.y - 0.5f, Pos.z + 0.5f),
		Vec3(Pos.x + 0.5f, Pos.y - 0.5f, Pos.z - 0.5f),
		Vec3(Pos.x - 0.5f, Pos.y - 0.5f, Pos.z - 0.5f),
		TextureIndexInAtlas,
		VoxelNormalIndex_Down);	
}

inline int32_t NeighbourVoxelExistAndAir(voxel_chunk* Chunk, int32_t i, int32_t j, int32_t k){
	int32_t Result = 0;

	if((i >= 0 && (i < IVAN_VOXEL_CHUNK_WIDTH)) &&
		(j >= 0 && (j < IVAN_VOXEL_CHUNK_HEIGHT)) &&
		(k >= 0 && (k < IVAN_VOXEL_CHUNK_WIDTH)) &&
		Chunk->Voxels[IVAN_GET_VOXEL_INDEX(i, k, j)] == VoxelMaterial_None)
	{
		Result = 1;
	}

	return(Result);
}

inline void PushFaceWorkForVoxel(	
	voxel_chunk* Chunk,
	voxel_chunk_mesh* Mesh,
	int32_t InitX, 
	int32_t InitY, 
	int32_t InitZ,
	uint8_t TextureIndexInAtlas,
	vec3 VoxelPos,
	do_face_work_prot* DoFaceWork)
{
	if(NeighbourVoxelExistAndAir(
		Chunk,
		InitX,
		InitY,
		InitZ))
	{
		DoFaceWork(Mesh, VoxelPos, TextureIndexInAtlas);
	}
}

#if 0
INTERNAL_FUNCTION void 
GenerateVoxelMeshForChunk(
	voxel_chunk_mesh* Result, voxel_chunk* Chunk, 
	game_assets* Assets, voxel_atlas_id VoxelAtlasID);

INTERNAL_FUNCTION uint32_t
PrecalculateVoxelMeshSize(voxel_chunk* Chunk, uint32_t BytesPerVertex);
#else
INTERNAL_FUNCTION voxel_mesh_generation_context
InitVoxelMeshGeneration(
	voxel_chunk* Chunk, 
	game_assets* Assets,
	voxel_atlas_id VoxelAtlasID);

INTERNAL_FUNCTION void
FinalizeVoxelMeshGeneration(
	voxel_chunk_mesh* Mesh,
	voxel_mesh_generation_context* Context);
#endif

#endif