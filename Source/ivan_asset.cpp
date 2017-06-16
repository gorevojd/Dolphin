enum finalize_asset_operation{
    FinalizeAsset_None,
    FinalizeAsset_Font,
    FinalizeAsset_Bitmap,
};

struct load_asset_work{
    task_with_memory* Task;
    asset* Asset;

    platform_file_handle* Handle;
    uint64 Offset;
    uint64 Size;
    void* Destination;

    finalize_asset_operation FinalizeOperation;
    uint32 FinalState;

    platform_texture_op_queue* TextureOpQueue;
};

INTERNAL_FUNCTION void 
AddOp(platform_texture_op_queue* Queue, texture_op* Source){
    BeginTicketMutex(&Queue->Mutex);

    Assert(Queue->FirstFree);
    texture_op* Dest = Queue->FirstFree;
    Queue->FirstFree = Dest->Next;

    *Dest = *Source;
    Assert(Dest->Next == 0);

    if(Queue->Last){
        Queue->Last = Queue->Last->Next = Dest;
    }
    else{
        Queue->First = Queue->Last = Dest;
    }

    EndTicketMutex(&Queue->Mutex);
}

INTERNAL_FUNCTION void LoadAssetWorkDirectly(load_asset_work* Work){
    TIMED_BLOCK();

    Platform.ReadDataFromFile(Work->Handle, Work->Offset, Work->Size, Work->Destination);
    if(PlatformNoFileErrors(Work->Handle)){
        switch(Work->FinalizeOperation){
            case FinalizeAsset_None:{

            }break;

            case FinalizeAsset_Font:{
                loaded_font* Font = &Work->Asset->Header->Font;
                dda_font* DDA = &Work->Asset->DDA.Font;
                for(uint32 GlyphIndex = 1;
                    GlyphIndex < DDA->GlyphCount;
                    GlyphIndex++)
                {
                    dda_font_glyph* Glyph = Font->Glyphs + GlyphIndex;

                    Assert(Glyph->UnicodeCodePoint < DDA->OnePastHighestCodepoint);
                    Assert((uint32)(uint16)GlyphIndex == GlyphIndex);
                    Font->UnicodeMap[Glyph->UnicodeCodePoint] = (uint16)GlyphIndex;
                }
            }break;

            case FinalizeAsset_Bitmap:{
                loaded_bitmap* Bitmap = &Work->Asset->Header->Bitmap;
                texture_op Op = {};
                Op.IsAllocate = true;
                Op.Allocate.Width = Bitmap->Width;
                Op.Allocate.Height = Bitmap->Height;
                Op.Allocate.Data = Bitmap->Memory;
                Op.Allocate.ResultHandle = &Bitmap->TextureHandle;
                AddOp(Work->TextureOpQueue, &Op);
            }break;
        }
    }
	
    GD_COMPLETE_WRITES_BEFORE_FUTURE;

    if(!PlatformNoFileErrors(Work->Handle)){
        ZeroSize(Work->Destination, Work->Size);
    }

    Work->Asset->State = Work->FinalState;
}

INTERNAL_FUNCTION PLATFORM_WORK_QUEUE_CALLBACK(LoadAssetWork){
    load_asset_work* Work = (load_asset_work*)Data;

    LoadAssetWorkDirectly(Work);

    EndTaskWithMemory(Work->Task);
}

inline asset_file* 
GetFile(game_assets* Assets, uint32 FileIndex){
    Assert(FileIndex < Assets->FileCount);
    asset_file* Result = Assets->Files + FileIndex;

    return(Result);
}

inline platform_file_handle*
GetFileHandleFor(game_assets* Assets, uint32 FileIndex){
    Assert(FileIndex < Assets->FileCount);
    platform_file_handle* Result = GetFile(Assets, FileIndex)->Handle;

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

INTERNAL_FUNCTION bool32 GenerationHasCompleted(game_assets* Assets, uint32 CheckID){
    bool32 Result = true;

    for(uint32 Index = 0;
        Index < Assets->InFlightGenerationCount;
        Index++)
    {
        if(Assets->InFlightGenerations[Index] == CheckID){
            Result = false;
            break;
        }
    }

    return(Result);
}

INTERNAL_FUNCTION asset_memory_header*
RequestAssetMemory(game_assets* Assets, uint32 Size, uint32 AssetIndex, asset_header_type AssetType){
    TIMED_BLOCK();

    asset_memory_header* Result = 0;

    BeginAssetLock(Assets);

    asset_memory_block* Block = FindBlockForSize(Assets, Size);
    for(;;){
        if(Block && (Size <= Block->Size)){
            Block->Flags |= AssetMemory_Used;

            Result = (asset_memory_header*)(Block + 1);

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
                if((Asset->State >= AssetState_Loaded) && 
                    (GenerationHasCompleted(Assets, Asset->Header->GenerationID)))
                {
                    uint32 AssetIndex = Header->AssetIndex;
                    asset* Asset = Assets->Assets + AssetIndex;

                    Assert(Asset->State == AssetState_Loaded);

                    RemoveAssetHeaderFromList(Header);

                    if(Asset->Header->AssetType == AssetType_Bitmap){
                        texture_op Op = {};
                        Op.IsAllocate = false;
                        Op.Deallocate.Handle = Asset->Header->Bitmap.TextureHandle;
                        AddOp(Assets->TextureOpQueue, &Op);
                    }

                    Block = (asset_memory_block*)Asset->Header - 1;
                    Block->Flags &= ~AssetMemory_Used;

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

    if(Result){
        Result->AssetType = AssetType;
        Result->AssetIndex = AssetIndex;
        Result->TotalSize = Size;
        InsertAssetHeaderAtFront(Assets, Result);
    }

    EndAssetLock(Assets);

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

INTERNAL_FUNCTION void LoadBitmapAsset(game_assets* Assets, bitmap_id ID, bool32 Immediate){
    TIMED_BLOCK();

    asset* Asset = Assets->Assets + ID.Value;

    if(ID.Value){
        if(AtomicCompareExchangeUInt32((uint32 volatile *)&Assets->Assets[ID.Value].State,
            AssetState_Queued,
            AssetState_Unloaded) == AssetState_Unloaded)
        {
            task_with_memory* Task = 0;

            if(!Immediate){
                Task = BeginTaskWithMemory(Assets->TranState, false);
            }

            if(Immediate || Task){
                dda_bitmap* Info = &Asset->DDA.Bitmap;
                
                asset_memory_size Size = {};
                uint32 Width = Info->Dimension[0];
                uint32 Height = Info->Dimension[1];
                Size.Data = Height * Width * 4;
                Size.Total = Size.Data + sizeof(asset_memory_header);

                Asset->Header = (asset_memory_header*)RequestAssetMemory(Assets, Size.Total, ID.Value, AssetType_Bitmap);

                loaded_bitmap* Bitmap = &Asset->Header->Bitmap;
                Bitmap->AlignPercentage = Vec2(Info->AlignPercentage[0], Info->AlignPercentage[1]);
                Bitmap->WidthOverHeight = (float)Info->Dimension[0] / (float)Info->Dimension[1];
                Bitmap->Width = Info->Dimension[0];
                Bitmap->Height = Info->Dimension[1];
                Bitmap->TextureHandle = 0;
                Bitmap->Memory = (Asset->Header + 1);

                load_asset_work Work;
                Work.Task = Task;
                Work.Asset = Assets->Assets + ID.Value;
                Work.Handle = GetFileHandleFor(Assets, Asset->FileIndex);
                Work.Offset = Asset->DDA.DataOffset;
                Work.Size = Size.Data;
                Work.Destination = Bitmap->Memory;
                Work.FinalizeOperation = FinalizeAsset_Bitmap;
                Work.TextureOpQueue = Assets->TextureOpQueue;

                Work.FinalState = (AssetState_Loaded);

                if(Task){
                    load_asset_work* TaskWork = PushStruct(&Task->Arena, load_asset_work);
                    *TaskWork = Work;
                    Platform.AddEntry(Assets->TranState->LowPriorityQueue, LoadAssetWork, TaskWork);
                }
                else{
                    LoadAssetWorkDirectly(&Work);
                }
            }
            else{
                Asset->State = AssetState_Unloaded;
            }
        }
        else if(Immediate){
            asset_state volatile* State = (asset_state volatile*)&Asset->State;
            while(*State == AssetState_Queued){

            }
        }
    }
}

INTERNAL_FUNCTION void LoadSoundAsset(game_assets* Assets, sound_id ID){
    TIMED_BLOCK();

    if(ID.Value &&
        AtomicCompareExchangeUInt32((uint32 volatile *)&Assets->Assets[ID.Value].State,
            AssetState_Queued,
            AssetState_Unloaded) == AssetState_Unloaded)
    {
        task_with_memory* Task = BeginTaskWithMemory(Assets->TranState, false);
        if(Task){

            asset* Asset = Assets->Assets + ID.Value;
            dda_sound* Info = &Asset->DDA.Sound;

            asset_memory_size Size = {};
            Size.Data = Info->ChannelCount * Info->SampleCount * sizeof(int16);
            Size.Total = Size.Data + sizeof(asset_memory_header);

            Asset->Header = (asset_memory_header*)RequestAssetMemory(Assets, Size.Total, ID.Value, AssetType_Sound);
            loaded_sound* Sound = &Asset->Header->Sound;

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
            Work->FinalizeOperation = FinalizeAsset_None;
            Work->FinalState = AssetState_Loaded;

            AddAssetHeaderToList(Assets, ID.Value, Size);

            Platform.AddEntry(Assets->TranState->LowPriorityQueue, LoadAssetWork, Work);
        }
        else{
            Assets->Assets[ID.Value].State = AssetState_Unloaded;
        }
    }
}

INTERNAL_FUNCTION void
LoadFontAsset(game_assets* Assets, font_id ID, bool32 Immediate){
    TIMED_BLOCK();

    asset* Asset = Assets->Assets + ID.Value;
    if(ID.Value){
        if(AtomicCompareExchangeUInt32(
            (uint32 *)&Asset->State,
            AssetState_Queued, 
            AssetState_Unloaded) == AssetState_Unloaded)
        {
            task_with_memory* Task = 0;

            if(!Immediate){
                Task = BeginTaskWithMemory(Assets->TranState, false);
            }

            if(Immediate || Task){
                dda_font* Info = &Asset->DDA.Font;

                uint32 HorizontalAdvanceSize = sizeof(float) * Info->GlyphCount * Info->GlyphCount;
                uint32 GlyphsSize = Info->GlyphCount * sizeof(dda_font_glyph);
                uint32 UnicodeMapSize = sizeof(uint16) * Info->OnePastHighestCodepoint;
                uint32 SizeOfData = GlyphsSize + HorizontalAdvanceSize;
                uint32 SizeTotal = SizeOfData + sizeof(asset_memory_header) + UnicodeMapSize;

                Asset->Header = RequestAssetMemory(Assets, SizeTotal, ID.Value, AssetType_Font);

                loaded_font* Font = &Asset->Header->Font;
                Font->BitmapIDOffset = GetFile(Assets, Asset->FileIndex)->FontBitmapIDOffset;
                Font->Glyphs = (dda_font_glyph*)(Asset->Header + 1);
                Font->HorizontalAdvance = (float*)((uint8*)Font->Glyphs + GlyphsSize);
                Font->UnicodeMap = (uint16*)((uint8*)Font->HorizontalAdvance + HorizontalAdvanceSize);

                ZeroSize(Font->UnicodeMap, UnicodeMapSize);

                load_asset_work Work;
                Work.Task = Task;
                Work.Asset = Assets->Assets + ID.Value;
                Work.Handle = GetFileHandleFor(Assets, Asset->FileIndex);
                Work.Offset = Asset->DDA.DataOffset;
                Work.Size = SizeOfData;
                Work.Destination = Font->Glyphs;
                Work.FinalizeOperation = FinalizeAsset_Font;
                Work.FinalState = AssetState_Loaded;
                if(Task){
                    load_asset_work* TaskWork = PushStruct(&Task->Arena, load_asset_work);
                    *TaskWork = Work;
                    Platform.AddEntry(Assets->TranState->LowPriorityQueue, LoadAssetWork, TaskWork);
                }
                else{
                    LoadAssetWorkDirectly(&Work);
                }

                int a = 1;
            }
            else{
                Asset->State = AssetState_Unloaded;
            }

        }
        else if(Immediate){
            asset_state volatile* State = (asset_state volatile*)&Asset->State;
            while(*State == AssetState_Queued){

            }
        }
    }
}

INTERNAL_FUNCTION void
LoadVoxelAtlasAsset(game_assets* Assets, voxel_atlas_id ID, bool32 Immediate){
    TIMED_BLOCK();

    asset* Asset = Assets->Assets + ID.Value;
    if(ID.Value){
        if(AtomicCompareExchangeUInt32(
            (uint32 *)&Asset->State,
            AssetState_Queued, 
            AssetState_Unloaded) == AssetState_Unloaded)
        {
            task_with_memory* Task = 0;

            if(!Immediate){
                Task = BeginTaskWithMemory(Assets->TranState, false);
            }

            if(Immediate || Task){
                dda_voxel_atlas* Info = &Asset->DDA.VoxelAtlas;

                uint32 MaterialsSize = VoxelMaterial_Count * sizeof(voxel_tex_coords_set);
                uint32 SizeOfData = MaterialsSize;
                uint32 SizeTotal = SizeOfData + sizeof(asset_memory_header);

                Asset->Header = RequestAssetMemory(Assets, SizeTotal, ID.Value, AssetType_VoxelAtlas);

                loaded_voxel_atlas* Atlas = &Asset->Header->VoxelAtlas;
                Atlas->BitmapIDOffset = GetFile(Assets, Asset->FileIndex)->VoxelAtlasBitmapIDOffset;
                Atlas->Materials = (voxel_tex_coords_set*)(Asset->Header + 1);

                load_asset_work Work;
                Work.Task = Task;
                Work.Asset = Assets->Assets + ID.Value;
                Work.Handle = GetFileHandleFor(Assets, Asset->FileIndex);
                Work.Offset = Asset->DDA.DataOffset;
                Work.Size = SizeOfData;
                Work.Destination = Atlas->Materials;
                Work.FinalizeOperation = FinalizeAsset_None;
                Work.FinalState = AssetState_Loaded;

                if(Task){
                    load_asset_work* TaskWork = PushStruct(&Task->Arena, load_asset_work);
                    *TaskWork = Work;
                    Platform.AddEntry(Assets->TranState->LowPriorityQueue, LoadAssetWork, TaskWork);
                }
                else{
                    LoadAssetWorkDirectly(&Work);
                }
            }
            else{
                Asset->State = AssetState_Unloaded;
            }

        }
        else if(Immediate){
            asset_state volatile* State = (asset_state volatile*)&Asset->State;
            while(*State == AssetState_Queued){

            }
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
    TIMED_BLOCK();

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
    TIMED_BLOCK();

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
    TIMED_BLOCK();

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

inline font_id
GetFirstFontFrom(game_assets* Assets, asset_type_id TypeID){
    font_id Result = {GetFirstAssetFrom(Assets, TypeID)};
    return(Result);
}

inline font_id
GetRandomFontFrom(game_assets* Assets, asset_type_id TypeID, random_series* Series){
    font_id Result = {GetRandomAssetFrom(Assets, TypeID, Series)};
    return(Result);
}

inline font_id
GetBestMatchFontFrom(
    game_assets* Assets,
    asset_type_id TypeID,
    asset_vector* MatchVector, asset_vector* WeightVector)
{
    font_id Result = {GetBestMatchAssetFrom(Assets, TypeID, MatchVector, WeightVector)};
    return(Result);
}


INTERNAL_FUNCTION game_assets* 
AllocateGameAssets(
    memory_arena* Arena,
    size_t Size, 
    transient_state* TranState,
    platform_texture_op_queue* TextureOpQueue)
{
    game_assets* Assets = PushStruct(Arena, game_assets);
    Assets->TranState = TranState;
    Assets->TextureOpQueue = TextureOpQueue;

    Assets->NextGenerationID = 0;
    Assets->InFlightGenerationCount = 0;

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

        File->FontBitmapIDOffset = 0;
        File->VoxelAtlasBitmapIDOffset = 0;
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

                        if(SourceType->TypeID == Asset_FontGlyph){
                            File->FontBitmapIDOffset = AssetCount - SourceType->FirstAssetIndex;
                        }

                        if(SourceType->TypeID == Asset_VoxelAtlasTexture){
                            File->VoxelAtlasBitmapIDOffset = AssetCount - SourceType->FirstAssetIndex;
                        }

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

inline void PrefetchBitmap(game_assets* Assets, bitmap_id ID){LoadBitmapAsset(Assets, ID, false);}
inline void PrefetchSound(game_assets* Assets, sound_id ID){LoadSoundAsset(Assets, ID);}
inline void PrefetchFont(game_assets* Assets, font_id ID){LoadFontAsset(Assets, ID, false);}

inline uint32 GetGlyphFromCodePoint(dda_font* Info, loaded_font* Font, uint32 CodePoint){
    uint32 Result = 0;
    if(CodePoint < Info->OnePastHighestCodepoint){
        Result = Font->UnicodeMap[CodePoint];
        Assert(Result < Info->GlyphCount);
    }

    return(Result);
}

INTERNAL_FUNCTION float 
GetHorizontalAdvanceForPair(dda_font* Info, loaded_font* Font, uint32 PrevCodePoint, uint32 NextCodePoint){
    uint32 PrevGlyph = GetGlyphFromCodePoint(Info, Font, PrevCodePoint);
    uint32 NextGlyph = GetGlyphFromCodePoint(Info, Font, NextCodePoint);

    float Result = Font->HorizontalAdvance[PrevGlyph * Info->GlyphCount + NextGlyph];

    return(Result);
}

INTERNAL_FUNCTION bitmap_id
GetBitmapForGlyph(game_assets* Assets, dda_font* Info, loaded_font* Font, uint32 CodePoint){
    uint32 Glyph = GetGlyphFromCodePoint(Info, Font, CodePoint);
    bitmap_id Result = Font->Glyphs[Glyph].BitmapID;
    Result.Value += Font->BitmapIDOffset;

    return(Result);
}

INTERNAL_FUNCTION float 
GetLineAdvanceFor(dda_font* Info){
    float Result = Info->AscenderHeight + Info->DescenderHeight + Info->ExternalLeading;

    return(Result);
}

INTERNAL_FUNCTION float 
GetStartingBaselineY(dda_font* Info){
    float Result = Info->AscenderHeight;

    return(Result);
}