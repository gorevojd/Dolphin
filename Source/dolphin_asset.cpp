#pragma pack(push, 1)
struct bitmap_header{
    uint16 FileType; /*File type, always 4d42 ("BM")*/
    uint32 FileSize; /*Size of the file in bytes*/
    uint16 Reserved1; /*Always 0*/
    uint16 Reserved2; /*Always 0*/
    uint32 BitmapOffset; /*Starting position of image data in bytes*/

    uint32 Size; /*Size of header in bytes*/
    int32 Width; /*Image width in pixels*/
    int32 Height; /*Image height in pixels*/
    uint16 Planes; /*Number of color planes*/
    uint16 BitsPerPixel; /*Number of bits per pixel*/

    uint32 Compression; /*Compression methods used*/
    uint32 SizeOfBitmap; /*Size of bitmap in bytes*/
    int32 HorzResolution;/*Horizontal resolution in pixels per meter*/
    int32 VertResolution;/*Vertical resolution in pixels per meter*/
    uint32 ColorsUsed;/*Number of colors in the image*/
    uint32 ColorsImportant;/*Minimum number of important colors*/

    uint32 RedMask; /*Mask identifying bits of red component*/
    uint32 GreenMask; /*Mask identifying bits of green component*/
    uint32 BlueMask; /*Mask identifying bits of blue component*/
    uint32 AlphaMask; /*Mask identifying bits of Alpha component*/
    uint32 CSType; /*Color space type*/
    int32 RedX; /*X coordinate of red endpoint*/
    int32 RedY; /*Y coordinate of red endpoint*/
    int32 RedZ; /*Z coordinate of red endpoint*/
    int32 GreenX; /*X coordinate of green endpoint*/
    int32 GreenY; /*Y coordinate of green endpoint*/
    int32 GreenZ; /*Z coordinate of green endpoint*/
    int32 BlueX; /*X coordinate of blue endpoint*/
    int32 BlueY; /*Y coordinate of blue endpoint*/
    int32 BlueZ; /*Z coordinate of blue endpoint*/
    uint32 GammaRed; /*Gamma red coordinate scale value*/
    uint32 GammaGreen; /*Gamma green coordinate scale value*/
    uint32 GammaBlue; /*Gamma blue coordinate scale value*/
};

struct WAVE_header{
    uint32 RIFFID;
    uint32 Size;
    uint32 WAVEID;
};

#define RIFF_CODE(a, b, c, d) (((uint32)(a) << 0) | ((uint32)(b) << 8) | ((uint32)(c) << 16) | ((uint32)(d) << 24))
enum{
    WAVE_ChunkID_fmt = RIFF_CODE('f', 'm', 't', ' '),
    WAVE_ChunkID_RIFF = RIFF_CODE('R', 'I', 'F', 'F'),
    WAVE_ChunkID_WAVE = RIFF_CODE('W', 'A', 'V', 'E'),
    WAVE_ChunkID_data = RIFF_CODE('d', 'a', 't', 'a'),
};

struct WAVE_chunk{
    uint32 Id;
    uint32 Size;
};

struct WAVE_fmt{
    uint16 wFormatTag;
    uint16 nChannels;
    uint32 nSamplesPerSec;
    uint32 nAvgBytesPerSec;
    uint16 nBlockAlign;
    uint16 wBitsPerSample;
    uint16 cbSize;
    uint16 wValidBitsPerSample;
    uint32 dwChannelMask;
    uint8 SubFormat[16];
};
#pragma pack(pop)

struct riff_iterator{
    uint8* At;
    uint8* Stop;
};

inline riff_iterator ParseChunkAt(void* At, void* Stop){
    riff_iterator Iter;

    Iter.At = (uint8*)At;
    Iter.Stop = (uint8*)Stop;

    return(Iter);
}

inline riff_iterator
NextChunk(riff_iterator Iter){
    WAVE_chunk* Chunk = (WAVE_chunk*)Iter.At;
    uint32 Size = (Chunk->Size + 1) & ~1;
    Iter.At += sizeof(WAVE_chunk) + Size;

    return(Iter);
}

inline bool32
IsValid(riff_iterator Iter){
    bool32 Result = (Iter.At < Iter.Stop);

    return(Result);
}

inline void*
GetChunkData(riff_iterator Iter){
    void* Result = (Iter.At + sizeof(WAVE_chunk));

    return(Result);
}

inline uint32
GetType(riff_iterator Iter){
    WAVE_chunk* Chunk = (WAVE_chunk*)Iter.At;
    uint32 Result = Chunk->Id;

    return(Result);
}

inline uint32
GetChunkDataSize(riff_iterator Iter){
    WAVE_chunk* Chunk = (WAVE_chunk*)Iter.At;
    uint32 Result = Chunk->Size;

    return(Result);
}

INTERNAL_FUNCTION loaded_sound
LoadWAV(char* FileName, uint32 SectionFirstSampleIndex, uint32 SectionSampleCount){
    loaded_sound Result = {};

    debug_read_file_result ReadResult = PlatformReadEntireFile(FileName);
    if(ReadResult.ContentsSize != 0){
        WAVE_header* Header = (WAVE_header*)ReadResult.Contents;
        Assert(Header->RIFFID  == WAVE_ChunkID_RIFF);
        Assert(Header->WAVEID == WAVE_ChunkID_WAVE);

        int16* SampleData = 0;
        uint32 ChannelCount = 0;
        uint32 SampleDataSize = 0;

        for(riff_iterator Iter = ParseChunkAt(Header + 1, (uint8*)(Header + 1) + Header->Size - 4);
            IsValid(Iter);
            Iter = NextChunk(Iter))
        {
            switch(GetType(Iter)){

                case WAVE_ChunkID_fmt:{
                    WAVE_fmt* fmt = (WAVE_fmt*)GetChunkData(Iter);
                    ChannelCount = fmt->nChannels;
                }break;

                case WAVE_ChunkID_data:{
                    SampleData = (int16 *)GetChunkData(Iter);
                    SampleDataSize = GetChunkDataSize(Iter);
                }break;
            }
        }

        Assert(ChannelCount && SampleData);

        Result.ChannelCount = ChannelCount;
        uint32 SampleCount = SampleDataSize / (ChannelCount * sizeof(int16));
        
        if(ChannelCount == 1){
            Result.Samples[0] = SampleData;
            Result.Samples[1] = 0;
        }
        else if(ChannelCount == 2){
            Result.Samples[0] = SampleData;
            Result.Samples[1] = SampleData + Result.SampleCount;

            for(uint32 SampleIndex = 0;
                SampleIndex < Result.SampleCount;
                SampleIndex++)
            {
                int16 Source = SampleData[2 * SampleIndex];
                SampleData[2 * SampleIndex] = SampleData[SampleIndex];
                SampleData[SampleIndex] = Source;
            }
        }
        else{
            Assert(!"Invalid channel count in WAV file");
        }

        Result.ChannelCount = 1;

        bool32 AtEnd = true;

        if(SectionSampleCount){
            Assert((SectionFirstSampleIndex + SectionSampleCount) <= SampleCount);
            AtEnd = ((SectionFirstSampleIndex + SectionSampleCount) == SampleCount);
            SampleCount = SectionSampleCount;
            for(uint32 ChannelIndex = 0;
                ChannelIndex < Result.ChannelCount;
                ChannelIndex++)
            {
                Result.Samples[ChannelIndex] += SectionFirstSampleIndex;
            }
        }

/*        if(AtEnd){
            for(uint32 ChannelIndex = 0;
                ChannelIndex < Result.ChannelCount;
                ChannelIndex++)
            {
                for(uint32 SampleIndex = SampleCount;
                    SampleIndex < (SampleCount + 8);
                    SampleIndex++)
                {
                    Result.Samples[ChannelIndex][SampleIndex] = 0;
                }
            }
        }
*/


        Result.SampleCount = SampleCount;
    }

    return(Result);
}

inline gdVec2 TopDownAlign(loaded_bitmap* Bitmap, gdVec2 Align){
    Align.y = (real32)(Bitmap->Height - 1) - Align.y;

    Align.x = SafeRatio0(Align.x, (real32)Bitmap->Width);
    Align.y = SafeRatio0(Align.y, (real32)Bitmap->Height);

    return(Align);
}

inline void SetTopDownAlign(hero_bitmaps* Bitmap, gdVec2 Align){
    Align = TopDownAlign(&Bitmap->Head, Align);
    Bitmap->Head.AlignPercentage = Align;
    Bitmap->Cape.AlignPercentage = Align;
    Bitmap->Torso.AlignPercentage = Align;
}

#if 0
INTERNAL_FUNCTION loaded_bitmap
LoadBitmap(debug_platform_read_entire_file* ReadEntireFile, char* FileName, int32 AlignX, int32 TopDownAlignY){

    loaded_bitmap Result = {};

    debug_read_file_result ReadResult = ReadEntireFile(FileName);
    if (ReadResult.ContentsSize != 0){
        bitmap_header* Header = (bitmap_header*)ReadResult.Contents;
        uint32* Pixels = (uint32*)((uint8*)ReadResult.Contents + Header->BitmapOffset);
        Result.Width = Header->Width;
        Result.Height = Header->Height;
        Result.Memory = Pixels;
        Result.AlignPercentage = TopDownAlign(&Result, gd_vec2(AlignX, TopDownAlignY));
        Result.WidthOverHeight = SafeRatio0((real32)Result.Width, (real32)Result.Height);

        uint32 RedMask = Header->RedMask;
        uint32 GreenMask = Header->GreenMask;
        uint32 BlueMask = Header->BlueMask;
        uint32 AlphaMask = ~(RedMask | GreenMask | BlueMask);

        bit_scan_result RedShift = FindLeastSignificantSetBit(RedMask);
        bit_scan_result GreenShift = FindLeastSignificantSetBit(GreenMask);
        bit_scan_result BlueShift = FindLeastSignificantSetBit(BlueMask);
        bit_scan_result AlphaShift = FindLeastSignificantSetBit(AlphaMask);

        Assert(Header->Compression == 3);

        Assert(RedShift.Found);
        Assert(GreenShift.Found);
        Assert(BlueShift.Found);
        Assert(AlphaShift.Found);


        uint32* Pixel = Pixels;
        for (int j = 0; j < Result.Height; j++){
            for (int i = 0; i < Result.Width; i++){
                uint32 SrcPixel = *Pixel;

                gdVec4 Color = {
                    (real32)(((SrcPixel >> RedShift.Index) & 0xFF)),
                    (real32)(((SrcPixel >> GreenShift.Index) & 0xFF)),
                    (real32)(((SrcPixel >> BlueShift.Index) & 0xFF)),
                    (real32)(((SrcPixel >> AlphaShift.Index) & 0xFF)) };

#if 1
                Color = SRGB255ToLinear1(Color);
                //Premultiplied Alpha

                real32 Alpha = Color.a;
                Color.r = Color.r * Alpha;
                Color.g = Color.g * Alpha;
                Color.b = Color.b * Alpha;
                Color = Linear1ToSRGB255(Color);
#endif

                *Pixel++ = (((uint32)(Color.a + 0.5f) << 24) |
                    ((uint32)(Color.r + 0.5f) << 16) |
                    ((uint32)(Color.g + 0.5f) << 8) |
                    ((uint32)(Color.b + 0.5f) << 0));
            }
        }

    }
    return Result;
}

#else

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include "stb_image.h"

INTERNAL_FUNCTION loaded_bitmap
LoadBMP(char* FileName, gdVec2 AlignPercentage = gd_vec2(0.0f, 0.0f), bool32 FlipOnLoad = true){
    loaded_bitmap Result = {};

    stbi_set_flip_vertically_on_load(FlipOnLoad);

    debug_read_file_result ReadResult = PlatformReadEntireFile(FileName);

    Result.Memory = stbi_load_from_memory(
        (stbi_uc*)ReadResult.Contents,
        ReadResult.ContentsSize,
        &Result.Width,
        &Result.Height,
        0,
        STBI_rgb_alpha);
    Result.AlignPercentage = AlignPercentage;
    Result.WidthOverHeight = SafeRatio0((real32)Result.Width, (real32)Result.Height);

    uint32* Pixel = (uint32*)Result.Memory;
    for (int j = 0; j < Result.Height; j++){
        for (int i = 0; i < Result.Width; i++){
            uint32 SrcPixel = *Pixel;

            gdVec4 Color = {
                (SrcPixel & 0xFF),
                ((SrcPixel >> 8) & 0xFF),
                ((SrcPixel >> 16) & 0xFF),
                ((SrcPixel >> 24) & 0xFF) };

#if 1
            Color = SRGB255ToLinear1(Color);
            //Premultiplied Alpha

            real32 Alpha = Color.a;
            Color.r = Color.r * Alpha;
            Color.g = Color.g * Alpha;
            Color.b = Color.b * Alpha;
            Color = Linear1ToSRGB255(Color);
#endif

            *Pixel++ = (((uint32)(Color.a + 0.5f) << 24) |
                ((uint32)(Color.r + 0.5f) << 16) |
                ((uint32)(Color.g + 0.5f) << 8) |
                ((uint32)(Color.b + 0.5f) << 0));
        }
    }

    return(Result);
}

INTERNAL_FUNCTION loaded_bitmap
LoadBMP(char* FileName, bool32 FlipOnLoad = true){
    loaded_bitmap Result = LoadBMP(FileName, gd_vec2(0.0f, 0.0f), FlipOnLoad);
    Result.AlignPercentage = gd_vec2(0.0f, 0.0f);
    return(Result);
}
#endif


struct load_bitmap_work{
    game_assets* Assets;
    bitmap_id ID;
    task_with_memory* Task;
    loaded_bitmap* Bitmap;
};

INTERNAL_FUNCTION PLATFORM_WORK_QUEUE_CALLBACK(LoadBitmapWork){
    load_bitmap_work* Work = (load_bitmap_work*)Data;

    asset_bitmap_info* Info = Work->Assets->BitmapInfos + Work->ID.Value;
    *Work->Bitmap = LoadBMP(Info->FileName, Info->AlignPercentage);

    GD_COMPLETE_WRITES_BEFORE_FUTURE;

    Work->Assets->Bitmaps[Work->ID.Value].State = AssetState_Loaded;
    Work->Assets->Bitmaps[Work->ID.Value].Bitmap = Work->Bitmap;
    EndTaskWithMemory(Work->Task);
}

INTERNAL_FUNCTION void LoadBitmapAsset(game_assets* Assets, bitmap_id ID){
    if(ID.Value && 
        AtomicCompareExchangeUInt32((uint32*)&Assets->Bitmaps[ID.Value].State,
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
            Assets->Bitmaps[ID.Value].State = AssetState_Unloaded;
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

    asset_sound_info* Info = Work->Assets->SoundInfos + Work->ID.Value;
    *Work->Sound = LoadWAV(Info->FileName, Info->FirstSampleIndex, Info->SampleCount);

    GD_COMPLETE_WRITES_BEFORE_FUTURE;

    Work->Assets->Sounds[Work->ID.Value].Sound = Work->Sound;
    Work->Assets->Sounds[Work->ID.Value].State = AssetState_Loaded;

    EndTaskWithMemory(Work->Task);
}

INTERNAL_FUNCTION void LoadSoundAsset(game_assets* Assets, sound_id ID){
    if(ID.Value &&
        AtomicCompareExchangeUInt32((uint32*)&Assets->Sounds[ID.Value].State,
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
            Assets->Sounds[ID.Value].State = AssetState_Unloaded;
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
        for(uint32 TagIndex = Asset->FirstTagIndex;
            TagIndex < Asset->OnePastLastTagIndex;
            TagIndex++)
        {
            asset_tag* Tag = Assets->Tags + TagIndex;

            real32 A = MatchVector->Data[Tag->ID];
            real32 B = Tag->Value;
            real32 D0 = GD_ABS(A - B);
            real32 D1 = GD_ABS((A - Assets->TagRange[Tag->ID] * GD_SIGN(A)) - B);
            real32 Diffrence = GD_MIN(D0, D1);

            real32 Weighted = WeightVector->Data[Tag->ID] * Diffrence;
            TotalWeightedDiff += Weighted;
        }

        if(BestDiff > TotalWeightedDiff){
            BestDiff = TotalWeightedDiff;
            Result = Asset->SlotID;
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

        asset* Asset = Assets->Assets + Type->FirstAssetIndex + Choice;
        Result = Asset->SlotID;
    }

    return(Result);
}

INTERNAL_FUNCTION uint32
GetFirstSlotFrom(game_assets* Assets, asset_type_id TypeID){
    uint32 Result = 0;

    asset_type* Type = Assets->AssetTypes + TypeID;
    if(Type->FirstAssetIndex != Type->OnePastLastAssetIndex){
        asset* Asset = Assets->Assets + Type->FirstAssetIndex;
        Result = Asset->SlotID;
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

INTERNAL_FUNCTION bitmap_id
DEBUGAddBitmapInfo(game_assets* Assets, char* FileName, gdVec2 AlignPercentage){
    Assert(Assets->DEBUGUsedBitmapCount < Assets->BitmapCount);
    bitmap_id ID = {Assets->DEBUGUsedBitmapCount++};

    asset_bitmap_info* Info = Assets->BitmapInfos + ID.Value;
    Info->FileName = PushString(&Assets->Arena, FileName);
    Info->AlignPercentage = AlignPercentage;

    return(ID);
}

INTERNAL_FUNCTION sound_id
DEBUGAddSoundInfo(game_assets* Assets, char* FileName, uint32 FirstSampleIndex, uint32 SampleCount){
    Assert(Assets->DEBUGUsedSoundCount < Assets->SoundCount);
    sound_id ID = {Assets->DEBUGUsedSoundCount++};

    asset_sound_info* Info = Assets->SoundInfos + ID.Value;
    Info->FileName = PushString(&Assets->Arena, FileName);
    Info->NextIDToPlay.Value = 0;
    Info->FirstSampleIndex = FirstSampleIndex;
    Info->SampleCount = SampleCount;

    return(ID);
}

INTERNAL_FUNCTION void 
BeginAssetType(game_assets* Assets, asset_type_id TypeID){
    Assert(Assets->DEBUGAssetType == 0);

    Assets->DEBUGAssetType = Assets->AssetTypes + TypeID;
    Assets->DEBUGAssetType->FirstAssetIndex = Assets->DEBUGUsedAssetCount;
    Assets->DEBUGAssetType->OnePastLastAssetIndex = Assets->DEBUGAssetType->FirstAssetIndex;
}

INTERNAL_FUNCTION void
AddBitmapAsset(game_assets* Assets, char* FileName, gdVec2 AlignPercentage = gd_vec2(0.0f, 0.0f)){
    Assert(Assets->DEBUGAssetType);
    Assert(Assets->DEBUGAssetType->OnePastLastAssetIndex < Assets->AssetCount);

    asset* Asset = Assets->Assets + Assets->DEBUGAssetType->OnePastLastAssetIndex++;
    Asset->FirstTagIndex = Assets->DEBUGUsedTagCount;
    Asset->OnePastLastTagIndex = Asset->FirstTagIndex;
    Asset->SlotID = DEBUGAddBitmapInfo(Assets, FileName, AlignPercentage).Value;

    Assets->DEBUGAsset = Asset;
}

INTERNAL_FUNCTION asset*
AddSoundAsset(game_assets* Assets, char* FileName, uint32 FirstSampleIndex = 0, uint32 SampleCount = 0){
    Assert(Assets->DEBUGAssetType);
    Assert(Assets->DEBUGAssetType->OnePastLastAssetIndex < Assets->AssetCount);

    asset* Asset = Assets->Assets + Assets->DEBUGAssetType->OnePastLastAssetIndex++;
    Asset->FirstTagIndex = Assets->DEBUGUsedTagCount;
    Asset->OnePastLastTagIndex = Asset->FirstTagIndex;
    Asset->SlotID = DEBUGAddSoundInfo(Assets, FileName, FirstSampleIndex, SampleCount).Value;

    Assets->DEBUGAsset = Asset;

    return(Asset);
}

INTERNAL_FUNCTION void
AddTag(game_assets* Assets, asset_tag_id ID, real32 Value){
    Assert(Assets->DEBUGAsset);

    ++Assets->DEBUGAsset->OnePastLastTagIndex;
    asset_tag* Tag = Assets->Tags + Assets->DEBUGUsedTagCount++;

    Tag->ID = ID;
    Tag->Value = Value;
}

INTERNAL_FUNCTION void
EndAssetType(game_assets* Assets){
    Assert(Assets->DEBUGAssetType);

    Assets->DEBUGUsedAssetCount = Assets->DEBUGAssetType->OnePastLastAssetIndex;
    Assets->DEBUGAssetType = 0;
    Assets->DEBUGAsset = 0;
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
    Assets->TagRange[Tag_FacingDirection] = GD_MATH_TAU;

    Assets->BitmapCount = Asset_Count * 256;
    Assets->BitmapInfos = PushArray(Arena, Assets->BitmapCount, asset_bitmap_info);
    Assets->Bitmaps = PushArray(Arena, Assets->BitmapCount, asset_slot);

    Assets->SoundCount = Asset_Count * 256;
    Assets->SoundInfos = PushArray(Arena, Assets->SoundCount, asset_sound_info);
    Assets->Sounds = PushArray(Arena, Assets->SoundCount, asset_slot);

    Assets->AssetCount = Assets->BitmapCount + Assets->SoundCount;
    Assets->Assets = PushArray(Arena, Assets->AssetCount, asset);

    Assets->TagCount = 1024 * Asset_Count;
    Assets->Tags = PushArray(Arena, Assets->TagCount, asset_tag);

    Assets->DEBUGUsedBitmapCount = 1;
    Assets->DEBUGUsedSoundCount = 1;
    Assets->DEBUGUsedAssetCount = 1;

    BeginAssetType(Assets, Asset_Backdrop);
    AddBitmapAsset(Assets, "../Data/HH/test/test_background.bmp");
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_LastOfUs);
    AddBitmapAsset(Assets, "../Data/Images/last.jpg");
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Tree);
    AddBitmapAsset(Assets, "../Data/HH/test2/tree00.bmp");
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_StarWars);
    AddBitmapAsset(Assets, "../Data/Images/star_wars.jpg");
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Witcher);
    AddBitmapAsset(Assets, "../Data/Images/witcher.jpg");
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Assassin);
    AddBitmapAsset(Assets, "../Data/Images/assassin.jpg");
    EndAssetType(Assets);

    real32 AngleRight = 0.0f * GD_MATH_TAU;
    real32 AngleBack = 0.25f * GD_MATH_TAU;
    real32 AngleLeft = 0.5f * GD_MATH_TAU;
    real32 AngleFront = 0.75f * GD_MATH_TAU;

    gdVec2 HeroAlign = {0.5f, 0.156682029f};

    BeginAssetType(Assets, Asset_Head);
    AddBitmapAsset(Assets, "../Data/HH/test/test_hero_right_head.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleRight);
    AddBitmapAsset(Assets, "../Data/HH/test/test_hero_back_head.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleBack);
    AddBitmapAsset(Assets, "../Data/HH/test/test_hero_left_head.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleLeft);
    AddBitmapAsset(Assets, "../Data/HH/test/test_hero_front_head.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleFront);
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Cape);
    AddBitmapAsset(Assets, "../Data/HH/test/test_hero_right_cape.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleRight);
    AddBitmapAsset(Assets, "../Data/HH/test/test_hero_back_cape.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleBack);
    AddBitmapAsset(Assets, "../Data/HH/test/test_hero_left_cape.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleLeft);
    AddBitmapAsset(Assets, "../Data/HH/test/test_hero_front_cape.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleFront);
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Torso);
    AddBitmapAsset(Assets, "../Data/HH/test/test_hero_right_torso.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleRight);
    AddBitmapAsset(Assets, "../Data/HH/test/test_hero_back_torso.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleBack);
    AddBitmapAsset(Assets, "../Data/HH/test/test_hero_left_torso.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleLeft);
    AddBitmapAsset(Assets, "../Data/HH/test/test_hero_front_torso.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleFront);
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Grass);
    AddBitmapAsset(Assets, "../Data/HH/Test2/grass00.bmp");
    AddBitmapAsset(Assets, "../Data/HH/Test2/grass01.bmp");
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Stone);
    AddBitmapAsset(Assets, "../Data/HH/Test2/ground00.bmp");
    AddBitmapAsset(Assets, "../Data/HH/Test2/ground01.bmp");
    AddBitmapAsset(Assets, "../Data/HH/Test2/ground02.bmp");
    AddBitmapAsset(Assets, "../Data/HH/Test2/ground03.bmp");
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Tuft);
    AddBitmapAsset(Assets, "../Data/HH/Test2/tuft00.bmp");
    AddBitmapAsset(Assets, "../Data/HH/Test2/tuft01.bmp");
    AddBitmapAsset(Assets, "../Data/HH/Test2/tuft02.bmp");
    EndAssetType(Assets);


    /*Loading sounds*/
    BeginAssetType(Assets, Asset_Bloop);
    AddSoundAsset(Assets, "../Data/HH/test3/bloop_00.wav");
    AddSoundAsset(Assets, "../Data/HH/test3/bloop_01.wav");
    AddSoundAsset(Assets, "../Data/HH/test3/bloop_02.wav");
    AddSoundAsset(Assets, "../Data/HH/test3/bloop_03.wav");
    AddSoundAsset(Assets, "../Data/HH/test3/bloop_04.wav");
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Crack);
    AddSoundAsset(Assets, "../Data/HH/test3/crack_00.wav");
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Drop);
    AddSoundAsset(Assets, "../Data/HH/test3/drop_00.wav");
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Glide);
    AddSoundAsset(Assets, "../Data/HH/test3/glide_00.wav");
    EndAssetType(Assets);

    uint32 OneMusicChunk = 10 * 44100;
    uint32 TotalMusicSampleCount = 7468095;
    BeginAssetType(Assets, Asset_Music);
    asset* LastMusic = 0;
    for(uint32 FirstSampleIndex = 0;
        FirstSampleIndex < TotalMusicSampleCount;
        FirstSampleIndex += OneMusicChunk)
    {
        uint32 SampleCount = TotalMusicSampleCount - FirstSampleIndex;
        if(SampleCount > OneMusicChunk){
            SampleCount = OneMusicChunk;
        }
        asset *ThisMusic = AddSoundAsset(Assets, "../Data/HH/test3/music_test.wav", FirstSampleIndex, SampleCount);
        if(LastMusic){
            Assets->SoundInfos[LastMusic->SlotID].NextIDToPlay.Value = ThisMusic->SlotID;
        }
        LastMusic = ThisMusic;
    }
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Puhp);
    AddSoundAsset(Assets, "../Data/HH/test3/puhp_00.wav");
    AddSoundAsset(Assets, "../Data/HH/test3/puhp_01.wav");
    EndAssetType(Assets);

    return(Assets);
}

inline void PrefetchBitmap(game_assets* Assets, bitmap_id ID){LoadBitmapAsset(Assets, ID);}
inline void PrefetchSound(game_assets* Assets, sound_id ID){LoadSoundAsset(Assets, ID);}
