#include "ivan.h"
#include "ivan_opengl.cpp"
#include "ivan_render_group.cpp"
#include "ivan_asset.cpp"
#include "ivan_audio.cpp"

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
    
    Platform.AddEntry = Memory->PlatformAPI.AddEntry;
    Platform.CompleteAllWork = Memory->PlatformAPI.CompleteAllWork;
    
    Platform.GetAllFilesOfTypeBegin = Memory->PlatformAPI.GetAllFilesOfTypeBegin;
    Platform.GetAllFilesOfTypeEnd = Memory->PlatformAPI.GetAllFilesOfTypeEnd;
    Platform.OpenNextFile = Memory->PlatformAPI.OpenNextFile;
    Platform.ReadDataFromFile = Memory->PlatformAPI.ReadDataFromFile;
    Platform.FileError = Memory->PlatformAPI.FileError;

    Platform.DEBUGReadEntireFile = Memory->PlatformAPI.DEBUGReadEntireFile;
    Platform.DEBUGFreeFileMemory = Memory->PlatformAPI.DEBUGFreeFileMemory;

#ifdef INTERNAL_BUILD
    DebugGlobalMemory = Memory;
#endif
    game_state* GameState = (game_state*)Memory->PermanentStorage;

    if (!Memory->IsInitialized){

        InitializeMemoryArena(
            &GameState->PermanentArena,
            Memory->PermanentStorageSize - sizeof(game_state),
            (uint8*)Memory->PermanentStorage + sizeof(game_state));
        
        GameState->EffectsSeries = RandomSeed(1234);

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
    render_group* RenderGroup = AllocateRenderGroup(TranState->Assets, &TranState->TranArena, GD_MEGABYTES(10));
    real32 WidthOfMonitor = 0.635f;
    real32 MetersToPixels = (real32)Buffer->Width / WidthOfMonitor / 8;
    //real32 MetersToPixels = 1;
    SetOrthographic(RenderGroup, Buffer->Width, Buffer->Height, MetersToPixels);

    int temp1 = ArrayCount(Input->Controllers[0].Buttons);
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) ==
        (ArrayCount(Input->Controllers[0].Buttons)));

    int ToneHz = 256;

    vec2 MoveVector = Vec2(0.0f);

    for (int ControllerIndex = 0;
        ControllerIndex < ArrayCount(Input->Controllers);
        ControllerIndex++)
    {
        game_controller_input* Controller = &Input->Controllers[ControllerIndex];

        if (Controller->MoveUp.EndedDown){
            ToneHz = 170;
            MoveVector.y -= 10;
            GameState->HeroFacingDirection = 0.25f * IVAN_MATH_TAU;
        }

        if (Controller->MoveDown.EndedDown){
            ToneHz = 400;
            MoveVector.y += 10;
            GameState->HeroFacingDirection = 0.75f * IVAN_MATH_TAU;
        }

        if (Controller->MoveRight.EndedDown){
            MoveVector.x += 10;
            GameState->HeroFacingDirection = 0.0f * IVAN_MATH_TAU;
        }

        if (Controller->MoveLeft.EndedDown){
            MoveVector.x -= 10;
            GameState->HeroFacingDirection = 0.5f * IVAN_MATH_TAU;
        }

        if(Controller->Space.EndedDown && Controller->Space.HalfTransitionCount == 0){
            PlaySound(&GameState->AudioState, GetFirstSoundFrom(TranState->Assets, Asset_Bloop));
        }

        real32 PlayerSpeed = 0.4f;
        GameState->PlayerPos = GameState->PlayerPos + Normalize0(MoveVector) * Input->DeltaTime * PlayerSpeed;

        /*
        if (Controller->IsAnalog){
            OffsetX -= Controller->StickAverageX * 10;
            OffsetY += Controller->StickAverageY * 10;
        }
        */
    }

    PushClear(RenderGroup, Vec4(0.1f, 0.1f, 0.1f, 1.0f));
    PushBitmap(RenderGroup, GetFirstBitmapFrom(TranState->Assets, Asset_Assassin), 4.0f, Vec3(0.0f));

    for (int i = 0; i < 5; i++){
        if (Input->MouseButtons[i].EndedDown){
            PushRectangle(RenderGroup, Vec3(10 + i * 20, 10, 0.0f), Vec2(10, 10), Vec4(0.0f, 1.0f, 0.0f, 1.0f));
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

    real32 PlayerSizeConst = 1.0f;
    PushBitmap(RenderGroup, HeroBitmaps.Torso, PlayerSizeConst, Vec3(GameState->PlayerPos, 0.0f));
    PushBitmap(RenderGroup, HeroBitmaps.Cape, PlayerSizeConst, Vec3(GameState->PlayerPos, 0.0f));
    PushBitmap(RenderGroup, HeroBitmaps.Head, PlayerSizeConst, Vec3(GameState->PlayerPos, 0.0f));

    PushRectangle(RenderGroup, Vec3(Input->MouseP.xy, 0.0f), Vec2(10, 10), Vec4(0.0f, 1.0f, 0.0f, 1.0f));

    GameState->Time += Input->DeltaTime;
    real32 Angle = GameState->Time;
    vec2 Origin = Vec2(400, 300);
    real32 AngleW = 0.5f;
    vec2 XAxis = 300.0f * Vec2(Cos(Angle  * AngleW), Sin(Angle * AngleW));
    vec2 YAxis = 300.0f * Vec2(-Sin(Angle * AngleW), Cos(Angle * AngleW));
    real32 OffsetX = Cos(Angle * 0.4f) / 4.0f;
    
    MatchVector.Data[Tag_Color] = GetFloatRepresentOfColor(Vec3(1.0f, 0.0f, 0.0f));
    WeightVector.Data[Tag_Color] = 1.0f;

    bitmap_id ParticleBitmap = GetBestMatchBitmapFrom(TranState->Assets, Asset_Diamond, &MatchVector, &WeightVector);
    //bitmap_id ParticleBitmap = GetFirstBitmapFrom(TranState->Assets, Asset_Diamond);
#if 0
    for(int i = 0; i < 10; i++){
        PushBitmap(RenderGroup, BitmapToPush, 0.5f + sinf(GameState->Time) / 5.0f, Vec3(-1.2f, i * 0.1f - 0.5f, 0) + Vec3(OffsetX, 0, 0));
        PushBitmap(RenderGroup, BitmapToPush, 0.5f + sinf(GameState->Time) / 5.0f, Vec3(-1.0f, i * 0.1f - 0.5f, 0) + Vec3(OffsetX, 0, 0));
        PushBitmap(RenderGroup, BitmapToPush, 0.5f + sinf(GameState->Time) / 5.0f, Vec3(-0.8f, i * 0.1f - 0.5f, 0) + Vec3(OffsetX, 0, 0));
        PushBitmap(RenderGroup, BitmapToPush, 0.5f + sinf(GameState->Time) / 5.0f, Vec3(-0.6f, i * 0.1f - 0.5f, 0) + Vec3(OffsetX, 0, 0));
        PushBitmap(RenderGroup, BitmapToPush, 0.5f + sinf(GameState->Time) / 5.0f, Vec3(-0.4f, i * 0.1f - 0.5f, 0) + Vec3(OffsetX, 0, 0));
        PushBitmap(RenderGroup, BitmapToPush, 0.5f + sinf(GameState->Time) / 5.0f, Vec3(-0.2f, i * 0.1f - 0.5f, 0) + Vec3(OffsetX, 0, 0));
        PushBitmap(RenderGroup, BitmapToPush, 0.5f + sinf(GameState->Time) / 5.0f, Vec3(0.0f, i * 0.1f - 0.5f, 0) + Vec3(OffsetX, 0, 0));
        PushBitmap(RenderGroup, BitmapToPush, 0.5f + sinf(GameState->Time) / 5.0f, Vec3(0.2f, i * 0.1f - 0.5f, 0) + Vec3(OffsetX, 0, 0));
        PushBitmap(RenderGroup, BitmapToPush, 0.5f + sinf(GameState->Time) / 5.0f, Vec3(0.4f, i * 0.1f - 0.5f, 0) + Vec3(OffsetX, 0, 0));
        PushBitmap(RenderGroup, BitmapToPush, 0.5f + sinf(GameState->Time) / 5.0f, Vec3(0.6f, i * 0.1f - 0.5f, 0) + Vec3(OffsetX, 0, 0));
        PushBitmap(RenderGroup, BitmapToPush, 0.5f + sinf(GameState->Time) / 5.0f, Vec3(0.8f, i * 0.1f - 0.5f, 0) + Vec3(OffsetX, 0, 0));
        PushBitmap(RenderGroup, BitmapToPush, 0.5f + sinf(GameState->Time) / 5.0f, Vec3(1.0f, i * 0.1f - 0.5f, 0) + Vec3(OffsetX, 0, 0));
    }
#else
    for(uint32 ParticleSpawnIndex = 0;
        ParticleSpawnIndex < 2;
        ParticleSpawnIndex++)
    {
        particle* Particle = GameState->Particles + GameState->NextParticle++;
        if(GameState->NextParticle >= ArrayCount(GameState->Particles)){
            GameState->NextParticle = 0;
        }

        Particle->P = Vec3(RandomBetween(&GameState->EffectsSeries, -0.05f, 0.05f), 0.0f, 0.0f);
        Particle->dP = Vec3(
            RandomBetween(&GameState->EffectsSeries, -0.1f, 0.1f),
            RandomBetween(&GameState->EffectsSeries, -1.0f, -1.4f), 0.0f);
        Particle->ddP = Vec3(
            0.0f, 0.89f, 0.0f);
        Particle->Color = Vec4(1.0f, 1.0f, 1.0f, 2.0f);
        Particle->dColor = Vec4(0, 0, 0, -0.5f);
    }

    for(uint32 ParticleIndex = 0;
        ParticleIndex < ArrayCount(GameState->Particles);
        ParticleIndex++)
    {
        particle* Particle = GameState->Particles + ParticleIndex;

        Particle->P += Input->DeltaTime * Particle->dP + 0.5f * Particle->ddP * Input->DeltaTime * Input->DeltaTime;
        Particle->dP += Input->DeltaTime * Particle->ddP;
        Particle->Color += Input->DeltaTime * Particle->dColor;

        vec4 Color;
        Color.r = IVAN_MATH_CLAMP01(Particle->Color.r);
        Color.g = IVAN_MATH_CLAMP01(Particle->Color.g);
        Color.b = IVAN_MATH_CLAMP01(Particle->Color.b);
        Color.a = IVAN_MATH_CLAMP01(Particle->Color.a);

        if(Color.a > 0.9f){
            Color.a = 0.9f * (1.0f - (Color.a - 1.0f) / (-0.1f));
            //Color.a = 2.0f - Color.a;
        }

        float BounceCoefficient = 0.4f;
        float FrictionCoefficient = 0.8f;
        if(Particle->P.y >= 0.0f){
            Particle->dP.y = -Particle->dP.y * BounceCoefficient;
            Particle->dP.x = Particle->dP.x * FrictionCoefficient;
        }

        PushBitmap(RenderGroup, ParticleBitmap, 0.4f, Particle->P, Color);
    }
#endif

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
