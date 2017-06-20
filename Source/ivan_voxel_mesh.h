/*
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
		4 bits per x and z and 7 bits per y. So after
		building mesh we can apply model transform
		matrix to transpose our chunk to needed place.
*/

#ifndef IVAN_VOXEL_MESH_H
#define IVAN_VOXEL_MESH_H

#include "ivan_voxel_shared.h"

struct voxel_chunk_mesh{
#define VOXEL_CHUNK_MESH_SOA 0
	/*Vertices*/
	vec3* Positions;
	vec2* TexCoords;
	vec3* Normals;

	uint32_t VerticesCount;
	uint32_t ActiveVertexIndex;

	/*Indices*/
	uint32_t* Indices;

	uint32_t IndicesCount;
	uint32_t ActiveIndexIndex;
};

inline void PushVoxelMeshChunkFace(
	voxel_chunk_mesh* Mesh,
	vec3 P0, vec2 T0,
	vec3 P1, vec2 T1,
	vec3 P2, vec2 T2,
	vec3 P3, vec2 T3,
	vec3 N)
{
	uint32_t Index = Mesh->ActiveVertexIndex;
	uint32_t IndexIndex = Mesh->ActiveIndexIndex;

	/*Positions*/
	Mesh->Positions[Index] = P0;
	Mesh->Positions[Index + 1] = P1;
	Mesh->Positions[Index + 2] = P2;
	Mesh->Positions[Index + 3] = P3;

	/*Normals*/
	Mesh->Normals[Index] = N;
	Mesh->Normals[Index + 1] = N;
	Mesh->Normals[Index + 2] = N;
	Mesh->Normals[Index + 3] = N;

	/*TextureCoords*/
	Mesh->TexCoords[Index] = T0;
	Mesh->TexCoords[Index + 1] = T1;
	Mesh->TexCoords[Index + 2] = T2;
	Mesh->TexCoords[Index + 3] = T3;

	/*Indices*/
	Mesh->Indices[IndexIndex] = Index;
	Mesh->Indices[IndexIndex + 1] = Index + 1;
	Mesh->Indices[IndexIndex + 2] = Index + 2;

	Mesh->Indices[IndexIndex + 3] = Index;
	Mesh->Indices[IndexIndex + 4] = Index + 2;
	Mesh->Indices[IndexIndex + 5] = Index + 3;

	/*Incrementing*/
	Mesh->ActiveVertexIndex += 4;
	Mesh->ActiveIndexIndex += 6; 
}

inline void DoFaceWorkAtFront(
	voxel_chunk_mesh* Mesh,
	vec3 Pos,
	voxel_face_tex_coords_set* FaceT)
{
	PushVoxelMeshChunkFace(
		Mesh,
		Vec3(Pos.x - 0.5f, Pos.y + 0.5f, Pos.z + 0.5f), FaceT->T0,
		Vec3(Pos.x + 0.5f, Pos.y + 0.5f, Pos.z + 0.5f), FaceT->T1,
		Vec3(Pos.x + 0.5f, Pos.y - 0.5f, Pos.z + 0.5f), FaceT->T2,
		Vec3(Pos.x - 0.5f, Pos.y - 0.5f, Pos.z + 0.5f), FaceT->T3,
		Vec3(0.0f, 0.0f, 1.0f));	
}

inline void DoFaceWorkAtBack(
	voxel_chunk_mesh* Mesh,
	vec3 Pos,
	voxel_face_tex_coords_set* FaceT)
{
	PushVoxelMeshChunkFace(
		Mesh,
		Vec3(Pos.x + 0.5f, Pos.y + 0.5f, Pos.z - 0.5f), FaceT->T0,
		Vec3(Pos.x - 0.5f, Pos.y + 0.5f, Pos.z - 0.5f), FaceT->T1,
		Vec3(Pos.x - 0.5f, Pos.y - 0.5f, Pos.z - 0.5f), FaceT->T2,
		Vec3(Pos.x + 0.5f, Pos.y - 0.5f, Pos.z - 0.5f), FaceT->T3,
		Vec3(0.0f, 0.0f, -1.0f));
}

inline void DoFaceWorkAtLeft(
	voxel_chunk_mesh* Mesh,
	vec3 Pos,
	voxel_face_tex_coords_set* FaceT)
{
	PushVoxelMeshChunkFace(
		Mesh,
		Vec3(Pos.x - 0.5f, Pos.y + 0.5f, Pos.z - 0.5f), FaceT->T0,
		Vec3(Pos.x - 0.5f, Pos.y + 0.5f, Pos.z + 0.5f), FaceT->T1,
		Vec3(Pos.x - 0.5f, Pos.y - 0.5f, Pos.z + 0.5f), FaceT->T2,
		Vec3(Pos.x - 0.5f, Pos.y - 0.5f, Pos.z - 0.5f), FaceT->T3,
		Vec3(-1.0f, 0.0f, 0.0f));	
}

inline void DoFaceWorkAtRight(
	voxel_chunk_mesh* Mesh,
	vec3 Pos,
	voxel_face_tex_coords_set* FaceT)
{
	PushVoxelMeshChunkFace(
		Mesh,
		Vec3(Pos.x + 0.5f, Pos.y + 0.5f, Pos.z + 0.5f), FaceT->T0,
		Vec3(Pos.x + 0.5f, Pos.y + 0.5f, Pos.z - 0.5f), FaceT->T1,
		Vec3(Pos.x + 0.5f, Pos.y - 0.5f, Pos.z - 0.5f), FaceT->T2,
		Vec3(Pos.x + 0.5f, Pos.y - 0.5f, Pos.z + 0.5f), FaceT->T3,
		Vec3(1.0f, 0.0f, 0.0f));
}

inline void DoFaceWorkAtTop(
	voxel_chunk_mesh* Mesh,
	vec3 Pos,
	voxel_face_tex_coords_set* FaceT)
{
	PushVoxelMeshChunkFace(
		Mesh,
		Vec3(Pos.x - 0.5f, Pos.y + 0.5f, Pos.z - 0.5f), FaceT->T0,
		Vec3(Pos.x + 0.5f, Pos.y + 0.5f, Pos.z - 0.5f), FaceT->T1,
		Vec3(Pos.x + 0.5f, Pos.y + 0.5f, Pos.z + 0.5f), FaceT->T2,
		Vec3(Pos.x - 0.5f, Pos.y + 0.5f, Pos.z + 0.5f), FaceT->T3,
		Vec3(0.0f, 1.0f, 0.0f));	
}

inline void DoFaceWorkAtBottom(
	voxel_chunk_mesh* Mesh,
	vec3 Pos,
	voxel_face_tex_coords_set* FaceT)
{
	PushVoxelMeshChunkFace(
		Mesh,
		Vec3(Pos.x - 0.5f, Pos.y - 0.5f, Pos.z + 0.5f), FaceT->T0,
		Vec3(Pos.x + 0.5f, Pos.y - 0.5f, Pos.z + 0.5f), FaceT->T1,
		Vec3(Pos.x + 0.5f, Pos.y - 0.5f, Pos.z - 0.5f), FaceT->T2,
		Vec3(Pos.x - 0.5f, Pos.y - 0.5f, Pos.z - 0.5f), FaceT->T3,
		Vec3(0.0f, -1.0f, 0.0f));	
}

inline int32_t NeighbourVoxelExistAndAir(voxel_chunk* Chunk, int32_t i, int32_t j, int32_t k){
	int32_t Result = 0;

	if((i >= 0 && (i < IVAN_VOXEL_CHUNK_WIDTH)) &&
		(j >= 0 && (j < IVAN_VOXEL_CHUNK_WIDTH)) &&
		(k >= 0 && (k < IVAN_VOXEL_CHUNK_HEIGHT)) &&
		Chunk->Voxels[IVAN_GET_VOXEL_INDEX(i, j, k)].IsAir)
	{
		voxel UpVoxel = Chunk->Voxels[IVAN_GET_VOXEL_INDEX(i, j, k + 1)];
		voxel DownVoxel = Chunk->Voxels[IVAN_GET_VOXEL_INDEX(i, j, k - 1)];
		voxel RightVoxel = Chunk->Voxels[IVAN_GET_VOXEL_INDEX(i + 1, j, k)];
		voxel LeftVoxel = Chunk->Voxels[IVAN_GET_VOXEL_INDEX(i - 1, j, k)];
		voxel FrontVoxel = Chunk->Voxels[IVAN_GET_VOXEL_INDEX(i, j + 1, k)];
		voxel BackVoxel = Chunk->Voxels[IVAN_GET_VOXEL_INDEX(i, j - 1, k)];
	
		Result = 1;
	}

	return(Result);
}

inline void PushFaceWorkForDownVoxel(
	voxel_chunk* Chunk,
	voxel_chunk_mesh* Mesh,
	int32_t InitX, 
	int32_t InitY, 
	int32_t InitZ,
	voxel_face_tex_coords_set* FaceT,
	vec3 VoxelPos)
{
	if(NeighbourVoxelExistAndAir(
		Chunk,
		InitX,
		InitY - 1,
		InitZ))
	{
		DoFaceWorkAtBottom(Mesh, VoxelPos, FaceT);
	}
}

inline void PushFaceWorkForUpperVoxel(
	voxel_chunk* Chunk,
	voxel_chunk_mesh* Mesh, 
	int32_t InitX, 
	int32_t InitY, 
	int32_t InitZ,
	voxel_face_tex_coords_set* FaceT,
	vec3 VoxelPos)
{
	if(NeighbourVoxelExistAndAir(
		Chunk,
		InitX,
		InitY + 1,
		InitZ))
	{
		DoFaceWorkAtTop(Mesh, VoxelPos, FaceT);
	}
}

inline void PushFaceWorkForLeftVoxel(
	voxel_chunk* Chunk,
	voxel_chunk_mesh* Mesh, 
	int32_t InitX, 
	int32_t InitY, 
	int32_t InitZ,
	voxel_face_tex_coords_set* FaceT,
	vec3 VoxelPos)
{
	if(NeighbourVoxelExistAndAir(
		Chunk,
		InitX - 1,
		InitY,
		InitZ))
	{
		DoFaceWorkAtLeft(Mesh, VoxelPos, FaceT);
	}
}

inline void PushFaceWorkForRightVoxel(
	voxel_chunk* Chunk,
	voxel_chunk_mesh* Mesh, 
	int32_t InitX, 
	int32_t InitY, 
	int32_t InitZ,
	voxel_face_tex_coords_set* FaceT,
	vec3 VoxelPos)
{
	if(NeighbourVoxelExistAndAir(
		Chunk,
		InitX + 1,
		InitY,
		InitZ))
	{
		DoFaceWorkAtRight(Mesh, VoxelPos, FaceT);
	}
}

inline void PushFaceWorkForFrontVoxel(
	voxel_chunk* Chunk,
	voxel_chunk_mesh* Mesh, 
	int32_t InitX, 
	int32_t InitY, 
	int32_t InitZ,
	voxel_face_tex_coords_set* FaceT,
	vec3 VoxelPos)
{
	if(NeighbourVoxelExistAndAir(
		Chunk,
		InitX,
		InitY,
		InitZ + 1))
	{
		DoFaceWorkAtFront(Mesh, VoxelPos, FaceT);
	}
}

inline void PushFaceWorkForBackVoxel(
	voxel_chunk* Chunk,
	voxel_chunk_mesh* Mesh, 
	int32_t InitX, 
	int32_t InitY, 
	int32_t InitZ,
	voxel_face_tex_coords_set* FaceT,
	vec3 VoxelPos)
{
	if(NeighbourVoxelExistAndAir(
		Chunk,
		InitX,
		InitY,
		InitZ - 1))
	{
		DoFaceWorkAtBack(Mesh, VoxelPos, FaceT);
	}
}
#endif