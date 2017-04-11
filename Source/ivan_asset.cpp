struct load_asset_work{
    task_with_memory* Task;
    asset* Asset;

    platform_file_handle* Handle;
    uint64 Offset;
    uint64 Size;
    void* Destination;

    uint32 FinalState;
};

INTERNAL_FUNCTION PLATFORM_WORK_QUEUE_CALLBACK(LoadAssetWork){
    load_asset_work* Work = (load_asset_work*)Data;

    Platform.ReadDataFromFile(Work->Handle, Work->Offset, Work->Size, Work->Destination);


    GD_COMPLETE_WRITES_BEFORE_FUTURE;

    if(PlatformNoFileErrors(Work->Handle))
    {
        Work->Asset->State = AssetState_Loaded;
    }
    else{
        int a = 1;
    }

    EndTaskWithMemory(Work->Task);
}

inline platform_file_handle*
GetFileHandleFor(game_assets* Assets, uint32 FileIndex){
    Assert(FileIndex < Assets->FileCount);
    platform_file_handle* Result = Assets->Files[FileIndex].Handle;

    return(Result);
}

INTERNAL_FUNCTION asset_memory_block*
InsertBlock(asset_memory_block* Prev, uint64 Size, void* Memory){
    Assert(Size > sizeof(asset_memory_block));
    asset_memory_block* Block = (asset_memory_block*)Memory;
    Block->Flags = 0;
    Block->Size = Size - sizeof(asset_memory_block);
    Block->Prev = Prev;
    Block->Next = Prev->Next;
    Block->Prev->Next = Block;
    Block->Next->Prev = Block;
    return(Block);
}

inline void InsertAssetHeaderAtFront(game_assets* Assets, asset_memory_header* Header){
    asset_memory_header* Sentinel = &Assets->LoadedAssetSentinel;

    Header->Prev = Sentinel;
    Header->Next = Sentinel->Next;

    Header->Next->Prev = Header;
    Header->Prev->Next = Header;
}

inline void RemoveAssetHeaderFromList(asset_memory_header* Header){
    Header->Prev->Next = Header->Next;
    Header->Next->Prev = Header->Prev;

    Header->Next = Header->Prev = 0;
}

INTERNAL_FUNCTION void MoveHeaderToFront(game_assets* Assets, asset* Asset){
    if(!IsLocked(Asset)){
        asset_memory_header* Header = Asset->Header;

        RemoveAssetHeaderFromList(Header);
        InsertAssetHeaderAtFront(Assets, Header);
    }
}

INTERNAL_FUNCTION asset_memory_block*
FindBlockForSize(game_assets* Assets, memory_index Size){
    asset_memory_block* Result = 0;

    for(asset_memory_block* Block = Assets->MemorySentinel.Next;
        Block != &Assets->MemorySentinel;
        Block = Block->Next)
    {
        if(!(Block->Flags & AssetMemory_Used)){
            if(Block->Size >= Size){
                Result = Block;
                break;
            }
        }
    }

    return(Result);
}

INTERNAL_FUNCTION bool32 
MergeIfPossible(game_assets* Assets, asset_memory_block* First, asset_memory_block* Second){
    bool32 Result = false;

    if((First != &Assets->MemorySentinel) &&
        (Second != &Assets->MemorySentinel))
    {
        if(!(First->Flags & AssetMemory_Used) &&
            !(Second->Flags & AssetMemory_Used))
        {
            uint8* ExpectedSecond = (uint8*)First + sizeof(asset_memory_block) + First->Size;
            if((uint8*)Second == ExpectedSecond){
                Second->Next->Prev = Second->Prev;
                Second->Prev->Next = Second->Next;

                First->Size += sizeof(asset_memory_block) + Second->Size;

                Result = true;
            }
        }
    }

    return(Result);
}

INTERNAL_FUNCTION void*
RequestAssetMemory(game_assets* Assets, memory_index Size){
    void* Result = 0;

    asset_memory_block* Block = FindBlockForSize(Assets, Size);
    for(;;){
        if(Block && (Size <= Block->Size)){
            Block->Flags |= AssetMemory_Used;

            Result = (uint8*)(Block + 1);

            memory_index RemainingSize = Block->Size - Size;
            memory_index BlockSplitThreshold = 4096;
            if(RemainingSize > BlockSplitThreshold){
                Block->Size -= RemainingSize;
                InsertBlock(Block, RemainingSize, (uint8*)Result + Size);
            }
            else{

            }

            break;
        }
        else{
            for(asset_memory_header* Header = Assets->LoadedAssetSentinel.Prev;
                Header != &Assets->LoadedAssetSentinel;
                Header = Header->Prev)
            {
                asset* Asset = Assets->Assets + Header->AssetIndex;
                if(GetState(Asset) >= AssetState_Loaded){
                    uint32 AssetIndex = Header->AssetIndex;
                    asset* Asset = Assets->Assets + AssetIndex;

                    Assert(GetState(Asset) == AssetState_Loaded);
                    Assert(!IsLocked(Asset));

                    RemoveAssetHeaderFromList(Header);

                    Block = (asset_memory_block*)Asset->Header - 1;
                    Block->Flags &= AssetMemory_Used;

                    if(MergeIfPossible(Assets, Block->Prev, Block)){
                        Block = Block->Prev;
                    }

                    MergeIfPossible(Assets, Block, Block->Next);

                    Asset->State = AssetState_Unloaded;
                    Asset->Header = 0;
                    break;
                }
            }
        }
    }

    return(Result);
}


struct asset_memory_size{
    uint32 Total;
    uint32 Data;
};

inline void 
AddAssetHeaderToList(game_assets* Assets, uint32 AssetIndex, asset_memory_size Size){
    asset_memory_header* Header = Assets->Assets[AssetIndex].Header;
    Header->AssetIndex = AssetIndex;
    Header->TotalSize = Size.Total;
    InsertAssetHeaderAtFront(Assets, Header);
}

INTERNAL_FUNCTION void LoadBitmapAsset(game_assets* Assets, bitmap_id ID){
    if(ID.Value && 
        AtomicCompareExchangeUInt32((uint32 volatile *)&Assets->Assets[ID.Value].State,
            AssetState_Queued,
            AssetState_Unloaded) == AssetState_Unloaded)
    {
        task_with_memory* Task = BeginTaskWithMemory(Assets->TranState);
        if(Task){
            asset* Asset = Assets->Assets + ID.Value;
            dda_bitmap* Info = &Asset->DDA.Bitmap;
            
            //loaded_bitmap* Bitmap = PushStruct(&Assets->Arena, loaded_bitmap);
            asset_memory_size Size = {};
            uint32 Width = Info->Dimension[0];
            uint32 Height = Info->Dimension[1];
            Size.Data = Height * Width * 4;
            Size.Total = Size.Data + sizeof(asset_memory_header);

            Asset->Header = (asset_memory_header*)RequestAssetMemory(Assets, Size.Total);

            loaded_bitmap* Bitmap = &Asset->Header->Bitmap;
            Bitmap->AlignPercentage = Vec2(Info->AlignPercentage[0], Info->AlignPercentage[1]);
            Bitmap->WidthOverHeight = (float)Info->Dimension[0] / (float)Info->Dimension[1];
            Bitmap->Width = Info->Dimension[0];
            Bitmap->Height = Info->Dimension[1];
            Bitmap->Memory = (Asset->Header + 1);

            load_asset_work* Work = PushStruct(&Task->Arena, load_asset_work);
            Work->Task = Task;
            Work->Asset = Assets->Assets + ID.Value;
            Work->Handle = GetFileHandleFor(Assets, Asset->FileIndex);
            Work->Offset = Asset->DDA.DataOffset;
            Work->Size = Size.Data;
            Work->Destination = Bitmap->Memory;
            Work->FinalState = (AssetState_Loaded);

            AddAssetHeaderToList(Assets, ID.Value, Size);

            Platform.AddEntry(Assets->TranState->LowPriorityQueue, LoadAssetWork, Work);
        }
        else{
            Assets->Assets[ID.Value].State = AssetState_Unloaded;
        }
    }
}

INTERNAL_FUNCTION void LoadSoundAsset(game_assets* Assets, sound_id ID){
    if(ID.Value &&
        AtomicCompareExchangeUInt32((uint32 volatile *)&Assets->Assets[ID.Value].State,
            AssetState_Queued,
            AssetState_Unloaded) == AssetState_Unloaded)
    {
        task_with_memory* Task = BeginTaskWithMemory(Assets->TranState);
        if(Task){

            asset* Asset = Assets->Assets + ID.Value;
            dda_sound* Info = &Asset->DDA.Sound;

            asset_memory_size Size = {};
            Size.Data = Info->ChannelCount * Info->SampleCount * sizeof(int16);
            Size.Total = Size.Data + sizeof(asset_memory_header);

            Asset->Header = (asset_memory_header*)RequestAssetMemory(Assets, Size.Total);
            loaded_sound* Sound = &Asset->Header->Sound;

            //loaded_sound* Sound = PushStruct(&Assets->Arena, loaded_sound);
            Sound->SampleCount = Info->SampleCount;
            Sound->ChannelCount = Info->ChannelCount;

            uint32 ChannelSize = Sound->SampleCount * sizeof(int16);

            void* Memory = (Asset->Header + 1);
            int16* SoundAt = (int16*)Memory;
            for(uint32 ChannelIndex = 0;
                ChannelIndex < Sound->ChannelCount;
                ChannelIndex++)
            {
                Sound->Samples[ChannelIndex] = SoundAt;
                SoundAt += ChannelSize;
            }

            load_asset_work* Work = PushStruct(&Task->Arena, load_asset_work);
            Work->Task = Task;
            Work->Asset = Assets->Assets + ID.Value;
            Work->Handle = GetFileHandleFor(Assets, Asset->FileIndex);
            Work->Offset = Asset->DDA.DataOffset;
            Work->Size = Size.Data;
            Work->Destination = Memory;
            Work->FinalState = (AssetState_Loaded);

            AddAssetHeaderToList(Assets, ID.Value, Size);

            Platform.AddEntry(Assets->TranState->LowPriorityQueue, LoadAssetWork, Work);
        }
        else{
            Assets->Assets[ID.Value].State = AssetState_Unloaded;
        }
    }
}

INTERNAL_FUNCTION uint32 
GetBestMatchAssetFrom(
    game_assets* Assets,
    asset_type_id TypeID,
    asset_vector* MatchVector,
    asset_vector* WeightVector)
{
    uint32 Result = 0;

    real32 BestDiff = F32_MAX;
    asset_type* Type = Assets->AssetTypes + TypeID;
    for(uint32 AssetIndex = Type->FirstAssetIndex;
        AssetIndex < Type->OnePastLastAssetIndex;
        AssetIndex++)
    {
        asset* Asset = Assets->Assets + AssetIndex;

        real32 TotalWeightedDiff = 0.0;
        for(uint32 TagIndex = Asset->DDA.FirstTagIndex;
            TagIndex < Asset->DDA.OnePastLastTagIndex;
            TagIndex++)
        {
            dda_tag* Tag = Assets->Tags + TagIndex;

            real32 A = MatchVector->Data[Tag->ID];
            real32 B = Tag->Value;
            real32 D0 = IVAN_MATH_ABS(A - B);
            real32 D1 = IVAN_MATH_ABS((A - Assets->TagRange[Tag->ID] * IVAN_MATH_SIGN(A)) - B);
            real32 Diffrence = GD_MIN(D0, D1);

            real32 Weighted = WeightVector->Data[Tag->ID] * Diffrence;
            TotalWeightedDiff += Weighted;
        }

        if(BestDiff > TotalWeightedDiff){
            BestDiff = TotalWeightedDiff;
            Result = AssetIndex;
        }
    }

    return(Result);
}

INTERNAL_FUNCTION uint32
GetRandomAssetFrom(game_assets* Assets, asset_type_id TypeID, random_series* Series)
{
    uint32 Result = 0;

    asset_type* Type = Assets->AssetTypes + TypeID;
    if(Type->FirstAssetIndex != Type->OnePastLastAssetIndex){
        uint32 Count = (Type->OnePastLastAssetIndex - Type->FirstAssetIndex);
        uint32 Choice = RandomChoiceFromCount(Series, Count);

        Result = Type->FirstAssetIndex + Choice;
    }

    return(Result);
}

INTERNAL_FUNCTION uint32
GetFirstAssetFrom(game_assets* Assets, asset_type_id TypeID){
    uint32 Result = 0;

    asset_type* Type = Assets->AssetTypes + TypeID;
    if(Type->FirstAssetIndex != Type->OnePastLastAssetIndex){
        Result = Type->FirstAssetIndex;
    }

    return(Result);
}

inline bitmap_id
GetBestMatchBitmapFrom(
    game_assets* Assets,
    asset_type_id TypeID,
    asset_vector* MatchVector, asset_vector* WeightVector)
{
    bitmap_id Result = {GetBestMatchAssetFrom(Assets, TypeID, MatchVector, WeightVector)};
    return(Result);
}

inline bitmap_id
GetFirstBitmapFrom(game_assets* Assets, asset_type_id TypeID){
    bitmap_id Result = {GetFirstAssetFrom(Assets, TypeID)};
    return(Result);
}

inline bitmap_id
GetRandomBitmapFrom(game_assets* Assets, asset_type_id TypeID, random_series* Series){
    bitmap_id Result = {GetRandomAssetFrom(Assets, TypeID, Series)};
    return(Result);
}

inline sound_id
GetBestMatchSoundFrom(
    game_assets* Assets,
    asset_type_id TypeID,
    asset_vector* MatchVector, 
    asset_vector* WeightVector)
{
    sound_id Result = {GetBestMatchAssetFrom(Assets, TypeID, MatchVector, WeightVector)};
    return(Result);
}

inline sound_id
GetFirstSoundFrom(game_assets* Assets, asset_type_id TypeID){
    sound_id Result = {GetFirstAssetFrom(Assets, TypeID)};
    return(Result);
}

inline sound_id
GetRandomSoundFrom(game_assets* Assets, asset_type_id TypeID, random_series* Series){
    sound_id Result = {GetRandomAssetFrom(Assets, TypeID, Series)};
    return(Result);
}

INTERNAL_FUNCTION game_assets* 
AllocateGameAssets(memory_arena* Arena, size_t Size, transient_state* TranState){
    game_assets* Assets = PushStruct(Arena, game_assets);
    Assets->TranState = TranState;

    Assets->MemorySentinel.Flags = 0;
    Assets->MemorySentinel.Size = 0;
    Assets->MemorySentinel.Prev = &Assets->MemorySentinel;
    Assets->MemorySentinel.Next = &Assets->MemorySentinel;

    InsertBlock(&Assets->MemorySentinel, Size, PushSize(Arena, Size));

    Assets->LoadedAssetSentinel.Next = &Assets->LoadedAssetSentinel;
    Assets->LoadedAssetSentinel.Prev = &Assets->LoadedAssetSentinel;


    for(uint32 TagType = 0;
        TagType < Tag_Count;
        ++TagType)
    {
        Assets->TagRange[TagType] = 1000000.0f;
    }
    Assets->TagRange[Tag_FacingDirection] = IVAN_MATH_TAU;

    Assets->TagCount = 1;
    Assets->AssetCount = 1;


    platform_file_group* FileGroup = Platform.GetAllFilesOfTypeBegin("dda");
    Assets->FileCount = FileGroup->FileCount;
    Assets->Files = PushArray(Arena, Assets->FileCount, asset_file);
    
    for(uint32 FileIndex = 0;
        FileIndex < Assets->FileCount;
        FileIndex++)
    {
        asset_file* File = Assets->Files + FileIndex;

        File->TagBase = Assets->TagCount;

        ZeroStruct(File->Header);
        File->Handle = Platform.OpenNextFile(FileGroup, FileIndex);
        Platform.ReadDataFromFile(File->Handle, 0, sizeof(File->Header), &File->Header);

        uint32 AssetTypeArraySize = File->Header.AssetTypeCount * sizeof(dda_asset_type);
        File->AssetTypeArray = (dda_asset_type*)PushSize(Arena, AssetTypeArraySize);
        Platform.ReadDataFromFile(
            File->Handle, 
            File->Header.AssetTypeOffset, 
            AssetTypeArraySize, 
            File->AssetTypeArray);

        /*Delete this*/
        dda_asset TempAsset;
        Platform.ReadDataFromFile(
            File->Handle,
            File->Header.AssetOffset,
            sizeof(dda_asset),
            &TempAsset);

        if(File->Header.MagicValue != DDA_MAGIC_VALUE){
            Platform.FileError(File->Handle, "DDA file has invalid magic value");
        }

        if(File->Header.Version > DDA_VERSION){
            Platform.FileError(File->Handle, "DDA file has invalid version");
        }

        if(PlatformNoFileErrors(File->Handle)){
            Assets->TagCount += (File->Header.TagCount - 1);
            Assets->AssetCount += (File->Header.AssetCount - 1);
        }
        else{
            INVALID_CODE_PATH;
        }
    }

    Platform.GetAllFilesOfTypeEnd(FileGroup);

    /*Allocating metadata*/
    Assets->Assets = PushArray(Arena, Assets->AssetCount, asset);
    Assets->Tags = PushArray(Arena, Assets->TagCount, dda_tag);

    /*Setting first tag as null*/
    ZeroStruct(Assets->Tags[0]);

    /*Loading tags*/
    for(uint32 FileIndex = 0;
        FileIndex < Assets->FileCount;
        FileIndex++)
    {
        asset_file* File = Assets->Files + FileIndex;
        if(PlatformNoFileErrors(File->Handle)){

            uint32 TagArraySize = sizeof(dda_tag) * (File->Header.TagCount - 1);
            Platform.ReadDataFromFile(
                File->Handle, 
                File->Header.TagOffset + sizeof(dda_tag),
                TagArraySize, 
                Assets->Tags + File->TagBase);
        }
    }


    ZeroStruct(*(Assets->Assets));
    uint32 AssetCount = 1;

    /*Merging*/
    for(uint32 DestTypeID = 0;
        DestTypeID < Asset_Count;
        DestTypeID++)
    {
        asset_type* DestType = Assets->AssetTypes + DestTypeID;
        DestType->FirstAssetIndex = AssetCount;

        for(uint32 FileIndex = 0;
            FileIndex < Assets->FileCount;
            FileIndex++)
        {
            asset_file* File = Assets->Files + FileIndex;
            if(PlatformNoFileErrors(File->Handle)){
                for(uint32 SourceIndex = 0;
                    SourceIndex < File->Header.AssetTypeCount;
                    SourceIndex++)
                {
                    dda_asset_type* SourceType = File->AssetTypeArray + SourceIndex;

                    if(SourceType->TypeID == DestTypeID){
                        uint32 AssetCountForType = 
                            (SourceType->OnePastLastAssetIndex - SourceType->FirstAssetIndex);
                        
                        temporary_memory TempMem = BeginTemporaryMemory(&TranState->TranArena);
                        dda_asset* DDAAssetArray = PushArray(
                            &TranState->TranArena,
                            AssetCountForType,
                            dda_asset);

                        Platform.ReadDataFromFile(
                            File->Handle,
                            File->Header.AssetOffset + SourceType->FirstAssetIndex * sizeof(dda_asset),
                            AssetCountForType * sizeof(dda_asset),
                            DDAAssetArray);

                        for(uint32 AssetIndex = 0;
                            AssetIndex < AssetCountForType;
                            AssetIndex++)
                        {
                            dda_asset* DDAAsset = DDAAssetArray + AssetIndex;

                            Assert(AssetCount < Assets->AssetCount);
                            asset* Asset = Assets->Assets + AssetCount++;

                            Asset->FileIndex = FileIndex;
                            Asset->DDA = *DDAAsset;
                            if(Asset->DDA.FirstTagIndex == 0){
                                Asset->DDA.FirstTagIndex = Asset->DDA.OnePastLastTagIndex = 0;
                            }
                            else{
                                Asset->DDA.FirstTagIndex += (File->TagBase - 1);
                                Asset->DDA.OnePastLastTagIndex += (File->TagBase - 1);
                            }
                        }

                        EndTemporaryMemory(TempMem);
                    }
                }
            }
        }

        DestType->OnePastLastAssetIndex = AssetCount;
    }

    Assert(AssetCount == Assets->AssetCount);

    return(Assets);
}

inline void PrefetchBitmap(game_assets* Assets, bitmap_id ID){LoadBitmapAsset(Assets, ID);}
inline void PrefetchSound(game_assets* Assets, sound_id ID){LoadSoundAsset(Assets, ID);}

