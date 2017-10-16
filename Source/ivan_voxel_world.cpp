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
			float NoiseScale3 = 100.0f;

			uint8_t DirtIndex = VoxelMaterial_Ground;
			uint8_t GroundIndex = VoxelMaterial_GrassyGround;

			float Noise1 = stb_perlin_noise3(
				(float)(ChunkPos.x + i) / 64.0f, 
				(float)ChunkPos.y / 64.0f, 
				(float)(ChunkPos.z + j) / 64.0f, 0, 0, 0);

			float Noise2 = stb_perlin_noise3(
				(float)(ChunkPos.x + i) / 256.0f, 
				(float)ChunkPos.y / 256.0f, 
				(float)(ChunkPos.z + j) / 256.0f, 0, 0, 0);

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
			else if(BiomNoise >= 0.1f && BiomNoise < 0.6f){
#if 0
				DirtIndex = VoxelMaterial_Ground;
				GroundIndex = VoxelMaterial_GrassyGround;
#else
				DirtIndex = VoxelMaterial_Ground;
				GroundIndex = VoxelMaterial_GrassyGround;
#endif
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

INTERNAL_FUNCTION void GenerateVoxelChunk(
	voxel_chunk_header* Header,
	voxel_chunk_manager* Manager)
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
			*ChunkWork = Work;
			Platform.AddEntry(Manager->TranState->HighPriorityQueue, GenerateVoxelChunkWork, ChunkWork);
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
			generate_mesh_work* MeshWork = PushStruct(&Task->Arena, generate_mesh_work);
			*MeshWork = Work;
			Platform.AddEntry(Manager->TranState->HighPriorityQueue, GenerateVoxelMeshWork, MeshWork);
		}
		else{
			//Not enough tasks. High usage
			INVALID_CODE_PATH;
		}
	}
	else{

	}
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

inline void FreeVoxelHashTable(
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
	}
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
#if !(IVAN_VOXEL_WORLD_MULTITHREADED)
INTERNAL_FUNCTION void 
UpdateVoxelChunks(
	voxel_chunk_manager* Manager,
	render_group* RenderGroup,
	vec3 CamPos)
{
	TIMED_FUNCTION();

	voxel_chunk_header* Index = 0;

	temporary_memory HashTempMem = BeginTemporaryMemory(&Manager->HashTableArena);
	temporary_memory TempTempMem = BeginTemporaryMemory(&Manager->TempArena);

	voxel_table_pair** HashTable = PushArray(HashTempMem.Arena, VOXEL_HASH_TABLE_SIZE, voxel_table_pair*);
	
	for(int32_t ClearIndex = 0;
		ClearIndex < VOXEL_HASH_TABLE_SIZE;
		ClearIndex++)
	{
		HashTable[ClearIndex] = 0;
	}

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
				Index->Prev->Next = Index->Next;
				Index->Next->Prev = Index->Prev;

				IVAN_COMPLETE_WRITES_BEFORE_FUTURE;

				if(Index->ChunkTask && (Index->ChunkState != VoxelChunkState_InProcess)){
					EndTaskWithMemory(Index->ChunkTask);
				}

				if(Index->MeshTask && (Index->MeshState != VoxelMeshState_InProcess)){
					Platform.DeallocateMemory(Index->Mesh->PUVN);
					EndTaskWithMemory(Index->MeshTask);
				}
			}
		}
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
			NewPair);
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

				voxel_table_pair NewPair = {};
				NewPair.Key = KeyToFind;
				NewPair.Value = NewHeader;
				InsertPairToVoxelHashTable(HashTable, VOXEL_HASH_TABLE_SIZE, NewPair);
				InsertChunkHeaderAtFront(Manager->VoxelChunkSentinel, NewHeader);
			}
		}
	}

	int ChunksTotal = 0;
	int ChunksInFrustrum = 0;
	Index = Manager->VoxelChunkSentinel->Next;
	for(Index; !Index->IsSentinel; Index = Index->Next){
		
		if(Index->ChunkState == VoxelChunkState_Generated){			
			voxel_chunk* Chunk = Index->Chunk;
			voxel_chunk* OldLeft = Chunk->LeftNeighbour;
			voxel_chunk* OldRight = Chunk->RightNeighbour;
			voxel_chunk* OldFront = Chunk->FrontNeighbour;
			voxel_chunk* OldBack = Chunk->BackNeighbour;

			IVAN_COMPLETE_READS_BEFORE_FUTURE;

			SetNeighboursForChunk(
				Chunk, 
				Chunk->HorizontalIndex,
				Chunk->VerticalIndex,
				HashTable);

			IVAN_COMPLETE_WRITES_BEFORE_FUTURE;

			if(NeighboursWasChanged(Chunk, OldRight, OldLeft, OldFront, OldBack)){

				if(Index->MeshTask && Index->MeshState == VoxelMeshState_Generated){
					Index->MeshState = VoxelMeshState_Unloaded;
					Platform.DeallocateMemory(Index->Mesh->PUVN);
					EndTaskWithMemory(Index->MeshTask);
				}

				GenerateVoxelMesh(Index, Manager);
			}

			if(Index->MeshState == VoxelMeshState_Generated){
				ChunksTotal++;
				vec4* Planes = RenderGroup->LastRenderSetup.Planes;

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

				if(TestResult){
					ChunksInFrustrum++;
				    PushVoxelChunkMesh(
				    	RenderGroup, 
				    	Index->Mesh,
			        	Manager->VoxelAtlasID, 
			        	GetPosForVoxelChunk(Index->Chunk));
				}
			}
		}
	}

	FreeVoxelHashTable(HashTable, VOXEL_HASH_TABLE_SIZE);

	EndTemporaryMemory(HashTempMem);
	EndTemporaryMemory(TempTempMem);

	Manager->CurrHorizontalIndex = IndX;
	Manager->CurrVerticalIndex = IndY;
}

#else
struct update_voxel_chunk_work{
	int32_t CamIndexX;
	int32_t CamIndexY;

	int32_t PrevCamIndexX;
	int32_t PrevCamIndexY;

	int32_t MinXOffset;
	int32_t MaxXOffset;
	int32_t MinYOffset;
	int32_t MaxYOffset;

	temporary_memory HashTempMem;
	voxel_table_pair** HashTable;

	int32_t ChunksViewDistance;

	voxel_chunk_thread_context* Context;

	render_group* RenderGroup;
};

INTERNAL_FUNCTION PLATFORM_WORK_QUEUE_CALLBACK(UpdateVoxelChunkWork){
	update_voxel_chunk_work* Work = (update_voxel_chunk_work*)Data;

	voxel_chunk_thread_context* Context = Work->Context;

	temporary_memory TempTempMem = BeginTemporaryMemory(&Context->TempArena);

	voxel_chunk_header* Index = 0;

	if((Work->PrevCamIndexX != Work->CamIndexX) ||
		(Work->PrevCamIndexY != Work->CamIndexY))
	{
		Index = Context->VoxelChunkSentinel->Next;
		for(Index; !Index->IsSentinel; Index = Index->Next){
			if((IVAN_CHUNK_ABS(Work->CamIndexX - Index->Chunk->HorizontalIndex) > Work->ChunksViewDistance) ||
				(IVAN_CHUNK_ABS(Work->CamIndexY - Index->Chunk->VerticalIndex) > Work->ChunksViewDistance))
			{
				IVAN_COMPLETE_WRITES_BEFORE_FUTURE;

				if(Index->ChunkTask && (Index->ChunkState == VoxelChunkState_Generated))
				{
					EndTaskWithMemory(Index->ChunkTask);

					AtomicCompareExchangeU32(
						(uint32 volatile *)&Index->ChunkState,
        				VoxelChunkState_Unloaded,
        				VoxelChunkState_Generated);
				}

				if(Index->MeshTask && (Index->MeshState == VoxelMeshState_Generated))
				{
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
		}
	}

	Index = Context->VoxelChunkSentinel->Prev;
	for(Index; !Index->IsSentinel; Index = Index->Prev){
		voxel_table_pair NewPair = {};
		NewPair.Key = GenerateKeyForChunkIndices(
			Index->Chunk->HorizontalIndex, 
			Index->Chunk->VerticalIndex);
		NewPair.NextPair = 0;
		NewPair.Value = Index;

		InsertPairToVoxelHashTable(
			Work->HashTable, 
			VOXEL_HASH_TABLE_SIZE,
			NewPair);
	}

	int32_t MinX = Work->CamIndexX + Work->MinXOffset;
	int32_t MaxX = Work->CamIndexX + Work->MaxXOffset;
	int32_t MinY = Work->CamIndexY + Work->MinYOffset;
	int32_t MaxY = Work->CamIndexY + Work->MaxYOffset;

	for(int VertIndex = MinY; VertIndex <= MaxY; VertIndex++){
		for(int HorzIndex = MinX; HorzIndex <= MaxX; HorzIndex++){
			
			uint64_t KeyToFind = GenerateKeyForChunkIndices(HorzIndex, VertIndex);
			voxel_table_pair* FoundPair = FindPairInVoxelHashTable(
				Work->HashTable, 
				VOXEL_HASH_TABLE_SIZE, 
				KeyToFind);

			if(!FoundPair){
				voxel_chunk_header* NewHeader = 0;
				if(Context->FirstFreeSentinel->Next->IsSentinel){
					NewHeader = PushStruct(&Context->ListArena, voxel_chunk_header);
				}
				else{
					NewHeader = Context->FirstFreeSentinel->Next;

					Context->FirstFreeSentinel->Next = NewHeader->Next;
					NewHeader->Next->Prev = Context->FirstFreeSentinel;
				}
				NewHeader->IsSentinel = false;
				NewHeader->Chunk = PushStruct(&Context->ListArena, voxel_chunk);
				NewHeader->Mesh = PushStruct(&Context->ListArena, voxel_chunk_mesh);
				NewHeader->Chunk->HorizontalIndex = HorzIndex;
				NewHeader->Chunk->VerticalIndex = VertIndex;
				NewHeader->NeedsToBeGenerated = true;
				
				GenerateVoxelChunk(NewHeader, Context->Manager);

				voxel_table_pair NewPair = {};
				NewPair.Key = KeyToFind;
				NewPair.Value = NewHeader;
				
				InsertPairToVoxelHashTable(Work->HashTable, VOXEL_HASH_TABLE_SIZE, NewPair);
				
				InsertChunkHeaderAtFront(Context, NewHeader);
			}
		}
	}

	Index = Context->VoxelChunkSentinel->Next;
	for(Index; !Index->IsSentinel; Index = Index->Next){
		
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
				Work->HashTable);

			if(NeighboursWasChanged(Chunk, OldRight, OldLeft, OldFront, OldBack)){

				if(Index->MeshTask && Index->MeshState == VoxelMeshState_Generated)
				{
					Platform.DeallocateMemory(Index->Mesh->PUVN);
					EndTaskWithMemory(Index->MeshTask);
					
					AtomicCompareExchangeU32(
						(uint32 volatile *)&Index->MeshState,
        				VoxelMeshState_Unloaded,
        				VoxelMeshState_Generated);
				}

				GenerateVoxelMesh(Index, Context->Manager);
			}


			if(Index->MeshState == VoxelMeshState_Generated && Index->MeshTask){
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
				Index->FrustumCullingTestResult = TestResult;
			}
		}
	}

	EndTemporaryMemory(TempTempMem);
}

INTERNAL_FUNCTION void UpdateVoxelChunksMultithreaded(
	voxel_chunk_manager* Manager,
	render_group* RenderGroup,
	vec3 CamPos)
{
	int32_t IndX;
	int32_t IndY;

	GetCurrChunkIndexBasedOnCamera(CamPos, &IndX, &IndY);

	update_voxel_chunk_work Works[ArrayCount(Manager->Contexts)];

	for(int32_t ContextIndex = 0;
		ContextIndex < ArrayCount(Manager->Contexts);
		ContextIndex++)
	{
		voxel_chunk_thread_context* Context = &Manager->Contexts[ContextIndex];
		if(Context->IsValid){
			temporary_memory ContextHashTableTempMem = BeginTemporaryMemory(&Context->HashTableArena);
			voxel_table_pair** ContextHashTable = PushArray(
				ContextHashTableTempMem.Arena, 
				VOXEL_HASH_TABLE_SIZE, 
				voxel_table_pair*);


			for(int32_t ClearIndex = 0;
				ClearIndex < VOXEL_HASH_TABLE_SIZE;
				ClearIndex++)
			{
				ContextHashTable[ClearIndex] = 0;
			}

			update_voxel_chunk_work* Work = &Works[ContextIndex];

			Work->CamIndexX = IndX;
			Work->CamIndexY = IndY;

			Work->PrevCamIndexX = Manager->CurrHorizontalIndex;
			Work->PrevCamIndexY = Manager->CurrVerticalIndex;

			Work->MinXOffset = Context->MinXOffset;
			Work->MaxXOffset = Context->MaxXOffset;
			Work->MinYOffset = Context->MinYOffset;
			Work->MaxYOffset = Context->MaxYOffset;

			/*Create new hash table for each thread context*/
			Work->HashTable = ContextHashTable;
			Work->HashTempMem = ContextHashTableTempMem;

			Work->ChunksViewDistance = Manager->ChunksViewDistance;

			Work->Context = Context;
			Work->RenderGroup = RenderGroup;

			Platform.AddEntry(Manager->TranState->VoxelMeshQueue, UpdateVoxelChunkWork, Work);
		}
	}
	Platform.CompleteAllWork(Manager->TranState->VoxelMeshQueue);

	for(int32_t ContextIndex = 0;
		ContextIndex < ArrayCount(Manager->Contexts);
		ContextIndex++)
	{
		voxel_chunk_thread_context* Context = &Manager->Contexts[ContextIndex];
		if(Context->IsValid){
			update_voxel_chunk_work* Work = &Works[ContextIndex];

			int ChunksTotal = 0;
			int ChunksInFrustrum = 0;
			voxel_chunk_header* Index = Context->VoxelChunkSentinel->Prev;
			for(Index; !Index->IsSentinel; Index = Index->Prev){
				if((Index->MeshState == VoxelMeshState_Generated) && Index->MeshTask){
					ChunksTotal++;

					if(Index->FrustumCullingTestResult){
						ChunksInFrustrum++;
						
						Assert(Index->Mesh->PUVN != 0);
					    PushVoxelChunkMesh(
					    	Work->RenderGroup, 
					    	Index->Mesh,
				        	Context->Manager->VoxelAtlasID, 
				        	GetPosForVoxelChunk(Index->Chunk));
					}
				}
			}
			
			FreeVoxelHashTable(Work->HashTable, VOXEL_HASH_TABLE_SIZE);
			
			EndTemporaryMemory(Work->HashTempMem);
		}
	}

	Manager->CurrHorizontalIndex = IndX;
	Manager->CurrVerticalIndex = IndY;

}
#endif

INTERNAL_FUNCTION voxel_chunk_manager* 
AllocateVoxelChunkManager(transient_state* TranState, game_assets* Assets)
{
	voxel_chunk_manager* Result = PushStruct(&TranState->TranArena, voxel_chunk_manager);
    
    Result->TranState = TranState;
    Result->ListMutex = {};
    Result->ListArenaMutex = {};
    Result->HashTableMutex = {};
    Result->HashTableArenaMutex = {};
    Result->TempArenaMutex = {};
    Result->RenderPushMutex = {};

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
    Result->ChunksViewDistance = 20;
    Result->CurrHorizontalIndex = 0;
    Result->CurrVerticalIndex = 0;

	SubArena(&Result->HashTableArena, &TranState->TranArena, IVAN_KILOBYTES(500));
	SubArena(&Result->ListArena, &TranState->TranArena, IVAN_MEGABYTES(5));
	SubArena(&Result->TempArena, &TranState->TranArena, IVAN_KILOBYTES(500));

	for(int32_t ContextIndex = 0;
		ContextIndex < ArrayCount(Result->Contexts);
		ContextIndex++)
	{
		voxel_chunk_thread_context* Context = &Result->Contexts[ContextIndex];

		*Context = {};
	}

	int32_t SideContextsCount = Sqrt(IVAN_VOXEL_CHUNK_CONTEXTS_COUNT);
	float SideDeltaReal = (float)(Result->ChunksViewDistance * 2 + 1) / (float)SideContextsCount;
	int32_t SideDeltaInt = (int32_t)(SideDeltaReal + 0.5f);
	
	if(SideDeltaInt == 0){
		SideDeltaInt = 1;
	}

	int32_t StartYIndex = -Result->ChunksViewDistance;
	int32_t CurrYIndex = StartYIndex;

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

	for(int32_t ContextIndex = 0; 
		ContextIndex < ArrayCount(Result->Contexts);
		ContextIndex++)
	{
		voxel_chunk_thread_context* Context = &Result->Contexts[ContextIndex];

		if(Context->IsValid){

			SubArena(&Context->HashTableArena, &TranState->TranArena, IVAN_KILOBYTES(20));
			SubArena(&Context->ListArena, &TranState->TranArena, IVAN_KILOBYTES(100));
			SubArena(&Context->TempArena, &TranState->TranArena, IVAN_KILOBYTES(10));

			Context->VoxelChunkSentinel = PushStruct(&TranState->TranArena, voxel_chunk_header);
			Context->VoxelChunkSentinel->Next = Context->VoxelChunkSentinel;
			Context->VoxelChunkSentinel->Prev = Context->VoxelChunkSentinel;
			Context->VoxelChunkSentinel->IsSentinel = true;

			Context->FirstFreeSentinel = PushStruct(&TranState->TranArena, voxel_chunk_header);
			Context->FirstFreeSentinel->Next = Context->FirstFreeSentinel;
			Context->FirstFreeSentinel->Prev = Context->FirstFreeSentinel;
			Context->FirstFreeSentinel->IsSentinel = true;

			Context->Manager = Result;
		}
	}

	return(Result);
}

