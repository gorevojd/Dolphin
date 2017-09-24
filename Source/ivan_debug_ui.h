#ifndef IVAN_DEBUG_UI_H
#define IVAN_DEBUG_UI_H

struct debug_state;

enum debug_text_op{
	DEBUGTextOp_DrawText,
	DEBUGTextOp_SizeText,
};

enum debug_interaction_type{
	DebugInteraction_None,

	DebugInteraction_NOP,

	DebugInteraction_AutoModifyVariable,

	DebugInteraction_ToggleValue,
	DebugInteraction_DragValue,
	DebugInteraction_TearValue,

	DebugInteraction_Resize,
	DebugInteraction_Move,

	DebugInteraction_Select,

	DebugInteraction_ToggleExpansion,

	DebugInteraction_SetUInt32,
	DebugInteraction_SetPointer,
};

struct layout{
	debug_state* DebugState;
	vec2 MouseP;
	vec2 BaseCorner;
	uint32 Depth;

	vec2 At;
	real32 LineAdvance;
	real32 NextYDelta;
	real32 SpacingX;
	real32 SpacingY;

	uint32 NoLineFeed;
	bool32 LineInitialized;
};

#endif