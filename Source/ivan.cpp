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
        if(AtomicCompareExchangeU32((uint32* volatile)&Task->BeingUsed, true, false) == false){
            FoundTask = Task;
            Task->Memory = BeginTemporaryMemory(&Task->Arena);
            break;
        }
    }
    return(FoundTask);
}

INTERNAL_FUNCTION task_with_memory* BeginTaskWithMemory(transient_state* TranState){
	task_with_memory* FoundTask = BeginTaskWithMemory_(TranState->Tasks, ArrayCount(TranState->Tasks));
    TranState->TasksInUse++;
	return(FoundTask);
}

INTERNAL_FUNCTION task_with_memory* BeginChunkTaskWithMemory(transient_state* TranState){
    task_with_memory* FoundTask = BeginTaskWithMemory_(TranState->ChunkTasks, ArrayCount(TranState->ChunkTasks));
    TranState->ChunkTasksInUse++;
    return(FoundTask);
}

INTERNAL_FUNCTION task_with_memory* BeginMeshTaskWithMemory(transient_state* TranState){
    task_with_memory* FoundTask = BeginTaskWithMemory_(TranState->MeshTasks, ArrayCount(TranState->MeshTasks));
    TranState->MeshTasksInUse++;
    return(FoundTask);
}

INTERNAL_FUNCTION void EndTaskWithMemory(task_with_memory* Task){
	EndTemporaryMemory(Task->Memory);

	IVAN_COMPLETE_WRITES_BEFORE_FUTURE;
	Task->BeingUsed = false;
}

INTERNAL_FUNCTION uint32
DEBUGGetMainGenerationID(game_memory* Memory){
    uint32 Result = 0;

    transient_state* TranState = (transient_state*)Memory->TransientStorage;
    if(TranState->IsInitialized){
        Result = TranState->MainGenerationID;
    }

    return(Result);
}

INTERNAL_FUNCTION game_assets*
DEBUGGetGameAssets(game_memory* Memory){
    game_assets* Assets = 0;

    transient_state *TranState = (transient_state *)Memory->TransientStorage;
    if(TranState->IsInitialized)
    {
        Assets = TranState->Assets;
    }

    return(Assets);
}

#if IVAN_INTERNAL
game_memory* DebugGlobalMemory;
debug_table* GlobalDebugTable;
#endif

IVAN_DLL_EXPORT GAME_UPDATE_AND_RENDER(GameUpdateAndRender){

    Platform = Memory->PlatformAPI;

#if IVAN_INTERNAL
    DebugGlobalMemory = Memory;
    GlobalDebugTable = Memory->DebugTable;
#endif

    TIMED_FUNCTION();
    
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
        TranState->VoxelMeshQueue = Memory->VoxelMeshQueue;

        TranState->TasksInUse = 0;
        for (int TaskIndex = 0;
            TaskIndex < ArrayCount(TranState->Tasks);
            TaskIndex++)
        {
            task_with_memory* Task = TranState->Tasks + TaskIndex;
            Task->BeingUsed = false;
            SubArena(&Task->Arena, &TranState->TranArena, IVAN_MEGABYTES(1));
        }

        TranState->MeshTasksInUse = 0;
        for(int MeshTaskIndex = 0;
            MeshTaskIndex < ArrayCount(TranState->MeshTasks);
            MeshTaskIndex++)
        {
            task_with_memory* Task = TranState->MeshTasks + MeshTaskIndex;
            Task->BeingUsed = false;
            SubArena(&Task->Arena, &TranState->TranArena, IVAN_KILOBYTES(1));
        }

        TranState->ChunkTasksInUse = 0;
        for(int ChunkTaskIndex = 0;
            ChunkTaskIndex < ArrayCount(TranState->ChunkTasks);
            ChunkTaskIndex++)
        {
            task_with_memory* Task = TranState->ChunkTasks + ChunkTaskIndex;
            Task->BeingUsed = false;
            SubArena(&Task->Arena, &TranState->TranArena, IVAN_KILOBYTES(66));
        }

        TranState->Assets = AllocateGameAssets(&TranState->TranArena, IVAN_MEGABYTES(64), TranState, &Memory->TextureOpQueue);
        //InitializeAnimatorController(&GameState->AnimatorController, , TranState->Assets);
        //InitParticleCache(&GameState->FontainCache, TranState->Assets);

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
        DeltaPitch = -(Input->DeltaMouseP.y) * IVAN_DEG_TO_RAD * MouseSpeed;
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
        real32 CameraSpeed = 20.0f;
        GameState->PlayerPos = GameState->PlayerPos + Normalize0(MoveVector) * Input->DeltaTime * PlayerSpeed;

        CameraMoveVector = Normalize0(CameraMoveVector) * Input->DeltaTime * CameraSpeed;
        GameState->Camera.P += GameState->Camera.Front * CameraMoveVector.z;
        GameState->Camera.P -= GameState->Camera.Left * CameraMoveVector.x;
        GameState->Camera.P += GameState->Camera.Up * CameraMoveVector.y;
    }


    PushClear(RenderGroup, Vec4(0.05f, 0.05f, 0.05f, 1.0f));
    //PushBitmap(RenderGroup, GetFirstBitmapFrom(TranState->Assets, Asset_LastOfUs), 4.0f, Vec3(0.0f));


#if IVAN_VOXEL_WORLD_MULTITHREADED
    UpdateVoxelChunksMultithreaded(TranState->VoxelChunkManager, RenderGroup, GameState->Camera.P);
#else
    UpdateVoxelChunks(TranState->VoxelChunkManager, RenderGroup, GameState->Camera.P);
#endif

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

/*
    render_group* RenderGroup,
    vec3 Offset,
    vec2 Dim,
    bool32 ScreenSpace = false,
    real32 Thickness = 4,
    vec4 Color = Vec4(0.0f, 0.0f, 0.0f, 1.0f))
*/

/*
    PushRectangleOutline(RenderGroup, Vec3(30,30, 0), Vec2(100, 100), true);
    PushRectangle(RenderGroup, Vec3(30, 30, 0), Vec2(100, 100), Vec4(1.0f, 0.6f, 0.0f, 1.0f), true);
*/
    PushRectangle(RenderGroup, Input->MouseP, Vec2(10, 10), Vec4(1.0f, 1.0f, 1.0f, 1.0f), true);

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

    //TiledRenderGroupToOutput(Memory->HighPriorityQueue, RenderGroup->Commands, (loaded_bitmap*)Buffer);
    
    EndTemporaryMemory(RenderMemory);
}

#if IVAN_INTERNAL
#include "ivan_debug.cpp"
#else
#endif

IVAN_DLL_EXPORT GAME_GET_SOUND_SAMPLES(GameGetSoundSamples){
    game_state* GameState = (game_state*)Memory->PermanentStorage;
    transient_state* TranState = (transient_state*)Memory->TransientStorage;

    OutputPlayingSounds(&GameState->AudioState, SoundOutput, TranState->Assets, &GameState->PermanentArena);
}
