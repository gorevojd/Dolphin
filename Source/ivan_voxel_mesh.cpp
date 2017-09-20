#include "ivan_voxel_mesh.h"

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

	*Mesh = {};

	IVAN_COMPLETE_READS_BEFORE_FUTURE;

	voxel_mesh_generation_context Context = InitVoxelMeshGeneration(Chunk, Work->Assets, Work->VoxelAtlasID);
	IVAN_COMPLETE_WRITES_BEFORE_FUTURE;
	Mesh->PUVN = (ivan_vertex_type*)Platform.AllocateMemory(Context.MemoryRequired);
	IVAN_COMPLETE_WRITES_BEFORE_FUTURE;
	FinalizeVoxelMeshGeneration(Mesh, &Context);

	IVAN_COMPLETE_WRITES_BEFORE_FUTURE;
	Work->Header->MeshTask = Work->Task;
	Work->Header->MeshState = VoxelMeshState_Generated;
}

inline voxel_mesh_generation_queue MakeVoxelMeshGenerationQueue(){
	voxel_mesh_generation_queue Result = {};

	Result.IsSentinel = 1;

	return(Result);
}

inline void PushFaceWorkToQueue(
	voxel_mesh_generation_queue* Queue,
	vec3 VoxelPos, 
	uint8_t TextureIndexInAtlas)
{
	voxel_mesh_generation_queue* NewEntry = (voxel_mesh_generation_queue*)
		malloc(sizeof(voxel_mesh_generation_queue));

	NewEntry->VoxelPos = VoxelPos;
	NewEntry->TextureIndexInAtlas = TextureIndexInAtlas;
	NewEntry->IsSentinel = 0;
	NewEntry->Next = Queue->Next;

	Queue->EntryCount++;
	Queue->Next = NewEntry;
}

inline void PushFaceWorkForNeighbourVoxel(	
	voxel_mesh_generation_queue* Queue,
	voxel_chunk* Chunk,
	int32_t InitX, 
	int32_t InitY, 
	int32_t InitZ,
	uint8_t TextureIndexInAtlas,
	vec3 VoxelPos)
{
	if(NeighbourVoxelExistAndAir(
		Chunk,
		InitX,
		InitY,
		InitZ))
	{
		PushFaceWorkToQueue(Queue, VoxelPos, TextureIndexInAtlas);
	}
}

inline void FreeVoxelMeshGenerationQueue(voxel_mesh_generation_queue* Queue){
	voxel_mesh_generation_queue* Temp = Queue->Next;

	voxel_mesh_generation_queue* NextAddres = 0;

	while(Temp != 0){
		NextAddres = Temp->Next;

		free(Temp);

		Temp = NextAddres;
	}
}

typedef enum voxel_mesh_generation_queue_type{
	VOXEL_MESH_QUEUE_TOP,
	VOXEL_MESH_QUEUE_BOTTOM,
	VOXEL_MESH_QUEUE_RIGHT,
	VOXEL_MESH_QUEUE_LEFT,
	VOXEL_MESH_QUEUE_FRONT,
	VOXEL_MESH_QUEUE_BACK,
} voxel_mesh_generation_queue_type;

inline void PerformWorkForVoxelMeshQueue(
	voxel_mesh_generation_queue* Queue,
	voxel_mesh_generation_queue_type Type,
	voxel_chunk_mesh* Mesh)
{
	voxel_mesh_generation_queue* Entry = Queue->Next;
	uint32_t EntriesDone = 0;

	switch(Type){
		case(VOXEL_MESH_QUEUE_TOP):{
			for(Entry;
				Entry != 0;
				Entry = Entry->Next)
			{
				DoFaceWorkAtTop(Mesh, Entry->VoxelPos, Entry->TextureIndexInAtlas);
				EntriesDone++;
			}
		}break;

		case(VOXEL_MESH_QUEUE_BOTTOM):{
			for(Entry;
				Entry != 0;
				Entry = Entry->Next)
			{
				DoFaceWorkAtBottom(Mesh, Entry->VoxelPos, Entry->TextureIndexInAtlas);
				EntriesDone++;
			}
		}break;

		case(VOXEL_MESH_QUEUE_RIGHT):{
			for(Entry;
				Entry != 0;
				Entry = Entry->Next)
			{
				DoFaceWorkAtRight(Mesh, Entry->VoxelPos, Entry->TextureIndexInAtlas);
				EntriesDone++;
			}
		}break;

		case(VOXEL_MESH_QUEUE_LEFT):{
			for(Entry;
				Entry != 0;
				Entry = Entry->Next)
			{
				DoFaceWorkAtLeft(Mesh, Entry->VoxelPos, Entry->TextureIndexInAtlas);
				EntriesDone++;
			}
		}break;

		case(VOXEL_MESH_QUEUE_FRONT):{
			for(Entry;
				Entry != 0;
				Entry = Entry->Next)
			{
				DoFaceWorkAtFront(Mesh, Entry->VoxelPos, Entry->TextureIndexInAtlas);
				EntriesDone++;
			}
		}break;

		case(VOXEL_MESH_QUEUE_BACK):{
			for(Entry;
				Entry != 0;
				Entry = Entry->Next)
			{
				DoFaceWorkAtBack(Mesh, Entry->VoxelPos, Entry->TextureIndexInAtlas);
				EntriesDone++;
			}
		}break;
	
		INVALID_DEFAULT_CASE;
	}

	Assert(EntriesDone == Queue->EntryCount);
}

INTERNAL_FUNCTION voxel_mesh_generation_context
InitVoxelMeshGeneration(
	voxel_chunk* Chunk, 
	game_assets* Assets,
	voxel_atlas_id VoxelAtlasID)
{
	voxel_mesh_generation_context Context_;
	voxel_mesh_generation_context* Context = &Context_;

	Context->GenerationID = BeginGeneration(Assets);

	LoadVoxelAtlasAsset(Assets, VoxelAtlasID, true);
	loaded_voxel_atlas* Atlas = GetVoxelAtlas(Assets, VoxelAtlasID, Context->GenerationID);

	Context->TopQueue = MakeVoxelMeshGenerationQueue();
	Context->BottomQueue = MakeVoxelMeshGenerationQueue();
	Context->RightQueue = MakeVoxelMeshGenerationQueue();
	Context->LeftQueue = MakeVoxelMeshGenerationQueue();
	Context->BackQueue = MakeVoxelMeshGenerationQueue();
	Context->FrontQueue = MakeVoxelMeshGenerationQueue();

	voxel_mesh_generation_queue* TopQueue = &Context->TopQueue;
	voxel_mesh_generation_queue* BottomQueue = &Context->BottomQueue;
	voxel_mesh_generation_queue* RightQueue = &Context->RightQueue;
	voxel_mesh_generation_queue* LeftQueue = &Context->LeftQueue;
	voxel_mesh_generation_queue* BackQueue = &Context->BackQueue;
	voxel_mesh_generation_queue* FrontQueue = &Context->FrontQueue;	

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
							PushFaceWorkToQueue(TopQueue, VoxelPos, TexSet->Sets[VoxelFaceTypeIndex_Top]);
						}

						if(DownVoxel == VoxelMaterial_None){
							PushFaceWorkToQueue(BottomQueue, VoxelPos, TexSet->Sets[VoxelFaceTypeIndex_Bottom]);
						}

						if(RightVoxel == VoxelMaterial_None){
							PushFaceWorkToQueue(RightQueue, VoxelPos, TexSet->Sets[VoxelFaceTypeIndex_Right]);
						}

						if(LeftVoxel == VoxelMaterial_None){
							PushFaceWorkToQueue(LeftQueue, VoxelPos, TexSet->Sets[VoxelFaceTypeIndex_Left]);
						}

						if(FrontVoxel == VoxelMaterial_None){
							PushFaceWorkToQueue(FrontQueue, VoxelPos, TexSet->Sets[VoxelFaceTypeIndex_Front]);
						}

						if(BackVoxel == VoxelMaterial_None){
							PushFaceWorkToQueue(BackQueue, VoxelPos, TexSet->Sets[VoxelFaceTypeIndex_Back]);
						}
					}
					else{
						int32_t IndexInNeighbourChunk = 0;

						if(i == 0){
							if(Chunk->LeftNeighbour){
								IndexInNeighbourChunk = IVAN_GET_VOXEL_INDEX(IVAN_VOXEL_CHUNK_WIDTH - 1, j, k);

								if(Chunk->LeftNeighbour->Voxels[IndexInNeighbourChunk] == VoxelMaterial_None){
									PushFaceWorkToQueue(LeftQueue, VoxelPos, TexSet->Sets[VoxelFaceTypeIndex_Left]);
								}
							}
							else{
#if IVAN_MESH_KEEP_SIDES_ON
								PushFaceWorkToQueue(LeftQueue, VoxelPos, TexSet->Sets[VoxelFaceTypeIndex_Left]);
#endif
							}
						}
						if(i == (IVAN_VOXEL_CHUNK_WIDTH - 1)){
							if(Chunk->RightNeighbour){
								IndexInNeighbourChunk = IVAN_GET_VOXEL_INDEX(0, j, k);

								if(Chunk->RightNeighbour->Voxels[IndexInNeighbourChunk] == VoxelMaterial_None){
									PushFaceWorkToQueue(RightQueue, VoxelPos, TexSet->Sets[VoxelFaceTypeIndex_Right]);
								}
							}
							else{
#if IVAN_MESH_KEEP_SIDES_ON
								PushFaceWorkToQueue(RightQueue, VoxelPos, TexSet->Sets[VoxelFaceTypeIndex_Right]);
#endif
							}
						}
						if(j == 0){
							if(Chunk->FrontNeighbour){
								IndexInNeighbourChunk = IVAN_GET_VOXEL_INDEX(i, IVAN_VOXEL_CHUNK_WIDTH - 1, k);

								if(Chunk->FrontNeighbour->Voxels[IndexInNeighbourChunk] == VoxelMaterial_None){
									PushFaceWorkToQueue(FrontQueue, VoxelPos, TexSet->Sets[VoxelFaceTypeIndex_Front]);
								}
							}
							else{
#if IVAN_MESH_KEEP_SIDES_ON
								PushFaceWorkToQueue(FrontQueue, VoxelPos, TexSet->Sets[VoxelFaceTypeIndex_Front]);
#endif
							}
						}

						if(j == (IVAN_VOXEL_CHUNK_WIDTH - 1)){
							if(Chunk->BackNeighbour){
								IndexInNeighbourChunk = IVAN_GET_VOXEL_INDEX(i, 0, k);

								if(Chunk->BackNeighbour->Voxels[IndexInNeighbourChunk] == VoxelMaterial_None){
									PushFaceWorkToQueue(BackQueue, VoxelPos, TexSet->Sets[VoxelFaceTypeIndex_Back]);
								}
							}
							else{
#if IVAN_MESH_KEEP_SIDES_ON
								PushFaceWorkToQueue(BackQueue, VoxelPos, TexSet->Sets[VoxelFaceTypeIndex_Back]);
#endif
							}
						}

#if IVAN_MESH_KEEP_SIDES_ON
						if(k == 0){
							PushFaceWorkToQueue(BottomQueue, VoxelPos, TexSet->Sets[VoxelFaceTypeIndex_Bottom]);
						}

						if(k == (IVAN_VOXEL_CHUNK_HEIGHT - 1)){
							PushFaceWorkToQueue(TopQueue, VoxelPos, TexSet->Sets[VoxelFaceTypeIndex_Top]);
						}
#endif

						PushFaceWorkForNeighbourVoxel(
							RightQueue,
							Chunk,
							i + 1, k, j, 
							TexSet->Sets[VoxelFaceTypeIndex_Right], 
							VoxelPos);

						PushFaceWorkForNeighbourVoxel(
							LeftQueue,
							Chunk,
							i - 1, k, j, 
							TexSet->Sets[VoxelFaceTypeIndex_Left], 
							VoxelPos);

						PushFaceWorkForNeighbourVoxel(
							TopQueue,
							Chunk,
							i, k + 1, j, 
							TexSet->Sets[VoxelFaceTypeIndex_Top], 
							VoxelPos);

						PushFaceWorkForNeighbourVoxel(
							BottomQueue,
							Chunk,
							i, k - 1, j, 
							TexSet->Sets[VoxelFaceTypeIndex_Bottom], 
							VoxelPos);

						PushFaceWorkForNeighbourVoxel(
							FrontQueue,
							Chunk,
							i, k, j - 1, 
							TexSet->Sets[VoxelFaceTypeIndex_Front], 
							VoxelPos);

						PushFaceWorkForNeighbourVoxel(
							BackQueue,
							Chunk,
							i, k, j + 1, 
							TexSet->Sets[VoxelFaceTypeIndex_Back], 
							VoxelPos);
					}
				}
			}
		}
	}

	uint32_t TopQueueMemoryRequired = TopQueue->EntryCount * 6 * sizeof(ivan_vertex_type);
	uint32_t BottomQueueMemoryRequired = BottomQueue->EntryCount * 6 * sizeof(ivan_vertex_type);
	uint32_t RightQueueMemoryRequired = RightQueue->EntryCount * 6 * sizeof(ivan_vertex_type);
	uint32_t LeftQueueMemoryRequired = LeftQueue->EntryCount * 6 * sizeof(ivan_vertex_type);
	uint32_t FrontQueueMemoryRequired = FrontQueue->EntryCount * 6 * sizeof(ivan_vertex_type);
	uint32_t BackQueueMemoryRequired = BackQueue->EntryCount * 6 * sizeof(ivan_vertex_type);

	Context->MemoryRequired = 
		TopQueueMemoryRequired +
		BottomQueueMemoryRequired + 
		RightQueueMemoryRequired +
		LeftQueueMemoryRequired +
		FrontQueueMemoryRequired + 
		BackQueueMemoryRequired;

	Context->Assets = Assets;

	return(Context_);
}

INTERNAL_FUNCTION void
FinalizeVoxelMeshGeneration(
	voxel_chunk_mesh* Mesh,
	voxel_mesh_generation_context* Context)
{	
	PerformWorkForVoxelMeshQueue(&Context->BottomQueue, VOXEL_MESH_QUEUE_BOTTOM, Mesh);
	PerformWorkForVoxelMeshQueue(&Context->RightQueue, VOXEL_MESH_QUEUE_RIGHT, Mesh);
	PerformWorkForVoxelMeshQueue(&Context->LeftQueue, VOXEL_MESH_QUEUE_LEFT, Mesh);
	PerformWorkForVoxelMeshQueue(&Context->FrontQueue, VOXEL_MESH_QUEUE_FRONT, Mesh);
	PerformWorkForVoxelMeshQueue(&Context->BackQueue, VOXEL_MESH_QUEUE_BACK, Mesh);
	PerformWorkForVoxelMeshQueue(&Context->TopQueue, VOXEL_MESH_QUEUE_TOP, Mesh);

	FreeVoxelMeshGenerationQueue(&Context->TopQueue);
	FreeVoxelMeshGenerationQueue(&Context->BottomQueue);
	FreeVoxelMeshGenerationQueue(&Context->RightQueue);
	FreeVoxelMeshGenerationQueue(&Context->LeftQueue);
	FreeVoxelMeshGenerationQueue(&Context->FrontQueue);
	FreeVoxelMeshGenerationQueue(&Context->BackQueue);

	Mesh->VerticesCount = Mesh->ActiveVertexIndex;

	EndGeneration(Context->Assets, Context->GenerationID);
}
