#define VOXEL_HASH_TABLE_SIZE 256

INTERNAL_FUNCTION void SpawnTree(voxel_chunk* Chunk, uint8_t HorzIndex, uint8_t VertIndex){

}

INTERNAL_FUNCTION void GenerateVoxelChunk(voxel_chunk* Chunk, random_series* Series){

	float OneOverWidth = 1.0f / (float)IVAN_VOXEL_CHUNK_WIDTH;
	float OneOverHeight = 1.0f / (float)IVAN_VOXEL_CHUNK_HEIGHT;

	float StartHeight = 120.0f;

	vec3 ChunkPos = GetPosForVoxelChunk(Chunk);

	uint8_t TerrainHeights[IVAN_VOXEL_CHUNK_LAYER_COUNT];

	//TODO(Dima): Check cache-friendly variations of this loop
	for(int j = 0; j < IVAN_VOXEL_CHUNK_WIDTH; j++){
		for(int i = 0; i < IVAN_VOXEL_CHUNK_WIDTH; i++){

			float Noise1 = stb_perlin_noise3(
				(float)(ChunkPos.x + i) / 16.0f, 
				(float)ChunkPos.y / 16.0f, 
				(float)(ChunkPos.z + j) / 16.0f, 0, 0, 0);

			float Noise2 = stb_perlin_noise3(
				(float)(ChunkPos.x + i) / 64.0f, 
				(float)ChunkPos.y / 32.0f, 
				(float)(ChunkPos.z + j) / 64.0f, 0, 0, 0);

			float Noise3 = stb_perlin_noise3(
				(float)(ChunkPos.x + i) / 256.0f, 
				(float)ChunkPos.y / 256.0f, 
				(float)(ChunkPos.z + j) / 256.0f, 0, 0, 0);

			float BiomNoise = stb_perlin_noise3(
				(float)(ChunkPos.x + i) / 256.0f, 
				(float)ChunkPos.y / 256.0f, 
				(float)(ChunkPos.z + j) / 256.0f, 0, 0, 0);

			BiomNoise += 0.5f;

			float Multiplyer = BiomNoise;
			float NoiseScale1 = 5.0f * Multiplyer;
			float NoiseScale2 = 25.0f * Multiplyer;
			float NoiseScale3 = 100.0f * Multiplyer;

			uint8_t DirtIndex = VoxelMaterial_WinterGround;
			uint8_t GroundIndex = VoxelMaterial_SnowGround;

			if(BiomNoise < 0.1f){
				DirtIndex = VoxelMaterial_Sand;
				GroundIndex = VoxelMaterial_Sand;
			}
			else if(BiomNoise >= 0.1f && BiomNoise < 0.6f){
				DirtIndex = VoxelMaterial_Ground;
				GroundIndex = VoxelMaterial_GrassyGround;
			}

			float RandHeight = (Noise1 * NoiseScale1 + Noise2 * NoiseScale2 + Noise3 * NoiseScale3) + StartHeight;
			
			uint32_t RandHeightU32 = (uint8_t)(RandHeight + 0.5f);
			TerrainHeights[j * IVAN_VOXEL_CHUNK_WIDTH + i] = RandHeightU32;

			int32_t TempFlag = 0;
			for(int32_t k = 0; k < IVAN_VOXEL_CHUNK_HEIGHT; k++){
				uint8_t* TempVoxel = &Chunk->Voxels[IVAN_GET_VOXEL_INDEX(i, j, k)];
				if(TempFlag == 0){

					if(k == RandHeightU32){
						*TempVoxel = GroundIndex;
						TempFlag = 1;					
					}
					else{
						*TempVoxel = DirtIndex;
					}
				}
				else{
					*TempVoxel = VoxelMaterial_None;
				}
			}
		}
	}

	for(int i = 0; i < ArrayCount(TerrainHeights); i++){
		uint32_t RandNum = RandomChoiceFromCount(Series, 200);

		if(RandNum == 53){
			int StartHeight = TerrainHeights[i];
			int VoxelIndex = StartHeight * IVAN_VOXEL_CHUNK_LAYER_COUNT + i;
			if(Chunk->Voxels[VoxelIndex] == VoxelMaterial_GrassyGround ||
				Chunk->Voxels[VoxelIndex] == VoxelMaterial_Ground ||
				Chunk->Voxels[VoxelIndex] == VoxelMaterial_SnowGround ||
				Chunk->Voxels[VoxelIndex] == VoxelMaterial_WinterGround)
			{
				for(int k = StartHeight; k < StartHeight + 8; k++){
					if(k < 256){
						Chunk->Voxels[k * IVAN_VOXEL_CHUNK_LAYER_COUNT + i] = VoxelMaterial_Tree;
					}
				}
			}
		}
	}
}

struct generate_chunk_work{
	task_with_memory* Task;

	voxel_chunk_header* Header;
	random_series* Series;
};

INTERNAL_FUNCTION PLATFORM_WORK_QUEUE_CALLBACK(GenerateVoxelChunkWork){
	generate_chunk_work* Work = (generate_chunk_work*)Data;

	voxel_chunk* Chunk = Work->Header->Chunk;
	voxel_chunk_mesh* Mesh = Work->Header->Mesh;

	//TODO(Dima): Load those voxels from hash-table based file
	Chunk->Voxels = PushArray(&Work->Task->Arena, IVAN_MAX_VOXELS_IN_CHUNK, uint8_t);

	GD_COMPLETE_READS_BEFORE_FUTURE;
	GenerateVoxelChunk(Chunk, Work->Series);

	GD_COMPLETE_WRITES_BEFORE_FUTURE;
	Work->Header->ChunkState = VoxelChunkState_Generated;
	Work->Header->ChunkTask = Work->Task;

	//NOTE(Dima): Task will be ended when elements gets removed from the list
}

INTERNAL_FUNCTION void GenerateVoxelChunk(
	voxel_chunk_header* Header,
	voxel_chunk_manager* Manager)
{
	
	if(AtomicCompareExchangeUInt32((uint32 volatile *)&Header->ChunkState,
            VoxelChunkState_InProcess,
            VoxelChunkState_Unloaded) == VoxelChunkState_Unloaded)
	{
		task_with_memory* Task = BeginChunkTaskWithMemory(Manager->TranState);

		generate_chunk_work Work;
		Work.Task = Task;
		Work.Header = Header;
		Work.Series = &Manager->RandomSeries;

		if(Task){
			generate_chunk_work* ChunkWork = PushStruct(&Task->Arena, generate_chunk_work);
			*ChunkWork = Work;
			Platform.AddEntry(Manager->TranState->HighPriorityQueue, GenerateVoxelChunkWork, ChunkWork);
		}
		else{
			INVALID_CODE_PATH;
		}
	}
	else{
        voxel_chunk_state volatile* State = (voxel_chunk_state volatile*)&Header->ChunkState;
        while(*State == VoxelChunkState_InProcess){

        }
	}
}

INTERNAL_FUNCTION void GenerateVoxelMesh(
	voxel_chunk_header* Header,
	voxel_chunk_manager* Manager)
{
	if(AtomicCompareExchangeUInt32((uint32 volatile*)&Header->MeshState,
		VoxelMeshState_InProcess,
		VoxelMeshState_Unloaded) == VoxelMeshState_Unloaded)
	{
		task_with_memory* Task = BeginMeshTaskWithMemory(Manager->TranState);

		generate_mesh_work Work;
		Work.Task = Task;
		Work.Assets = Manager->TranState->Assets;
		Work.Header = Header;
		Work.VoxelAtlasID = Manager->VoxelAtlasID;

		if(Task){
			generate_mesh_work* MeshWork = PushStruct(&Task->Arena, generate_mesh_work);
			*MeshWork = Work;
			Platform.AddEntry(Manager->TranState->HighPriorityQueue, GenerateVoxelMeshWork, MeshWork);
		}
		else{
			INVALID_CODE_PATH;
		}
	}
	else{
		voxel_mesh_state volatile* State = (voxel_mesh_state volatile*)&Header->MeshState;
		while(*State == VoxelMeshState_InProcess){

		}
	}
}

inline void InsertPairToVoxelHashTable(
	voxel_table_pair** Table, 
	uint32_t TableSize, 
	voxel_table_pair Pair,
	memory_arena* Arena)
{
	uint32_t HashValue = VoxelHashFunc(Pair.Key);
	uint32_t Index = HashValue % TableSize;
	voxel_table_pair* FoundPair = Table[Index];

	if(!FoundPair){
		FoundPair = PushStruct(Arena, voxel_table_pair);
		FoundPair->NextPair = 0;
		FoundPair->Key = Pair.Key;
		FoundPair->Value = Pair.Value;

		Table[Index] = FoundPair;
	}
	else{
		while(FoundPair->NextPair != 0){
			FoundPair = FoundPair->NextPair;
		}
		
		FoundPair->NextPair = PushStruct(Arena, voxel_table_pair);
		FoundPair->NextPair->NextPair = 0;
		FoundPair->NextPair->Key = Pair.Key;
		FoundPair->NextPair->Value = Pair.Value;
	}
}

inline voxel_table_pair*
FindPairInVoxelHashTable(
	voxel_table_pair** Table,
	uint32_t TableSize,
	uint64_t KeyToFind)
{
	voxel_table_pair* Result = 0;

	uint32_t HashValue = VoxelHashFunc(KeyToFind);
	uint32_t Index = HashValue % TableSize;
	voxel_table_pair* FoundPair = Table[Index];

	if(FoundPair){
		while(FoundPair != 0){
			if(FoundPair->Key == KeyToFind){
				Result = FoundPair;
				break;
			}
			FoundPair = FoundPair->NextPair;
		}
	}

	return(Result);
}

inline void SetNeighboursForChunk(
	voxel_chunk* Chunk,
	int32_t HorzIndex,
	int32_t VertIndex,
	voxel_table_pair** HashTable)
{
	uint64_t LeftKey = GenerateKeyForChunkIndices(HorzIndex - 1, VertIndex);
	uint64_t RightKey = GenerateKeyForChunkIndices(HorzIndex + 1, VertIndex);
	uint64_t FrontKey = GenerateKeyForChunkIndices(HorzIndex, VertIndex - 1);
	uint64_t BackKey = GenerateKeyForChunkIndices(HorzIndex, VertIndex + 1);

	voxel_table_pair* LeftPair = FindPairInVoxelHashTable(HashTable, VOXEL_HASH_TABLE_SIZE, LeftKey);
	voxel_table_pair* RightPair = FindPairInVoxelHashTable(HashTable, VOXEL_HASH_TABLE_SIZE, RightKey);
	voxel_table_pair* FrontPair = FindPairInVoxelHashTable(HashTable, VOXEL_HASH_TABLE_SIZE, FrontKey);
	voxel_table_pair* BackPair = FindPairInVoxelHashTable(HashTable, VOXEL_HASH_TABLE_SIZE, BackKey);

	Chunk->LeftNeighbour = 0;
	Chunk->RightNeighbour = 0;
	Chunk->FrontNeighbour = 0;
	Chunk->BackNeighbour = 0;
	
	if(LeftPair){
		Chunk->LeftNeighbour = LeftPair->Value->Chunk;
	}

	if(RightPair){
		Chunk->RightNeighbour = RightPair->Value->Chunk;
	}

	if(FrontPair){
		Chunk->FrontNeighbour = FrontPair->Value->Chunk;
	}

	if(BackPair){
		Chunk->BackNeighbour = BackPair->Value->Chunk;
	}
}

inline int32_t NeighboursWasChanged(
	voxel_chunk* New,
	voxel_chunk* OldRight,
	voxel_chunk* OldLeft,
	voxel_chunk* OldFront,
	voxel_chunk* OldBack)
{
	int32_t Result = 0;

	if((New->LeftNeighbour != OldLeft) ||
		(New->RightNeighbour != OldRight) ||
		(New->FrontNeighbour != OldFront) ||
		(New->BackNeighbour != OldBack))
	{
		Result = 1;
	}

	return(Result);	
}

#define IVAN_CHUNK_ABS(x) ((x) >= 0 ? (x) : -(x))
INTERNAL_FUNCTION void 
UpdateVoxelChunks(
	voxel_chunk_manager* Manager,
	render_group* RenderGroup,
	vec3 CamPos)
{
	TIMED_BLOCK();

	voxel_chunk_header* Index = 0;

	int32_t IndX;
	int32_t IndY;

	GetCurrChunkIndexBasedOnCamera(CamPos, &IndX, &IndY);

	if((Manager->CurrHorizontalIndex != IndX) ||
		(Manager->CurrVerticalIndex != IndY))
	{
		//NOTE(Dima): First - check for elements that are already in the list. Removing
		Index = Manager->VoxelChunkSentinel->Next;
		for(Index; !Index->IsSentinel; Index = Index->Next){
			if((IVAN_CHUNK_ABS(IndX - Index->Chunk->HorizontalIndex) > Manager->ChunksViewDistance) ||
				(IVAN_CHUNK_ABS(IndY - Index->Chunk->VerticalIndex) > Manager->ChunksViewDistance))
			{
				BeginTicketMutex(&Manager->Mutex);
				Index->Prev->Next = Index->Next;
				Index->Next->Prev = Index->Prev;

				GD_COMPLETE_WRITES_BEFORE_FUTURE;

				if(Index->ChunkTask && (Index->ChunkState != VoxelChunkState_InProcess)){
					EndTaskWithMemory(Index->ChunkTask);
				}

				if(Index->MeshTask && (Index->MeshState != VoxelMeshState_InProcess)){
					EndTaskWithMemory(Index->MeshTask);
				}

				EndTicketMutex(&Manager->Mutex);
			}
		}
	}

	temporary_memory TempMem = BeginTemporaryMemory(&Manager->HashTableArena);

	voxel_table_pair** HashTable = PushArray(TempMem.Arena, VOXEL_HASH_TABLE_SIZE, voxel_table_pair*);
	
	for(int32_t ClearIndex = 0;
		ClearIndex < VOXEL_HASH_TABLE_SIZE;
		ClearIndex++)
	{
		HashTable[ClearIndex] = 0;
	}

	Index = Manager->VoxelChunkSentinel->Prev;
	for(Index; !Index->IsSentinel; Index = Index->Prev){
		
		voxel_table_pair NewPair = {};
		NewPair.Key = GenerateKeyForChunkIndices(
			Index->Chunk->HorizontalIndex, 
			Index->Chunk->VerticalIndex);
		NewPair.NextPair = 0;
		NewPair.Value = Index;

		InsertPairToVoxelHashTable(
			HashTable, 
			VOXEL_HASH_TABLE_SIZE,
			NewPair,
			TempMem.Arena);
	}

	//NOTE(Dima): Second - check for elements that we are moving towards. Adding
	int32_t MinX = IndX - Manager->ChunksViewDistance;
	int32_t MaxX = IndX + Manager->ChunksViewDistance;
	int32_t MinY = IndY - Manager->ChunksViewDistance;
	int32_t MaxY = IndY + Manager->ChunksViewDistance;

	int32_t NewListInserts = 0;
	for(int VertIndex = MinY; VertIndex <= MaxY; VertIndex++){
		for(int HorzIndex = MinX; HorzIndex <= MaxX; HorzIndex++){
			
			uint64_t KeyToFind = GenerateKeyForChunkIndices(HorzIndex, VertIndex);
			voxel_table_pair* FoundPair = FindPairInVoxelHashTable(
				HashTable, 
				VOXEL_HASH_TABLE_SIZE, 
				KeyToFind);

			if(!FoundPair){
				voxel_chunk_header* NewHeader = PushStruct(&Manager->ListArena, voxel_chunk_header);
				NewHeader->IsSentinel = false;
				NewHeader->Chunk = PushStruct(&Manager->ListArena, voxel_chunk);
				NewHeader->Mesh = PushStruct(&Manager->ListArena, voxel_chunk_mesh);
				NewHeader->Chunk->HorizontalIndex = HorzIndex;
				NewHeader->Chunk->VerticalIndex = VertIndex;
				NewHeader->NeedsToBeGenerated = true;
				
				GenerateVoxelChunk(NewHeader, Manager);

				BeginTicketMutex(&Manager->Mutex);
				voxel_table_pair NewPair = {};
				NewPair.Key = KeyToFind;
				NewPair.Value = NewHeader;
				InsertPairToVoxelHashTable(HashTable, VOXEL_HASH_TABLE_SIZE, NewPair, TempMem.Arena);
				InsertChunkHeaderAtFront(Manager, NewHeader);
				EndTicketMutex(&Manager->Mutex);
			}
		}
	}

	Index = Manager->VoxelChunkSentinel->Next;
	for(Index; !Index->IsSentinel; Index = Index->Next){
		
		if(Index->ChunkState == VoxelChunkState_Generated){			
			voxel_chunk* Chunk = Index->Chunk;
			voxel_chunk* OldLeft = Chunk->LeftNeighbour;
			voxel_chunk* OldRight = Chunk->RightNeighbour;
			voxel_chunk* OldFront = Chunk->FrontNeighbour;
			voxel_chunk* OldBack = Chunk->BackNeighbour;

			GD_COMPLETE_READS_BEFORE_FUTURE;

			SetNeighboursForChunk(
				Chunk, 
				Chunk->HorizontalIndex,
				Chunk->VerticalIndex,
				HashTable);

			GD_COMPLETE_WRITES_BEFORE_FUTURE;

			if(NeighboursWasChanged(Chunk, OldRight, OldLeft, OldFront, OldBack)){

				if(Index->MeshTask && Index->MeshState != VoxelMeshState_InProcess){
					GD_COMPLETE_READS_BEFORE_FUTURE;
					Index->MeshState = VoxelMeshState_Unloaded;

					EndTaskWithMemory(Index->MeshTask);
				}

				GenerateVoxelMesh(Index, Manager);
			}

			if(Index->MeshState == VoxelMeshState_Generated){
			    PushVoxelChunkMesh(
			    	RenderGroup, 
			    	Index->Mesh,
		        	Manager->VoxelAtlasID, 
		        	GetPosForVoxelChunk(Index->Chunk));
			}
		}
	}
	
#if 1
	size_t MemoryToClearSize = Manager->HashTableArena.MemoryUsed - TempMem.Used;
	ZeroSize(HashTable, MemoryToClearSize);
#endif

	EndTemporaryMemory(TempMem);

	Manager->CurrHorizontalIndex = IndX;
	Manager->CurrVerticalIndex = IndY;
}

INTERNAL_FUNCTION voxel_chunk_manager* 
AllocateVoxelChunkManager(transient_state* TranState, game_assets* Assets)
{
	voxel_chunk_manager* Result = PushStruct(&TranState->TranArena, voxel_chunk_manager);
    
    Result->TranState = TranState;
    Result->Mutex = {};

    asset_vector VAMatchVector = {};
    VAMatchVector.Data[Tag_VoxelAtlasType] = VoxelAtlasType_Default;
    asset_vector VAWeightVector = {};
    VAWeightVector.Data[Tag_VoxelAtlasType] = 10.0f;
    Result->VoxelAtlasID = GetBestMatchVoxelAtlasFrom(Assets, Asset_VoxelAtlas, &VAMatchVector, &VAWeightVector);

    Result->VoxelChunkSentinel = PushStruct(&TranState->TranArena, voxel_chunk_header);
    Result->VoxelChunkSentinel->Next = Result->VoxelChunkSentinel;
    Result->VoxelChunkSentinel->Prev = Result->VoxelChunkSentinel;
    Result->VoxelChunkSentinel->IsSentinel = true;

    Result->RandomSeries = RandomSeed(123);

    Result->ChunksViewDistance = 7;
    Result->CurrHorizontalIndex = 0;
    Result->CurrVerticalIndex = 0;

	SubArena(&Result->HashTableArena, &TranState->TranArena, GD_KILOBYTES(500));
	SubArena(&Result->ListArena, &TranState->TranArena, GD_MEGABYTES(5));

	return(Result);
}