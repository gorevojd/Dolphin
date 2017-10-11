#ifndef IVAN_DEBUG_H
#define IVAN_DEBUG_H

#if IVAN_INTERNAL

#include "ivan_debug_ui.h"
#include "ivan_render_group.h"

#define DEBUG_FRAME_COUNT 256

struct debug_profile_node{
	struct debug_element* Element;
	struct debug_stored_event* FirstChild;
	struct debug_stored_event* NextSameParent;

	uint64 Duration;
	uint64 DurationOfChildren;
	uint64 ParentRelativeClock;
	uint32 Reserved;
	uint16 ThreadOrdinal;
	uint16 CoreIndex;
};

struct debug_stored_event{
	union{
		debug_stored_event* Next;
		debug_stored_event* NextFree;
	};

	uint32 FrameIndex;

	union{
		debug_event Event;
		debug_profile_node ProfileNode;
	};
};

struct debug_element_frame{
	debug_stored_event* OldestEvent;
	debug_stored_event* MostRecentEvent;
};

struct debug_element{
	char* OriginalGUID;
	char* GUID;
	char* Name;
	uint32 FileNameCount;
	uint32 LineNumber;

	debug_type Type;

	bool32 ValueWasEdited;

	debug_element_frame Frames[DEBUG_FRAME_COUNT];
	debug_element* NextInHash;
};

struct debug_frame{
	uint64 BeginClock;
	uint64 EndClock;
	real32 WallSecondsElapsed

	float FrameBarScale;

	uint32 FrameIndex;

	uint32 StoredDebugEvent;
	uint32 ProfileBlockCount;
	uint32 DataBlockCount;

	debug_stored_event* RootProfileNode;
};

struct open_debug_block{
	union{
		open_debug_block* Parent;
		open_debug_block* NextFree;
	};

	uint32 StartingFrameIndex;
	debug_element *Element;
	uint64 BeginClock;
	debug_stored_event* Node;

	debug_variable_link* Group;
};

struct debug_thread{
	union{
		debug_thread* Next;
		debug_thread* NextFree;
	};

	uint32 ID;
	uint32 LaneIndex;
	open_debug_block* FirstOpenCodeBlock;
	open_debug_block* FirstOpenDataBlock;
};

struct debug_state{
	bool32 Initialized;

	memory_arena DebugArena;
	memory_arena PerFrameArena;

	render_group RenderGroup;
	struct loaded_font* DebugFont;
	struct dda_font* DebugFontInfo;

	float FontScale;
	struct font_id FontID;
	float GlobalWidth;
	float GlobalHeight;

	debug_element* ElementHash[1024];

	uint32 TotalFrameCount;
	uint32 ViewingFrameOrdinal;

	uint32 MostRecentFrameOrdinal;
	uint32 CollationFrameOrdinal;
	uint32 OldestFrameOrdinal;
	debug_frame Frames[DEBUG_FRAME_COUNT];

	debug_element* RootProfileNode;

	uint32 FrameBarLaneCount;
	debug_thread* FirstThread;
	debug_thread* FirstFreeThread;
	open_debug_block* FirstFreeBlock;

	debug_stored_event* FirstFreeStoredEvent;

	uint32 RootInfoSize;
	char* RootInfo;

	uint32 ToolTipCount;
	char ToolTipText[16][256];
};

#endif

#endif