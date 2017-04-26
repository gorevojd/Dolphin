#include "asset_builder.h"

/*
    NOTE(Dima): Images are stored in gamma-corrected premultiplied-alpha format
*/

/*
    TODO(Dima): 
        Font Atlas
*/

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

INTERNAL_FUNCTION loaded_font*
LoadFont(char* FileName, char* FontName, int PixelHeight){
    loaded_font* Font = (loaded_font*)malloc(sizeof(loaded_font));

    AddFontResourceExA(FileName, FR_PRIVATE, 0);
    Font->Win32Handle = CreateFontA(
        PixelHeight,
        0, 0, 0,
        FW_NORMAL,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        FontName);

    SelectObject(GlobalFontDeviceContext, Font->Win32Handle);
    GetTextMetrics(GlobalFontDeviceContext, &Font->TextMetric);

    Font->MinCodePoint = INT_MAX;
    Font->MaxCodePoint = 0;

    Font->MaxGlyphCount = 5000;
    Font->GlyphCount = 0;

    uint32 GlyphFromCodepointArraySize = ONE_PAST_MAX_FONT_CODEPOINT * sizeof(uint32);
    Font->GlyphIndexFromCodePoint = (uint32*)malloc(GlyphFromCodepointArraySize);
    memset(Font->GlyphIndexFromCodePoint, 0, GlyphFromCodepointArraySize);

    Font->Glyphs = (dda_font_glyph*)malloc(sizeof(dda_font_glyph) * Font->MaxGlyphCount);
    size_t HorizontalAdvanceArraySize = sizeof(float) * Font->MaxGlyphCount * Font->MaxGlyphCount;
    Font->HorizontalAdvance = (float*)malloc(HorizontalAdvanceArraySize);
    memset(Font->HorizontalAdvance, 0, HorizontalAdvanceArraySize);

    Font->OnePastHighestCodepoint = 0;

    Font->GlyphCount = 1;
    Font->Glyphs[0].UnicodeCodePoint = 0;
    Font->Glyphs[0].BitmapID.Value = 0;

    return(Font);
}

INTERNAL_FUNCTION void
FinalizeFontKerning(loaded_font* Font){
    SelectObject(GlobalFontDeviceContext, Font->Win32Handle);

    DWORD KerningPairCount = GetKerningPairsW(GlobalFontDeviceContext, 0, 0);
    KERNINGPAIR* KerningPairs = (KERNINGPAIR*)malloc(KerningPairCount * sizeof(KERNINGPAIR));
    GetKerningPairsW(GlobalFontDeviceContext, KerningPairCount, KerningPairs);

    for(DWORD KernPairIndex = 0;
        KernPairIndex < KerningPairCount;
        KernPairIndex++)
    {
        KERNINGPAIR* Pair = KerningPairs + KernPairIndex;
        if((Pair->wFirst < ONE_PAST_MAX_FONT_CODEPOINT) &&
            (Pair->wSecond < ONE_PAST_MAX_FONT_CODEPOINT))
        {
            uint32 First = Font->GlyphIndexFromCodePoint[Pair->wFirst];
            uint32 Second = Font->GlyphIndexFromCodePoint[Pair->wSecond];
            if((First != 0) && (Second != 0)){
                Font->HorizontalAdvance[First * Font->MaxGlyphCount + Second] += (float)Pair->iKernAmount;
            }
        }
    }

    free(KerningPairs);
}

INTERNAL_FUNCTION void 
FreeFont(loaded_font* Font){
    if(Font){
        DeleteObject(Font->Win32Handle);
        free(Font->Glyphs);
        free(Font->HorizontalAdvance);
        free(Font->GlyphIndexFromCodePoint);
        free(Font);
    }
}

INTERNAL_FUNCTION void InitializeFontDC(){
    GlobalFontDeviceContext = CreateCompatibleDC(GetDC(0));

    BITMAPINFO Info = {};
    Info.bmiHeader.biSize = sizeof(Info.bmiHeader);
    Info.bmiHeader.biWidth = MAX_FONT_WIDTH;
    Info.bmiHeader.biHeight = MAX_FONT_HEIGHT;
    Info.bmiHeader.biPlanes = 1;
    Info.bmiHeader.biBitCount = 32;
    Info.bmiHeader.biCompression = BI_RGB;
    Info.bmiHeader.biSizeImage = 0;
    Info.bmiHeader.biXPelsPerMeter = 0;
    Info.bmiHeader.biYPelsPerMeter = 0;
    Info.bmiHeader.biClrUsed = 0;
    Info.bmiHeader.biClrImportant = 0;
    HBITMAP Bitmap = CreateDIBSection(GlobalFontDeviceContext, &Info, DIB_RGB_COLORS, &GlobalFontBits, 0, 0);
    SelectObject(GlobalFontDeviceContext, Bitmap);
    SetBkColor(GlobalFontDeviceContext, RGB(0, 0, 0));  
}

INTERNAL_FUNCTION loaded_bitmap
LoadGlyphBitmap(loaded_font* Font, uint32 Codepoint, dda_asset* Asset){
    loaded_bitmap Result = {};

    uint32 GlyphIndex = Font->GlyphIndexFromCodePoint[Codepoint];

    SelectObject(GlobalFontDeviceContext, Font->Win32Handle);

    memset(GlobalFontBits, 0x00, MAX_FONT_WIDTH * MAX_FONT_HEIGHT * sizeof(uint32));

    wchar_t FakePoint = (wchar_t)Codepoint;

    SIZE Size;
    GetTextExtentPoint32W(GlobalFontDeviceContext, &FakePoint, 1, &Size);

    int PreStepX = 128;

    int BoundWidth = Size.cx + 2 * PreStepX;
    if(BoundWidth > MAX_FONT_WIDTH){
        BoundWidth = MAX_FONT_WIDTH;
    }
    int BoundHeight = Size.cy;
    if(BoundHeight > MAX_FONT_HEIGHT){
        BoundHeight = MAX_FONT_HEIGHT;
    }

    SetTextColor(GlobalFontDeviceContext, RGB(255, 255, 255));
    TextOutW(GlobalFontDeviceContext, PreStepX, 0, &FakePoint, 1);

    int MinX = 10000;
    int MinY = 10000;
    int MaxX = -10000;
    int MaxY = -10000;

    for(int j  = 0; j < BoundHeight; j++){

        uint32 *Pixel = (uint32*)GlobalFontBits + (MAX_FONT_HEIGHT - 1 - j) * MAX_FONT_WIDTH;
        
        for(int i = 0; i < BoundWidth; i++){
            if(*Pixel != 0)
            {
                if(MinX > i)
                {
                    MinX = i;                    
                }

                if(MinY > j)
                {
                    MinY = j;                    
                }
                
                if(MaxX < i)
                {
                    MaxX = i;                    
                }

                if(MaxY < j)
                {
                    MaxY = j;                    
                }
            }

            ++Pixel;
        }
    }

    float KerningChange = 0;
    if(MinX <= MaxX){
        int Width = (MaxX - MinX) + 1;
        int Height = (MaxY - MinY) + 1;

        Result.Width = Width + 2;
        Result.Height = Height + 2;
        Result.Memory = malloc(Result.Height * Result.Width * 4);
        Result.Free = Result.Memory;

        memset(Result.Memory, 0, Result.Height * Result.Width * 4);

        uint8* DestRow = (uint8*)Result.Memory + (Result.Height - 1 - 1) * Result.Width * 4;
        uint32* SourceRow = (uint32*)GlobalFontBits + (MAX_FONT_HEIGHT - 1 - MinY) * MAX_FONT_WIDTH;
        
        for(int j = MinY; j < MaxY; j++){
            
            uint32* Source = (uint32*)SourceRow + MinX;
            uint32* Dest = (uint32*)DestRow + 1;    

            for(int i = MinX; i < MaxX; i++){
                uint32 Pixel = *Source;

                float Gray = (float)(Pixel & 0xFF);
                vec4 Texel = {255.0f, 255.0f, 255.0f, Gray};
                Texel = SRGB255ToLinear1(Texel);
                Texel.rgb *= Texel.a;
                Texel = Linear1ToSRGB255(Texel);

               *Dest++ = (((uint32)(Texel.a + 0.5f) << 24) |
                   ((uint32)(Texel.r + 0.5f) << 16) |
                   ((uint32)(Texel.g + 0.5f) << 8) |
                   ((uint32)(Texel.b + 0.5f) << 0));

               Source++;
            }

            DestRow -= Result.Width * 4;
            SourceRow -= MAX_FONT_WIDTH;
        }

        Asset->Bitmap.AlignPercentage[0] = (1.0f) / (float)Result.Width;
        Asset->Bitmap.AlignPercentage[1] = (1.0f + (MaxY - (BoundHeight - Font->TextMetric.tmDescent))) / (float)Result.Height;
    
        KerningChange = (float)(MinX - PreStepX);
    }

    INT ThisWidth;
    GetCharWidth32W(GlobalFontDeviceContext, Codepoint, Codepoint, &ThisWidth);
    float CharAdvance = (float)ThisWidth;

    for(uint32 OtherGlyphIndex = 0;
        OtherGlyphIndex < Font->MaxGlyphCount;
        OtherGlyphIndex++)
    {
        Font->HorizontalAdvance[GlyphIndex * Font->MaxGlyphCount + OtherGlyphIndex] += CharAdvance - KerningChange;
        if(OtherGlyphIndex != 0){
            Font->HorizontalAdvance[OtherGlyphIndex * Font->MaxGlyphCount + GlyphIndex] += KerningChange;
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

struct added_asset{
    uint32 ID;
    dda_asset* DDA;
    asset_source* Source;
};

INTERNAL_FUNCTION added_asset AddAsset(game_assets* Assets){
    Assert(Assets->DEBUGAssetType);
    Assert(Assets->DEBUGAssetType->OnePastLastAssetIndex < ArrayCount(Assets->Assets));

    uint32 Index = Assets->DEBUGAssetType->OnePastLastAssetIndex++;
    asset_source* Source = Assets->AssetSources + Index;
    dda_asset* DDA = Assets->Assets + Index;
    DDA->FirstTagIndex = Assets->TagCount;
    DDA->OnePastLastTagIndex = DDA->FirstTagIndex;

    Assets->AssetIndex = Index;

    added_asset Result;
    Result.ID = Index;
    Result.DDA = DDA;
    Result.Source = Source;
    return(Result);
}

INTERNAL_FUNCTION bitmap_id 
AddCharacterAsset(game_assets* Assets, loaded_font* Font, uint32 Codepoint){
    added_asset Asset = AddAsset(Assets);
    Asset.DDA->Bitmap.AlignPercentage[0] = 0.0f;
    Asset.DDA->Bitmap.AlignPercentage[1] = 0.0f;
    Asset.Source->Type = AssetType_FontGlyph;
    Asset.Source->Glyph.Font = Font;
    Asset.Source->Glyph.Codepoint = Codepoint;

    bitmap_id Result = {Asset.ID};

    Assert(Font->GlyphCount < Font->MaxGlyphCount);
    uint32 GlyphIndex = Font->GlyphCount++;
    dda_font_glyph* Glyph = Font->Glyphs + GlyphIndex;
    Glyph->UnicodeCodePoint = Codepoint;
    Glyph->BitmapID = Result;
    Font->GlyphIndexFromCodePoint[Codepoint] = GlyphIndex;

    if(Font->OnePastHighestCodepoint <= Codepoint){
        Font->OnePastHighestCodepoint = Codepoint + 1;
    }

    return(Result);
}

INTERNAL_FUNCTION bitmap_id
AddBitmapAsset(
	game_assets* Assets,
	char* FileName,
	vec2 AlignPercentage = Vec2(0.5f))
{

    added_asset Asset = AddAsset(Assets);

	Asset.DDA->Bitmap.AlignPercentage[0] = AlignPercentage.x;
	Asset.DDA->Bitmap.AlignPercentage[1] = AlignPercentage.y;

	Asset.Source->Type = AssetType_Bitmap;
	Asset.Source->Bitmap.FileName = FileName;

    bitmap_id Result = {Asset.ID};
	return(Result);
}

INTERNAL_FUNCTION sound_id
AddSoundAsset(
	game_assets* Assets,
	char* FileName,
	unsigned int FirstSampleIndex = 0,
	unsigned int SampleCount = 0)
{
    added_asset Asset = AddAsset(Assets);

	Asset.DDA->Sound.SampleCount = SampleCount;
	Asset.DDA->Sound.Chain = DDASoundChain_None;

	Asset.Source->Type = AssetType_Sound;
	Asset.Source->Sound.FileName = FileName;
	Asset.Source->Sound.FirstSampleIndex = FirstSampleIndex;

    sound_id Result = {Asset.ID};
	return(Result);
}

INTERNAL_FUNCTION font_id
AddFontAsset(game_assets* Assets, loaded_font* Font){
    added_asset Asset = AddAsset(Assets);
    Asset.DDA->Font.OnePastHighestCodepoint = Font->OnePastHighestCodepoint;
    Asset.DDA->Font.GlyphCount = Font->GlyphCount;
    Asset.DDA->Font.AscenderHeight = (float)Font->TextMetric.tmAscent;
    Asset.DDA->Font.DescenderHeight = (float)Font->TextMetric.tmDescent;
    Asset.DDA->Font.ExternalLeading = (float)Font->TextMetric.tmExternalLeading;
    Asset.Source->Type = AssetType_Font;
    Asset.Source->Font.Font = Font;

    font_id Result = {Asset.ID};
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
                        Source->Sound.FileName,
                        Source->Sound.FirstSampleIndex,
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

                case(AssetType_FontGlyph):
                case(AssetType_Bitmap):{
                    //loaded_bitmap Bitmap = LoadBMP(Source->FileName);

                    loaded_bitmap Bitmap;
                    if(Source->Type == AssetType_FontGlyph){
                        Bitmap = LoadGlyphBitmap(Source->Glyph.Font, Source->Glyph.Codepoint, DDA);
                    }
                    else{
                        Assert(Source->Type == AssetType_Bitmap);
                        Bitmap = LoadBMP(Source->Bitmap.FileName);
                    }

                    DDA->Bitmap.Dimension[0] = Bitmap.Width;
                    DDA->Bitmap.Dimension[1] = Bitmap.Height;

                    fwrite(Bitmap.Memory, Bitmap.Width * Bitmap.Height * 4, 1, fp);

                    free(Bitmap.Free);
                }break;

                case(AssetType_Model):{

                }break;

                case(AssetType_Font):{
                    loaded_font* Font = Source->Font.Font;
                    
                    FinalizeFontKerning(Font);

                    uint32 GlyphsSize = Font->GlyphCount * sizeof(dda_font_glyph);
                    fwrite(Font->Glyphs, GlyphsSize, 1, fp);

                    uint8* HorizontalAdvance = (uint8*)Font->HorizontalAdvance;
                    for(uint32 GlyphIndex = 0;
                        GlyphIndex < Font->GlyphCount;
                        GlyphIndex++)
                    {
                        /*Here we write one row of horizontal advances for GlyphIndex*/
                        uint32 HorizontalAdvanceSliceSize = sizeof(float) * Font->GlyphCount;
                        fwrite(HorizontalAdvance, HorizontalAdvanceSliceSize, 1, fp);
                        HorizontalAdvance += sizeof(float) * Font->MaxGlyphCount;
                    }
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

INTERNAL_FUNCTION void WriteFonts(){
    game_assets Assets_;
    game_assets* Assets = &Assets_;
    Initialize(Assets);

    loaded_font *Fonts[] = {
        LoadFont("c:/Windows/Fonts/arial.ttf", "Arial", 128),
        LoadFont("c:/Windows/Fonts/LiberationMono-Regular.ttf", "Liberation Mono", 20),
    };

    BeginAssetType(Assets, Asset_FontGlyph);
    for(uint32 FontIndex = 0;
        FontIndex < ArrayCount(Fonts);
        FontIndex++)
    {
        loaded_font* Font = Fonts[FontIndex];

        AddCharacterAsset(Assets, Font, ' ');
        for(uint32 Character = '!';
            Character <= '~';
            Character++)
        {
            AddCharacterAsset(Assets, Font, Character);
        }
    }
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Font);
    AddFontAsset(Assets, Fonts[0]);
    AddTag(Assets, Tag_FontType, FontType_Default);
    AddFontAsset(Assets, Fonts[1]);
    AddTag(Assets, Tag_FontType, FontType_Debug);
    EndAssetType(Assets);

    WriteDDA(Assets, "../Data/asset_pack_fonts.dda");
}

INTERNAL_FUNCTION void WriteHero(){
    game_assets Assets_;
    game_assets* Assets = &Assets_;
    Initialize(Assets);

    real32 AngleRight = 0.0f * IVAN_MATH_TAU;
    real32 AngleBack = 0.25f * IVAN_MATH_TAU;
    real32 AngleLeft = 0.5f * IVAN_MATH_TAU;
    real32 AngleFront = 0.75f * IVAN_MATH_TAU;

    vec2 HeroAlign = {0.5f, 1.0f - 0.156682029f};

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

    BeginAssetType(Assets, Asset_Particle);
    AddBitmapAsset(Assets, "../Data/Images/ShockSpell.png");
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

    InitializeFontDC();

    WriteHero();
    WriteNonHero();
    WriteSounds();
    WriteFonts();

    system("pause");
	return(0);
}
