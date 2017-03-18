#include "asset_builder.h"

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

struct loaded_file{
	unsigned int DataSize;
	void* Data;
};

loaded_file ReadEntireFile(char* FileName){
	loaded_file Result = {};

	FILE* In = fopen(FileName, "rb");
	if(In){
		fseek(In, 0, SEEK_END);
		Result.DataSize = ftell(In);
		fseek(In, 0, SEEK_SET);

		Result.Data = malloc(Result.DataSize);
		fread(Result.Data, Result.DataSize, 1, In);
		fclose(In);
	}
	else{
		printf("ERROR: Can not open the file %s\n", FileName);
	}

	return(Result);
}

struct loaded_bitmap{
	int Width;
	int Height;
	void* Memory;

	void* Free;
};

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include "stb_image.h"

INTERNAL_FUNCTION loaded_bitmap
LoadBMP(char* FileName, bool32 FlipOnLoad = false){
    loaded_bitmap Result = {};

    stbi_set_flip_vertically_on_load(FlipOnLoad);

    loaded_file ReadResult = ReadEntireFile(FileName);

    Result.Free = ReadResult.Data;
    Result.Memory = stbi_load_from_memory(
        (stbi_uc*)ReadResult.Data,
        ReadResult.DataSize,
        &Result.Width,
        &Result.Height,
        0,
        STBI_rgb_alpha);

    uint32* Pixel = (uint32*)Result.Memory;
    for (int j = 0; j < Result.Height; j++){
        for (int i = 0; i < Result.Width; i++){
            uint32 SrcPixel = *Pixel;

            vec4 Color = {
                (SrcPixel & 0xFF),
                ((SrcPixel >> 8) & 0xFF),
                ((SrcPixel >> 16) & 0xFF),
                ((SrcPixel >> 24) & 0xFF) };

#if 1

#if 1
            /*Gamma-corrected premultiplied alpha*/

            Color = SRGB255ToLinear1(Color);
            real32 Alpha = Color.a;
            Color.r = Color.r * Alpha;
            Color.g = Color.g * Alpha;
            Color.b = Color.b * Alpha;
            Color = Linear1ToSRGB255(Color);
#else
            /*Premultiplied alpha*/
            Color.rgb *= Color.a;
#endif

#endif

            *Pixel++ = (((uint32)(Color.a + 0.5f) << 24) |
                ((uint32)(Color.r + 0.5f) << 16) |
                ((uint32)(Color.g + 0.5f) << 8) |
                ((uint32)(Color.b + 0.5f) << 0));
        }
    }

    return(Result);
}

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

struct loaded_sound{
	uint32 SampleCount;
	uint32 ChannelCount;
	int16* Samples[2];

	void* Free;
};

INTERNAL_FUNCTION loaded_sound
LoadWAV(char* FileName, uint32 SectionFirstSampleIndex, uint32 SectionSampleCount){
    loaded_sound Result = {};

    loaded_file ReadResult = ReadEntireFile(FileName);
    if(ReadResult.DataSize != 0){

        Result.Free = ReadResult.Data;

        WAVE_header* Header = (WAVE_header*)ReadResult.Data;
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

        int16* NewChannel0 = 0;
        int16* NewChannel1 = 0;
        
        if(ChannelCount == 1){

            NewChannel0 = (int16*)malloc(SampleCount * sizeof(int16) + 8 * sizeof(int16));

            Result.Samples[0] = NewChannel0;
            Result.Samples[1] = NewChannel1;

            for(uint32 SampleIndex = SectionFirstSampleIndex;
                SampleIndex < SectionFirstSampleIndex + SectionSampleCount;
                SampleIndex++)
            {
                NewChannel0[SampleIndex - SectionFirstSampleIndex] = SampleData[SampleIndex];
            }
        }
        else if(ChannelCount == 2){


            NewChannel0 = (int16*)malloc(SampleCount * sizeof(int16) + 8 * sizeof(int16));
            NewChannel1 = (int16*)malloc(SampleCount * sizeof(int16) + 8 * sizeof(int16));

            Result.Samples[0] = NewChannel0;
            Result.Samples[1] = NewChannel1;       
            
            for(uint32 SampleIndex = SectionFirstSampleIndex;
                SampleIndex < SectionFirstSampleIndex + SectionSampleCount;
                SampleIndex++)
            {
                NewChannel0[SampleIndex - SectionFirstSampleIndex] = SampleData[SampleIndex * 2];
                NewChannel1[SampleIndex - SectionFirstSampleIndex] = SampleData[SampleIndex * 2 + 1];
            }
        }
        else{
            Assert(!"Invalid channel count in WAV file");
        }

        bool32 AtEnd = true;

        if(SectionSampleCount){
            Assert((SectionFirstSampleIndex + SectionSampleCount) <= SampleCount);
            AtEnd = ((SectionFirstSampleIndex + SectionSampleCount) == SampleCount);
            SampleCount = SectionSampleCount;
        }

        if(AtEnd){
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

        Result.SampleCount = SampleCount;
    }


    return(Result);
}

INTERNAL_FUNCTION void 
BeginAssetType(game_assets* Assets, asset_type_id TypeID){
	Assert(Assets->DEBUGAssetType == 0);

	Assets->DEBUGAssetType = Assets->AssetTypes + TypeID;
	Assets->DEBUGAssetType->TypeID = TypeID;
	Assets->DEBUGAssetType->FirstAssetIndex = Assets->AssetCount;
	Assets->DEBUGAssetType->OnePastLastAssetIndex = Assets->DEBUGAssetType->FirstAssetIndex;
}

INTERNAL_FUNCTION bitmap_id
AddBitmapAsset(
	game_assets* Assets,
	char* FileName,
	vec2 AlignPercentage = Vec2(0.5f))
{
	Assert(Assets->DEBUGAssetType);
	Assert(Assets->DEBUGAssetType->OnePastLastAssetIndex < ArrayCount(Assets->Assets));

	bitmap_id Result = {Assets->DEBUGAssetType->OnePastLastAssetIndex++};
	asset_source* Source = Assets->AssetSources + Result.Value;
	dda_asset* DDA = Assets->Assets + Result.Value;
	DDA->FirstTagIndex = Assets->TagCount;
	DDA->OnePastLastTagIndex = DDA->FirstTagIndex;
	DDA->Bitmap.AlignPercentage[0] = AlignPercentage.x;
	DDA->Bitmap.AlignPercentage[1] = 1.0f - AlignPercentage.y;

	Source->Type = AssetType_Bitmap;
	Source->FileName = FileName;

	Assets->AssetIndex = Result.Value;

	return(Result);
}

INTERNAL_FUNCTION sound_id
AddSoundAsset(
	game_assets* Assets,
	char* FileName,
	unsigned int FirstSampleIndex = 0,
	unsigned int SampleCount = 0)
{
	Assert(Assets->DEBUGAssetType);
	Assert(Assets->DEBUGAssetType->OnePastLastAssetIndex < ArrayCount(Assets->Assets));

	sound_id Result = {Assets->DEBUGAssetType->OnePastLastAssetIndex++};
	asset_source* Source = Assets->AssetSources + Result.Value;
	dda_asset* DDA = Assets->Assets + Result.Value;
	DDA->FirstTagIndex = Assets->TagCount;
	DDA->OnePastLastTagIndex = DDA->FirstTagIndex;
	DDA->Sound.SampleCount = SampleCount;
	DDA->Sound.Chain = DDASoundChain_None;

	Source->Type = AssetType_Sound;
	Source->FileName = FileName;
	Source->FirstSampleIndex = FirstSampleIndex;

	Assets->AssetIndex = Result.Value;

	return(Result);
}

INTERNAL_FUNCTION void
AddTag(game_assets* Assets, asset_tag_id ID, float Value){
	Assert(Assets->AssetIndex);

	dda_asset* DDA = Assets->Assets + Assets->AssetIndex;
	++DDA->OnePastLastTagIndex;
	dda_tag* Tag = Assets->Tags + Assets->TagCount++;

	Tag->ID = ID;
	Tag->Value = Value;
}

INTERNAL_FUNCTION void
EndAssetType(game_assets* Assets){
	Assert(Assets->DEBUGAssetType);
	Assets->AssetCount = Assets->DEBUGAssetType->OnePastLastAssetIndex;
	Assets->DEBUGAssetType = 0;
	Assets->AssetIndex = 0;
}

INTERNAL_FUNCTION void
WriteDDA(game_assets* Assets, char* FileName){
    FILE* fp = fopen(FileName, "wb");
    if(fp){
        dda_header Header = {};
        Header.MagicValue = DDA_MAGIC_VALUE;
        Header.Version = DDA_VERSION;
        Header.TagCount = Assets->TagCount;
        Header.AssetTypeCount = Asset_Count;
        Header.AssetCount = Assets->AssetCount;

        uint32 TagArraySize = Header.TagCount * sizeof(dda_tag);
        uint32 AssetTypeArraySize = Header.AssetTypeCount * sizeof(dda_asset_type);
        uint32 AssetArraySize = Header.AssetCount * sizeof(dda_asset);

        Header.TagOffset = sizeof(Header);
        Header.AssetTypeOffset = Header.TagOffset + TagArraySize;
        Header.AssetOffset = Header.AssetTypeOffset + AssetTypeArraySize;

        fwrite(&Header, sizeof(Header), 1, fp);
        fwrite(Assets->Tags, TagArraySize, 1, fp);
        fwrite(Assets->AssetTypes, AssetTypeArraySize, 1, fp);
        fseek(fp, AssetArraySize, SEEK_CUR);

        for(uint32 AssetIndex = 1;
            AssetIndex < Header.AssetCount;
            AssetIndex++)
        {
            asset_source* Source = Assets->AssetSources + AssetIndex;
            dda_asset* DDA = Assets->Assets + AssetIndex;

            DDA->DataOffset = ftell(fp);

            switch(Source->Type){
                case(AssetType_Sound):{
                    loaded_sound WAV = LoadWAV(
                        Source->FileName,
                        Source->FirstSampleIndex,
                        DDA->Sound.SampleCount);

                    DDA->Sound.SampleCount = WAV.SampleCount;
                    DDA->Sound.ChannelCount = WAV.ChannelCount;

                    for(uint32 ChannelIndex = 0;
                        ChannelIndex < WAV.ChannelCount;
                        ChannelIndex++)
                    {
                        fwrite(WAV.Samples[ChannelIndex], DDA->Sound.SampleCount * sizeof(int16), 1, fp);
                    }

                    free(WAV.Free);
                }break;

                case(AssetType_Bitmap):{
                    loaded_bitmap Bitmap = LoadBMP(Source->FileName);

                    DDA->Bitmap.Dimension[0] = Bitmap.Width;
                    DDA->Bitmap.Dimension[1] = Bitmap.Height;

                    fwrite(Bitmap.Memory, Bitmap.Width * Bitmap.Height * 4, 1, fp);

                    free(Bitmap.Free);
                }break;

                case(AssetType_Model):{

                }break;

                default:{
                    INVALID_CODE_PATH;
                }break;
            }


        }
        
        fseek(fp, (uint32)Header.AssetOffset, SEEK_SET);
        fwrite(Assets->Assets, AssetArraySize, 1, fp);

        fclose(fp);
    }
    else{
        printf("Error while opening the file T_T\n");
    }

}

INTERNAL_FUNCTION void Initialize(game_assets* Assets){
    Assets->TagCount = 1;
    Assets->AssetCount = 1;
    Assets->DEBUGAssetType = 0;
    Assets->AssetIndex = 0;

    Assets->AssetTypeCount = Asset_Count;
    memset(Assets->AssetTypes, 0, sizeof(Assets->AssetTypes));
}

INTERNAL_FUNCTION void WriteHero(){
    game_assets Assets_;
    game_assets* Assets = &Assets_;
    Initialize(Assets);

    real32 AngleRight = 0.0f * IVAN_MATH_TAU;
    real32 AngleBack = 0.25f * IVAN_MATH_TAU;
    real32 AngleLeft = 0.5f * IVAN_MATH_TAU;
    real32 AngleFront = 0.75f * IVAN_MATH_TAU;

    vec2 HeroAlign = {0.5f, 0.156682029f};

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

    WriteDDA(Assets, "../Data/asset_pack_hero.dda");
}

INTERNAL_FUNCTION void WriteNonHero(){
    game_assets Assets_;
    game_assets* Assets = &Assets_;
    Initialize(Assets);

    BeginAssetType(Assets, Asset_Backdrop);
    AddBitmapAsset(Assets, "../Data/HH/test/test_background.bmp");
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_LastOfUs);
    AddBitmapAsset(Assets, "../Data/Images/last.jpg");
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Tree);
    AddBitmapAsset(Assets, "../Data/HH/test2/tree00.bmp", Vec2(0.5f, 0.3f));
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

    BeginAssetType(Assets, Asset_Heart);
    AddBitmapAsset(Assets, "../Data/Images/128/heart.png");
    EndAssetType(Assets);

    float ColorBlue = GetFloatRepresentOfColor(Vec3(0.0f, 0.0f, 1.0f));
    float ColorGreen = GetFloatRepresentOfColor(Vec3(0.0f, 1.0f, 0.0f));
    float ColorRed = GetFloatRepresentOfColor(Vec3(1.0f, 0.0f, 0.0f));

    BeginAssetType(Assets, Asset_Diamond);
    AddBitmapAsset(Assets, "../Data/Images/128/gemBlue.png");
    AddTag(Assets, Tag_Color, ColorBlue);
    AddBitmapAsset(Assets, "../Data/Images/128/gemGreen.png");
    AddTag(Assets, Tag_Color, ColorGreen);
    AddBitmapAsset(Assets, "../Data/Images/128/gemRed.png");
    AddTag(Assets, Tag_Color, ColorRed);
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Bottle);
    AddBitmapAsset(Assets, "../Data/Images/128/potionBlue.png");
    AddTag(Assets, Tag_Color, ColorBlue);
    AddBitmapAsset(Assets, "../Data/Images/128/potionGreen.png");
    AddTag(Assets, Tag_Color, ColorGreen);
    AddBitmapAsset(Assets, "../Data/Images/128/potionRed.png");
    AddTag(Assets, Tag_Color, ColorRed);
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Book);
    AddBitmapAsset(Assets, "../Data/Images/128/tome.png");
    EndAssetType(Assets);
    
    WriteDDA(Assets, "../Data/asset_pack_non_hero.dda");
}

INTERNAL_FUNCTION void WriteSounds(){
    game_assets Assets_;
    game_assets* Assets = &Assets_;
    Initialize(Assets);

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

    BeginAssetType(Assets, Asset_Puhp);
    AddSoundAsset(Assets, "../Data/HH/test3/puhp_00.wav");
    AddSoundAsset(Assets, "../Data/HH/test3/puhp_01.wav");
    EndAssetType(Assets);

    uint32 OneMusicChunk = 10 * 44100;
    uint32 TotalMusicSampleCount = 7468095;
    BeginAssetType(Assets, Asset_Music);
    for(uint32 FirstSampleIndex = 0;
        FirstSampleIndex < TotalMusicSampleCount;
        FirstSampleIndex += OneMusicChunk)
    {
        uint32 SampleCount;
        if(FirstSampleIndex + OneMusicChunk < TotalMusicSampleCount){
            SampleCount = OneMusicChunk;
        }
        else{
            SampleCount = TotalMusicSampleCount - FirstSampleIndex;
        }


        sound_id ThisMusic = AddSoundAsset(Assets, "../Data/HH/test3/music_test.wav", FirstSampleIndex, SampleCount);
        if((FirstSampleIndex + OneMusicChunk) < TotalMusicSampleCount){
            Assets->Assets[ThisMusic.Value].Sound.Chain = DDASoundChain_Advance;
        }
    }
    EndAssetType(Assets);

    WriteDDA(Assets, "../Data/asset_pack_sounds.dda");
}

int main(int ArgCount, char** Args){

    WriteHero();
    WriteNonHero();
    WriteSounds();

	return(0);
}
