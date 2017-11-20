#include "ivan.h"
#include "ivan_render_group.cpp"
#include "ivan_asset.cpp"
#include "ivan_audio.cpp"
#include "ivan_particle.cpp"
#include "ivan_render.cpp"
#include "ivan_voxel_mesh.cpp"
#include "ivan_voxel_world.cpp"
#include "ivan_cutscene.cpp"
#include "ivan_texted.cpp"
#include "ivan_anim.cpp"
#include "ivan_world_mode.cpp"

INTERNAL_FUNCTION void OverlayCycleCounters(game_memory* Memory, render_group* RenderGroup);

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
		GameState->EngineMode = EngineMode_World;

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

    loaded_bitmap DrawBuffer = {};
    DrawBuffer.Width = RenderCommands->Width;
    DrawBuffer.Height = RenderCommands->Height;

    bool32 Rerun = false;
    do{
        switch(GameState->EngineMode){
            case EngineMode_TitleScreen:{

            }break;

            case EngineMode_CutScene:{

            }break;

            case EngineMode_World:{
                Rerun = UpdateAndRenderWorld(
                    GameState, GameState->WorldMode,
                    TranState, Input, 
                    RenderGroup, &DrawBuffer);
            }break;

            case EngineMode_TextEditor:{

            }break;

            INVALID_DEFAULT_CASE;
        }
    }while(Rerun);

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
