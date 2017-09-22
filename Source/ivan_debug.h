#ifndef IVAN_DEBUG_H
#define IVAN_DEBUG_H

#include "ivan_debug_ui.h"
#include "ivan_render_group.h"

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
};

#endif