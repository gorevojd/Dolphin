#include "dolphin.h"
#include "dolphin_opengl.cpp"
#include "dolphin_render_group.cpp"
#include "dolphin_asset.cpp"
#include "dolphin_audio.cpp"

INTERNAL_FUNCTION loaded_bitmap
MakeEmptyBitmap(memory_arena* Arena, int32 Width, int32 Height){
    loaded_bitmap Result = {};

    uint32 BytesPerPixel = 4;

    Result.Memory = (uint8*)Arena->BaseAddress + Arena->MemoryUsed;
    Result.Width = Width;
    Result.Height = Height;

    PushSize(Arena, Width * Height * BytesPerPixel);

    return(Result);
}

INTERNAL_FUNCTION task_with_memory* BeginTaskWithMemory(transient_state* TranState){
	task_with_memory* FoundTask = 0;
	for (int i = 0; i < ArrayCount(TranState->Tasks); i++){
		task_with_memory* Task = TranState->Tasks + i;
		if (!Task->BeingUsed){
			FoundTask = Task;
			Task->BeingUsed = true;
			Task->Memory = BeginTemporaryMemory(&Task->Arena);
			break;
		}
	}
	return(FoundTask);
}

INTERNAL_FUNCTION void EndTaskWithMemory(task_with_memory* Task){
	EndTemporaryMemory(Task->Memory);

	GD_COMPLETE_WRITES_BEFORE_FUTURE;
	Task->BeingUsed = false;
}

#ifdef INTERNAL_BUILD
game_memory* DebugGlobalMemory;
#endif
platform_add_entry* PlatformAddEntry;
platform_complete_all_work* PlatformCompleteAllWork;

GD_DLL_EXPORT GAME_UPDATE_AND_RENDER(GameUpdateAndRender){
    PlatformAddEntry = Memory->PlatformAddEntry;
    PlatformCompleteAllWork = Memory->PlatformCompleteAllWork;
    PlatformReadEntireFile = Memory->DEBUGReadEntireFile;
#ifdef INTERNAL_BUILD
    DebugGlobalMemory = Memory;
#endif
    game_state* GameState = (game_state*)Memory->PermanentStorage;



    if (!Memory->IsInitialized){

        InitializeMemoryArena(
            &GameState->PermanentArena,
            Memory->PermanentStorageSize - sizeof(game_state),
            (uint8*)Memory->PermanentStorage + sizeof(game_state));
        
        InitializeAudioState(&GameState->AudioState, &GameState->PermanentArena);

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

    transient_state* TranState = (transient_state*)Memory->TransientStorage;
    if (!TranState->IsInitialized){
        InitializeMemoryArena(
            &TranState->TranArena,
            Memory->TransientStorageSize - sizeof(transient_state),
            (uint8*)Memory->TransientStorage + sizeof(transient_state));

        TranState->GroundBitmap = MakeEmptyBitmap(&GameState->PermanentArena, Buffer->Width, Buffer->Height);
        //RenderGround(&TranState->GroundBitmap, GameState);

        TranState->LowPriorityQueue = Memory->LowPriorityQueue;
        TranState->HighPriorityQueue = Memory->HighPriorityQueue;

        for (int TaskIndex = 0;
            TaskIndex < ArrayCount(TranState->Tasks);
            TaskIndex++)
        {
            task_with_memory* Task = TranState->Tasks + TaskIndex;
            Task->BeingUsed = false;
            SubArena(&Task->Arena, &TranState->TranArena, GD_MEGABYTES(1));
        }

        TranState->Assets = AllocateGameAssets(&TranState->TranArena, GD_MEGABYTES(64), TranState);

        PlaySound(&GameState->AudioState, GetFirstSoundFrom(TranState->Assets, Asset_Music));

        TranState->IsInitialized = true;
    }

    temporary_memory RenderMemory = BeginTemporaryMemory(&TranState->TranArena);
    render_group* RenderGroup = AllocateRenderGroup(TranState->Assets, &TranState->TranArena, GD_MEGABYTES(10), 1);

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
            GameState->HeroFacingDirection = 0.25f * GD_MATH_TAU;
        }

        if (Controller->MoveDown.EndedDown){
            ToneHz = 400;
            MoveVector.y += 10;
            GameState->HeroFacingDirection = 0.75f * GD_MATH_TAU;
        }

        if (Controller->MoveRight.EndedDown){
            MoveVector.x += 10;
            GameState->HeroFacingDirection = 0.0f * GD_MATH_TAU;
        }

        if (Controller->MoveLeft.EndedDown){
            MoveVector.x -= 10;
            GameState->HeroFacingDirection = 0.5f * GD_MATH_TAU;
        }

        if(Controller->Space.EndedDown && Controller->Space.HalfTransitionCount){
            PlaySound(&GameState->AudioState, GetFirstSoundFrom(TranState->Assets, Asset_Bloop));
        }

        real32 PlayerSpeed = 100;
        GameState->PlayerPos = GameState->PlayerPos + gd_vec2_norm0(MoveVector) * Input->DeltaTime * PlayerSpeed;

        /*
        if (Controller->IsAnalog){
            OffsetX -= Controller->StickAverageX * 10;
            OffsetY += Controller->StickAverageY * 10;
        }
        */
    }

    PushClear(RenderGroup, gd_vec4(0.1f, 0.1f, 0.1f, 1.0f));
    PushBitmap(RenderGroup, GetFirstBitmapFrom(TranState->Assets, Asset_StarWars), Buffer->Height, gd_vec3_zero());

    for (int i = 0; i < 5; i++){
        if (Input->MouseButtons[i].EndedDown){
            PushRectangle(RenderGroup, gd_vec3(10 + i * 20, 10, 0.0f), gd_vec2(10, 10), gd_vec4(0.0f, 1.0f, 0.0f, 1.0f));
        }
    }

    hero_bitmap_ids HeroBitmaps = {};
    asset_vector MatchVector = {};
    MatchVector.Data[Tag_FacingDirection] = GameState->HeroFacingDirection;
    asset_vector WeightVector = {};
    WeightVector.Data[Tag_FacingDirection] = 1.0f;

    HeroBitmaps.Head = GetBestMatchBitmapFrom(TranState->Assets, Asset_Head, &MatchVector, &WeightVector);
    HeroBitmaps.Cape = GetBestMatchBitmapFrom(TranState->Assets, Asset_Cape, &MatchVector, &WeightVector);
    HeroBitmaps.Torso = GetBestMatchBitmapFrom(TranState->Assets, Asset_Torso, &MatchVector, &WeightVector);

    PushBitmap(RenderGroup, HeroBitmaps.Torso, 200, gd_vec3_from_vec2(GameState->PlayerPos, 0.0f));
    PushBitmap(RenderGroup, HeroBitmaps.Cape, 200, gd_vec3_from_vec2(GameState->PlayerPos, 0.0f));
    PushBitmap(RenderGroup, HeroBitmaps.Head, 200, gd_vec3_from_vec2(GameState->PlayerPos, 0.0f));

    PushRectangle(RenderGroup, gd_vec3_from_vec2(Input->MouseP.xy, 0.0f), gd_vec2(10, 10), gd_vec4(0.0f, 1.0f, 0.0f, 1.0f));

    GameState->Time += Input->DeltaTime;
    real32 Angle = GameState->Time;
    gdVec2 Origin = gd_vec2(400, 300);
    real32 AngleW = 0.5f;
    gdVec2 XAxis = 300.0f * gd_vec2(gd_cos(Angle  * AngleW), gd_sin(Angle * AngleW));
    gdVec2 YAxis = 300.0f * gd_vec2(-gd_sin(Angle * AngleW), gd_cos(Angle * AngleW));
    real32 OffsetX = gd_cos(Angle * 0.4f) * 200.0f;
    
    gdRect2 Rectangle1 = gd_rect2(gd_vec2(40, 40), gd_vec2(100, 200));
    PushRectangle(RenderGroup, gd_vec3(Rectangle1.Pos.x, Rectangle1.Pos.y, 0), Rectangle1.Dimension, gd_vec4(1.0f, 0.6f, 0.0f, 1.0f));
    PushRectangleOutline(RenderGroup, gd_vec3(Rectangle1.Pos.x, Rectangle1.Pos.y, 0), Rectangle1.Dimension);
    
    PushBitmap(RenderGroup, GetFirstBitmapFrom(TranState->Assets, Asset_Tree), 150, gd_vec3(200, 200, 0) + gd_vec3(OffsetX, 0, 0));

    TiledRenderGroupToOutput(Memory->HighPriorityQueue, RenderGroup, (loaded_bitmap*)Buffer);

    EndTemporaryMemory(RenderMemory);
}

GD_DLL_EXPORT GAME_GET_SOUND_SAMPLES(GameGetSoundSamples){
    game_state* GameState = (game_state*)Memory->PermanentStorage;
    transient_state* TranState = (transient_state*)Memory->TransientStorage;
/*
    audio_state* AudioState,
    game_sound_output_buffer* SoundOutput,
    game_assets* Assets,
    memory_arena* TempArena)
*/
    OutputPlayingSounds(&GameState->AudioState, SoundOutput, TranState->Assets, &GameState->PermanentArena);
}