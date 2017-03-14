struct load_bitmap_work{
    game_assets* Assets;
    bitmap_id ID;
    task_with_memory* Task;
    loaded_bitmap* Bitmap;
};

INTERNAL_FUNCTION PLATFORM_WORK_QUEUE_CALLBACK(LoadBitmapWork){
    load_bitmap_work* Work = (load_bitmap_work*)Data;

    dda_asset* DDAAsset = &Work->Assets->Assets[Work->ID.Value];
    dda_bitmap* Info = &DDAAsset->Bitmap;
    loaded_bitmap* Bitmap = Work->Bitmap;

    Bitmap->AlignPercentage = Vec2(Info->AlignPercentage[0], Info->AlignPercentage[1]);
    Bitmap->WidthOverHeight = (float)Info->Dimension[0] / (float)Info->Dimension[1];
    Bitmap->Width = Info->Dimension[0];
    Bitmap->Height = Info->Dimension[1];
    Bitmap->Memory = Work->Assets->DDAContents + DDAAsset->DataOffset;

    GD_COMPLETE_WRITES_BEFORE_FUTURE;

    Work->Assets->Slots[Work->ID.Value].State = AssetState_Loaded;
    Work->Assets->Slots[Work->ID.Value].Bitmap = Work->Bitmap;
    EndTaskWithMemory(Work->Task);
}

INTERNAL_FUNCTION void LoadBitmapAsset(game_assets* Assets, bitmap_id ID){
    if(ID.Value && 
        AtomicCompareExchangeUInt32((uint32 volatile *)&Assets->Slots[ID.Value].State,
            AssetState_Queued,
            AssetState_Unloaded) == AssetState_Unloaded)
    {
        task_with_memory* Task = BeginTaskWithMemory(Assets->TranState);
        if(Task){
            load_bitmap_work *Work = PushStruct(&Task->Arena, load_bitmap_work);

            Work->Assets = Assets;
            Work->ID = ID;
            Work->Task = Task;
            Work->Bitmap = PushStruct(&Assets->Arena, loaded_bitmap);

            PlatformAddEntry(Assets->TranState->LowPriorityQueue, LoadBitmapWork, Work);
        }
        else{
            Assets->Slots[ID.Value].State = AssetState_Unloaded;
        }
    }
}

struct load_sound_work
{
    game_assets* Assets;
    sound_id ID;
    task_with_memory* Task;
    loaded_sound* Sound;
};

INTERNAL_FUNCTION PLATFORM_WORK_QUEUE_CALLBACK(LoadSoundWork){
    load_sound_work* Work = (load_sound_work*)Data;

    dda_asset* DDAAsset = &Work->Assets->Assets[Work->ID.Value];
    dda_sound* Info = &DDAAsset->Sound;
    loaded_sound* Sound = Work->Sound;

    Sound->SampleCount = Info->SampleCount;
    Sound->ChannelCount = Info->ChannelCount;
    Assert(Sound->ChannelCount <= ArrayCount(Sound->Samples));
    uint64 SampleDataOffset = DDAAsset->DataOffset;
    for(uint32 ChannelIndex = 0;
        ChannelIndex < Sound->ChannelCount;
        ChannelIndex++)
    {
        Sound->Samples[ChannelIndex] = (int16*)(Work->Assets->DDAContents + SampleDataOffset);
        SampleDataOffset += Sound->SampleCount * sizeof(int16);
    }

    GD_COMPLETE_WRITES_BEFORE_FUTURE;

    Work->Assets->Slots[Work->ID.Value].Sound = Work->Sound;
    Work->Assets->Slots[Work->ID.Value].State = AssetState_Loaded;

    EndTaskWithMemory(Work->Task);
}

INTERNAL_FUNCTION void LoadSoundAsset(game_assets* Assets, sound_id ID){
    if(ID.Value &&
        AtomicCompareExchangeUInt32((uint32 volatile *)&Assets->Slots[ID.Value].State,
            AssetState_Queued,
            AssetState_Unloaded) == AssetState_Unloaded)
    {
        task_with_memory* Task = BeginTaskWithMemory(Assets->TranState);
        if(Task){
            load_sound_work* Work = PushStruct(&Task->Arena, load_sound_work);

            Work->Assets = Assets;
            Work->ID = ID;
            Work->Task = Task;
            Work->Sound = PushStruct(&Assets->Arena, loaded_sound);

            PlatformAddEntry(Assets->TranState->LowPriorityQueue, LoadSoundWork, Work);
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
        dda_asset* Asset = Assets->Assets + AssetIndex;

        real32 TotalWeightedDiff = 0.0;
        for(uint32 TagIndex = Asset->FirstTagIndex;
            TagIndex < Asset->OnePastLastTagIndex;
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

    Assets->AssetCount = 2 * 256 * Asset_Count;
    Assets->Assets = PushArray(Arena, Assets->AssetCount, dda_asset);
    Assets->Slots = PushArray(Arena, Assets->AssetCount, asset_slot);

    Assets->TagCount = 1024 * Asset_Count;
    Assets->Tags = PushArray(Arena, Assets->TagCount, dda_tag);

    debug_read_file_result ReadResult = PlatformReadEntireFile("../Data/test.dda");
    if(ReadResult.ContentsSize != 0){
        dda_header* Header = (dda_header*)ReadResult.Contents;
        Assert(Header->MagicValue == DDA_MAGIC_VALUE);
        Assert(Header->Version == DDA_VERSION);

        Assets->AssetCount = Header->AssetCount;
        Assets->Assets = (dda_asset*)((uint8*)ReadResult.Contents + Header->AssetOffset);
        Assets->Slots = PushArray(Arena, Assets->AssetCount, asset_slot);

        Assets->TagCount = Header->TagCount;
        Assets->Tags = (dda_tag*)((uint8*)ReadResult.Contents + Header->TagOffset);

        dda_asset_type* DDATypes = (dda_asset_type*)((uint8*)ReadResult.Contents + Header->AssetTypeOffset);
        for(uint32 TypeIndex = 0;
            TypeIndex < Asset_Count;
            TypeIndex++)
        {
            dda_asset_type* Source = DDATypes + TypeIndex;

            if(Source->TypeID < Asset_Count){
                asset_type* Dest = Assets->AssetTypes + Source->TypeID;

                Dest->FirstAssetIndex = Source->FirstAssetIndex;
                Dest->OnePastLastAssetIndex = Source->OnePastLastAssetIndex;
            }
        }

        Assets->DDAContents = (uint8*)ReadResult.Contents;
    }

    return(Assets);
}

inline void PrefetchBitmap(game_assets* Assets, bitmap_id ID){LoadBitmapAsset(Assets, ID);}
inline void PrefetchSound(game_assets* Assets, sound_id ID){LoadSoundAsset(Assets, ID);}
