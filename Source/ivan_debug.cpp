#include "ivan_debug.h"
#include "ivan_debug_ui.cpp"

struct debug_parsed_name{
	uint32 HashValue;
	uint32 FileNameCount;
	uint32 LineNumber;

	uint32 NameLength;
	char* Name;
};

inline debug_parsed_name
DebugParseName(char* GUID, char* ProperName){
	debug_parsed_name Result = {};

	uint32 PipeCount = 0;
	uint32 NameStartsAt = 0;
	char *Scan = GUID;

	for(;*Scan;++Scan){
		if(*Scan == '|'){
			if(PipeCount == 0){
				Result.FileNameCount = (uint32)(Scan - GUID);
				Result.LineNumber = IntFromString(Scan + 1);
			}
			else if(PipeCount == 1){

			}
			else{
				NameStartsAt = (uint32)(Scan - GUID + 1);
			}

			++PipeCount;
		}

		Result.HashValue = 65599 * Result.HashValue + *Scan;
	}

	Result.NameLength = StringLength(ProperName);
	Result.Name = ProperName;

	return(Result);
}

inline debug_element* 
GetElementFromGUID(debug_state* DebugState, uint32 Index, char* GUID){
	debug_element* Result = 0;

	for(debug_element* Chain = DebugState->ElementHash[Index];
		Chain;
		Chain = Chain->NextInHash)
	{
		if(StringsAreEqual(Chain->GUID, GUID)){
			Result = Chain;
			break;
		}
	}
}

inline debug_element*
GetElementFromGUID(debug_state* DebugState, char* GUID){
	debug_element* Result = 0;

	if(GUID){
		debug_parsed_name ParsedName = DebugParseName(GUID, 0);
		uint32 Index = (ParsedName.HashValue % ArrayCount(DebugState->ElementHash));

		Result = GetElementFromGUID(DebugState, Index, GUID);
	}

	return(Result);
}

inline debug_id
DebugIDFromLink(debug_tree* Tree, debug_variable_link* Link){
	debug_id Result = {};

	Result.Value[0] = Tree;
	Result.Value[1] = Link;

	return(Result);
}

inline debug_id
DebugIDFromGUID(debug_tree* Tree, char* GUID){
	debug_id Result = {};

	Result.Value[0] = Tree;
	Result.Value[1] = GUID;

	return(Result);
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

INTERNAL_FUNCTION debug_tree*
AddTree(debug_state* DebugState, debug_variable_link* Group, vec2 AtP){
	debug_tree* Tree = PushStruct(&DebugState->DebugArena, debug_tree);

	Tree->UIP = AtP;
	Tree->Group = Group;

	DLIST_INSERT(&DebugState->TreeSentinel, Tree);

	return(Tree);
}

inline void 
BeginDebugStatistic(debug_statistic* Stat){
	Stat->Min = Real32Maximum;
	Stat->Max = Real32Minimum;
	Stat->Sum = 0.0f;
	Stat->Count = 0;
}

inline void 
AccumDebugStatistic(debug_statistic* Stat, real64 Value){
	++Stat->Count;

	if(Stat->Min > Value){
		Stat->Min = Value;
	}

	if(Stat->Max < Value){
		Stat->Max = Value;
	}

	Stat->Sum += Value;
}

inline void EndDebugStatistic(debug_statistic* Stat){
	if(Stat->Count){
		Stat->Avg = Stat->Sum / (real64)Stat->Count;
	}
	else{
		Stat->Min = 0.0f;
		Stat->Max = 0.0f;
		Stat->Avg = 0.0f;
	}
}

INTERNAL_FUNCTION memory_index
DEBUGEventToText(char* Buffer, char* End, debug_element* Element, debug_event* Event, uint32 Flags){
	char* At = Buffer;

	if(Flags & DEBUGVarToText_AddDebugUI){
		At += PrintFormatto(End - At, At, "#define DEBUGUI_");
	}

	if(Flags & DEBUGVarToText_AddName){
		char* UseName = (Flags & DEBUGVarToText_ShowEntireGUID) ? Event->GUID : Event->Name;
		At += PrintFormatto(End - At, At, "%s%s", UseName, (Flags & DEBUGVarToText_Colon) ? ":" : "");
	}

	if(Flags & DEBUGVarToText_AddValue){
		switch(Event->Type){
			case DebugType_real32:{
				At += PrintFormatto(End - At, At, "%f", Event->Value_real32);
				if(Flags & DEBUGVarToText_FloatSuffix){
					*At++ = 'f';
				}
			}break;

			case DebugType_bool32{
				if(Flags & DEBUGVarToText_PrettyBools){
					At += PrintFormatto(End - At, At, "%s",
						Event-Value_bool32 ? "true" : "false");
				}
				else{
					At += PrintFormatto(End - At, At, "%d", Event->Value_bool32);
				}
			} break;

			case DebugType_int32{
				At += PrintFormatto(End - At, At, "%d", Event->Value_int32);
			}break;

			case DebugType_uint32:{
				At += PrintFormatto(End - At, At, "%u", Event->Value_uint32);
			}break;

			case DebugType_vec2:{
				At += PrintFormatto(End - At, At, "Vec2(%f, %f)", Event->Value_vec2.x, Event->Value_vec2.y);
			}break;

			case DebugType_vec3:{
				At += PrintFormatto(End - At, At, "Vec3(%f, %f, %f)", 
					Event->Value_vec3.x, 
					Event->Value_vec3.y, 
					Event->Value_vec3.z);
			}break;

			case DebugType_vec4:{
				At += PrintFormatto(End - At, At, "Vec4(%f, %f, %f, %f)",
					Event->Value_vec4.x,
					Event->Value_vec4.y,
					Event->Value_vec4.z,
					Event->Value_vec4.w);
			}break;

			case DebugType_rectangle2:{
				At += PrintFormatto(End - At, At, "Rect2(%f, %f, %f -> %f, %f, %f)",
					Event->Value_rectangle3.Min.x,
					Event->Value_rectangle3.Min.y,
					Event->Value_rectangle3.Min.z,
					Event->Value_rectangle3.Max.x,
					Event->Value_rectangle3.Max.y,
					Event->Value_rectangle3.Max.z);
			}break;

			case DebugType_bitmap_id:{

			}break;

			default:{
				At += PrintFormatto(End - At, At, "UNHANDLED: %s", Event->GUID);
			}break;
		}
	}

	if(Flags & DebugVarToText_LineFeedEnd){
		*At++ = '\n';
	}

	if(Flags & DebugVarToText_NullTerminator){
		*At++ = 0;
	}

	return(At - Buffer);
}

INTERNAL_FUNCTION debug_view*
GetOrCreateDebugViewFor(debug_state* DebugState, debug_id ID){
	uint32 HashIndex = ((U32FromPointer(ID.Value[0]) >> 2) + (U32FromPointer(ID.Value[1]) >> 2)) % ArrayCount(DebugState->ViewHash);
	debug_view** HashSlot = DebugState->ViewHash + HashIndex;

	debug_view* Result = 0;
	for(debug_view* Search = *HashSlot;
		Search;
		Search = Search->NextInHash)
	{
		if(DebugIDsAreEqual(Search->ID, ID)){
			Result = Search;
			break;
		}
	}

	if(!Result){
		Result = PushStruct(&DebugState->DebugArena, debug_view);
		Result->ID = ID;
		Result->Type = DebugViewType_Unknown;
		Result->NextInHash = *HashSlot;
		*HashSlot = Result;
	}

	return(Result);
}

inline debug_interaction
ElementInteraction(debug_state *DebugState, debug_id DebugID, debug_interaction_type Type, debug_element *Element){    
    debug_interaction ItemInteraction = {};
    ItemInteraction.ID = DebugID;    
    ItemInteraction.Type = Type;
    ItemInteraction.Element = Element;

    return(ItemInteraction);
}

inline debug_interaction
DebugIDInteraction(debug_interaction_type Type, debug_id ID){
    debug_interaction ItemInteraction = {};
    ItemInteraction.ID = ID;
    ItemInteraction.Type = Type;

    return(ItemInteraction);
}

inline debug_interaction
DebugLinkInteraction(debug_interaction_type Type, debug_variable_link *Link){
    debug_interaction ItemInteraction = {};
    ItemInteraction.Link = Link;
    ItemInteraction.Type = Type;

    return(ItemInteraction);
}

INTERNAL_FUNCTION bool32
IsSelected(debug_state *DebugState, debug_id ID){
    bool32 Result = false;

    for(u32 Index = 0;
        Index < DebugState->SelectedIDCount;
        ++Index)
   	{
        if(DebugIDsAreEqual(ID, DebugState->SelectedID[Index])){
            Result = true;
            break;
        }
    }

    return(Result);
}

INTERNAL_FUNCTION void
ClearSelection(debug_state *DebugState)
{
    DebugState->SelectedIDCount = 0;
}

INTERNAL_FUNCTION void
AddToSelection(debug_state *DebugState, debug_id ID)
{
    if((DebugState->SelectedIDCount < ArrayCount(DebugState->SelectedID)) &&
       !IsSelected(DebugState, ID))
    {
        DebugState->SelectedID[DebugState->SelectedIDCount++] = ID;
    }
}

INTERNAL_FUNCTION void
DEBUG_HIT(debug_id ID, real32 ZValue)
{
    debug_state *DebugState = DEBUGGetState();
    if(DebugState)
    {
        DebugState->NextHotInteraction = DebugIDInteraction(DebugInteraction_Select, ID);
    }
}

INTERNAL_FUNCTION bool32
DEBUG_HIGHLIGHTED(debug_id ID, vec4 *Color)
{
    bool32 Result = false;

    debug_state *DebugState = DEBUGGetState();
    if(DebugState)
    {
        if(IsSelected(DebugState, ID))
        {
            *Color = Vec4(0, 1, 1, 1);
            Result = true;
        }

        if(DebugIDsAreEqual(DebugState->HotInteraction.ID, ID))
        {
            *Color = Vec4(1, 1, 0, 1);
            Result = true;
        }
    }

    return(Result);
}

INTERNAL_FUNCTION bool32
DEBUG_REQUESTED(debug_id ID)
{
    bool32 Result = false;

    debug_state *DebugState = DEBUGGetState();
    if(DebugState)
    {
        Result = IsSelected(DebugState, ID)
            || DebugIDsAreEqual(DebugState->HotInteraction.ID, ID);
    }

    return(Result);
}

INTERNAL_FUNCTION uint64
GetTotalClocks(debug_element_frame *Frame)
{
    uint64 Result = 0;
    for(debug_stored_event *Event = Frame->OldestEvent;
        Event;
        Event = Event->Next)
    {
        Result += Event->ProfileNode.Duration;
    }
    return(Result);
}

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

inline open_debug_block*
AllocateOpenDebugBlock(
	debug_state* DebugState, 
	debug_element* Element,
	uint32 FrameIndex, 
	debug_event* Event,
	open_debug_block **FristOpenBlock)
{
	open_debug_block* Result = 0;
	FREELIST_ALLOCATE(Result, DebugState->FirstFreeBlock,
		PushStruct(&DebugState->DebugArena, open_debug_block));

	Result->StartingFrameIndex = FrameIndex;
	Result->BeginClock = Event->Clock;
	Result->Element = Element;
	Result->NextFree = 0;

	Result->Parent = *FirstOpenBlock;
	*FirstOpenBlock = Result;

	return(Result);
}

inline void DeallocateOpenDebugBlock(debug_state* DebugState, open_debug_block** FirstOpenBlock){
	open_debug_block* FreeBlock = *FirstOpenBlock;
	*FirstOpenBlock = FreeBlock->Parent;

	FreeBlock->NextFree = DebugState->FirstFreeBlock;
	DebugState->FirstFreeBlock = FreeBlock;
}

inline void IncrementFrameOrdinal(uint32* Ordinal){
	*Ordinal = (*Ordinal + 1) % DEBUG_FRAME_COUNT;
}

INTERNAL_FUNCTION void
FreeOldestFrame(debug_state* DebugState){
	FreeFrame(DebugState, DebugState->OldestFrameOrdinal);

	if(DebugState->OldestFrameOrdinal == DebugState->MostRecentFrameOrdinal){
		IncrementFrameOrdinal(&DebugState->MostRecentFrameOrdinal);
	}
	IncrementFrameOrdinal(&DebugState->OldestFrameOrdinal;)
}

inline debug_frame*
GetCollationFrame(debug_state* DebugState){
	debug_frame* Result = DebugState->Frames + DebugState->CollationFrameOrdinal;

	return(Result);
}

INTERNAL_FUNCTION debug_stored_event*
StoreEvent(debug_state* DebugState, debug_element* Element, debug_event* Event){
	debug_stored_event* Result = 0;
	while(!Result){
		Result = DebugState->FirstFreeStoredEvent;
		if(Result){
			DebugState->FirstFreeStoredEvent = Result->NextFree;
		}
		else{
			Result = PushStruct(&DebugState->PerFrameArena, debug_stored_event);
		}
	}

	debug_frame* CollationFrame = GetCollationFrame(DebugState);

	Result->Next = 0;
	Result->FrameIndex = CollationFrame->FrameIndex;
	Result->Event = *Event;

	CollationFrame->StoredEventCount++;

	debug_element_frame* Frame = Element->Frames + DebugState->CollationFrameOrdinal;
	if(Frame->MostRecentEvent){
		Frame->MostRecentEvent = Frame->MostRecentEvent->Next = Result;
	}
	else{
		Frame->OldestEvent = Frame->MostRecentEvent = Result;
	}

	return(Result);
}

INTERNAL_FUNCTION void
CollateDebugRecords(debug_state* DebugState, uint32 EventCount, debug_event* EventArray){
	for(uint32 EventIndex = 0;
		EventIndex < EventCount;
		EventIndex++)
	{
		debug_event* Event = EventArray + EventIndex;
		if(Event->Type == DebugType_FrameMarker){
			debug_frame* CollationFrame = GetCollationFrame(DebugState);

			CollationFrame->EndClock = Event->Clock;
			if(CollationFrame->RootProfileNode){
				CollationFrame->RootProfileNode->ProfileNode.Duration = 
					(CollationFrame->EndClock - CollationFrame->BeginClock);
			}

			CollationFrame->WallSecondsElapsed = Event->Value_real32;
		
			real32 ClockRange = (real32)(CollationFrame->EndClock - CollationFrame->BeginClock);
			++DebugState->TotalFrameCount;

			if(DebugState->Paused){
				FreeFrame(DebugState, DebugState->CollationFrameOrdinal);
			}
			else{
				DebugState->MostRecentFrameOrdinal = DebugState->CollationFrameOrdinal;
				IncrementFrameOrdinal(&DebugState->CollationFrameOrdinal);
				if(DebugState->CollationFrameOrdinal == DebugState->OldestFrameOrdinal){
					FreeOldestFrame(DebugState);
				}
				CollationFrame = GetCollationFrame(DebugState);
			}

			InitFrame(DebugState, EVent->Clock, CollationFrame);
		}
		else{
			debug_frame* CollationFrame = GetCollationFrame(DebugState);

			Assert(CollationFrame);

			uint32 FrameIndex = DebugState->TotalFrameCount - 1;
			debug_thread* Thread = GetDebugThread(DebugState, Event->ThreadID);
			uint64 RelativeClock = Event->Clock - CollationFrame->BeginClock

			debug_variable_link* DefaultParentGroup = DebugState->RootGroup;
			if(Thread->FirstOpenDataBlock){
				DefaultParentGroup = Thread->FirstOpenDataBlock->Group;
			}

			case DebugType_BeginBlock:{
				++CollationFrame->ProfileBlockCount;
				debug_element* Element = GetElementFromEvent(DebugState, Event, DebugState->ProfilGroup,
					DebugElement_AddToGroup);

				debug_stored_event* ParentEvent = CollationFrame->RootProfileNode;
				uint64 ClockBasis = CollationFrame->BeginClock;
				if(Thread->FirstOpenCodeBlock){
					ParentEvent = Thread->FirstOpenCodeBlock->Node;
					ClockBasis = Thread->FirstOpenCodeBlock->BeginClock;
				}
				else if(!ParentEvent){
					debug_event NullEvent = {};
					ParentEvent = StoreEvent(DebugState, DebugState->RootProfileElement, &NullEvent);
					debug_profile_node* Node = &ParentEvent->ProfileNode;
					Node->Element = 0;
					Node->FirstChild = 0;
					Node->NextSameParent = 0;
					Node->ParentRelativeClock = 0;
					Node->Duration = 0;
					Node->DurationOfChildren = 0;
					Node->ThreadOrdinal = 0;
					Node->CoreIndex = 0;

					ClockBasis = CollationFrame->BeginClock;
					CollationFrame->RootProfileNode = ParentEvent;
				}

				debug_stored_event* StoredEvent = StoreEvent(DebugState, Element, Event);
				debug_profile_node* Node = &StoredEvent->ProfileNode;
				Node->Element = Element;
				Node->FirstChild = 0;
				Node->ParentRelativeClock = Event->Clock - ClockBasis;
				Node->Duration = 0;
				Node->DurationOfChildren = 0;
				Node->ThreadOrdinal = (uint16)Thread->LaneIndex;
				Node->CoreIndex = Event->CoreIndex

				Node->NextSameParent = Parentevent->ProfileNode.FirstChild;
				ParentEvent->ProfileNode.FirstChild = StoredEvent;

				open_debug_block* DebugBlock = AllocateOpenDebugBlock(
					DebugState, Element, FrameIndex, Event,
					&Thread->FirstOpenCodeBlock);
				DebugBlock->Node = StoredEvent;
			}break;

			case DebugType_EndBlock:{
				if(Thread->FirstOpenCodeBlock){
					open_debug_block* MatchingBlock = Thread->FirstOpenCodeBlock;
					Assert(Thread->ID == Event->ThreadID);

					debug_profile_node* Node = &MatchingBlock->Node->ProfileNode;
					Node->Duration = Event->Clock - MatchingBlock->BeginClock;

					DeallocateOpenDebugBlock(DebugState, &Thread->FirstOpenCodeBlock);

					if(Thread->FirstOpenCodeBlock){
						debug_profile_node* ParentNode = 
							&Thread->FirstOpenCodeBlock->Node->ProfileNode;
							Parentnode->DurationOfChildren += Node->Duration;
					}
				}
			}break;	

			case DebugType_OpenDataBlock:{
				++CollationFrame->DataBlockCount;
				open_debug_block* DebugBlock = AllocateOpenDebugBlock(
					DebugState, 0, FrameIndex, Event, &Thread->FirstOpenDataBlock);

				debug_parsed_name ParsedName = DebugParseName(Event->GUID, Event->Name);
				DebugBlock->Group = 
					GetGroupForHierarchicalName(DebugState, DefaultParentGroup, ParsedName.Name, true);
			}	
		}
	}
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
	TIMED_FUNCTION();

	render_group* RenderGroup = &DebugState->RenderGroup;
	?????PushSortBarrier(RenderGroup, true);

	debug_event* HotEvent = 0;
	debug_platform_memory_stats MemStats = Platform.DEBUGGetMemoryStats();

	debug_frame* MostRecentFrame = DebugState->Frames + DebugState->ViewingFrameOrdinal;
	PrintFormatto(DebugState->RootInfoSize, DebugState->RootInfo,
		"%.02fms %de %dp %dd - Mem: %lu blocks, %lu used",
		MostRecentFrame->WallSecondsElapsed * 1000.0f,
		MostRecentFrame->StoredEventCount,
		MostRecentFrame->ProfileBlockCount,
		MostRecentFrame->DataBlockCount,
		MemStats.BlockCount,
		MemStats.TotalUsed,
		MemStats.TotalSize,
		Input->MouseX,
		Input->MouseY);

	DebugState->AltUI = Input->MouseButtons[PlatformMouseButton_Right].EndedDown;
	vec2 MouseP = Unproject(RenderGroup, &RenderGroup->GameXForm, Vec2(Input->MouseX, Input->MouseY), 0.0f).xy;
	DebugState->MouseTextLayout = BeginLayout(DebugState, MouseP, MouseP);
	DrawTrees(DebugState->MouseP);
	EndLayout(&DebugState->MouseTextLayout);
	DEBUGInteract(DebugState, Input, MouseP);

	DrawTooltips(DebugState);

	EndRenderGroup(&DebugState->RenderGroup);

	ZeroStruct(DebugState->NextHotInteraction);
}

IVAN_DLL_EXPORT DEBUG_GAME_FRAME_END(DEBUGGameFrameEnd){

	ZeroStruct(GlobalDebugTable->EditEvent);

	GlobalDebugTable->CurrentEventArrayIndex = !GlobalDebugTable->CurrentEventArrayIndex;
	uint64 ArrayIndex_EventIndex = AtomicExchangeU64(
		&GlobalDebugTable->EventArrayIndex_EventIndex,
		(uint64)GlobalDebugTable->CurrentEventArrayIndex << 32);

	uint32 EventArrayIndex = ArrayIndex_EventIndex >> 32;
	Assert(EventArrayIndex <= 1);
	uint32 EventCount = ArrayIndex_EventIndex & 0xFFFFFFFF;

	if(!Memory->DebugState){
		Memory->DebugState = DEBUGInit(
			RenderCommands->Width,
			RenderCommands->Height);
	}

	debug_state* DebugState = Memory->DebugState;
	if(DebugState){
		game_assets* Assets = DEBUGGetGameAssets(Memory);

		DEBUGStart(
			DebugState, 
			RenderCommands, 
			Assets, 
			DEBUGGetMainGenerationID(Memory), 
			RenderCommands->Width,
			RenderCommands->Height);

		CollateDebugRecords(DebugState, EventCount, GlobalDebugTable->Events[EventArrayIndex]);

		DEBUGEnd(DebugState, Input);
	}
}