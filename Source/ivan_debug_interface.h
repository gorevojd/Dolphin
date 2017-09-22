#ifndef IVAN_DEBUG_INTERFACE_H

#if IVAN_INTERNAL
enum debug_type{
	DebugType_Unknown,

	DebugType_FrameMarker,
	DebugType_BeginBlock,
	DebugType_EndBlock,
};

struct debug_event{
	uint64 Clock;
	char* GUID;
	uint16 ThreadID;
	uint16 CoreIndex;
	uint8 Type;
};

struct debug_table{

	uint32 RecordIncrement;
	uint32 CurrentEventArrayIndex;

	uint64 volatile EventArrayIndex_EventIndex;
	debug_event Events[2][16 * 65636];
};

extern debug_table* GlobalDebugTable;

#define UNIQUE_FILE_COUNTER_STRING__(A, B, C, D) A "|" #B "|" #C "|" D
#define UNIQUE_FILE_COUNTER_STRING_(A, B, C, D) UNIQUE_FILE_COUNTER_STRING__(A, B, C, D)
#define DEBUG_NAME(Name) UNIQUE_FILE_COUNTER_STRING_(__FILE__, __LINE__, __COUNTER__, Name)

#define RecordDebugEvent(EventType, GUIDInit)		\
uint64 ArrayIndex_EventIndex = AtomicAddU64(&GlobalDebugTable->EventArrayIndex_EventIndex, GlobalDebugTable->RecordIncrement);	\
	uint32 EventIndex = ArrayIndex_EventIndex & 0xFFFFFFFF;	\
	Assert(EventIndex < ArrayCount(GlobalDebugTable->Events[0])); 	\
	debug_event* Event = GlobalDebugTable->Events[ArrayIndex_EventIndex >> 32] + EventIndex;	\
	Event->Clock = __rdtsc();	\
	Event->Type = (uint8)EventType;	\
	Event->CoreIndex = 0;	\
	Event->ThreadID = (uint16)GetThreadID();	\
	Event->GUID = GUIDInit;

#define FRAME_MARKER(SecondsElapsedInit) 	\
	{RecordDebugEvent(DebugType_FrameMarker, DEBUG_NAME("Frame Marker")); 	\
	Event->Value_r32 = SecondsElapsedInit;}

#define TIMED_BLOCK__(GUID, Number, ...) timed_block TimedBlock_##Number(GUID, ## __VA_ARGS__)
#define TIMED_BLOCK_(GUID, Number, ...) TIMED_BLOCK__(GUID, Nubmer, ## __VA_ARGS__)
#define TIMED_BLOCK(Name, ...) TIMED_BLOCK_(DEBUG_NAME(Name), __COUNTER__, ## __VA_ARGS__)
#define TIMED_FUNCTION(...) TIMED_BLOCK_(DEBUG_NAME(__FUNCTION__), ## __VA_ARGS__)

#define BEGIN_BLOCK_(GUID) {RecordDebugEvent(DebugType_BeginBlock, GUID);}
#define END_BLOCK_(GUID) {RecordDebugEvent(DebugType_EndBlock, GUID);}

#define BEGIN_BLOCK(Name) BEGIN_BLOCK_(DEBUG_NAME(Name))
#define END_BLOCK() END_BLOCK_(DEBUG_NAME("END_BLOCK_"))

struct timed_block{
	timed_block(char* GUID, uint32 HitCountInit = 1){
		BEGIN_BLOCK_(GUID);
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

#define IVAN_DEBUG_INTERFACE_H
#endif