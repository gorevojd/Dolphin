#include "ivan.h"
#include "ivan_render_group.cpp"
#include "ivan_asset.cpp"
#include "ivan_audio.cpp"
#include "ivan_particle.cpp"
#include "ivan_render.cpp"

INTERNAL_FUNCTION void OverlayCycleCounters(game_memory* Memory, render_group* RenderGroup);

INTERNAL_FUNCTION task_with_memory* BeginTaskWithMemory(transient_state* TranState, bool32 DependsOnGameMode){
	task_with_memory* FoundTask = 0;
    
	for (int i = 0; i < ArrayCount(TranState->Tasks); i++){
		task_with_memory* Task = TranState->Tasks + i;
		if (!Task->BeingUsed){
			FoundTask = Task;
			Task->BeingUsed = true;
            Task->DependsOnGameMode = DependsOnGameMode;
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

#if 0
    Platform.AddEntry = Memory->PlatformAPI.AddEntry;
    Platform.CompleteAllWork = Memory->PlatformAPI.CompleteAllWork;
    
    Platform.GetAllFilesOfTypeBegin = Memory->PlatformAPI.GetAllFilesOfTypeBegin;
    Platform.GetAllFilesOfTypeEnd = Memory->PlatformAPI.GetAllFilesOfTypeEnd;
    Platform.OpenNextFile = Memory->PlatformAPI.OpenNextFile;
    Platform.ReadDataFromFile = Memory->PlatformAPI.ReadDataFromFile;
    Platform.FileError = Memory->PlatformAPI.FileError;

    Platform.DEBUGReadEntireFile = Memory->PlatformAPI.DEBUGReadEntireFile;
    Platform.DEBUGFreeFileMemory = Memory->PlatformAPI.DEBUGFreeFileMemory;

    Platform.AllocateMemory = Memory->PlatformAPI.AllocateMemory;
    Platform.DeallocateMemory = Memory->PlatformAPI.DeallocateMemory;

    Platform.AllocateTexture = Memory->PlatformAPI.AllocateTexture;
    Platform.DeallocateTexture = Memory->PlatformAPI.DeallocateTexture;
#else
    Platform = Memory->PlatformAPI;
#endif


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

        TranState->Assets = AllocateGameAssets(&TranState->TranArena, GD_MEGABYTES(64), TranState, &Memory->TextureOpQueue);
        InitParticleCache(&GameState->FontainCache, TranState->Assets);

        //PlaySound(&GameState->AudioState, GetFirstSoundFrom(TranState->Assets, Asset_Music));
#if 0
        voxel_chunk VoxelChunk;
        VoxelChunk.Voxels = (voxel*)malloc(IVAN_MAX_VOXELS_IN_CHUNK * sizeof(voxel));
        GenerateVoxelChunk(&TranState->TranArena, &VoxelChunk, 0, 0, 0);
        voxel_chunk_mesh MeshResult;
        MeshResult.Positions = (vec3*)malloc(IVAN_MAX_MESH_CHUNK_VERTEX_COUNT * sizeof(vec3));
        MeshResult.TexCoords = (vec2*)malloc(IVAN_MAX_MESH_CHUNK_VERTEX_COUNT * sizeof(vec2));
        MeshResult.Normals = (vec3*)malloc(IVAN_MAX_MESH_CHUNK_VERTEX_COUNT * sizeof(vec3));
        MeshResult.Indices = (uint32_t*)malloc(IVAN_MAX_MESH_CHUNK_FACE_COUNT * sizeof(uint32_t));

        voxel_atlas_id VoxelAtlasID = GetFirstVoxelAtlasFrom(TranState->Assets, Asset_VoxelAtlas);

        GenerateVoxelMeshForChunk(
            &MeshResult, 
            &VoxelChunk, 
            TranState->Assets,
            VoxelAtlasID);
#endif
        TranState->IsInitialized = true;
    }
    


    temporary_memory RenderMemory = BeginTemporaryMemory(&TranState->TranArena);
    render_group* RenderGroup = AllocateRenderGroup(TranState->Assets, &TranState->TranArena, GD_MEGABYTES(10));
    BeginRender(RenderGroup);
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
            MoveVector.y += 10;
            GameState->HeroFacingDirection = 0.25f * IVAN_MATH_TAU;
        }

        if (Controller->MoveDown.EndedDown){
            ToneHz = 400;
            MoveVector.y -= 10;
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
    PushBitmap(RenderGroup, GetFirstBitmapFrom(TranState->Assets, Asset_LastOfUs), 4.0f, Vec3(0.0f));

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

#if 0
    voxel_atlas_id VoxelAtlasID = GetFirstVoxelAtlasFrom(TranState->Assets, Asset_VoxelAtlas);
    loaded_bitmap* VoxAtlBmp = PushVoxelAtlas(RenderGroup, VoxelAtlasID);
    if(VoxAtlBmp){
        
    }
#endif

/*
    SpawnFontain(&GameState->FontainCache, Vec3(0.0f, 0.0f, 0.0f));    
    UpdateAndRenderParticleSystems(&GameState->FontainCache, Input->DeltaTime, RenderGroup, Vec3(0.0f));
*/
    OverlayCycleCounters(Memory, RenderGroup);

    DEBUGTextReset(RenderGroup);

#if 1
    TiledRenderGroupToOutput(Memory->HighPriorityQueue, RenderGroup, (loaded_bitmap*)Buffer);
#else
    rectangle2 ClipRect;
    ClipRect.Min.x = 0;
    ClipRect.Min.y = 0;
    ClipRect.Max.x = Buffer->Width;
    ClipRect.Max.y = Buffer->Height;

    RenderGroupToOutput(RenderGroup, (loaded_bitmap*)Buffer, ClipRect);
#endif
    EndRender(RenderGroup);

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