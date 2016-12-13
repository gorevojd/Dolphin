#include "dolphin.h"
#include "dolphin_random.h"
#include "dolphin_asset.cpp"
#include "dolphin_render_group.cpp"

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

INTERNAL_FUNCTION loaded_bitmap
LoadBitmap(thread_context* Thread, debug_platform_read_entire_file* ReadEntireFile, char* FileName){

	loaded_bitmap Result = {};

	debug_read_file_result ReadResult = ReadEntireFile(Thread, FileName);
	if (ReadResult.ContentsSize != 0){
		bitmap_header* Header = (bitmap_header*)ReadResult.Contents;
		uint32* Pixels = (uint32*)((uint8*)ReadResult.Contents + Header->BitmapOffset);
		Result.Width = Header->Width;
		Result.Height = Header->Height;
		Result.Memory = Pixels;
		Result.BytesPerPixel = Header->BitsPerPixel / 8;

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

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

INTERNAL_FUNCTION loaded_bitmap
LoadBitmapWithSTB(thread_context* Thread, debug_platform_read_entire_file* ReadEntireFile, char* FileName, bool32 FlipOnLoad = true){
	loaded_bitmap Result = {};

	stbi_set_flip_vertically_on_load(FlipOnLoad);
	
	debug_read_file_result ReadResult = ReadEntireFile(Thread, FileName);

	Result.Memory = stbi_load_from_memory(
		(stbi_uc*)ReadResult.Contents,
		ReadResult.ContentsSize,
		&Result.Width,
		&Result.Height,
		&Result.BytesPerPixel, 
		STBI_rgb_alpha);

	uint32* Pixel = (uint32*)Result.Memory;
	for (int j = 0; j < Result.Height; j++){
		for (int i = 0; i < Result.Width; i++){
			uint32 SrcPixel = *Pixel;
			
			uint8 Red = (SrcPixel & 0xFF);
			uint8 Green = ((SrcPixel >> 8) & 0xFF);
			uint8 Blue = ((SrcPixel >> 16) & 0xFF);
			uint8 Alpha = ((SrcPixel >> 24) & 0xFF);

			*Pixel++ = ((Alpha << 24) | 
				(Red << 16) |
				(Green << 8) |
				(Blue) << 0);
		}
	}

	return(Result);
}

INTERNAL_FUNCTION void
GameOutputSound(game_sound_output_buffer* SoundBuffer, int Frequency){

	LOCAL_PERSIST real32 Phase = 0;
	real32 Volume = 4000;
	real32 Freq = (real32)Frequency;
	real32 WaveFrequency = (real32)Freq / (real32)SoundBuffer->SamplesPerSecond;

	//FIXED(Dima): BUG(Dima): Sound changing it's frequency every 3 - 5 seconds

	int16* SampleOut = (int16*)SoundBuffer->Samples;
	for (int SampleIndex = 0;
		SampleIndex < SoundBuffer->SampleCount;
		++SampleIndex)
	{

		real32 Omega = 2.0f * GD_MATH_PI * WaveFrequency;
		int16 SampleValue = (int16)(sinf(Phase) * Volume);

		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;

		Phase += Omega;
		if (Phase > 2.0f * GD_MATH_PI){
			Phase -= 2.0f * GD_MATH_PI;
		}
	}
}



INTERNAL_FUNCTION loaded_bitmap
MakeEmptyBitmap(memory_arena* Arena, int32 Width, int32 Height){
	loaded_bitmap Result = {};

	uint32 BytesPerPixel = 4;

	Result.Memory = (uint8*)Arena->BaseAddress + Arena->MemoryUsed;
	Result.Width = Width;
	Result.Height = Height;
	Result.BytesPerPixel = BytesPerPixel;

	PushSize(Arena, Width * Height * BytesPerPixel);

	return(Result);
}

INTERNAL_FUNCTION void
RenderGround(loaded_bitmap* Buffer, game_state* State)
{

	random_series Series = RandomSeed(123);

	gdVec2 ScreenCenter = { (real32)Buffer->Width * 0.5f, (real32)Buffer->Height * 0.5f };

	for (int GroundIndex = 0;
		GroundIndex < 60;
		GroundIndex++)
	{
		loaded_bitmap* BitmapToRender = State->Assets.Stones + RandomChoiceFromCount(&Series, ArrayCount(State->Assets.Stones));

		gdVec2 BitmapOffset = { (real32)BitmapToRender->Width * 0.5f,
			(real32)BitmapToRender->Height * 0.5f };

		gdVec2 GroundLocalPosition =
		{ RandomFromNegativeOneToOne(&Series),
		RandomFromNegativeOneToOne(&Series) };

		gdVec2 GroundScreenPosition = ScreenCenter + GroundLocalPosition * 256.0f - BitmapOffset;

		RenderBitmap(
			(loaded_bitmap*)Buffer,
			BitmapToRender,
			GroundScreenPosition);
	}

	for (int GrassIndex = 0;
		GrassIndex < 15;
		GrassIndex++)
	{
		loaded_bitmap* BitmapToRender = State->Assets.Grasses + RandomChoiceFromCount(&Series, ArrayCount(State->Assets.Grasses));

		gdVec2 BitmapOffset = { (real32)BitmapToRender->Width * 0.5f,
			(real32)BitmapToRender->Height * 0.5f };

		gdVec2 GrassLocalPosition =
		{ RandomFromNegativeOneToOne(&Series),
		RandomFromNegativeOneToOne(&Series) };

		gdVec2 GrassScreenPosition = ScreenCenter + GrassLocalPosition * 256.0f - BitmapOffset;

		RenderBitmap(
			(loaded_bitmap*)Buffer,
			BitmapToRender,
			GrassScreenPosition);

	}
}


GAME_UPDATE_AND_RENDER(GameUpdateAndRender){

	game_state* GameState = (game_state*)Memory->MemoryBlock;
	if (!Memory->IsInitialized){

		InitializeMemoryArena(
			&GameState->PermanentArena,
			Memory->MemoryBlockSize - sizeof(game_state),
			(uint8*)Memory->MemoryBlock + sizeof(game_state));

		hero_bitmaps* Bitmap;
		Bitmap = &GameState->Assets.HeroBitmaps[0];
		Bitmap->Head = LoadBitmap(Thread, Memory->DEBUGReadEntireFile, "../Data/HH/Test/test_hero_right_head.bmp");
		Bitmap->Torso = LoadBitmap(Thread, Memory->DEBUGReadEntireFile, "../Data/HH/Test/test_hero_right_torso.bmp");
		Bitmap->Cape = LoadBitmap(Thread, Memory->DEBUGReadEntireFile, "../Data/HH/Test/test_hero_right_cape.bmp");
		Bitmap->Align = gd_vec2(72, 182);
		Bitmap++;

		Bitmap->Head = LoadBitmap(Thread, Memory->DEBUGReadEntireFile, "../Data/HH/Test/test_hero_back_head.bmp");
		Bitmap->Torso = LoadBitmap(Thread, Memory->DEBUGReadEntireFile, "../Data/HH/Test/test_hero_back_torso.bmp");
		Bitmap->Cape = LoadBitmap(Thread, Memory->DEBUGReadEntireFile, "../Data/HH/Test/test_hero_back_cape.bmp");
		Bitmap->Align = gd_vec2(72, 182);
		Bitmap++;

		Bitmap->Head = LoadBitmap(Thread, Memory->DEBUGReadEntireFile, "../Data/HH/Test/test_hero_left_head.bmp");
		Bitmap->Torso = LoadBitmap(Thread, Memory->DEBUGReadEntireFile, "../Data/HH/Test/test_hero_left_torso.bmp");
		Bitmap->Cape = LoadBitmap(Thread, Memory->DEBUGReadEntireFile, "../Data/HH/Test/test_hero_left_cape.bmp");
		Bitmap->Align = gd_vec2(72, 182);
		Bitmap++;

		Bitmap->Head = LoadBitmap(Thread, Memory->DEBUGReadEntireFile, "../Data/HH/Test/test_hero_front_head.bmp");
		Bitmap->Torso = LoadBitmap(Thread, Memory->DEBUGReadEntireFile, "../Data/HH/Test/test_hero_front_torso.bmp");
		Bitmap->Cape = LoadBitmap(Thread, Memory->DEBUGReadEntireFile, "../Data/HH/Test/test_hero_front_cape.bmp");
		Bitmap->Align = gd_vec2(72, 182);
		Bitmap++;

		GameState->Assets.Grasses[0] = LoadBitmap(Thread, Memory->DEBUGReadEntireFile, "../Data/HH/Test2/grass00.bmp");
		GameState->Assets.Grasses[1] = LoadBitmap(Thread, Memory->DEBUGReadEntireFile, "../Data/HH/Test2/grass01.bmp");

		GameState->Assets.Stones[0] = LoadBitmap(Thread, Memory->DEBUGReadEntireFile, "../Data/HH/Test2/ground00.bmp");
		GameState->Assets.Stones[1] = LoadBitmap(Thread, Memory->DEBUGReadEntireFile, "../Data/HH/Test2/ground01.bmp");
		GameState->Assets.Stones[2] = LoadBitmap(Thread, Memory->DEBUGReadEntireFile, "../Data/HH/Test2/ground02.bmp");
		GameState->Assets.Stones[3] = LoadBitmap(Thread, Memory->DEBUGReadEntireFile, "../Data/HH/Test2/ground03.bmp");

		GameState->Assets.Tufts[0] = LoadBitmap(Thread, Memory->DEBUGReadEntireFile, "../Data/HH/Test2/tuft00.bmp");
		GameState->Assets.Tufts[1] = LoadBitmap(Thread, Memory->DEBUGReadEntireFile, "../Data/HH/Test2/tuft01.bmp");

		*GetBitmap(&GameState->Assets, GAI_LastOfUs) = LoadBitmapWithSTB(Thread, Memory->DEBUGReadEntireFile, "../Data/Images/witcher.jpg");
		*GetBitmap(&GameState->Assets, GAI_Tree) = LoadBitmap(Thread, Memory->DEBUGReadEntireFile, "../Data/HH/test2/tree00.bmp");
		*GetBitmap(&GameState->Assets, GAI_Backdrop) = LoadBitmap(Thread, Memory->DEBUGReadEntireFile, "../Data/HH/test/test_background.bmp");


		uint32 Min = RandomNumberTable[0];
		uint32 Max = RandomNumberTable[0];
		int RandArrSize = ArrayCount(RandomNumberTable);
		for (int i = 0; i < RandArrSize; i++){
			if (RandomNumberTable[i] < Min){
				Min = RandomNumberTable[i];
			}
			if (RandomNumberTable[i] > Max){
				Max = RandomNumberTable[i];
			}
		}


		Memory->IsInitialized = true;
	}

	transient_state* TranState = (transient_state*)Memory->TempMemoryBlock;
	if (!TranState->IsInitialized){
		InitializeMemoryArena(
			&TranState->TranArena,
			Memory->TempMemoryBlockSize - sizeof(transient_state),
			(uint8*)Memory->TempMemoryBlock + sizeof(transient_state));

		TranState->GroundBitmap = MakeEmptyBitmap(&GameState->PermanentArena, Buffer->Width, Buffer->Height);
		RenderGround(&TranState->GroundBitmap, GameState);

		TranState->IsInitialized = true;
	}

	temporary_memory RenderMemory = BeginTemporaryMemory(&TranState->TranArena);
	render_group* RenderGroup = AllocateRenderGroup(&TranState->TranArena, GD_MEGABYTES(4), 42);

	int temp1 = ArrayCount(Input->Controllers[0].Buttons);
	Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) ==
		(ArrayCount(Input->Controllers[0].Buttons)));

	int ToneHz = 256;

	gdVec2 MoveVector = gd_vec2_zero();

	for (int ControllerIndex = 0;
		ControllerIndex < ArrayCount(Input->Controllers);
		ControllerIndex++)
	{
		game_controller_input* Controller = &Input->Controllers[ControllerIndex];

		if (Controller->MoveUp.EndedDown){
			ToneHz = 170;
			MoveVector.y -= 10;
			GameState->HeroFacingDirection = 1;
		}

		if (Controller->MoveDown.EndedDown){
			ToneHz = 400;
			MoveVector.y += 10;
			GameState->HeroFacingDirection = 3;
		}

		if (Controller->MoveRight.EndedDown){
			MoveVector.x += 10;
			GameState->HeroFacingDirection = 0;
		}

		if (Controller->MoveLeft.EndedDown){
			MoveVector.x -= 10;
			GameState->HeroFacingDirection = 2;
		}

		real32 PlayerSpeed = 100;
		GameState->PlayerPos = GameState->PlayerPos + gd_vec2_norm0(MoveVector) * Input->DeltaTime * PlayerSpeed;

		// if (Controller->IsAnalog){
		//     OffsetX -= Controller->StickAverageX * 10;
		//     OffsetY += Controller->StickAverageY * 10;
		// }
	}

	//GameOutputSound(SoundOutput, ToneHz);
	//DEBUGRenderGrad(Buffer, OffsetX, OffsetY);


	PushClear(RenderGroup, gd_vec4(0.1f, 0.1f, 0.1f, 1.0f));

	for (int i = 0; i < 5; i++){
		if (Input->MouseButtons[i].EndedDown){
			PushRectangle(RenderGroup, gd_vec3(10 + i * 20, 10, 0.0f), gd_vec2(10, 10), gd_vec4(0.0f, 1.0f, 0.0f, 1.0f));
		}
	}

	//PushBitmap(RenderGroup, GetBitmap(&GameState->Assets, GAI_LastOfUs), gd_vec3_zero(), gd_vec2_zero(), gd_vec4(1.0f, 1.0f, 1.0f, 1.0f));
	
	hero_bitmaps* HeroBitmaps = &GameState->Assets.HeroBitmaps[GameState->HeroFacingDirection];
	
	PushBitmap(RenderGroup, &HeroBitmaps->Head, gd_vec3(800, 500, 0), gd_vec2_zero(), gd_vec4(1.0f, 1.0f, 1.0f, 1.0f));

	for (int i = 0; i < 4; i++){
		PushBitmap(RenderGroup, GetBitmap(&GameState->Assets, GAI_Tree), gd_vec3(200 + i * 150, 600, 0), gd_vec2_zero(), gd_vec4(1.0f, 1.0f, 1.0f, 1.0f));
	}

	//
	//PushBitmap(RenderGroup, &HeroBitmaps->Torso, gd_vec3_from_vec2(GameState->PlayerPos, 0.0f), HeroBitmaps->Align);
	//PushBitmap(RenderGroup, &HeroBitmaps->Cape, gd_vec3_from_vec2(GameState->PlayerPos, 0.0f), HeroBitmaps->Align);
	//PushBitmap(RenderGroup, &HeroBitmaps->Head, gd_vec3_from_vec2(GameState->PlayerPos, 0.0f), HeroBitmaps->Align);

	//RenderBitmap(Buffer, GetBitmap(&GameState->Assets, GAI_Backdrop), 0, 0);

	//PushRectangle(RenderGroup, gd_vec3(100, 100, 0), gd_vec2(500, 300), gd_vec4(0.5f, 0.5f, 0.5f, 1.0f));
	//PushRectangleOutline(RenderGroup, gd_vec3(100, 100, 0), gd_vec2(500, 300));
	PushRectangle(RenderGroup, gd_vec3_from_vec2(Input->MouseP.xy, 0.0f), gd_vec2(10, 10), gd_vec4(0.0f, 1.0f, 0.0f, 1.0f));

	GameState->Time += Input->DeltaTime;
	real32 Angle = GameState->Time;
	gdVec2 Origin = gd_vec2(400, 300);
	real32 AngleW = 0.5f;
	gdVec2 XAxis = 400.0f * gd_vec2(gd_cos(Angle  * AngleW), gd_sin(Angle * AngleW));
	gdVec2 YAxis = 400.0f * gd_vec2(-gd_sin(Angle * AngleW), gd_cos(Angle * AngleW));
	real32 OffsetX = cosf(Angle * 0.5f) * 200.0f;
	//gdVec2 XAxis = 100.0f * gd_vec2(1, 0);
	//gdVec2 YAxis = 100.0f * gd_vec2(0, 1);
	PushCoordinateSystem(RenderGroup,
		Origin + gd_vec2(OffsetX, 0.0f) - 0.5f * XAxis - 0.5f * YAxis,
		XAxis, YAxis,
		gd_vec4(
		1.0f,
		1.0f,
		1.0f,
		1.0f),
		GetBitmap(&GameState->Assets, GAI_Tree));

	RenderGroupToOutput(RenderGroup, (loaded_bitmap*)Buffer);

	EndTemporaryMemory(RenderMemory);
}
