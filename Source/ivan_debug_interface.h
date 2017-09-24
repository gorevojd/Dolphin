#ifndef IVAN_DEBUG_INTERFACE_H

#if IVAN_INTERNAL

struct debug_id{
	void* Value[2];
};

enum debug_type{
	DebugType_Unknown,

	DebugType_FrameMarker,
	DebugType_BeginBlock,
	DebugType_EndBlock,

	DebugType_OpenDataBlock,
	DebugType_CloseDataBlock,

	DebugType_bool32,
	DebugType_real32,
	DebugType_uint32,
	DebugType_int32,
	DebugType_vec2,
	DebugType_vec3,
	DebugType_vec4,
	DebugType_rectangle2,
	DebugType_rectangle3,
	DebugType_bitmap_id,
	DebugType_sound_id,
	DebugType_font_id,
	DebugType_memory_arena_p,

	DebugType_ThreadIntervalGraph,
	DebugType_FrameBarGraph,
	DebugType_LastFrameInfo,
	DebugType_DebugMemoryInfo,
	DebugType_FrameSlider,
	DebugType_TopClocksList,
	DebugType_ArenaOccupancy,
};

typedef struct memory_arena* memory_arena_p;

struct debug_event{
	uint64 Clock;
	char* GUID;
	uint16 ThreadID;
	uint16 CoreIndex;
	uint8 Type;
	char* Name;

	union{
		debug_id DebugID;
		debug_event* Value_debug_event;

		bool32 Value_bool32;
		int32 Value_int32;
		uint32 Value_uint32;
		float Value_real32;
		vec2 Value_vec2;
		vec3 Value_vec3;
		vec4 Value_vec4;
		rectangle2 Value_rectangle2;
		rectangle3 Value_rectangle3;
		bitmap_id Value_bitmap_id;
		sound_id Value_sound_id;
		font_id Value_font_id;
		memory_arena_p Value_memory_arena_p;
	};
};

struct debug_table{

	debug_event EditEvent;
	uint32 RecordIncrement;
	uint32 CurrentEventArrayIndex;

	uint64 volatile EventArrayIndex_EventIndex;
	debug_event Events[2][16 * 65636];
};

extern debug_table* GlobalDebugTable;

#define UNIQUE_FILE_COUNTER_STRING__(A, B, C, D) A "|" #B "|" #C
#define UNIQUE_FILE_COUNTER_STRING_(A, B, C, D) UNIQUE_FILE_COUNTER_STRING__(A, B, C, D)
#define DEBUG_NAME(Name) UNIQUE_FILE_COUNTER_STRING_(__FILE__, __LINE__, __COUNTER__, Name)

#define DEBUGSetEventRecording(Enabled) (GlobalDebugTable->RecordIncrement = (Enabled) ? 1 : 0)

#define RecordDebugEvent(EventType, GUIDInit, NameInit)		\
uint64 ArrayIndex_EventIndex = AtomicAddU64(&GlobalDebugTable->EventArrayIndex_EventIndex, GlobalDebugTable->RecordIncrement);	\
	uint32 EventIndex = ArrayIndex_EventIndex & 0xFFFFFFFF;	\
	Assert(EventIndex < ArrayCount(GlobalDebugTable->Events[0])); 	\
	debug_event* Event = GlobalDebugTable->Events[ArrayIndex_EventIndex >> 32] + EventIndex;	\
	Event->Clock = __rdtsc();	\
	Event->Type = (uint8)EventType;	\
	Event->CoreIndex = 0;	\
	Event->ThreadID = (uint16)GetThreadID();	\
	Event->GUID = GUIDInit;	\
	Event->Name = NameInit;

#define FRAME_MARKER(SecondsElapsedInit) 	\
	{RecordDebugEvent(DebugType_FrameMarker, DEBUG_NAME("Frame Marker"), "Frame Marker"); 	\
	Event->Value_real32 = SecondsElapsedInit;}

#define TIMED_BLOCK__(GUID, Name) timed_block TimedBlock_##Number(GUID, Name)
#define TIMED_BLOCK_(GUID, Name) TIMED_BLOCK__(GUID, Name)
#define TIMED_BLOCK(Name) TIMED_BLOCK_(DEBUG_NAME(Name), Name)
#define TIMED_FUNCTION(...) TIMED_BLOCK_(DEBUG_NAME(__FUNCTION__), (char*)__FUNCTION__)

#define BEGIN_BLOCK_(GUID, Name) {RecordDebugEvent(DebugType_BeginBlock, GUID, Name);}
#define END_BLOCK_(GUID, Name) {RecordDebugEvent(DebugType_EndBlock, GUID, Name);}

#define BEGIN_BLOCK(Name) BEGIN_BLOCK_(DEBUG_NAME(Name), Name)
#define END_BLOCK() END_BLOCK_(DEBUG_NAME("END_BLOCK_"), "END_BLOCK_")

struct timed_block{
	timed_block(char* GUID, char* Name){
		BEGIN_BLOCK_(GUID, Name);
	}

	~timed_block(){
		END_BLOCK();
	}
};
#else

#define TIMED_BLOCK(...)
#define TIMED_FUNCTION(...)
#define BEGIN_BLOCK(...)
#define END_BLOCK(...)
#define FRAME_MARKER(...)

#endif

#if defined(__cplusplus) && IVAN_INTERNAL

#define DEBUGValueSetEventData_(type) \
inline void DEBUGValueSetEventData(debug_event* Event, type Ignored, void* Value)	\
{	\
	Event->Type = DebugType_##type;	\
	if(GlobalDebugTable->EditEvent.GUID == Event->GUID){	\
		*(type*)Value = GlobalDebugTable->EditEvent.Value_##type;	\
	}	\
	Event->Value_##type = *(type*)Value;\
}

DEBUGValueSetEventData_(real32);
DEBUGValueSetEventData_(uint32);
DEBUGValueSetEventData_(int32);
DEBUGValueSetEventData_(vec2);
DEBUGValueSetEventData_(vec3);
DEBUGValueSetEventData_(vec4);
DEBUGValueSetEventData_(rectangle2);
DEBUGValueSetEventData_(rectangle3);
DEBUGValueSetEventData_(bitmap_id);
DEBUGValueSetEventData_(sound_id);
DEBUGValueSetEventData_(font_id);
DEBUGValueSetEventData_(memory_arena_p);

struct debug_data_block{
	debug_data_block(char* GUID, char* Name){
		RecordDebugEvent(DebugType_OpenDataBlock, GUID, Name);
	}

	~debug_data_block(void){
		RecordDebugEvent(DebugType_CloseDataBlock, DEBUG_NAME("End Data Block"), "End Data Block");
	}
};

#define DEBUG_DATA_BLOCK(Name) = debug_data_block DataBlock__(DEBUG_NAME(Name), Name)
#define DEBUG_BEGIN_DATA_BLOCK(Name) RecordDebugEvent(DebugType_OpenDataBlock, DEBUG_NAME(NAME), Name)
#define DEBUG_END_DATA_BLOCK(Name) RecordDebugEvent(DebugType_CloseDataBlock, DEBUG_NAME("End Data Block"), "End Data Block")

#define GET_DEBUG_MOUSE_P() GlobalDebugTable->MouseP
#define SET_DEBUG_MOUSE_P(P) GlobalDebugTable->MouseP = (P);

#define DEBUG_VALUE(Value)	\
{	\
	RecordDebugEvent(DebugType_Unknown, DEBUG_NAME(#Value), #Value);	\
	DEBUGValueSetEventData(Event, Value, (void*)&(Value));	\
}

#define DEBUG_B32(Value)	\
{	\
	RecordDebugEvent(DebugType_Unknown, DEBUG_NAME(#Value), #Value);	\
	DEBUGValueSetEventData(Event, (int32)0, (void*)&Value);	\
	Event-Type = DebugType_bool32;	\
}

#define DEBUG_UI_ELEMENT(Type, Name)	\
{	\
	RecordDebugEvent(Type, #Name, #Name);	\
}

#define DEBUG_BEGIN_ARRAY(...)
#define DEBUG_END_ARRAY(...)

inline debug_id DEBUG_POINTER_ID(void* Pointer){
	debug_id ID = {Pointer};

	return(ID);
}

#define DEBUG_UI_ENABLED 1

INTERNAL_FUNCTION void DEBUG_HIT(debug_id ID, float ZValue);
INTERNAL_FUNCTION bool32 DEBUG_HIGHLIGHTED(debug_id ID, vec4* Color);
INTERNAL_FUNCTION bool32 DEBUG_REQUESTED(debug_id ID);

#else

inline debug_id DEBUG_POINTER_ID(void *Pointer) {
	debug_id NullID = {}; 
	return(NullID);
}

#define GET_DEBUG_MOUSE_P(...)
#define SET_DEBUG_MOUSE_P(...)

#define DEBUG_DATA_BLOCK(...)
#define DEBUG_VALUE(...)
#define DEBUG_BEGIN_ARRAY(...)
#define DEBUG_END_ARRAY(...)
#define DEBUG_UI_ENABLED 0
#define DEBUG_HIT(...)
#define DEBUG_HIGHLIGHTED(...) 0
#define DEBUG_REQUESTED(...) 0
#define DEBUG_BEGIN_DATA_BLOCK(...)
#define DEBUG_END_DATA_BLOCK(...)
#define DEBUG_B32(...)
#define DEBUGSetEventRecording(...)

#endif

#define IVAN_DEBUG_INTERFACE_H
#endif