#define VOXEL_HASH_TABLE_SIZE 1024

#define IVAN_VOXEL_GENERATOR_BIOMS 1
#define IVAN_VOXEL_GENERATOR_TREES 1
#define IVAN_VOXEL_WORLD_FRUSTUM_CULLING 1

INTERNAL_FUNCTION void BuildColumnForChunk(
	voxel_chunk* Chunk, 
	int32 LocalXIndex, 
	int32 LocalZIndex, 
	uint32 StartHeight, 
	uint32 Height,
	uint8 Material)
{
	if((LocalXIndex >= 0 && LocalXIndex < IVAN_VOXEL_CHUNK_WIDTH) &&
		(LocalZIndex >= 0 && LocalZIndex < IVAN_VOXEL_CHUNK_WIDTH))
	{
		for(int k = StartHeight; k < StartHeight + Height; k++){
			if(k < IVAN_VOXEL_CHUNK_HEIGHT){
				Chunk->Voxels[k * IVAN_VOXEL_CHUNK_LAYER_COUNT + LocalZIndex * IVAN_VOXEL_CHUNK_WIDTH + LocalXIndex] = Material;
			}
		}
	}
	else{
#if 0
		Assert(LocalXIndex >= -16 && LocalXIndex < 32);
		Assert(LocalZIndex >= -16 && LocalZIndex < 32);

		int32 NeededLocalX = 0;
		int32 NeededLocalZ = 0;

		voxel_chunk* NeededChunk = Chunk;

		if(LocalXIndex < 0 && Chunk->LeftNeighbour){
			NeededLocalX = LocalXIndex + 16;
			NeededChunk = NeededChunk->LeftNeighbour;
		}
		else if(LocalXIndex >= 16 && Chunk->RightNeighbour){
			NeededLocalX = LocalXIndex - 16;
			NeededChunk = NeededChunk->RightNeighbour;
		}

		if(LocalZIndex < 0 && Chunk->FrontNeighbour){
			NeededLocalZ = LocalZIndex + 16;
			NeededChunk = NeededChunk->FrontNeighbour;
		}else if(LocalZIndex >= 16 && Chunk->BackNeighbour){
			NeededLocalZ = LocalZIndex - 16;
			NeededChunk = NeededChunk->BackNeighbour;
		}

#if 1
		BuildColumnForChunk(NeededChunk, NeededLocalX, NeededLocalZ, StartHeight, Height, Material);
#else
		for(int k = StartHeight; k < StartHeight + Height; k++){
			if(k < IVAN_VOXEL_CHUNK_HEIGHT){
				Chunk->NeededChunk[k * IVAN_VOXEL_CHUNK_LAYER_COUNT + NeededLocalZ * IVAN_VOXEL_CHUNK_WIDTH + NeededLocalX] = Material;
			}
		}
#endif
#endif
	}
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

			float NoiseScale1 = 3.0f;
			float NoiseScale2 = 25.0f;
			float NoiseScale3 = 160.0f;

			uint8_t DirtIndex = VoxelMaterial_Ground;
			uint8_t GroundIndex = VoxelMaterial_GrassyGround;

			float Noise1 = stb_perlin_noise3(
				(float)(ChunkPos.x + i) / 64.0f, 
				(float)ChunkPos.y / 64.0f, 
				(float)(ChunkPos.z + j) / 64.0f, 0, 0, 0);

			float Noise2 = stb_perlin_noise3(
				(float)(ChunkPos.x + i) / 128.0f, 
				(float)ChunkPos.y / 128.0f, 
				(float)(ChunkPos.z + j) / 128.0f, 0, 0, 0);

			float Noise3 = stb_perlin_noise3(
				(float)(ChunkPos.x + i) / 256.0f, 
				(float)ChunkPos.y / 256.0f, 
				(float)(ChunkPos.z + j) / 256.0f, 0, 0, 0);

#if IVAN_VOXEL_GENERATOR_BIOMS
			float BiomNoise = stb_perlin_noise3(
				(float)(ChunkPos.x + i) / 256.0f, 
				(float)ChunkPos.y / 256.0f, 
				(float)(ChunkPos.z + j) / 256.0f, 0, 0, 0);

			BiomNoise += 0.5f;

			float Multiplyer = BiomNoise;
			NoiseScale1 *= Multiplyer;
			NoiseScale2 *= Multiplyer;
			NoiseScale3 *= Multiplyer;


			if(BiomNoise < 0.1f){
				DirtIndex = VoxelMaterial_Sand;
				GroundIndex = VoxelMaterial_Sand;
			}
			else if(BiomNoise >= 0.1f && BiomNoise < 0.9f){
#if 0
				DirtIndex = VoxelMaterial_Ground;
				GroundIndex = VoxelMaterial_GrassyGround;
#else
				DirtIndex = VoxelMaterial_Ground;
				GroundIndex = VoxelMaterial_GrassyGround;
#endif
			}
			else{
				DirtIndex = VoxelMaterial_WinterGround;
				GroundIndex = VoxelMaterial_SnowGround;
			}
#endif

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

#if IVAN_VOXEL_GENERATOR_TREES
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
#if 0
				for(int k = StartHeight; k < StartHeight + 8; k++){
					if(k < 256){
						Chunk->Voxels[k * IVAN_VOXEL_CHUNK_LAYER_COUNT + i] = VoxelMaterial_Tree;
					}
				}
#else
				int32 XIndex = i & (IVAN_VOXEL_CHUNK_WIDTH - 1);
				int32 YIndex = i / IVAN_VOXEL_CHUNK_WIDTH;

				BuildColumnForChunk(Chunk, 
					XIndex, 
					YIndex,
					StartHeight,
					8,
					VoxelMaterial_Tree);

				BuildColumnForChunk(Chunk,
					XIndex,
					YIndex,
					StartHeight + 8,
					2,
					VoxelMaterial_Leaves);

				BuildColumnForChunk(Chunk,
					XIndex + 1,
					YIndex,
					StartHeight + 4,
					6,
					VoxelMaterial_Leaves);

				BuildColumnForChunk(Chunk,
					XIndex - 1,
					YIndex,
					StartHeight + 4,
					6,
					VoxelMaterial_Leaves);

				BuildColumnForChunk(Chunk,
					XIndex,
					YIndex + 1,
					StartHeight + 4,
					6,
					VoxelMaterial_Leaves);

				BuildColumnForChunk(Chunk,
					XIndex,
					YIndex - 1,
					StartHeight + 4,
					6,
					VoxelMaterial_Leaves);

				BuildColumnForChunk(Chunk,
					XIndex + 1,
					YIndex + 1,
					StartHeight + 4,
					6,
					VoxelMaterial_Leaves);

				BuildColumnForChunk(Chunk,
					XIndex + 1,
					YIndex - 1,
					StartHeight + 4,
					6,
					VoxelMaterial_Leaves);

				BuildColumnForChunk(Chunk,
					XIndex - 1,
					YIndex - 1,
					StartHeight + 4,
					6,
					VoxelMaterial_Leaves);

				BuildColumnForChunk(Chunk,
					XIndex - 1,
					YIndex + 1,
					StartHeight + 4,
					6,
					VoxelMaterial_Leaves);
#endif
			}
		}
	}
#endif
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

	//*Chunk = {};
	Chunk->Voxels = 0;
	Chunk->VoxelsCount = 0;
	Chunk->LeftNeighbour = 0;
	Chunk->RightNeighbour = 0;
	Chunk->FrontNeighbour = 0;
	Chunk->BackNeighbour = 0;

	//TODO(Dima): Load those voxels from hash-table based file
	Chunk->Voxels = PushArray(&Work->Task->Arena, IVAN_MAX_VOXELS_IN_CHUNK, uint8_t);
	Assert(Chunk->Voxels);

	IVAN_COMPLETE_READS_BEFORE_FUTURE;
	GenerateVoxelChunk(Chunk, Work->Series);

	IVAN_COMPLETE_WRITES_BEFORE_FUTURE;
	Work->Header->ChunkState = VoxelChunkState_Generated;
	Work->Header->ChunkTask = Work->Task;

	//NOTE(Dima): Task will be ended when elements gets removed from the list
}

inline void InsertPairToVoxelHashTable(
	voxel_table_pair** Table, 
	uint32_t TableSize,
	voxel_table_pair Pair)
{
	uint32_t HashValue = VoxelHashFunc(Pair.Key);
	uint32_t Index = HashValue % TableSize;
	voxel_table_pair* FoundPair = Table[Index];

	if(!FoundPair){
		FoundPair = (voxel_table_pair*)malloc(sizeof(voxel_table_pair));
		FoundPair->NextPair = 0;
		FoundPair->Key = Pair.Key;
		FoundPair->Value = Pair.Value;

		Table[Index] = FoundPair;
	}
	else{
		while(FoundPair->NextPair != 0){
			FoundPair = FoundPair->NextPair;
		}
		
		FoundPair->NextPair = (voxel_table_pair*)malloc(sizeof(voxel_table_pair));
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

INTERNAL_FUNCTION void ClearVoxelHashTable(
	voxel_table_pair** Table,
	uint32_t TableSize)
{
	for(int32_t TableIt = 0;
		TableIt < TableSize;
		TableIt++)
	{
		Table[TableIt] = 0;
	}
}

INTERNAL_FUNCTION void FreeVoxelHashTable(
	voxel_table_pair** Table,
	uint32_t TableSize)
{
	for(int32_t TableIt = 0;
		TableIt < TableSize;
		TableIt++)
	{
		voxel_table_pair* Pair = Table[TableIt];
		while(Pair != 0){
			voxel_table_pair* NextPair = Pair->NextPair;
			free(Pair);
			Pair = NextPair;
		}
		Table[TableIt] = 0;
	}
}

INTERNAL_FUNCTION voxel_chunk* GetChunkAtIndices(voxel_table_pair** Table, int32_t HorizontalIndex, int32_t VerticalIndex){
	voxel_chunk* Result = 0;
	uint64_t Key = GenerateKeyForChunkIndices(HorizontalIndex, VerticalIndex);

	voxel_table_pair* Pair = FindPairInVoxelHashTable(Table, VOXEL_HASH_TABLE_SIZE, Key);

	if(Pair){
		if(Pair->Value->ChunkState == VoxelChunkState_Generated){
			Result = Pair->Value->Chunk;
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
	
	if(LeftPair && LeftPair->Value->ChunkState == VoxelChunkState_Generated){
		Chunk->LeftNeighbour = LeftPair->Value->Chunk;
		Assert(Chunk->LeftNeighbour->Voxels);
	}

	if(RightPair && RightPair->Value->ChunkState == VoxelChunkState_Generated){
		Chunk->RightNeighbour = RightPair->Value->Chunk;
		Assert(Chunk->RightNeighbour->Voxels);
	}

	if(FrontPair && FrontPair->Value->ChunkState == VoxelChunkState_Generated){
		Chunk->FrontNeighbour = FrontPair->Value->Chunk;
		Assert(Chunk->FrontNeighbour->Voxels);
	}

	if(BackPair && BackPair->Value->ChunkState == VoxelChunkState_Generated){
		Chunk->BackNeighbour = BackPair->Value->Chunk;
		Assert(Chunk->BackNeighbour->Voxels);
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

INTERNAL_FUNCTION void GenerateVoxelChunk(
	voxel_chunk_header* Header,
	voxel_chunk_manager* Manager,
	voxel_table_pair** Table)
{
	if(AtomicCompareExchangeU32((uint32 volatile *)&Header->ChunkState,
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

			voxel_chunk* Chunk = PushStruct(&Task->Arena, voxel_chunk);
			Work.Header->Chunk = Chunk;
			Chunk->HorizontalIndex = Header->HorzIndex;
			Chunk->VerticalIndex = Header->VertIndex;
			*ChunkWork = Work;

			Work.Header->Chunk->LeftChunk = GetChunkAtIndices(Manager->Tables[Manager->TableIndex], Chunk->HorizontalIndex - 1, Chunk->VerticalIndex);
			Work.Header->Chunk->RightChunk = GetChunkAtIndices(Manager->Tables[Manager->TableIndex], Chunk->HorizontalIndex + 1, Chunk->VerticalIndex);
			Work.Header->Chunk->FrontChunk = GetChunkAtIndices(Manager->Tables[Manager->TableIndex], Chunk->HorizontalIndex, Chunk->VerticalIndex - 1);
			Work.Header->Chunk->BackChunk = GetChunkAtIndices(Manager->Tables[Manager->TableIndex], Chunk->HorizontalIndex, Chunk->VerticalIndex + 1);

#if 0
			Platform.AddEntry(Manager->TranState->HighPriorityQueue, GenerateVoxelChunkWork, ChunkWork);
#else
			Chunk->Voxels = 0;
			Chunk->VoxelsCount = 0;

#if 1
			Chunk->LeftNeighbour = 0;
			Chunk->RightNeighbour = 0;
			Chunk->FrontNeighbour = 0;
			Chunk->BackNeighbour = 0;

			SetNeighboursForChunk(
				Chunk, 
				Chunk->HorizontalIndex,
				Chunk->VerticalIndex,
				Table);
#endif

			Chunk->Voxels = PushArray(&Task->Arena, IVAN_MAX_VOXELS_IN_CHUNK, uint8_t);

			Assert(Chunk->Voxels);

			GenerateVoxelChunk(Chunk, &Manager->RandomSeries);

			IVAN_COMPLETE_WRITES_BEFORE_FUTURE;
			Header->ChunkState = VoxelChunkState_Generated;
			Header->ChunkTask = Task;
#endif
		}
		else{
			//Not enough tasks. High usage
			INVALID_CODE_PATH;
		}
	}
	else{

	}
}

INTERNAL_FUNCTION void GenerateVoxelMesh(
	voxel_chunk_header* Header,
	voxel_chunk_manager* Manager)
{
	if(AtomicCompareExchangeU32((uint32 volatile*)&Header->MeshState,
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
			voxel_chunk_mesh* Mesh = PushStruct(&Task->Arena, voxel_chunk_mesh);
			Work.Header->Mesh = Mesh;
			generate_mesh_work* MeshWork = PushStruct(&Task->Arena, generate_mesh_work);
			*MeshWork = Work;

#if 1
			Platform.AddEntry(Manager->TranState->HighPriorityQueue, GenerateVoxelMeshWork, MeshWork);
#else
			*Header->Mesh = {};

			voxel_mesh_generation_context Context = InitVoxelMeshGeneration(Header->Chunk, Manager->TranState->Assets, Manager->VoxelAtlasID);
			Header->Mesh->PUVN = (ivan_vertex_type*)Platform.AllocateMemory(Context.MemoryRequired);
			FinalizeVoxelMeshGeneration(Header->Mesh, &Context);

			IVAN_COMPLETE_WRITES_BEFORE_FUTURE;
			Header->MeshTask = Task;
			Header->MeshState = VoxelMeshState_Generated;
#endif
		}
		else{
			//Not enough tasks. High usage
			INVALID_CODE_PATH;
		}
	}
	else{

	}
}

//NOTE(DIMA): Chunk size dependent
inline int32_t TestChunkOnPlane(vec4 Pl, voxel_chunk* Chunk){
	int32_t Result = 0;

	float TestRadius = IVAN_VOXEL_CHUNK_WIDTH * 0.5f * IVAN_MATH_SQRT_TWO;

	vec3 ChunkP = GetPosForVoxelChunk(Chunk);

	for(int TestPosIndex = 0;
		TestPosIndex < 16;
		TestPosIndex++)
	{
		vec3 TestPos;
		TestPos.x = ChunkP.x + 8.0f;
		TestPos.y = ChunkP.y + (float)(TestPosIndex * 16 + 8);
		TestPos.z = ChunkP.z + 8.0f;
	
		float TestValue = Pl.A * TestPos.x + Pl.B * TestPos.y + Pl.C * TestPos.z + Pl.D + TestRadius;

		if(TestValue >= 0.0f){
			Result = true;
			break;
		}
	}

	return(Result);
}

#define IVAN_CHUNK_ABS(x) ((x) >= 0 ? (x) : -(x))

struct update_voxel_chunk_work{
	int32_t MinXOffset;
	int32_t MaxXOffset;
	int32_t MinYOffset;
	int32_t MaxYOffset;

	int32_t CamIndexX;
	int32_t CamIndexY;

	int32_t PrevCamIndexX;
	int32_t PrevCamIndexY;

	voxel_chunk_thread_context* Context;

	voxel_table_pair** ReadTable;

	voxel_chunk_manager* Manager;

	render_group* RenderGroup;
};

INTERNAL_FUNCTION PLATFORM_WORK_QUEUE_CALLBACK(UpdateVoxelChunkWork){
	TIMED_FUNCTION();

	update_voxel_chunk_work* Work = (update_voxel_chunk_work*)Data;

	voxel_chunk_thread_context* Context = Work->Context;

	voxel_chunk_header* Index = 0;

	if((Work->PrevCamIndexX != Work->CamIndexX) ||
		(Work->PrevCamIndexY != Work->CamIndexY))
	{
		//NOTE(Dima): First - check for elements that are already in the list. Removing
		Index = Context->VoxelChunkSentinel->Next;
		for(Index; !Index->IsSentinel;){
			
			voxel_chunk_header* NextForIndex = Index->Next;
			
			if((IVAN_CHUNK_ABS(Work->CamIndexX - Index->Chunk->HorizontalIndex) > Context->Manager->ChunksViewDistance) ||
				(IVAN_CHUNK_ABS(Work->CamIndexY - Index->Chunk->VerticalIndex) > Context->Manager->ChunksViewDistance))
			{

				if(Index->ChunkTask && (Index->ChunkState == VoxelChunkState_Generated)){
					EndTaskWithMemory(Index->ChunkTask);

					AtomicCompareExchangeU32(
						(uint32 volatile *)&Index->ChunkState,
        				VoxelChunkState_Unloaded,
        				VoxelChunkState_Generated);
				}

				if(Index->MeshTask && (Index->MeshState == VoxelMeshState_Generated)){
					Platform.DeallocateMemory(Index->Mesh->PUVN);
					EndTaskWithMemory(Index->MeshTask);

					AtomicCompareExchangeU32(
						(uint32 volatile *)&Index->MeshState,
        				VoxelMeshState_Unloaded,
        				VoxelMeshState_Generated);
				}

				if(Index->MeshState == VoxelMeshState_Unloaded &&
					Index->ChunkState == VoxelChunkState_Unloaded)
				{
					Index->Prev->Next = Index->Next;
					Index->Next->Prev = Index->Prev;
					
					Index->Next = Context->FirstFreeSentinel->Next;
					Index->Prev = Context->FirstFreeSentinel;
					Index->Next->Prev = Index;
					Index->Prev->Next = Index;
				}
			}
			
			Index = NextForIndex;
		}
	}

	//NOTE(Dima): Second - check for elements that we are moving towards. Adding
	int32_t MinX = Work->CamIndexX + Work->MinXOffset;
	int32_t MaxX = Work->CamIndexX + Work->MaxXOffset;
	int32_t MinY = Work->CamIndexY + Work->MinYOffset;
	int32_t MaxY = Work->CamIndexY + Work->MaxYOffset;

	int32_t NewListInserts = 0;
	for(int VertIndex = MinY; VertIndex <= MaxY; VertIndex++){
		for(int HorzIndex = MinX; HorzIndex <= MaxX; HorzIndex++){
			
			uint64_t KeyToFind = GenerateKeyForChunkIndices(HorzIndex, VertIndex);
			voxel_table_pair* FoundPair = FindPairInVoxelHashTable(
				Work->ReadTable, 
				VOXEL_HASH_TABLE_SIZE, 
				KeyToFind);

			if(!FoundPair){
				voxel_chunk_header* NewHeader;

				if(!Context->FirstFreeSentinel->Next->IsSentinel){
					NewHeader = Context->FirstFreeSentinel->Next;

					NewHeader->Next->Prev = NewHeader->Prev;
					NewHeader->Prev->Next = NewHeader->Next;
				}
				else{
					NewHeader = PushStruct(&Context->ListArena, voxel_chunk_header);
				}

				NewHeader->IsSentinel = false;
				NewHeader->HorzIndex = HorzIndex;
				NewHeader->VertIndex = VertIndex;
				
				InsertChunkHeaderAtFront(Context->VoxelChunkSentinel, NewHeader);
				
				//Think about writing to temp table...
				//InsertPairToVoxelHashTable(TempTable, VOXEL_HASH_TABLE_SIZE, NewPair);
			}
		}
	}

	int ChunksTotal = 0;
	int ChunksInFrustrum = 0;
	Index = Context->VoxelChunkSentinel->Next;
	for(Index; !Index->IsSentinel; Index = Index->Next){
		GenerateVoxelChunk(Index, Context->Manager, Work->ReadTable);
		
		if(Index->ChunkState == VoxelChunkState_Generated){
			voxel_chunk* Chunk = Index->Chunk;
			voxel_chunk* OldLeft = Chunk->LeftNeighbour;
			voxel_chunk* OldRight = Chunk->RightNeighbour;
			voxel_chunk* OldFront = Chunk->FrontNeighbour;
			voxel_chunk* OldBack = Chunk->BackNeighbour;

			SetNeighboursForChunk(
				Chunk, 
				Chunk->HorizontalIndex,
				Chunk->VerticalIndex,
				Work->ReadTable);

			if(NeighboursWasChanged(Chunk, OldRight, OldLeft, OldFront, OldBack)){

				if(Index->MeshTask && Index->MeshState == VoxelMeshState_Generated){
					Platform.DeallocateMemory(Index->Mesh->PUVN);
					EndTaskWithMemory(Index->MeshTask);

					AtomicCompareExchangeU32(
						(uint32 volatile *)&Index->MeshState,
        				VoxelMeshState_Unloaded,
        				VoxelMeshState_Generated);
				}

			}
			if(Index->MeshState == VoxelMeshState_Unloaded){
				GenerateVoxelMesh(Index, Context->Manager);
			}

			if(Index->MeshState == VoxelMeshState_Generated){
				ChunksTotal++;
				//BeginTicketMutex(&Work->RenderGroup->PlanesMutex);
				vec4* Planes = Work->RenderGroup->LastRenderSetup.Planes;

				int32_t TestResult = 1;
#if IVAN_VOXEL_WORLD_FRUSTUM_CULLING
				for(int PlaneIndex = 0;
					PlaneIndex < CameraPlane_Count;
					PlaneIndex++)
				{										
					if(!TestChunkOnPlane(Planes[PlaneIndex], Index->Chunk)){
						TestResult = 0;
						break;
					}
				}
#endif
				//EndTicketMutex(&Work->RenderGroup->PlanesMutex);

				Index->FrustumCullingTestResult = TestResult;
			}
		}
	}
}

INTERNAL_FUNCTION void UpdateVoxelChunks(
	voxel_chunk_manager* Manager,
	render_group* RenderGroup,
	vec3 CamPos)
{

	int32_t IndX;
	int32_t IndY;

	Manager->CamPos = CamPos;

	GetCurrChunkIndexBasedOnCamera(CamPos, &IndX, &IndY);

	update_voxel_chunk_work Works[ArrayCount(Manager->Contexts)];

	uint32 ReadTableIndex = Manager->TableIndex;
	uint32 WriteTableIndex;
	if(ReadTableIndex){
		WriteTableIndex = 0;
	}
	else{
		WriteTableIndex = 1;
	}

	voxel_table_pair** ReadTable = Manager->Tables[ReadTableIndex];
	voxel_table_pair** WriteTable = Manager->Tables[WriteTableIndex];

	for(int32_t ContextIndex = 0;
		ContextIndex < ArrayCount(Manager->Contexts);
		ContextIndex++)
	{
		voxel_chunk_thread_context* Context = &Manager->Contexts[ContextIndex];
		
		if(Context->IsValid){
			update_voxel_chunk_work* Work = &Works[ContextIndex];

			Work->CamIndexX = IndX;
			Work->CamIndexY = IndY;

			Work->ReadTable = ReadTable;

			Work->PrevCamIndexX = Manager->CurrHorizontalIndex;
			Work->PrevCamIndexY = Manager->CurrVerticalIndex;

			Work->MinXOffset = Context->MinXOffset;
			Work->MaxXOffset = Context->MaxXOffset;
			Work->MinYOffset = Context->MinYOffset;
			Work->MaxYOffset = Context->MaxYOffset;

			Work->Context = Context;
			Work->RenderGroup = RenderGroup;

			Platform.AddEntry(Manager->TranState->VoxelMeshQueue, UpdateVoxelChunkWork, Work);
		}
	}
	Platform.CompleteAllWork(Manager->TranState->VoxelMeshQueue);
	
	FreeVoxelHashTable(WriteTable, VOXEL_HASH_TABLE_SIZE);

	for(uint32 ContextIndex = 0;
		ContextIndex < ArrayCount(Manager->Contexts);
		ContextIndex++)
	{
		voxel_chunk_thread_context* Context = &Manager->Contexts[ContextIndex];
		if(Context->IsValid){
			update_voxel_chunk_work* Work = &Works[ContextIndex];

			voxel_chunk_header* Index = Context->VoxelChunkSentinel->Prev;
			for(Index; !Index->IsSentinel; Index = Index->Prev){
				
				voxel_table_pair NewPair;
				NewPair.Key = GenerateKeyForChunkIndices(
					Index->Chunk->HorizontalIndex, 
					Index->Chunk->VerticalIndex);
				NewPair.NextPair = 0;
				NewPair.Value = Index;

				InsertPairToVoxelHashTable(
					WriteTable, 
					VOXEL_HASH_TABLE_SIZE,
					NewPair);

				if((Index->MeshState == VoxelMeshState_Generated) && Index->MeshTask){
					if(Index->FrustumCullingTestResult){
						Assert(Index->Mesh->PUVN != 0);
						PushVoxelChunkMesh(
							RenderGroup,
							Index->Mesh,
							Context->Manager->VoxelAtlasID,
							GetPosForVoxelChunk(Index->Chunk));

					}
				}
			}
		}
	}

	Assert(WriteTableIndex != ReadTableIndex);
	AtomicCompareExchangeU32((uint32* volatile)&Manager->TableIndex,
		WriteTableIndex,
		ReadTableIndex);

	Manager->CurrHorizontalIndex = IndX;
	Manager->CurrVerticalIndex = IndY;
}


INTERNAL_FUNCTION voxel_chunk_manager* 
AllocateVoxelChunkManager(transient_state* TranState, game_assets* Assets)
{
	voxel_chunk_manager* Result = PushStruct(&TranState->TranArena, voxel_chunk_manager);
    
    Result->TranState = TranState;

    Result->ChangeMutex = {};

    asset_vector VAMatchVector = {};
    VAMatchVector.Data[Tag_VoxelAtlasType] = VoxelAtlasType_Default;
    asset_vector VAWeightVector = {};
    VAWeightVector.Data[Tag_VoxelAtlasType] = 10.0f;
    Result->VoxelAtlasID = GetBestMatchVoxelAtlasFrom(Assets, Asset_VoxelAtlas, &VAMatchVector, &VAWeightVector);

    Result->RandomSeries = RandomSeed(123);
    Result->ChunksViewDistance = 20;
    Result->CurrHorizontalIndex = 0;
    Result->CurrVerticalIndex = 0;

	SubArena(&Result->HashTableArena, &TranState->TranArena, IVAN_KILOBYTES(500));

	Result->TableIndex = 0;
	Result->Tables[0] = PushArray(&Result->HashTableArena, VOXEL_HASH_TABLE_SIZE, voxel_table_pair*);
	Result->Tables[1] = PushArray(&Result->HashTableArena, VOXEL_HASH_TABLE_SIZE, voxel_table_pair*);

	ClearVoxelHashTable(Result->Tables[0], VOXEL_HASH_TABLE_SIZE);
	ClearVoxelHashTable(Result->Tables[1], VOXEL_HASH_TABLE_SIZE);

	for(int32_t ContextIndex = 0;
		ContextIndex < ArrayCount(Result->Contexts);
		ContextIndex++)
	{
		voxel_chunk_thread_context* Context = &Result->Contexts[ContextIndex];

		*Context = {};
	}

	int32_t SideContextsCount = Sqrt(IVAN_VOXEL_CONTEXTS_COUNT);
	float SideDeltaReal = (float)(Result->ChunksViewDistance * 2 + 1) / (float)SideContextsCount;
	int32_t SideDeltaInt = (int32_t)(SideDeltaReal + 0.5f);
	
	if(SideDeltaInt == 0){
		SideDeltaInt = 1;
	}

	int32_t StartYIndex = -Result->ChunksViewDistance;
	int32_t CurrYIndex = StartYIndex;

	uint32 ContextsValid = 0;
	for(int32_t ContextVertIndex = 0;
		ContextVertIndex < SideContextsCount;
		ContextVertIndex++)
	{
		int32_t StartXIndex = -Result->ChunksViewDistance;
		int32_t CurrXIndex = StartXIndex;
		
		for(int32_t ContextHorzIndex = 0;
			ContextHorzIndex < SideContextsCount;
			ContextHorzIndex++)
		{
			voxel_chunk_thread_context* Context = 
				&Result->Contexts[ContextVertIndex * SideContextsCount + ContextHorzIndex];
			
			Context->IsValid = true;
			ContextsValid++;

			Context->MinXOffset = CurrXIndex;
			Context->MinYOffset = CurrYIndex;
			Context->MaxXOffset = CurrXIndex + SideDeltaInt - 1;
			Context->MaxYOffset = CurrYIndex + SideDeltaInt - 1;

			if(CurrXIndex + SideDeltaInt > Result->ChunksViewDistance){
				Context->MaxXOffset = Result->ChunksViewDistance;
			}

			if(CurrYIndex + SideDeltaInt > Result->ChunksViewDistance){
				Context->MaxYOffset = Result->ChunksViewDistance;
			}

			CurrXIndex += SideDeltaInt;
		}
		CurrYIndex += SideDeltaInt;
	}

    uint32 ListArenaMemSize = IVAN_MEGABYTES(5);

    uint32 ContextListMemory = ListArenaMemSize / ContextsValid;

	for(int32_t ContextIndex = 0; 
		ContextIndex < ArrayCount(Result->Contexts);
		ContextIndex++)
	{
		voxel_chunk_thread_context* Context = &Result->Contexts[ContextIndex];

		if(Context->IsValid){
			SubArena(&Context->ListArena, &TranState->TranArena, ContextListMemory);

			Context->VoxelChunkSentinel = PushStruct(&Context->ListArena, voxel_chunk_header);
			Context->VoxelChunkSentinel->Next = Context->VoxelChunkSentinel;
			Context->VoxelChunkSentinel->Prev = Context->VoxelChunkSentinel;
			Context->VoxelChunkSentinel->IsSentinel = true;

			Context->FirstFreeSentinel = PushStruct(&Context->ListArena, voxel_chunk_header);
			Context->FirstFreeSentinel->Next = Context->FirstFreeSentinel;
			Context->FirstFreeSentinel->Prev = Context->FirstFreeSentinel;
			Context->FirstFreeSentinel->IsSentinel = true;

			Context->Manager = Result;

			Context->ResultMutex = {};

#if 0
			Context->Result = PushStruct(&Context->ListArena, voxel_chunk_header);
			Context->Result->Next = Context->Result;
			Context->Result->Prev = Context->Result;
			Context->Result->IsSentinel = true;

			Context->ResultFree = PushStruct(&Context->ListArena, voxel_chunk_header);
			Context->ResultFree->Next = Context->ResultFree;
			Context->ResultFree->Prev = Context->ResultFree;
			Context->ResultFree->IsSentinel = true;
#endif
		}
	}

	return(Result);
}

