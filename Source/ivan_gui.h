#ifndef IVAN_GUI
#define IVAN_GUI

enum gui_text_op{
	GUITextOp_DrawText,
	GUITextOp_SizeText,
};

enum gui_element_type{
	GUIElement_None,

	GUIElement_Button,
	GUIElement_Slider,
	GUIElement_Menu,
	GUIElement_Label,
};

struct gui_state{
	b32 Initialized;

	render_group* RenderGroup;

	dda_font* GUIFontInfo;
	loaded_font* GUIFont;

	font_id FontID;

	float FontScale;
};

#endif