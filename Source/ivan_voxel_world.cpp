#define IVAN_VOXEL_HASH_TABLE_SIZE 256

INTERNAL_FUNCTION void GenerateVoxelChunk(
	voxel_chunk* Chunk)
{
	float OneOverWidth = 1.0f / (float)IVAN_VOXEL_CHUNK_WIDTH;
	float OneOverHeight = 1.0f / (float)IVAN_VOXEL_CHUNK_HEIGHT;

	float StartHeight = 30.0f;

	vec3 ChunkPos = GetPosForVoxelChunk(Chunk);

	//TODO(Dima): Check cache-friendly variations of this loop
	for(int j = 0; j < IVAN_VOXEL_CHUNK_WIDTH; j++){
		for(int i = 0; i < IVAN_VOXEL_CHUNK_WIDTH; i++){
			float RandHeight = stb_perlin_noise3(
				(float)(ChunkPos.x + i) / 16.0f, 
				(float)ChunkPos.y / 16.0f, 
				(float)(ChunkPos.z + j) / 16.0f, 0, 0, 0) * 15.0f + StartHeight;
			uint32_t RandHeightU32 = (uint32_t)(RandHeight + 0.5f);

			//IMPORTANT(Dima): Do not change IsAir sence because of this
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
}

struct generate_chunk_work{
	task_with_memory* Task;

	game_assets* Assets;

	voxel_chunk_header* Header;

	voxel_atlas_id VoxelAtlasID;
};

INTERNAL_FUNCTION PLATFORM_WORK_QUEUE_CALLBACK(GenerateVoxelChunkWork){
	generate_chunk_work* Work = (generate_chunk_work*)Data;

	voxel_chunk* Chunk = Work->Header->Chunk;
	voxel_chunk_mesh* Mesh = Work->Header->Mesh;

	//TODO(Dima): Load those voxels from hash-table based file
	Chunk->Voxels = PushArray(&Work->Task->Arena, IVAN_MAX_VOXELS_IN_CHUNK, voxel);
    Mesh->PUVN = PushArray(&Work->Task->Arena, (IVAN_MAX_MESH_CHUNK_FACE_COUNT * 6) >> 1, uint32_t);

    GD_COMPLETE_READS_BEFORE_FUTURE;
    
	GenerateVoxelChunk(Chunk);
    GenerateVoxelMeshForChunk(Mesh, Chunk, Work->Assets, Work->VoxelAtlasID);

    GD_COMPLETE_WRITES_BEFORE_FUTURE;
    Work->Header->State = VoxelChunkState_Generated;
    Work->Header->Task = Work->Task;

    //NOTE(Dima): Task will be ended when elements gets removed from the list
}

INTERNAL_FUNCTION void GenerateVoxelChunk(
	voxel_chunk_header* Header,
	voxel_chunk_manager* Manager)
{
	
	if(AtomicCompareExchangeUInt32((uint32 volatile *)&Header->State,
            VoxelChunkState_InProcess,
            VoxelChunkState_Unloaded) == VoxelChunkState_Unloaded)
	{
		task_with_memory* Task = BeginTaskWithMemory(Manager->TranState, false);

		generate_chunk_work Work;
		Work.Task = Task;
		Work.Assets = Manager->TranState->Assets;
		Work.Header = Header;
		Work.VoxelAtlasID = Manager->VoxelAtlasID;

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
        voxel_chunk_state volatile* State = (voxel_chunk_state volatile*)&Header->State;
        while(*State == VoxelChunkState_InProcess){

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


#define IVAN_CHUNK_ABS(x) ((x) >= 0 ? (x) : -(x))
INTERNAL_FUNCTION void 
UpdateVoxelChunks(
	voxel_chunk_manager* Manager,
	render_group* RenderGroup,
	vec3 CamPos)
{
	voxel_chunk_header* Index = 0;

	int32_t IndX;
	int32_t IndY;

	GetCurrChunkIndexBasedOnCamera(CamPos, &IndX, &IndY);

	int32_t DeletesCount = 0;
	if((Manager->CurrHorizontalIndex != IndX) ||
		(Manager->CurrVerticalIndex != IndY))
	{
		//NOTE(Dima): First - check for elements that are already in the list. Removing
		Index = Manager->VoxelChunkSentinel->Next;
		for(Index; !Index->IsSentinel; Index = Index->Next){
#if 1
			if((IVAN_CHUNK_ABS(IndX - Index->Chunk->HorizontalIndex) > Manager->ChunksViewDistance) ||
				(IVAN_CHUNK_ABS(IndY - Index->Chunk->VerticalIndex) > Manager->ChunksViewDistance))
			{
				BeginTicketMutex(&Manager->Mutex);
				Index->Prev->Next = Index->Next;
				Index->Next->Prev = Index->Prev;

				GD_COMPLETE_WRITES_BEFORE_FUTURE;

				if(Index->Task && (Index->State != VoxelChunkState_InProcess)){
					EndTaskWithMemory(Index->Task);
				}
				EndTicketMutex(&Manager->Mutex);

				DeletesCount++;
			}
#endif
		}
	}

	temporary_memory TempMem = BeginTemporaryMemory(&Manager->HashTableArena);

	voxel_table_pair** HashTable = PushArray(TempMem.Arena, IVAN_VOXEL_HASH_TABLE_SIZE, voxel_table_pair*);
	
	for(int32_t ClearIndex = 0;
		ClearIndex < IVAN_VOXEL_HASH_TABLE_SIZE;
		ClearIndex++)
	{
		HashTable[ClearIndex] = 0;
	}

	int32_t HashTableInserts = 0;
	Index = Manager->VoxelChunkSentinel->Prev;
	for(Index; !Index->IsSentinel; Index = Index->Prev){
		
		voxel_table_pair NewPair = {};
		NewPair.Key = GenerateKeyForChunkIndices(
			Index->Chunk->HorizontalIndex, 
			Index->Chunk->VerticalIndex);
		NewPair.Value = Index;
		NewPair.NextPair = 0;

		InsertPairToVoxelHashTable(
			HashTable, 
			IVAN_VOXEL_HASH_TABLE_SIZE,
			NewPair,
			TempMem.Arena);

		HashTableInserts++;
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
				IVAN_VOXEL_HASH_TABLE_SIZE, 
				KeyToFind);

			if(!FoundPair){
				/*
					NOTE(Dima): 
						1) Here we use our TranArena instead of temporary table arena.
						2) Adding element to list is here
				*/
				NewListInserts++;

				//BUG(Dima): Memory allocates from the beginning
				voxel_chunk_header* NewHeader = PushStruct(&Manager->ListArena, voxel_chunk_header);
				NewHeader->Chunk = PushStruct(&Manager->ListArena, voxel_chunk);
				NewHeader->Mesh = PushStruct(&Manager->ListArena, voxel_chunk_mesh);
				NewHeader->Chunk->HorizontalIndex = HorzIndex;
				NewHeader->Chunk->VerticalIndex = VertIndex;
				NewHeader->IsSentinel = false;
				NewHeader->TempNumber = Manager->TempLstCount++;

				//GD_COMPLETE_READS_BEFORE_FUTURE;
				GenerateVoxelChunk(NewHeader, Manager);
				GD_COMPLETE_WRITES_BEFORE_FUTURE;

				//NOTE(Dima): Maybe insert to table too???	
				BeginTicketMutex(&Manager->Mutex);
				InsertChunkHeaderAtFront(Manager, NewHeader);
				EndTicketMutex(&Manager->Mutex);
			}
		}
	}
	
#if 1
	size_t MemoryToClearSize = Manager->HashTableArena.MemoryUsed - TempMem.Used;
	ZeroSize(HashTable, MemoryToClearSize);
#endif

	EndTemporaryMemory(TempMem);

	int32_t RenderCalls = 0;
	Index = Manager->VoxelChunkSentinel->Next;
	for(Index; !Index->IsSentinel; Index = Index->Next){

		if(Index->State == VoxelChunkState_Generated)
		{
		    PushVoxelChunkMesh(
		    	RenderGroup, 
		    	Index->Mesh,
	        	Manager->VoxelAtlasID, 
	        	GetPosForVoxelChunk(Index->Chunk));
	    	
	    	RenderCalls++;
		}

	}

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
    VAMatchVector.Data[Tag_VoxelAtlasType] = VoxelAtlasType_Minecraft;
    asset_vector VAWeightVector = {};
    VAWeightVector.Data[Tag_VoxelAtlasType] = 10.0f;
    Result->VoxelAtlasID = GetBestMatchVoxelAtlasFrom(Assets, Asset_VoxelAtlas, &VAMatchVector, &VAWeightVector);

    Result->VoxelChunkSentinel = PushStruct(&TranState->TranArena, voxel_chunk_header);
    Result->VoxelChunkSentinel->Next = Result->VoxelChunkSentinel;
    Result->VoxelChunkSentinel->Prev = Result->VoxelChunkSentinel;
    Result->VoxelChunkSentinel->IsSentinel = true;
   	Result->VoxelChunkSentinel->TempNumber = -1;

    Result->ChunksViewDistance = 3;
    Result->CurrHorizontalIndex = 0;
    Result->CurrVerticalIndex = 0;
    Result->TempLstCount = 0;

	SubArena(&Result->HashTableArena, &TranState->TranArena, GD_KILOBYTES(500));
	SubArena(&Result->ListArena, &TranState->TranArena, GD_MEGABYTES(5));

	return(Result);
}