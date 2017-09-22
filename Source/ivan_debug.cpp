#include "ivan_debug.h"
#include "ivan_debug_ui.cpp"

INTERNAL_FUNCTION void 
OutputDebugRecords(debug_state* DebugState){
    
    render_group* RenderGroup = &DebugState->RenderGroup;

#if 0
    for(int CounterIndex = 0;
        CounterIndex < ArrayCount(DebugState->DebugRecordArray);
        CounterIndex++)
    {
        debug_record* Counter = DebugState->DebugRecordArray + CounterIndex;

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
#endif

    char TextBuffer[256];

    stbsp_sprintf(TextBuffer, "ViewPosition: x:%.2f y:%.2f z:%.2f", 
        RenderGroup->LastRenderSetup.CameraP.x,
        RenderGroup->LastRenderSetup.CameraP.y,
        RenderGroup->LastRenderSetup.CameraP.z);

    TextOutAt(DebugState, Vec2(10, 20), TextBuffer);
    TextOutAt(DebugState, Vec2(10, 50), "That's not the shape of my heart");
}

INTERNAL_FUNCTION void
OverlayCycleCounters(debug_state* DebugState){
	//TextOut(RenderGrou, "\\#900DEBUG \\#090CYCLE \\#940COUNTS");
    
    OutputDebugRecords(DebugState);
}

inline debug_state* 
DEBUGGetState(game_memory* Memory){
	debug_state* DebugState = 0;
	if(Memory){
		DebugState = (debug_state*)Memory->DebugStorage;
		if(!DebugState->Initialized){
			DebugState = 0;
		}
	}
	return(DebugState);
}

inline debug_state*
DEBUGGetState(){
	debug_state* Result = DEBUGGetState(DebugGlobalMemory);

	return(Result);
}

INTERNAL_FUNCTION void DEBUGStart(
	debug_state* DebugState, 
	game_render_commands* Commands,
	game_assets* Assets,
	uint32 MainGenerationID,
	uint32 Width, uint32 Height)
{

	if(!DebugState->Initialized){
		memory_index TotalMemorySize = DebugGlobalMemory->DebugStorageSize - sizeof(debug_state);
		InitializeMemoryArena(&DebugState->DebugArena, TotalMemorySize, DebugState + 1);
		SubArena(&DebugState->PerFrameArena, &DebugState->DebugArena, (TotalMemorySize / 2));


		DebugState->Initialized = true;
	}

	DebugState->RenderGroup = BeginRenderGroup(Assets, Commands, MainGenerationID);

	DebugState->DebugFont = PushFont(&DebugState->RenderGroup, DebugState->FontID);
	DebugState->DebugFontInfo = GetFontInfo(DebugState->RenderGroup.Assets, DebugState->FontID);

 	asset_vector MatchVector = {};
    asset_vector WeightVector = {};
    MatchVector.Data[Tag_FontType] = (float)FontType_Debug;
    WeightVector.Data[Tag_FontType] = 1.0f;
    DebugState->FontID = GetBestMatchFontFrom(Assets, Asset_Font, &MatchVector, &WeightVector);

    DebugState->FontScale = 1.0f;
    SetOrthographic(&DebugState->RenderGroup, Width, Height, 1.0f);
}

INTERNAL_FUNCTION void DEBUGEnd(debug_state* DebugState, game_input* Input){
	render_group* RenderGroup = &DebugState->RenderGroup;

	EndRenderGroup(RenderGroup);
}

IVAN_DLL_EXPORT DEBUG_GAME_FRAME_END(DEBUGGameFrameEnd){
	debug_state* DebugState = (debug_state*)Memory->DebugStorage;
	if(DebugState){
		game_assets* Assets = DEBUGGetGameAssets(Memory);

		DEBUGStart(
			DebugState, 
			RenderCommands, 
			Assets, 
			DEBUGGetMainGenerationID(Memory), 
			RenderCommands->Width,
			RenderCommands->Height);

		OverlayCycleCounters(DebugState);


		DEBUGEnd(DebugState, Input);
	}
}