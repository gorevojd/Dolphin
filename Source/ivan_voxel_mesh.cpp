//#include "ivan_voxel_mesh.h"

//TODO(Dima): How do I actually want to allocate MESH result???? And where???

INTERNAL_FUNCTION void 
GenerateVoxelMeshForChunk(
	voxel_chunk_mesh* Result, 
	voxel_chunk* Chunk, 
	game_assets* Assets,
	voxel_atlas_id VoxelAtlasID)
{
	uint32_t GenerationID = BeginGeneration(Assets);
	
	LoadVoxelAtlasAsset(Assets, VoxelAtlasID, true);
	loaded_voxel_atlas* Atlas = GetVoxelAtlas(Assets, VoxelAtlasID, GenerationID);

	for(int32_t k = 0; k < IVAN_VOXEL_CHUNK_HEIGHT ; k++){
		for(int32_t j = 0; j < IVAN_VOXEL_CHUNK_WIDTH; j++){
			for(int32_t i = 0; i < IVAN_VOXEL_CHUNK_WIDTH; i++){

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
						voxel FrontVoxel = Chunk->Voxels[IVAN_GET_VOXEL_INDEX(i, j - 1, k)];
						voxel BackVoxel = Chunk->Voxels[IVAN_GET_VOXEL_INDEX(i, j + 1, k)];

						if(UpVoxel.IsAir){
							DoFaceWorkAtTop(Result, VoxelPos, &TexSet->Sets[VoxelFaceTypeIndex_Top]);
						}

						if(DownVoxel.IsAir){
							DoFaceWorkAtBottom(Result, VoxelPos, &TexSet->Sets[VoxelFaceTypeIndex_Bottom]);
						}

						if(RightVoxel.IsAir){
							DoFaceWorkAtRight(Result, VoxelPos, &TexSet->Sets[VoxelFaceTypeIndex_Right]);
						}

						if(LeftVoxel.IsAir){
							DoFaceWorkAtLeft(Result, VoxelPos, &TexSet->Sets[VoxelFaceTypeIndex_Left]);
						}

						if(FrontVoxel.IsAir){
							DoFaceWorkAtFront(Result, VoxelPos, &TexSet->Sets[VoxelFaceTypeIndex_Front]);
						}

						if(BackVoxel.IsAir){
							DoFaceWorkAtBack(Result, VoxelPos, &TexSet->Sets[VoxelFaceTypeIndex_Back]);
						}
					}
					else{

						if(i == 0){
							DoFaceWorkAtLeft(Result, VoxelPos, &TexSet->Sets[VoxelFaceTypeIndex_Left]);
						}

						if(i == (IVAN_VOXEL_CHUNK_WIDTH - 1)){
							DoFaceWorkAtRight(Result, VoxelPos, &TexSet->Sets[VoxelFaceTypeIndex_Right]);
						}

						if(j == 0){
							DoFaceWorkAtFront(Result, VoxelPos, &TexSet->Sets[VoxelFaceTypeIndex_Front]);
						}

						if(j == (IVAN_VOXEL_CHUNK_WIDTH - 1)){
							DoFaceWorkAtBack(Result, VoxelPos, &TexSet->Sets[VoxelFaceTypeIndex_Back]);
						}

						if(k == 0){
							DoFaceWorkAtBottom(Result, VoxelPos, &TexSet->Sets[VoxelFaceTypeIndex_Bottom]);
						}

						if(k == (IVAN_VOXEL_CHUNK_HEIGHT - 1)){
							DoFaceWorkAtTop(Result, VoxelPos, &TexSet->Sets[VoxelFaceTypeIndex_Top]);
						}

						PushFaceWorkForRightVoxel(
							Chunk, Result, 
							i, k, j, 
							&TexSet->Sets[VoxelFaceTypeIndex_Right], 
							VoxelPos);

						PushFaceWorkForLeftVoxel(
							Chunk, Result, 
							i, k, j, 
							&TexSet->Sets[VoxelFaceTypeIndex_Left], 
							VoxelPos);

						PushFaceWorkForUpperVoxel(
							Chunk, Result, 
							i, k, j, 
							&TexSet->Sets[VoxelFaceTypeIndex_Top], 
							VoxelPos);

						PushFaceWorkForDownVoxel(
							Chunk, Result, 
							i, k, j, 
							&TexSet->Sets[VoxelFaceTypeIndex_Bottom], 
							VoxelPos);

						PushFaceWorkForFrontVoxel(
							Chunk, Result, 
							i, k, j, 
							&TexSet->Sets[VoxelFaceTypeIndex_Front], 
							VoxelPos);

						PushFaceWorkForBackVoxel(
							Chunk, Result, 
							i, k, j, 
							&TexSet->Sets[VoxelFaceTypeIndex_Back], 
							VoxelPos);

					}
				}
			}
		}
	}

	Result->IndicesCount = Result->ActiveIndexIndex;
	Result->VerticesCount = Result->ActiveVertexIndex;

	EndGeneration(Assets, GenerationID);
}
