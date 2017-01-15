#include "dolphin_asset.h"


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
#pragma pack(pop)

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
LoadBitmap(debug_platform_read_entire_file* ReadEntireFile, char* FileName, int32 AlignX, int32 TopDownAlignY, bool32 FlipOnLoad = true){
    loaded_bitmap Result = {};

    stbi_set_flip_vertically_on_load(FlipOnLoad);

    debug_read_file_result ReadResult = ReadEntireFile(FileName);

    Result.Memory = stbi_load_from_memory(
        (stbi_uc*)ReadResult.Contents,
        ReadResult.ContentsSize,
        &Result.Width,
        &Result.Height,
        0,
        STBI_rgb_alpha);
    Result.AlignPercentage = TopDownAlign(&Result, gd_vec2(AlignX, TopDownAlignY));
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
LoadBitmap(debug_platform_read_entire_file* ReadEntireFile, char* FileName, bool32 FlipOnLoad = true){
    loaded_bitmap Result = LoadBitmap(ReadEntireFile, FileName, 0, 0, FlipOnLoad);
    Result.AlignPercentage = gd_vec2(0.0f, 0.0f);
    return(Result);
}
#endif


struct load_bitmap_work{
    game_assets* Assets;
    bitmap_id ID;
    char* FileName;
    task_with_memory* Task;
    struct loaded_bitmap* Bitmap;

    bool32 HasAlignment;
    int32 AlignX;
    int32 TopDownAlignY;
};

INTERNAL_FUNCTION PLATFORM_WORK_QUEUE_CALLBACK(LoadBitmapWork){
    load_bitmap_work* Work = (load_bitmap_work*)Data;
    if(Work->HasAlignment){
        *Work->Bitmap = LoadBitmap(PlatformReadEntireFile, Work->FileName, Work->AlignX, Work->TopDownAlignY);
    }
    else{
        *Work->Bitmap = LoadBitmap(PlatformReadEntireFile, Work->FileName);
    }

    GD_COMPLETE_WRITES_BEFORE_FUTURE;

    Work->Assets->Bitmaps[Work->ID.Value].State = AssetState_Loaded;
    Work->Assets->Bitmaps[Work->ID.Value].Bitmap = Work->Bitmap;
    EndTaskWithMemory(Work->Task);
}

INTERNAL_FUNCTION void LoadBitmapAsset(game_assets* Assets, bitmap_id ID){
    if(AtomicCompareExchangeUInt32((uint32*)&Assets->Bitmaps[ID.Value].State, AssetState_Queued, AssetState_Unloaded) == AssetState_Unloaded){       
        task_with_memory* Task = BeginTaskWithMemory(Assets->TranState);

        if (Task){
            load_bitmap_work* Work = PushStruct(&Task->Arena, load_bitmap_work);
            Work->Assets = Assets;
            Work->ID = ID;
            Work->FileName = "";
            Work->Task = Task;
            Work->Bitmap = PushStruct(&Assets->Arena, loaded_bitmap);

            switch (ID.Value){
                case Asset_Backdrop:{
                    Work->FileName = "../Data/HH/test/test_background.bmp";
                }break;
                case Asset_LastOfUs:{
                    Work->FileName = "../Data/Images/last.jpg";
                }break;
                case Asset_Tree:{
                    Work->FileName = "../Data/HH/test2/tree00.bmp";
                    Work->HasAlignment = true;
                    Work->AlignX = 40;
                    Work->TopDownAlignY = 80;
                }break;
                case Asset_StarWars:{
                    Work->FileName = "../Data/Images/star_wars.jpg";
                }break;
                case Asset_Witcher:{
                    Work->FileName = "../Data/Images/witcher.jpg";
                }break;
                case Asset_Assassin:{
                    Work->FileName = "../Data/Images/assassin.jpg";
                }break;
            }
            PlatformAddEntry(Assets->TranState->LowPriorityQueue, LoadBitmapWork, Work);
        }
    }
}

INTERNAL_FUNCTION void LoadSoundAsset(game_assets* Assets, sound_id ID){

}

INTERNAL_FUNCTION bitmap_id
GetFirstBitmapID(game_assets* Assets, asset_type_id TypeID){
    bitmap_id Result = {};

    asset_type* Type = Assets->AssetTypes + TypeID;
    if(Type->FirstAssetIndex != Type->OnePastLastAssetIndex){
        asset* Asset = Assets->Assets + Type->FirstAssetIndex;
        Result.Value = Asset->SlotID;
    }

    return(Result);
}

INTERNAL_FUNCTION game_assets* 
AllocateGameAssets(memory_arena* Arena, size_t Size, transient_state* TranState){
    game_assets* Assets = PushStruct(Arena, game_assets);
    SubArena(&Assets->Arena, Arena, Size);
    Assets->TranState = TranState;

    Assets->BitmapCount = Asset_Count;
    Assets->Bitmaps = PushArray(Arena, Assets->BitmapCount, asset_slot);

    Assets->SoundCount = 1;
    Assets->Sounds = PushArray(Arena, Assets->SoundCount, asset_slot);

    Assets->AssetCount = Assets->BitmapCount;
    Assets->Assets = PushArray(Arena, Assets->AssetCount, asset);

    for(int AssetID = 0;
        AssetID < Asset_Count;
        ++AssetID)
    {
        asset_type* Type = Assets->AssetTypes + AssetID;
        Type->FirstAssetIndex = AssetID;
        Type->OnePastLastAssetIndex = AssetID + 1;

        asset* Asset = Assets->Assets + Type->FirstAssetIndex;
        Asset->SlotID = Type->FirstAssetIndex;
    }

    hero_bitmaps* Bitmap;
    Bitmap = &Assets->HeroBitmaps[0];
    Bitmap->Head = LoadBitmap(PlatformReadEntireFile, "../Data/HH/Test/test_hero_right_head.bmp");
    Bitmap->Torso = LoadBitmap(PlatformReadEntireFile, "../Data/HH/Test/test_hero_right_torso.bmp");
    Bitmap->Cape = LoadBitmap(PlatformReadEntireFile, "../Data/HH/Test/test_hero_right_cape.bmp");
    Bitmap->Align = gd_vec2(72, 182);
    Bitmap++;

    Bitmap->Head = LoadBitmap(PlatformReadEntireFile, "../Data/HH/Test/test_hero_back_head.bmp");
    Bitmap->Torso = LoadBitmap(PlatformReadEntireFile, "../Data/HH/Test/test_hero_back_torso.bmp");
    Bitmap->Cape = LoadBitmap(PlatformReadEntireFile, "../Data/HH/Test/test_hero_back_cape.bmp");
    Bitmap->Align = gd_vec2(72, 182);
    Bitmap++;

    Bitmap->Head = LoadBitmap(PlatformReadEntireFile, "../Data/HH/Test/test_hero_left_head.bmp");
    Bitmap->Torso = LoadBitmap(PlatformReadEntireFile, "../Data/HH/Test/test_hero_left_torso.bmp");
    Bitmap->Cape = LoadBitmap(PlatformReadEntireFile, "../Data/HH/Test/test_hero_left_cape.bmp");
    Bitmap->Align = gd_vec2(72, 182);
    Bitmap++;

    Bitmap->Head = LoadBitmap(PlatformReadEntireFile, "../Data/HH/Test/test_hero_front_head.bmp");
    Bitmap->Torso = LoadBitmap(PlatformReadEntireFile, "../Data/HH/Test/test_hero_front_torso.bmp");
    Bitmap->Cape = LoadBitmap(PlatformReadEntireFile, "../Data/HH/Test/test_hero_front_cape.bmp");
    Bitmap->Align = gd_vec2(72, 182);
    Bitmap++;

    Assets->Grasses[0] = LoadBitmap(PlatformReadEntireFile, "../Data/HH/Test2/grass00.bmp");
    Assets->Grasses[1] = LoadBitmap(PlatformReadEntireFile, "../Data/HH/Test2/grass01.bmp");

    Assets->Stones[0] = LoadBitmap(PlatformReadEntireFile, "../Data/HH/Test2/ground00.bmp");
    Assets->Stones[1] = LoadBitmap(PlatformReadEntireFile, "../Data/HH/Test2/ground01.bmp");
    Assets->Stones[2] = LoadBitmap(PlatformReadEntireFile, "../Data/HH/Test2/ground02.bmp");
    Assets->Stones[3] = LoadBitmap(PlatformReadEntireFile, "../Data/HH/Test2/ground03.bmp");

    Assets->Tufts[0] = LoadBitmap(PlatformReadEntireFile, "../Data/HH/Test2/tuft00.bmp");
    Assets->Tufts[1] = LoadBitmap(PlatformReadEntireFile, "../Data/HH/Test2/tuft01.bmp");

    return(Assets);
}