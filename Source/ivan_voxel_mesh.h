//TODO(DIMA): Think about how to allocate memory for mesh
//TODO(DIMA): Voxel atlas texture extraction

/*
	IDEAS ABOUT OPTIMIZATION:
		1) Send per-vertex index of normal in normal array. 
		Extract it in the shader and assign. It will cost 3
		bits per vertex instead of 12 bytes = 96 bits.

		2) Send index of the texture in voxel atlas to shader
		and extract it there. MAYBE. Almost 10 - 16 bits will
		be enough instead of 8 bytes = 64 bits.

		3) Send x, y, z indices of the chunk to shader
		and calculate position there. MAYBE. Stupid idea.
		Because floats takes the same memory space as 
		integers xD
*/

#ifndef IVAN_VOXEL_MESH_H
#define IVAN_VOXEL_MESH_H

#include "ivan_voxel_shared.h"

struct voxel_chunk_mesh{
#define VOXEL_CHUNK_MESH_SOA 0
	/*Vertices*/
#if VOXEL_CHUNK_MESH_SOA
	float PositionsX[];
	float PositionsY[];
	float PositionsZ[];

	float TexCoordsX[];
	float TexCoordsY[];

	float NormalsX[];
	float NormalsY[];
	float NormalsZ[];
#else
	vec3* Positions;
	vec2* TexCoords;
	vec3* Normals;
#endif

	uint32_t VerticesCount;
	uint32_t ActiveVertexIndex;

	/*Indices*/
	uint32_t* Indices;

	uint32_t IndicesCount;
	uint32_t ActiveIndexIndex;
};

enum voxel_push_face_work_type{
	VOXEL_PUSH_AT_TOP,
	VOXEL_PUSH_AT_BOTTOM,
	VOXEL_PUSH_AT_LEFT,
	VOXEL_PUSH_AT_RIGHT,
	VOXEL_PUSH_AT_FRONT,
	VOXEL_PUSH_AT_BACK,
};

struct voxel_push_face_work_queue{
	voxel_push_face_work_queue* Sentinel;
	voxel_push_face_work_queue* Next;

	voxel_face_tex_coords_set* Set;

	vec3 Pos;
};

inline void PushFaceWork(
	voxel_push_face_work_queue* Queue, 
	vec3 Pos,
	voxel_face_tex_coords_set* FaceT,
	memory_arena* Arena)
{
	voxel_push_face_work_queue* NewEntry = PushStruct(Arena, voxel_push_face_work_queue);
	
	NewEntry->Pos = Pos;
	NewEntry->Set = FaceT;

	NewEntry->Next = Queue->Sentinel->Next;
	Queue->Sentinel->Next = NewEntry;
}

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

#if VOXEL_CHUNK_MESH_SOA
	/*Positions*/
	Mesh->PositionsX[Index] = P0.x;
	Mesh->PositionsY[Index] = P0.y;
	Mesh->PositionsZ[Index] = P0.z;

	Mesh->PositionsX[Index + 1] = P1.x;
	Mesh->PositionsY[Index + 1] = P1.y;
	Mesh->PositionsZ[Index + 1] = P1.z;

	Mesh->PositionsX[Index + 2] = P2.x;
	Mesh->PositionsY[Index + 2] = P2.y;
	Mesh->PositionsZ[Index + 2] = P2.z;

	Mesh->PositionsX[Index + 3] = P3.x;
	Mesh->PositionsY[Index + 3] = P3.y;
	Mesh->PositionsZ[Index + 3] = P3.z;

	/*Normals*/
	Mesh->NormalsX[Index] = N.x;
	Mesh->NormalsY[Index] = N.y;
	Mesh->NormalsZ[Index] = N.z;

	Mesh->NormalsX[Index + 1] = N.x;
	Mesh->NormalsY[Index + 1] = N.y;
	Mesh->NormalsZ[Index + 1] = N.z;

	Mesh->NormalsX[Index + 2] = N.x;
	Mesh->NormalsY[Index + 2] = N.y;
	Mesh->NormalsZ[Index + 2] = N.z;

	Mesh->NormalsX[Index + 3] = N.x;
	Mesh->NormalsY[Index + 3] = N.y;
	Mesh->NormalsZ[Index + 3] = N.z;

	/*TextureCoords*/
	Mesh->TexCoordsX[Index] = T0.x;
	Mesh->TexCoordsY[Index] = T0.y;

	Mesh->TexCoordsX[Index + 1] = T1.x;
	Mesh->TexCoordsY[Index + 1] = T1.y;

	Mesh->TexCoordsX[Index + 2] = T2.x;
	Mesh->TexCoordsY[Index + 2] = T2.y;
	
	Mesh->TexCoordsX[Index + 3] = T3.x;
	Mesh->TexCoordsY[Index + 3] = T3.y;
#else
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
#endif
	
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

inline voxel_push_face_work_queue* MakePushFaceQueue(voxel_push_face_work_queue* NewQueue){

	voxel_push_face_work_queue* PNewQueue = NewQueue;
	
	PNewQueue->Sentinel = PNewQueue;
	PNewQueue->Next = 0;

	return(PNewQueue);
}

inline void DoPushFaceQueueWork(
	voxel_push_face_work_queue* Queue,
	voxel_chunk_mesh* Mesh,
	voxel_push_face_work_type QueueType)
{
	voxel_push_face_work_queue* Temp = 0;

	switch(QueueType){
		case(VOXEL_PUSH_AT_FRONT):{
			for(Temp = Queue->Sentinel->Next;
				Temp != 0;
				Temp = Temp->Next)
			{
				PushVoxelMeshChunkFace(
					Mesh,
					Vec3(Temp->Pos.x - 0.5f, Temp->Pos.y + 0.5f, Temp->Pos.z + 0.5f), Temp->Set->T0,
					Vec3(Temp->Pos.x + 0.5f, Temp->Pos.y + 0.5f, Temp->Pos.z + 0.5f), Temp->Set->T1,
					Vec3(Temp->Pos.x + 0.5f, Temp->Pos.y - 0.5f, Temp->Pos.z + 0.5f), Temp->Set->T2,
					Vec3(Temp->Pos.x - 0.5f, Temp->Pos.y - 0.5f, Temp->Pos.z + 0.5f), Temp->Set->T3,
					Vec3(0.0f, 0.0f, 1.0f));				
			}
		}break;
		case(VOXEL_PUSH_AT_BACK):{
			for(Temp = Queue->Sentinel->Next;
				Temp != 0;
				Temp = Temp->Next)
			{
				PushVoxelMeshChunkFace(
					Mesh,
					Vec3(Temp->Pos.x + 0.5f, Temp->Pos.y + 0.5f, Temp->Pos.z - 0.5f), Temp->Set->T0,
					Vec3(Temp->Pos.x - 0.5f, Temp->Pos.y + 0.5f, Temp->Pos.z - 0.5f), Temp->Set->T1,
					Vec3(Temp->Pos.x - 0.5f, Temp->Pos.y - 0.5f, Temp->Pos.z - 0.5f), Temp->Set->T2,
					Vec3(Temp->Pos.x + 0.5f, Temp->Pos.y - 0.5f, Temp->Pos.z - 0.5f), Temp->Set->T3,
					Vec3(0.0f, 0.0f, -1.0f));
			}
		}break;
		case(VOXEL_PUSH_AT_TOP):{
			for(Temp = Queue->Sentinel->Next;
				Temp != 0;
				Temp = Temp->Next)
			{
				PushVoxelMeshChunkFace(
					Mesh,
					Vec3(Temp->Pos.x - 0.5f, Temp->Pos.y + 0.5f, Temp->Pos.z - 0.5f), Temp->Set->T0,
					Vec3(Temp->Pos.x + 0.5f, Temp->Pos.y + 0.5f, Temp->Pos.z - 0.5f), Temp->Set->T1,
					Vec3(Temp->Pos.x + 0.5f, Temp->Pos.y + 0.5f, Temp->Pos.z + 0.5f), Temp->Set->T2,
					Vec3(Temp->Pos.x - 0.5f, Temp->Pos.y + 0.5f, Temp->Pos.z + 0.5f), Temp->Set->T3,
					Vec3(0.0f, 1.0f, 0.0f));
			}
		}break;
		case(VOXEL_PUSH_AT_BOTTOM):{
			for(Temp = Queue->Sentinel->Next;
				Temp != 0;
				Temp = Temp->Next)
			{
				PushVoxelMeshChunkFace(
					Mesh,
					Vec3(Temp->Pos.x - 0.5f, Temp->Pos.y - 0.5f, Temp->Pos.z + 0.5f), Temp->Set->T0,
					Vec3(Temp->Pos.x + 0.5f, Temp->Pos.y - 0.5f, Temp->Pos.z + 0.5f), Temp->Set->T1,
					Vec3(Temp->Pos.x + 0.5f, Temp->Pos.y - 0.5f, Temp->Pos.z - 0.5f), Temp->Set->T2,
					Vec3(Temp->Pos.x - 0.5f, Temp->Pos.y - 0.5f, Temp->Pos.z - 0.5f), Temp->Set->T3,
					Vec3(0.0f, -1.0f, 0.0f));				
			}
		}break;
		case(VOXEL_PUSH_AT_LEFT):{
			for(Temp = Queue->Sentinel->Next;
				Temp != 0;
				Temp = Temp->Next)
			{
				PushVoxelMeshChunkFace(
					Mesh,
					Vec3(Temp->Pos.x - 0.5f, Temp->Pos.y + 0.5f, Temp->Pos.z - 0.5f), Temp->Set->T0,
					Vec3(Temp->Pos.x - 0.5f, Temp->Pos.y + 0.5f, Temp->Pos.z + 0.5f), Temp->Set->T1,
					Vec3(Temp->Pos.x - 0.5f, Temp->Pos.y - 0.5f, Temp->Pos.z + 0.5f), Temp->Set->T2,
					Vec3(Temp->Pos.x - 0.5f, Temp->Pos.y - 0.5f, Temp->Pos.z - 0.5f), Temp->Set->T3,
					Vec3(-1.0f, 0.0f, 0.0f));
			}
		}break;
		case(VOXEL_PUSH_AT_RIGHT):{
			for(Temp = Queue->Sentinel->Next;
				Temp != 0;
				Temp = Temp->Next)
			{	
				PushVoxelMeshChunkFace(
					Mesh,
					Vec3(Temp->Pos.x + 0.5f, Temp->Pos.y + 0.5f, Temp->Pos.z + 0.5f), Temp->Set->T0,
					Vec3(Temp->Pos.x + 0.5f, Temp->Pos.y + 0.5f, Temp->Pos.z - 0.5f), Temp->Set->T1,
					Vec3(Temp->Pos.x + 0.5f, Temp->Pos.y - 0.5f, Temp->Pos.z - 0.5f), Temp->Set->T2,
					Vec3(Temp->Pos.x + 0.5f, Temp->Pos.y - 0.5f, Temp->Pos.z + 0.5f), Temp->Set->T3,
					Vec3(1.0f, 0.0f, 0.0f));
			}
		}break;
	}
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
	int32_t InitX, 
	int32_t InitY, 
	int32_t InitZ,
	voxel_face_tex_coords_set* FaceT,
	voxel_push_face_work_queue* Queue,
	vec3 VoxelPos,
	memory_arena* Arena)
{
	if(NeighbourVoxelExistAndAir(
		Chunk,
		InitX,
		InitY - 1,
		InitZ))
	{
		PushFaceWork(Queue, VoxelPos, FaceT, Arena);
	}
}

inline void PushFaceWorkForUpperVoxel(
	voxel_chunk* Chunk, 
	int32_t InitX, 
	int32_t InitY, 
	int32_t InitZ,
	voxel_face_tex_coords_set* FaceT,
	voxel_push_face_work_queue* Queue,
	vec3 VoxelPos,
	memory_arena* Arena)
{
	if(NeighbourVoxelExistAndAir(
		Chunk,
		InitX,
		InitY + 1,
		InitZ))
	{
		PushFaceWork(Queue, VoxelPos, FaceT, Arena);
	}
}

inline void PushFaceWorkForLeftVoxel(
	voxel_chunk* Chunk, 
	int32_t InitX, 
	int32_t InitY, 
	int32_t InitZ,
	voxel_face_tex_coords_set* FaceT,
	voxel_push_face_work_queue* Queue,
	vec3 VoxelPos,
	memory_arena* Arena)
{
	if(NeighbourVoxelExistAndAir(
		Chunk,
		InitX - 1,
		InitY,
		InitZ))
	{
		PushFaceWork(Queue, VoxelPos, FaceT, Arena);
	}
}

inline void PushFaceWorkForRightVoxel(
	voxel_chunk* Chunk, 
	int32_t InitX, 
	int32_t InitY, 
	int32_t InitZ,
	voxel_face_tex_coords_set* FaceT,
	voxel_push_face_work_queue* Queue,
	vec3 VoxelPos,
	memory_arena* Arena)
{
	if(NeighbourVoxelExistAndAir(
		Chunk,
		InitX + 1,
		InitY,
		InitZ))
	{
		PushFaceWork(Queue, VoxelPos, FaceT, Arena);
	}
}

inline void PushFaceWorkForFrontVoxel(
	voxel_chunk* Chunk, 
	int32_t InitX, 
	int32_t InitY, 
	int32_t InitZ,
	voxel_face_tex_coords_set* FaceT,
	voxel_push_face_work_queue* Queue,
	vec3 VoxelPos,
	memory_arena* Arena)
{
	if(NeighbourVoxelExistAndAir(
		Chunk,
		InitX,
		InitY,
		InitZ + 1))
	{
		PushFaceWork(Queue, VoxelPos, FaceT, Arena);
	}
}

inline void PushFaceWorkForBackVoxel(
	voxel_chunk* Chunk, 
	int32_t InitX, 
	int32_t InitY, 
	int32_t InitZ,
	voxel_face_tex_coords_set* FaceT,
	voxel_push_face_work_queue* Queue,
	vec3 VoxelPos,
	memory_arena* Arena)
{
	if(NeighbourVoxelExistAndAir(
		Chunk,
		InitX,
		InitY,
		InitZ - 1))
	{
		PushFaceWork(Queue, VoxelPos, FaceT, Arena);
	}
}

//TODO(Dima): How do I actually want to allocate MESH result???? And where???
inline void GenerateVoxelMeshForChunk(
	voxel_chunk_mesh* Result, 
	voxel_chunk* Chunk, 
	game_assets* Assets,
	voxel_atlas_id VoxelAtlasID)
{
	uint32_t GenerationID = BeginGeneration(Assets);
	
	LoadVoxelAtlasAsset(Assets, VoxelAtlasID, true);
	loaded_voxel_atlas* Atlas = GetVoxelAtlas(Assets, VoxelAtlasID, GenerationID);

	task_with_memory* Task = BeginTaskWithMemory(Assets->TranState, false);

	voxel_push_face_work_queue TopQueue_;
	voxel_push_face_work_queue BottomQueue_;
	voxel_push_face_work_queue LeftQueue_;
	voxel_push_face_work_queue RightQueue_;
	voxel_push_face_work_queue FrontQueue_;
	voxel_push_face_work_queue BackQueue_;

	voxel_push_face_work_queue* TopQueue = MakePushFaceQueue(&TopQueue_);
	voxel_push_face_work_queue* BottomQueue = MakePushFaceQueue(&BottomQueue_);
	voxel_push_face_work_queue* LeftQueue = MakePushFaceQueue(&LeftQueue_);
	voxel_push_face_work_queue* RightQueue = MakePushFaceQueue(&RightQueue_);
	voxel_push_face_work_queue* FrontQueue = MakePushFaceQueue(&FrontQueue_);
	voxel_push_face_work_queue* BackQueue = MakePushFaceQueue(&BackQueue_);
	
	for(int32_t k = 1; k < IVAN_VOXEL_CHUNK_HEIGHT - 1; k++){
		for(int32_t j = 1; j < IVAN_VOXEL_CHUNK_WIDTH - 1; j++){
			for(int32_t i = 1; i < IVAN_VOXEL_CHUNK_WIDTH - 1; i++){

				voxel ToCheck = Chunk->Voxels[IVAN_GET_VOXEL_INDEX(i, j, k)];

				vec3 VoxelPos;
				VoxelPos.x = i + 0.5f;
				VoxelPos.y = k + 0.5f;
				VoxelPos.z = j + 0.5f;
				
				voxel_tex_coords_set* TexSet = 0;
				if(Atlas){
					TexSet = &Atlas->Materials[ToCheck.Type];
				}

				if(!ToCheck.IsAir && TexSet){

					if((i >= 1 && i < (IVAN_VOXEL_CHUNK_WIDTH - 1)) &&
						(j >= 1 && j < (IVAN_VOXEL_CHUNK_WIDTH - 1)) &&
						(k >= 1 && k < (IVAN_VOXEL_CHUNK_HEIGHT - 1)))
					{
						voxel UpVoxel = Chunk->Voxels[IVAN_GET_VOXEL_INDEX(i, j, k + 1)];
						voxel DownVoxel = Chunk->Voxels[IVAN_GET_VOXEL_INDEX(i, j, k - 1)];
						voxel RightVoxel = Chunk->Voxels[IVAN_GET_VOXEL_INDEX(i + 1, j, k)];
						voxel LeftVoxel = Chunk->Voxels[IVAN_GET_VOXEL_INDEX(i - 1, j, k)];
						voxel FrontVoxel = Chunk->Voxels[IVAN_GET_VOXEL_INDEX(i, j + 1, k)];
						voxel BackVoxel = Chunk->Voxels[IVAN_GET_VOXEL_INDEX(i, j - 1, k)];

						/*
							VoxelFaceTypeIndex_Top,
							VoxelFaceTypeIndex_Bottom,
							VoxelFaceTypeIndex_Left,
							VoxelFaceTypeIndex_Right,
							VoxelFaceTypeIndex_Front,
							VoxelFaceTypeIndex_Back,
						*/

						if(UpVoxel.IsAir){
							PushFaceWork(
								TopQueue, VoxelPos,  
								&TexSet->Sets[VoxelFaceTypeIndex_Top], 
								Task->Memory.Arena);
						}

						if(DownVoxel.IsAir){
							PushFaceWork(
								BottomQueue, VoxelPos,  
								&TexSet->Sets[VoxelFaceTypeIndex_Bottom], 
								Task->Memory.Arena);
						}

						if(RightVoxel.IsAir){
							PushFaceWork(
								RightQueue, VoxelPos,  
								&TexSet->Sets[VoxelFaceTypeIndex_Right], 
								Task->Memory.Arena);
						}

						if(LeftVoxel.IsAir){
							PushFaceWork(
								LeftQueue, VoxelPos,  
								&TexSet->Sets[VoxelFaceTypeIndex_Left], 
								Task->Memory.Arena);
						}

						if(FrontVoxel.IsAir){
							PushFaceWork(
								FrontQueue, VoxelPos,  
								&TexSet->Sets[VoxelFaceTypeIndex_Front], 
								Task->Memory.Arena);
						}

						if(BackVoxel.IsAir){
							PushFaceWork(
								BackQueue, VoxelPos,  
								&TexSet->Sets[VoxelFaceTypeIndex_Back], 
								Task->Memory.Arena);
						}
					}
					else{

						if(i == 0){
							PushFaceWork(
								LeftQueue, VoxelPos,  
								&TexSet->Sets[VoxelFaceTypeIndex_Left], 
								Task->Memory.Arena);
						}

						if(i == (IVAN_VOXEL_CHUNK_WIDTH - 1)){
							PushFaceWork(
								RightQueue, VoxelPos,  
								&TexSet->Sets[VoxelFaceTypeIndex_Right], 
								Task->Memory.Arena);
						}

						if(j == 0){
							PushFaceWork(
								FrontQueue, VoxelPos,  
								&TexSet->Sets[VoxelFaceTypeIndex_Front], 
								Task->Memory.Arena);
						}

						if(j == (IVAN_VOXEL_CHUNK_WIDTH - 1)){
							PushFaceWork(
								BackQueue, VoxelPos,  
								&TexSet->Sets[VoxelFaceTypeIndex_Back], 
								Task->Memory.Arena);
						}

						if(k == 0){
							PushFaceWork(
								BottomQueue, VoxelPos,  
								&TexSet->Sets[VoxelFaceTypeIndex_Bottom], 
								Task->Memory.Arena);
						}

						if(k == (IVAN_VOXEL_CHUNK_HEIGHT - 1)){
							PushFaceWork(
								TopQueue, VoxelPos,  
								&TexSet->Sets[VoxelFaceTypeIndex_Top], 
								Task->Memory.Arena);
						}

						PushFaceWorkForRightVoxel(
							Chunk, 
							i, k, j, 
							&TexSet->Sets[VoxelFaceTypeIndex_Right], 
							RightQueue, VoxelPos, Task->Memory.Arena);

						PushFaceWorkForLeftVoxel(
							Chunk, 
							i, k, j, 
							&TexSet->Sets[VoxelFaceTypeIndex_Left], 
							LeftQueue, VoxelPos, Task->Memory.Arena);

						PushFaceWorkForUpperVoxel(
							Chunk, 
							i, k, j, 
							&TexSet->Sets[VoxelFaceTypeIndex_Top], 
							TopQueue, VoxelPos, Task->Memory.Arena);

						PushFaceWorkForDownVoxel(
							Chunk, 
							i, k, j, 
							&TexSet->Sets[VoxelFaceTypeIndex_Bottom], 
							BottomQueue, VoxelPos, Task->Memory.Arena);

						PushFaceWorkForFrontVoxel(
							Chunk, 
							i, k, j, 
							&TexSet->Sets[VoxelFaceTypeIndex_Front], 
							FrontQueue, VoxelPos, Task->Memory.Arena);

						PushFaceWorkForBackVoxel(
							Chunk, 
							i, k, j, 
							&TexSet->Sets[VoxelFaceTypeIndex_Back], 
							BackQueue, VoxelPos, Task->Memory.Arena);

					}
				}
			}
		}
	}

	DoPushFaceQueueWork(TopQueue, Result, VOXEL_PUSH_AT_TOP);
	DoPushFaceQueueWork(BottomQueue, Result, VOXEL_PUSH_AT_BOTTOM);
	DoPushFaceQueueWork(LeftQueue, Result, VOXEL_PUSH_AT_LEFT);
	DoPushFaceQueueWork(RightQueue, Result, VOXEL_PUSH_AT_RIGHT);
	DoPushFaceQueueWork(FrontQueue, Result, VOXEL_PUSH_AT_FRONT);
	DoPushFaceQueueWork(BackQueue, Result, VOXEL_PUSH_AT_BACK);

	EndTaskWithMemory(Task);

	EndGeneration(Assets, GenerationID);
}

#endif