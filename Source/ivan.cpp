#include "ivan.h"
#include "ivan_render_group.cpp"
#include "ivan_asset.cpp"
#include "ivan_audio.cpp"
#include "ivan_particle.cpp"
#include "ivan_render.cpp"
#include "ivan_voxel_mesh.cpp"
#include "ivan_voxel_world.cpp"

INTERNAL_FUNCTION void OverlayCycleCounters(game_memory* Memory, render_group* RenderGroup);

INTERNAL_FUNCTION void UpdateCameraVectors(
    camera_transform* Camera, 
    float DeltaYaw = 0.0f, 
    float DeltaPitch = 0.0f, 
    float DeltaRoll = 0.0f,
    vec3 WorldUp = Vec3(0.0f, 1.0f, 0.0f))
{
    float DeadEdge = 89.0f * IVAN_DEG_TO_RAD;

    Camera->Yaw += DeltaYaw;
    Camera->Pitch = IVAN_MATH_CLAMP(Camera->Pitch + DeltaPitch, -DeadEdge, DeadEdge);
    Camera->Roll += DeltaRoll;

    vec3 Front;
    Front.x = Cos(Camera->Yaw) * Cos(Camera->Pitch);
    Front.y = Sin(Camera->Pitch);
    Front.z = Sin(Camera->Yaw) * Cos(Camera->Pitch);

    vec3 Left = Normalize(Cross(WorldUp, Front));
    vec3 Up = Normalize(Cross(Front, Left));

    Camera->Front = Front;
    Camera->Left = Left;
    Camera->Up = Up;
}

inline task_with_memory* BeginTaskWithMemory_(task_with_memory* Tasks, int32 TasksCount){
    task_with_memory* FoundTask = 0;

    for (int i = 0; i < TasksCount; i++){
        task_with_memory* Task = &Tasks[i];
        if(AtomicCompareExchangeUInt32((uint32* volatile)&Task->BeingUsed, true, false) == false){
            FoundTask = Task;
            Task->Memory = BeginTemporaryMemory(&Task->Arena);
            break;
        }
    }
    return(FoundTask);
}

INTERNAL_FUNCTION task_with_memory* BeginTaskWithMemory(transient_state* TranState){
	task_with_memory* FoundTask = BeginTaskWithMemory_(TranState->Tasks, ArrayCount(TranState->Tasks));
	return(FoundTask);
}

INTERNAL_FUNCTION task_with_memory* BeginChunkTaskWithMemory(transient_state* TranState){
    task_with_memory* FoundTask = BeginTaskWithMemory_(TranState->ChunkTasks, ArrayCount(TranState->ChunkTasks));
    return(FoundTask);
}

INTERNAL_FUNCTION task_with_memory* BeginMeshTaskWithMemory(transient_state* TranState){
    task_with_memory* FoundTask = BeginTaskWithMemory_(TranState->MeshTasks, ArrayCount(TranState->MeshTasks));
    return(FoundTask);
}

INTERNAL_FUNCTION void EndTaskWithMemory(task_with_memory* Task){
	EndTemporaryMemory(Task->Memory);

	GD_COMPLETE_WRITES_BEFORE_FUTURE;
	Task->BeingUsed = false;
}

GLOBAL_VARIABLE float CursorY;
GLOBAL_VARIABLE float FontScale;
GLOBAL_VARIABLE font_id FontID;

INTERNAL_FUNCTION void DEBUGTextReset(render_group* RenderGroup){
    TIMED_BLOCK();

    asset_vector MatchVector = {};
    MatchVector.Data[Tag_FontType] = FontType_Debug;
    asset_vector WeightVector = {};
    WeightVector.Data[Tag_FontType] = 10.0f;
    FontID = GetBestMatchFontFrom(RenderGroup->Assets, Asset_Font, &MatchVector, &WeightVector);
    
    FontScale = 1.0f;

    dda_font* Info = GetFontInfo(RenderGroup->Assets, FontID);
    CursorY = GetStartingBaselineY(Info) * FontScale;
}

INTERNAL_FUNCTION void DEBUGTextOut(render_group* RenderGroup, char* String){
    TIMED_BLOCK();

    loaded_font* Font = PushFont(RenderGroup, FontID);
    
    if(Font){        

        dda_font* Info = GetFontInfo(RenderGroup->Assets, FontID);

        float CharScale = FontScale;
        float CursorX = 0;
        uint32 PrevCodePoint = 0;
        vec4 Color = Vec4(1.0f, 1.0f, 1.0f, 1.0f);

        char* Ptr = String;
        while(*Ptr){

            if((Ptr[0] == '\\') &&
                (Ptr[1] == '#') &&
                (Ptr[2] != 0) &&
                (Ptr[3] != 0) &&
                (Ptr[4] != 0))
            {
                float ColorScale = 1.0f / 9.0f;
                Color = Vec4(
                    IVAN_MATH_CLAMP01(ColorScale * (float)(Ptr[2] - '0')),
                    IVAN_MATH_CLAMP01(ColorScale * (float)(Ptr[3] - '0')),
                    IVAN_MATH_CLAMP01(ColorScale * (float)(Ptr[4] - '0')),
                    1.0f);
                Ptr += 5;
            }
            else if((Ptr[0] == '\\') &&
                (Ptr[1] == '^') &&
                (Ptr[2] != 0))
            {
                float ScaleScale = 1.0f / 9.0f;
                CharScale = FontScale * IVAN_MATH_CLAMP01(ScaleScale * (float)(Ptr[2] - '0'));
                Ptr += 3;
            }
            else{    
                uint32 CodePoint = *Ptr;

                float AdvanceX = CharScale * GetHorizontalAdvanceForPair(Info, Font, PrevCodePoint, CodePoint);
                CursorX += AdvanceX;

                if(CodePoint != ' '){

                    bitmap_id BitmapID = GetBitmapForGlyph(RenderGroup->Assets, Info, Font, CodePoint);
                    dda_bitmap* BitmapInfo = GetBitmapInfo(RenderGroup->Assets, BitmapID);

                    PushBitmap(RenderGroup, BitmapID, -CharScale * (float)BitmapInfo->Dimension[1], Vec3(CursorX, CursorY, 0), Color, true);
                }

                PrevCodePoint = CodePoint;
                Ptr++;
            }

        }

        CursorY += GetLineAdvanceFor(Info) * CharScale;
    }
}

#ifdef INTERNAL_BUILD
game_memory* DebugGlobalMemory;
#endif
platform_add_entry* PlatformAddEntry;
platform_complete_all_work* PlatformCompleteAllWork;

GD_DLL_EXPORT GAME_UPDATE_AND_RENDER(GameUpdateAndRender){
    TIMED_BLOCK();

    Platform = Memory->PlatformAPI;

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

        GameState->Camera = {};
        GameState->Camera.Left = {1.0f, 0.0f, 0.0f};
        GameState->Camera.Up = {0.0f, 1.0f, 0.0f};
        GameState->Camera.Front = {0.0f, 0.0f, 1.0f};

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

        for(int MeshTaskIndex = 0;
            MeshTaskIndex < ArrayCount(TranState->MeshTasks);
            MeshTaskIndex++)
        {
            task_with_memory* Task = TranState->MeshTasks + MeshTaskIndex;
            Task->BeingUsed = false;
            SubArena(&Task->Arena, &TranState->TranArena, GD_KILOBYTES(4700));
        }

        for(int ChunkTaskIndex = 0;
            ChunkTaskIndex < ArrayCount(TranState->ChunkTasks);
            ChunkTaskIndex++)
        {
            task_with_memory* Task = TranState->ChunkTasks + ChunkTaskIndex;
            Task->BeingUsed = false;
            SubArena(&Task->Arena, &TranState->TranArena, GD_KILOBYTES(66));
        }

        TranState->Assets = AllocateGameAssets(&TranState->TranArena, GD_MEGABYTES(64), TranState, &Memory->TextureOpQueue);
        InitParticleCache(&GameState->FontainCache, TranState->Assets);

        //PlaySound(&GameState->AudioState, GetFirstSoundFrom(TranState->Assets, Asset_Music));

        TranState->VoxelChunkManager = AllocateVoxelChunkManager(TranState, TranState->Assets);

        TranState->IsInitialized = true;
    }

    if(TranState->MainGenerationID){
        EndGeneration(TranState->Assets, TranState->MainGenerationID);
    }
    TranState->MainGenerationID = BeginGeneration(TranState->Assets);

    temporary_memory RenderMemory = BeginTemporaryMemory(&TranState->TranArena);
    render_group RenderGroup_ = BeginRenderGroup(
        TranState->Assets,
        RenderCommands,
        TranState->MainGenerationID);
    render_group* RenderGroup = &RenderGroup_;    

    real32 WidthOfMonitor = 0.635f;
    real32 MetersToPixels = (real32)Buffer->Width / WidthOfMonitor / 8;
    SetOrthographic(RenderGroup, Buffer->Width, Buffer->Height, MetersToPixels);

    SetCameraTransform(
        RenderGroup,
        0, 
        GameState->Camera.P, 
        GameState->Camera.Left,
        GameState->Camera.Up,
        GameState->Camera.Front);

    int temp1 = ArrayCount(Input->Controllers[0].Buttons);
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) ==
        (ArrayCount(Input->Controllers[0].Buttons)));

    vec2 MoveVector = Vec2(0.0f);
    vec3 CameraMoveVector = Vec3(0.0f);

    float MouseSpeed = 0.1f;
    float DeltaYaw = 0.0f;
    float DeltaPitch = 0.0f;
#if 0
    float DeltaYaw = (-(Input->MouseP.x - GameState->LastMouseP.x)) * IVAN_DEG_TO_RAD * MouseSpeed;
    float DeltaPitch = (Input->MouseP.y - GameState->LastMouseP.y) * IVAN_DEG_TO_RAD * MouseSpeed;
#else
    if(Input->CapturingMouse && (Input->DeltaMouseP.x != 0 || Input->DeltaMouseP.y != 0)){
        DeltaYaw = (Input->DeltaMouseP.x) * IVAN_DEG_TO_RAD * MouseSpeed;
        DeltaPitch = (Input->DeltaMouseP.y) * IVAN_DEG_TO_RAD * MouseSpeed;
    }
#endif
    UpdateCameraVectors(&GameState->Camera, DeltaYaw, DeltaPitch, 0.0f);
    
    for (int ControllerIndex = 0;
        ControllerIndex < ArrayCount(Input->Controllers);
        ControllerIndex++)
    {
        game_controller_input* Controller = &Input->Controllers[ControllerIndex];

        float AxisDeltaValue = 10.0f;
        if (Controller->MoveUp.EndedDown){
            MoveVector.y += 10;
            CameraMoveVector.z += AxisDeltaValue;
            GameState->HeroFacingDirection = 0.25f * IVAN_MATH_TAU;
        }

        if (Controller->MoveDown.EndedDown){
            MoveVector.y -= 10;
            CameraMoveVector.z -= AxisDeltaValue;
            GameState->HeroFacingDirection = 0.75f * IVAN_MATH_TAU;
        }

        if (Controller->MoveRight.EndedDown){
            MoveVector.x += 10;
            CameraMoveVector.x -= AxisDeltaValue;
            GameState->HeroFacingDirection = 0.0f * IVAN_MATH_TAU;
        }

        if (Controller->MoveLeft.EndedDown){
            MoveVector.x -= 10;
            CameraMoveVector.x += AxisDeltaValue;
            GameState->HeroFacingDirection = 0.5f * IVAN_MATH_TAU;
        }

        if(Controller->Space.EndedDown && Controller->Space.HalfTransitionCount == 0){
            PlaySound(&GameState->AudioState, GetFirstSoundFrom(TranState->Assets, Asset_Bloop));
        }

        real32 PlayerSpeed = 0.4f;
        real32 CameraSpeed = 2.0f;
        GameState->PlayerPos = GameState->PlayerPos + Normalize0(MoveVector) * Input->DeltaTime * PlayerSpeed;

        CameraMoveVector = CameraMoveVector * Input->DeltaTime * CameraSpeed;
        GameState->Camera.P += GameState->Camera.Front * CameraMoveVector.z;
        GameState->Camera.P -= GameState->Camera.Left * CameraMoveVector.x;
        GameState->Camera.P += GameState->Camera.Up * CameraMoveVector.y;
    }


    PushClear(RenderGroup, Vec4(0.1f, 0.1f, 0.1f, 1.0f));
    //PushBitmap(RenderGroup, GetFirstBitmapFrom(TranState->Assets, Asset_LastOfUs), 4.0f, Vec3(0.0f));

    UpdateVoxelChunks(TranState->VoxelChunkManager, RenderGroup, GameState->Camera.P);

    hero_bitmap_ids HeroBitmaps = {};
    asset_vector MatchVector = {};
    MatchVector.Data[Tag_FacingDirection] = GameState->HeroFacingDirection;
    asset_vector WeightVector = {};
    WeightVector.Data[Tag_FacingDirection] = 1.0f;

    HeroBitmaps.Head = GetBestMatchBitmapFrom(TranState->Assets, Asset_Head, &MatchVector, &WeightVector);
    HeroBitmaps.Cape = GetBestMatchBitmapFrom(TranState->Assets, Asset_Cape, &MatchVector, &WeightVector);
    HeroBitmaps.Torso = GetBestMatchBitmapFrom(TranState->Assets, Asset_Torso, &MatchVector, &WeightVector);

    real32 PlayerSizeConst = 0.8f;
    PushBitmap(RenderGroup, HeroBitmaps.Torso, PlayerSizeConst, Vec3(GameState->PlayerPos, 0.0f));
    PushBitmap(RenderGroup, HeroBitmaps.Cape, PlayerSizeConst, Vec3(GameState->PlayerPos, 0.0f));
    PushBitmap(RenderGroup, HeroBitmaps.Head, PlayerSizeConst, Vec3(GameState->PlayerPos, 0.0f));

    GameState->Time += Input->DeltaTime;
    GameState->LastMouseP = Input->MouseP;

    real32 Angle = GameState->Time;
    vec2 Origin = Vec2(400, 300);
    real32 AngleW = 0.5f;
    vec2 XAxis = 300.0f * Vec2(Cos(Angle  * AngleW), Sin(Angle * AngleW));
    vec2 YAxis = 300.0f * Vec2(-Sin(Angle * AngleW), Cos(Angle * AngleW));
    real32 OffsetX = Cos(Angle * 0.4f) / 4.0f;
    
/*
    SpawnFontain(&GameState->FontainCache, Vec3(0.0f, 0.0f, 0.0f));    
    UpdateAndRenderParticleSystems(&GameState->FontainCache, Input->DeltaTime, RenderGroup, Vec3(0.0f));
*/
    OverlayCycleCounters(Memory, RenderGroup);

    DEBUGTextReset(RenderGroup);

    //TiledRenderGroupToOutput(Memory->HighPriorityQueue, RenderGroup->Commands, (loaded_bitmap*)Buffer);
    
    EndTemporaryMemory(RenderMemory);
}

GD_DLL_EXPORT GAME_GET_SOUND_SAMPLES(GameGetSoundSamples){
    game_state* GameState = (game_state*)Memory->PermanentStorage;
    transient_state* TranState = (transient_state*)Memory->TransientStorage;

    OutputPlayingSounds(&GameState->AudioState, SoundOutput, TranState->Assets, &GameState->PermanentArena);
}

debug_record DebugRecordArray[__COUNTER__];

#include <stdio.h>

INTERNAL_FUNCTION void 
OutputDebugRecords(debug_record* Counters, uint32 CountersCount, render_group* RenderGroup){
    for(int CounterIndex = 0;
        CounterIndex < CountersCount;
        CounterIndex++)
    {
        debug_record* Counter = Counters + CounterIndex;

        uint64 HitCount_CycleCount = AtomicExchangeU64(&Counter->HitCount_CycleCount, 0);
        uint32 HitCount = (uint32)(HitCount_CycleCount >> 32);
        uint32 CycleCount = (uint32)(HitCount_CycleCount & 0xFFFFFFFF);

        if(HitCount){
            char TextBuffer[256];

            stbsp_sprintf(TextBuffer, "%32s(%4d): %10ucy %8uh %10ucy/h",
                Counter->FunctionName,
                Counter->LineNumber,
                CycleCount,
                HitCount,
                CycleCount / HitCount);

            DEBUGTextOut(RenderGroup, TextBuffer);
        }
    }

    char TextBuffer[256];

    stbsp_sprintf(TextBuffer, "ViewPosition: x:%.2f y:%.2f z:%.2f", 
        RenderGroup->LastRenderSetup.CameraP.x,
        RenderGroup->LastRenderSetup.CameraP.y,
        RenderGroup->LastRenderSetup.CameraP.z);
    DEBUGTextOut(RenderGroup, TextBuffer);
}

INTERNAL_FUNCTION void
OverlayCycleCounters(game_memory* Memory, render_group* RenderGroup){
#ifdef INTERNAL_BUILD
    DEBUGTextOut(RenderGroup, "\\#900DEBUG \\#090CYCLE \\#990\\COUNTS");
    
    OutputDebugRecords(
        DebugRecords_MainTranslationUnit, 
        ArrayCount(DebugRecords_MainTranslationUnit), 
        RenderGroup);
#endif
}