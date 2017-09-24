#ifndef IVAN_DEBUG_H
#define IVAN_DEBUG_H

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
};

#endif