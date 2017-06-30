//#include "ivan_voxel_mesh.h"

//TODO(Dima): How do I actually want to allocate MESH result???? And where???

#define IVAN_MESH_KEEP_SIDES_ON 0

struct generate_mesh_work{
	task_with_memory* Task;

	game_assets* Assets;

	voxel_chunk_header* Header;

	voxel_atlas_id VoxelAtlasID;
};

INTERNAL_FUNCTION PLATFORM_WORK_QUEUE_CALLBACK(GenerateVoxelMeshWork){
	generate_mesh_work* Work = (generate_mesh_work*)Data;

	voxel_chunk_mesh* Mesh = Work->Header->Mesh;
	voxel_chunk* Chunk = Work->Header->Chunk;

	Mesh->PUVN = PushArray(&Work->Task->Arena, (IVAN_MAX_MESH_CHUNK_FACE_COUNT * 6) >> 1, uint32_t);

	GD_COMPLETE_READS_BEFORE_FUTURE;
	GenerateVoxelMeshForChunk(Mesh, Chunk, Work->Assets, Work->VoxelAtlasID);

	GD_COMPLETE_WRITES_BEFORE_FUTURE;
	Work->Header->MeshState = VoxelMeshState_Generated;
	Work->Header->MeshTask = Work->Task;
}

INTERNAL_FUNCTION void 
GenerateVoxelMeshForChunk(
	voxel_chunk_mesh* Result, 
	voxel_chunk* Chunk, 
	game_assets* Assets,
	voxel_atlas_id VoxelAtlasID)
{
	uint32_t GenerationID = BeginGeneration(Assets);

	Result->VerticesCount = 0;
	Result->ActiveVertexIndex = 0;
	
	LoadVoxelAtlasAsset(Assets, VoxelAtlasID, true);
	loaded_voxel_atlas* Atlas = GetVoxelAtlas(Assets, VoxelAtlasID, GenerationID);

	for(int32_t k = 0; k < IVAN_VOXEL_CHUNK_HEIGHT ; k++){
		for(int32_t j = 0; j < IVAN_VOXEL_CHUNK_WIDTH; j++){
			for(int32_t i = 0; i < IVAN_VOXEL_CHUNK_WIDTH; i++){

				uint8_t ToCheck = Chunk->Voxels[IVAN_GET_VOXEL_INDEX(i, j, k)];

				vec3 VoxelPos;
				VoxelPos.x = i + 0.5f;
				VoxelPos.y = k + 0.5f;
				VoxelPos.z = j + 0.5f;
				
				voxel_tex_coords_set* TexSet = 0;
				if(Atlas){
					TexSet = &Atlas->Materials[ToCheck];
				}

				if(ToCheck != VoxelMaterial_None && TexSet){

					if((i >= 1 && i < (IVAN_VOXEL_CHUNK_WIDTH - 1)) &&
						(j >= 1 && j < (IVAN_VOXEL_CHUNK_WIDTH - 1)) &&
						(k >= 1 && k < (IVAN_VOXEL_CHUNK_HEIGHT - 1)))
					{
						uint8_t UpVoxel = Chunk->Voxels[IVAN_GET_VOXEL_INDEX(i, j, k + 1)];
						uint8_t DownVoxel = Chunk->Voxels[IVAN_GET_VOXEL_INDEX(i, j, k - 1)];
						uint8_t RightVoxel = Chunk->Voxels[IVAN_GET_VOXEL_INDEX(i + 1, j, k)];
						uint8_t LeftVoxel = Chunk->Voxels[IVAN_GET_VOXEL_INDEX(i - 1, j, k)];
						uint8_t FrontVoxel = Chunk->Voxels[IVAN_GET_VOXEL_INDEX(i, j - 1, k)];
						uint8_t BackVoxel = Chunk->Voxels[IVAN_GET_VOXEL_INDEX(i, j + 1, k)];

						if(UpVoxel == VoxelMaterial_None){
							DoFaceWorkAtTop(Result, VoxelPos, TexSet->Sets[VoxelFaceTypeIndex_Top]);
						}

						if(DownVoxel == VoxelMaterial_None){
							DoFaceWorkAtBottom(Result, VoxelPos, TexSet->Sets[VoxelFaceTypeIndex_Bottom]);
						}

						if(RightVoxel == VoxelMaterial_None){
							DoFaceWorkAtRight(Result, VoxelPos, TexSet->Sets[VoxelFaceTypeIndex_Right]);
						}

						if(LeftVoxel == VoxelMaterial_None){
							DoFaceWorkAtLeft(Result, VoxelPos, TexSet->Sets[VoxelFaceTypeIndex_Left]);
						}

						if(FrontVoxel == VoxelMaterial_None){
							DoFaceWorkAtFront(Result, VoxelPos, TexSet->Sets[VoxelFaceTypeIndex_Front]);
						}

						if(BackVoxel == VoxelMaterial_None){
							DoFaceWorkAtBack(Result, VoxelPos, TexSet->Sets[VoxelFaceTypeIndex_Back]);
						}
					}
					else{
						int32_t IndexInNeighbourChunk = 0;
						if(i == 0){
							//BUG(Dima): For some reasons Voxels of neighbour are not initialized
							if(Chunk->LeftNeighbour){
								IndexInNeighbourChunk = IVAN_GET_VOXEL_INDEX(IVAN_VOXEL_CHUNK_WIDTH - 1, j, k);

								if(Chunk->LeftNeighbour->Voxels[IndexInNeighbourChunk] == VoxelMaterial_None){
									DoFaceWorkAtLeft(Result, VoxelPos, TexSet->Sets[VoxelFaceTypeIndex_Left]);
								}
							}
							else{
#if IVAN_MESH_KEEP_SIDES_ON
								DoFaceWorkAtLeft(Result, VoxelPos, TexSet->Sets[VoxelFaceTypeIndex_Left]);
#endif
							}
						}
						if(i == (IVAN_VOXEL_CHUNK_WIDTH - 1)){
							if(Chunk->RightNeighbour){
								IndexInNeighbourChunk = IVAN_GET_VOXEL_INDEX(0, j, k);

								if(Chunk->RightNeighbour->Voxels[IndexInNeighbourChunk] == VoxelMaterial_None){
									DoFaceWorkAtRight(Result, VoxelPos, TexSet->Sets[VoxelFaceTypeIndex_Right]);
								}
							}
							else{
#if IVAN_MESH_KEEP_SIDES_ON
								DoFaceWorkAtRight(Result, VoxelPos, TexSet->Sets[VoxelFaceTypeIndex_Right]);
#endif
							}
						}
						if(j == 0){
							if(Chunk->FrontNeighbour){
								IndexInNeighbourChunk = IVAN_GET_VOXEL_INDEX(i, IVAN_VOXEL_CHUNK_WIDTH - 1, k);

								if(Chunk->FrontNeighbour->Voxels[IndexInNeighbourChunk] == VoxelMaterial_None){
									DoFaceWorkAtFront(Result, VoxelPos, TexSet->Sets[VoxelFaceTypeIndex_Front]);
								}
							}
							else{
#if IVAN_MESH_KEEP_SIDES_ON
								DoFaceWorkAtFront(Result, VoxelPos, TexSet->Sets[VoxelFaceTypeIndex_Front]);
#endif
							}
						}

						if(j == (IVAN_VOXEL_CHUNK_WIDTH - 1)){
							if(Chunk->BackNeighbour){
								IndexInNeighbourChunk = IVAN_GET_VOXEL_INDEX(i, 0, k);

								if(Chunk->BackNeighbour->Voxels[IndexInNeighbourChunk] == VoxelMaterial_None){
									DoFaceWorkAtBack(Result, VoxelPos, TexSet->Sets[VoxelFaceTypeIndex_Back]);
								}
							}
							else{
#if IVAN_MESH_KEEP_SIDES_ON
								DoFaceWorkAtBack(Result, VoxelPos, TexSet->Sets[VoxelFaceTypeIndex_Back]);
#endif
							}
						}

#if IVAN_MESH_KEEP_SIDES_ON
						if(k == 0){
							DoFaceWorkAtBottom(Result, VoxelPos, TexSet->Sets[VoxelFaceTypeIndex_Bottom]);
						}

						if(k == (IVAN_VOXEL_CHUNK_HEIGHT - 1)){
							DoFaceWorkAtTop(Result, VoxelPos, TexSet->Sets[VoxelFaceTypeIndex_Top]);
						}
#endif

						PushFaceWorkForRightVoxel(
							Chunk, Result, 
							i, k, j, 
							TexSet->Sets[VoxelFaceTypeIndex_Right], 
							VoxelPos);

						PushFaceWorkForLeftVoxel(
							Chunk, Result, 
							i, k, j, 
							TexSet->Sets[VoxelFaceTypeIndex_Left], 
							VoxelPos);

						PushFaceWorkForUpperVoxel(
							Chunk, Result, 
							i, k, j, 
							TexSet->Sets[VoxelFaceTypeIndex_Top], 
							VoxelPos);

						PushFaceWorkForDownVoxel(
							Chunk, Result, 
							i, k, j, 
							TexSet->Sets[VoxelFaceTypeIndex_Bottom], 
							VoxelPos);

						PushFaceWorkForFrontVoxel(
							Chunk, Result, 
							i, k, j, 
							TexSet->Sets[VoxelFaceTypeIndex_Front], 
							VoxelPos);

						PushFaceWorkForBackVoxel(
							Chunk, Result, 
							i, k, j, 
							TexSet->Sets[VoxelFaceTypeIndex_Back], 
							VoxelPos);

					}
				}
			}
		}
	}

	Result->VerticesCount = Result->ActiveVertexIndex;
	Result->MeshHandle = 0;

	EndGeneration(Assets, GenerationID);
}
