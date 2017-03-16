struct load_asset_work{
    task_with_memory* Task;
    asset_slot* Slot;

    platform_file_handle* Handle;
    uint64 Offset;
    uint64 Size;
    void* Destination;
};

INTERNAL_FUNCTION PLATFORM_WORK_QUEUE_CALLBACK(LoadAssetWork){
    load_asset_work* Work = (load_asset_work*)Data;

    Platform.ReadDataFromFile(Work->Handle, Work->Offset, Work->Size, Work->Destination);

    GD_COMPLETE_WRITES_BEFORE_FUTURE;

    if(PlatformNoFileErrors(Work->Handle))
    {
        Work->Slot->State = AssetState_Loaded;
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

INTERNAL_FUNCTION void LoadBitmapAsset(game_assets* Assets, bitmap_id ID){
    if(ID.Value && 
        AtomicCompareExchangeUInt32((uint32 volatile *)&Assets->Slots[ID.Value].State,
            AssetState_Queued,
            AssetState_Unloaded) == AssetState_Unloaded)
    {
        task_with_memory* Task = BeginTaskWithMemory(Assets->TranState);
        if(Task){
            asset* Asset = Assets->Assets + ID.Value;
            dda_bitmap* Info = &Asset->DDA.Bitmap;
            loaded_bitmap* Bitmap = PushStruct(&Assets->Arena, loaded_bitmap);

            Bitmap->AlignPercentage = Vec2(Info->AlignPercentage[0], Info->AlignPercentage[1]);
            Bitmap->WidthOverHeight = (float)Info->Dimension[0] / (float)Info->Dimension[1];
            Bitmap->Width = Info->Dimension[0];
            Bitmap->Height = Info->Dimension[1];
            uint32 MemorySize = Bitmap->Width * Bitmap->Height * 4;
            Bitmap->Memory = PushSize(&Assets->Arena, MemorySize);

            load_asset_work* Work = PushStruct(&Task->Arena, load_asset_work);
            Work->Task = Task;
            Work->Slot = Assets->Slots + ID.Value;
            Work->Handle = GetFileHandleFor(Assets, Asset->FileIndex);
            Work->Offset = Asset->DDA.DataOffset;
            Work->Size = MemorySize;
            Work->Destination = Bitmap->Memory;
            Work->Slot->Bitmap = Bitmap;

            Platform.AddEntry(Assets->TranState->LowPriorityQueue, LoadAssetWork, Work);
        }
        else{
            Assets->Slots[ID.Value].State = AssetState_Unloaded;
        }
    }
}

INTERNAL_FUNCTION void LoadSoundAsset(game_assets* Assets, sound_id ID){
    if(ID.Value &&
        AtomicCompareExchangeUInt32((uint32 volatile *)&Assets->Slots[ID.Value].State,
            AssetState_Queued,
            AssetState_Unloaded) == AssetState_Unloaded)
    {
        task_with_memory* Task = BeginTaskWithMemory(Assets->TranState);
        if(Task){

            asset* Asset = Assets->Assets + ID.Value;
            dda_sound* Info = &Asset->DDA.Sound;

            loaded_sound* Sound = PushStruct(&Assets->Arena, loaded_sound);
            Sound->SampleCount = Info->SampleCount;
            Sound->ChannelCount = Info->ChannelCount;
            uint32 ChannelSize = Sound->SampleCount * sizeof(int16);
            uint32 MemorySize = Sound->ChannelCount * ChannelSize;

            void* Memory = PushSize(&Assets->Arena, MemorySize);

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
            Work->Slot = Assets->Slots + ID.Value;
            Work->Handle = GetFileHandleFor(Assets, Asset->FileIndex);
            Work->Offset = Asset->DDA.DataOffset;
            Work->Size = MemorySize;
            Work->Destination = Memory;
            Work->Slot->Sound = Sound;

            Platform.AddEntry(Assets->TranState->LowPriorityQueue, LoadAssetWork, Work);
        }
        else{
            Assets->Slots[ID.Value].State = AssetState_Unloaded;
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
            real32 D0 = DOLPHIN_MATH_ABS(A - B);
            real32 D1 = DOLPHIN_MATH_ABS((A - Assets->TagRange[Tag->ID] * DOLPHIN_MATH_SIGN(A)) - B);
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
GetRandomSlotFrom(game_assets* Assets, asset_type_id TypeID, random_series* Series)
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
GetFirstSlotFrom(game_assets* Assets, asset_type_id TypeID){
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
    bitmap_id Result = {GetFirstSlotFrom(Assets, TypeID)};
    return(Result);
}

inline bitmap_id
GetRandomBitmapFrom(game_assets* Assets, asset_type_id TypeID, random_series* Series){
    bitmap_id Result = {GetRandomSlotFrom(Assets, TypeID, Series)};
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
    sound_id Result = {GetFirstSlotFrom(Assets, TypeID)};
    return(Result);
}

inline sound_id
GetRandomSoundFrom(game_assets* Assets, asset_type_id TypeID, random_series* Series){
    sound_id Result = {GetRandomSlotFrom(Assets, TypeID, Series)};
    return(Result);
}

INTERNAL_FUNCTION game_assets* 
AllocateGameAssets(memory_arena* Arena, size_t Size, transient_state* TranState){
    game_assets* Assets = PushStruct(Arena, game_assets);
    SubArena(&Assets->Arena, Arena, Size);
    Assets->TranState = TranState;

    for(uint32 TagType = 0;
        TagType < Tag_Count;
        ++TagType)
    {
        Assets->TagRange[TagType] = 1000000.0f;
    }
    Assets->TagRange[Tag_FacingDirection] = DOLPHIN_MATH_TAU;


#if 1
    
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
    Assets->Slots = PushArray(Arena, Assets->AssetCount, asset_slot);
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
#endif

    return(Assets);
}

inline void PrefetchBitmap(game_assets* Assets, bitmap_id ID){LoadBitmapAsset(Assets, ID);}
inline void PrefetchSound(game_assets* Assets, sound_id ID){LoadSoundAsset(Assets, ID);}
