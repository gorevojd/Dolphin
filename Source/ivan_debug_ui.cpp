#include "ivan_debug_ui.h"

INTERNAL_FUNCTION rectangle2 DEBUGTextOut(
    debug_state* DebugState, 
    debug_text_op Op,
    vec2 P, 
    char* String, 
    vec4 Color = Vec4(1, 1, 1, 1), 
    float AtZ = 0.0f)
{
	rectangle2 Result = InvertedInfinityRectangle();

	if(DebugState && DebugState->DebugFont){
		render_group* RenderGroup = &DebugState->RenderGroup;
	    loaded_font* Font = PushFont(RenderGroup, DebugState->FontID);
	    dda_font* Info = DebugState->DebugFontInfo;

        float CharScale = DebugState->FontScale;
        float CursorX = P.x;
        float CursorY = P.y;
        uint32 PrevCodePoint = 0;

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
                CharScale = DebugState->FontScale * IVAN_MATH_CLAMP01(ScaleScale * (float)(Ptr[2] - '0'));
                Ptr += 3;
            }
            else{    
                uint32 CodePoint = *Ptr;

                float AdvanceX = CharScale * GetHorizontalAdvanceForPair(Info, Font, PrevCodePoint, CodePoint);
                CursorX += AdvanceX;

                if(CodePoint != ' '){

                    bitmap_id BitmapID = GetBitmapForGlyph(RenderGroup->Assets, Info, Font, CodePoint);
                    dda_bitmap* BitmapInfo = GetBitmapInfo(RenderGroup->Assets, BitmapID);

                    vec3 BitmapOffset = Vec3(CursorX, CursorY, AtZ);
                    float BitmapScale = -CharScale * (float)BitmapInfo->Dimension[1];
                    
                    if(Op == DEBUGTextOp_DrawText){
#if 1
	                    PushBitmap(
	                        RenderGroup,
	                        BitmapID,
	                        BitmapScale,
	                        BitmapOffset + Vec3(2.0f, 2.0f, 0.0f),
	                        Vec4(0.0f, 0.0f, 0.0f, 1.0f),
	                        true);                    
#endif
	                    PushBitmap(
	                        RenderGroup,
	                        BitmapID,
	                        BitmapScale, 
	                        BitmapOffset, 
	                        Color, 
	                        true);
                    }
                    else{
                    	Assert(Op == DEBUGTextOp_SizeText);
                    	
                    	loaded_bitmap* Bitmap = GetBitmap(
                    		RenderGroup->Assets, 
                    		BitmapID, 
                    		RenderGroup->GenerationID);
                    	
                    	if(Bitmap){
                    		rectangle2 GlyphDim;
                    		GlyphDim.Min.x = BitmapOffset.x;
                    		GlyphDim.Min.y = BitmapOffset.y;
                    		GlyphDim.Max.x = BitmapOffset.x + Bitmap->Width;
                    		GlyphDim.Max.y = BitmapOffset.y + Bitmap->Height;
                    		Result = Union(Result, GlyphDim);
                    	}
                    }
                }

                PrevCodePoint = CodePoint;
                Ptr++;
            }
        }
	}

	return(Result);
}

inline void
TextOutAt(
	debug_state* DebugState, 
	vec2 P, 
	char* String, 
	vec4 Color = Vec4(1, 1, 1, 1),
	float AtZ = 0.0f)
{
	DEBUGTextOut(DebugState, DEBUGTextOp_DrawText, P, String, Color, AtZ);
}

inline rectangle2
GetTextSize(debug_state* DebugState, char* String){
	rectangle2 Result = DEBUGTextOut(DebugState, DEBUGTextOp_SizeText, Vec2(0, 0), String);

	return(Result);
}

inline float 
GetLineAdvance(debug_state* DebugState){
	float Result = GetLineAdvanceFor(DebugState->DebugFontInfo) * DebugState->FontScale;
}

inline float 
GetBaseline(debug_state* DebugState){
	float Result = DebugState->FontScale * GetStartingBaselineY(DebugState->DebugFontInfo);
	return(Result);
}

inline debug_interation
SetPointerInteraction(debug_id DebugID, void** Target, void* Value){
    debug_interation Result = {};
    Result.ID = DebugID;
    Result.Type = DebugInteraction_SetPointer;
    Result.Target = Target;
    Result.Pointer = Value;

    return(Result);
}

inline debug_interation
SetUInt32Interaction(debug_id DebugID, uint32* Target, uint32 Value){
    debug_interation Result = {};
    Result.ID = DebugID;
    Result.Type = DebugInteraction_SetUInt32;
    Result.Target =  Target;
    Result.UInt32 = Value;

    return(Result);
}

inline layout 
BeginLayout(debug_state* DebugState, vec2 MouseP, vec2 UpperLeftCorner){
    layout Layout = {};
    Layout.DebugState = DebugState;
    Layout.MouseP = MouseP;
    Layout.BaseCorner = Layout.At = UpperLeftCorner;
    Layout.LineAdvance = DebugState->FontScale * GetLineAdvanceFor(DebugState->DebugFontInfo);
    Layout.SpacingY = 4.0f;
    Layout.SpacingX = 4.0f;

    return(Layout);
}

inline void 
EndLayout(layout* Layout){

}

inline layout_element
BeginElementRectangle(layout* Layout, vec2* Dim){
    layout_element Element = {};

    Element.Layout = Layout;
    Element.Dim = Dim;

    return(Element);
}

inline void MakeElementSizable(layout_element* Element){
    Element->Size = Element->Dim;
}

inline void DefaultInteraction(layout_element* Element, debug_interation Interaction){
    Element->Interaction = Interaction;
}

inline void AdvanceElement(layout* Layout, rectangle2 ElRect){
    Layout->NextYDelta = Minimum(Layout->NextYDelta, GetMinCorner(ElRect).y - Layout->At.y);

    if(Layout->NoLineFeed){
        Layout->At.x = GetMaxCorner(ElRect).x + Layout->SpacingX;
    }
    else{
        Layout->At.y += Layout->NextYDelta - Layout->SpacingY;
        Layout->LineInitialized = false;
    }
}

inline void 
EndElement(layout_element* Element){
    layout* Layout = Element->Layout;
    debug_state* DebugState = Laout->DebugState;
    //object_transform

    if(!Layout->LineInitialized){
        Layout->At.x = Layout->BaseCorner.x + Layout->Depth * 2.0f * Layout->LineAdvance;
        Layout->LineInitialized = true;
        Layout->NextYDelta = 0.0f;
    }

    float SizeHandlePixels = 4.0f;

    vec2 Frame = {};
    if(Element->Size){
        Frame.x = SizeHandlePixels;
        Frame.y = SizeHandlePixels;
    }

    vec2 TotalDim = *Element->Dim + 2.0f * Frame;

    vec2 TotalMinCorner = Vec2(Layout->At.x, Layout->At.y - TotalDim.y);
    vec2 TotalMaxCorner = TatalMinCorner + TotalDim;

    vec2 InteriorMinCorner = TotalMinCorner + Frame;
    vec2 InteriorMaxCorner = InteriorMinCorner + *Element->Dim;

    rectangle2 TotalBounds = RectMinMax(TotalMinCorner, TotalMaxCorner);
    Element->Bounds = RectMinMax(InteriorMinCorner, InteriorMaxCorner);

    if(Element->Interaction.Type && IsInRectangle(Element->Bounds, Layout->MouseP)){
        DebugState->NextHotInteraction = Element->Interaction;
    }

    if(Element->Size){
        PushRectangle(&DebugState->RenderGroup, RectMinMax(Vec2(TotalMinCorner,x. InteriorMinCorner.y),
            Vec2(InteriorMinCorner.x, InteriorMaxCorner.y)), 0.0f, Vec4(0, 0, 0, 1));
        
        PushRectangle(&DebugState->RenderGroup, RectMinMax(Vec2(InteriorMaxCorner.x, InteriorMinCorner.y),
            Vec2(TotalMaxCorner.x, InteriorMaxCorner.y)), 0.0f, Vec4(0, 0, 0, 1));
        
        PushRectangle(&DebugState->RenderGroup, RectMinMax(Vec2(InteriorMinCorner.x, TotalMinCorner.y),
            Vec2(InteriorMaxCorner.x, InteriorMinCorner.y)), 0.0f, Vec4(0, 0, 0, 1));
        
        PushRectangle(&DebugState->RenderGroup, RectMinMax(Vec2(InteriorMinCorner.x, InteriorMaxCorner.y),
            Vec2(InteriorMaxCorner.x, TotalMaxCorner.y)), 0.0f, Vec4(0, 0, 0, 1));

        debug_interation SizeInteraction = {};
        SizeInteraction.Type = DebugInteraction_Resize;
        SizeInteraction.P = Element->Size;

        rectangle2 SizeBox = AddRadiusTo(
            RectMinMax(Vec2(InteriorMaxCorner.x, TotalMinCorner.y),
                Vec2(TotalMaxCorner.x, InteriorMinCorner.y)), Vec2(4.0f, 4.0f));
        PushRectangle(&DebugState->RenderGroup, SizeBox, 0.0f,
            (InteractionIsHot(DebugState, SizeInteraction) ? Vec4(1, 1, 0, 1) : Vec4(1, 1, 1, 1)));

        if(IsInRectangle(SizeBox, Layout->MouseP)){
            DebugState->NextHotInteraction = SizeInteraction;
        }
    }

    AdvanceElement(Layout, TotalBounds);
}

INTERNAL_FUNCTION vec2 
BasicTextElement(layout* Layout, char* Text, debug_interation ItemInteraction,
    vec4 ItemColor = Vec4(0.8f, 0.8f, 0.8f, 1.0f), vec4 HotColor = Vec4(1, 1, 1, 1),
    float Border = 0.0f, vec4 BackdropColor = Vec4(0, 0, 0, 0))
{
    debug_state* DebugState = Layout->DebugState;

    rectangle2 TextBounds = GetTextSize(DebugState, Text);
    vec2 Dim = {GetDim(TextBounds).x + 2.0f * Border, Layout->LineAdvance + 2.0f * Border};

    layout_element Element = BeginElementRectangle(Layout, &Dim);
    DefaultInteraction(&Element, ItemInteraction);
    EndElement(&Element);

    bool32 IsHot = InteractionIsHot(Layout->DebugState, ItemInteraction);

    if(BackdropColor.w > 0.0f){
        PushRectangle(&DebugState->RenderGroup, Element.Bounds, 0.0f, BackdropColor);
    }

    TextOutAt(DebugState, Vec2(GetMinCorner(Element.Bounds).x + Border,
        GetMaxCorner(Element.Bounds).y - Border - DebugState->FontScale * GetStartingBaselineY(DebugState->DebugFontInfo)),
        Text, IsHot ? HotColor : ItemColor);

    return(Dim);
}

INTERNAL_FUNCTION void
BeginRow(layout* Layout){
    Layout->NoLineFeed++;
}

INTERNAL_FUNCTION void 
Label(layout* Layout, char* Name){
    debug_interation NullInteraction = {};
    BasicTextElement(Layout, Name, NullInteraction, Vec4(1, 1, 1, 1), Vec4(1, 1, 1, 1));
}

INTERNAL_FUNCTION void ActionButton(layout* Layout, char* Name, debug_interation Interaction){
    BasicTextElement(Layout, Name, Interaction,
        Vec4(0.5f, 0.5f, 0.5f, 1.0f), Vec4(1, 1, 1, 1),
        4.0f, Vec4(0, 0.5f, 1.0f, 1.0f));
}

INTERNAL_FUNCTION void BooleanButton(layout* Layout, char* Name, bool32 Highlight, debug_interation){
    BasicTextElement(Layout, Name, Interaction,
        Highlight ? Vec4(1, 1, 1, 1) : Vec4(0.5f, 0.5f, 0.5f, 1.0f), Vec4(1, 1, 1, 1),
        4.0f, Vec4(0.0f, 0.5f, 1.0f, 1.0f));
}

INTERNAL_FUNCTION void
EndRow(layout* Layout){
    Assert(Layout->NoLineFeed > 0);
    --Layout->NoLineFeed;

    AdvanceElement(Layout, RectMinMax(Layout->At, Layout->At));
}

struct tooltip_buffer{
    uint32 Size;
    char* Data;
}

INTERNAL_FUNCTION tooltip_buffer
AddTooltip(debug_state* DebugState){
    tooltip_buffer Result;
    Result.Size = sizeof(DebugState->ToolTipText[0]);
    if(DebugState->ToolTipCount < ArrayCount(DebugState->ToolTipText)){
        Result.Data = DebugState->ToolTipText[DebugState->ToolTipCount++];
    }
    else{
        Result.Data = DebugState->ToolTipText[DebugState->ToolTipCount - 1];
    }

    return(Result);
}

INTERNAL_FUNCTION void DrawTooltips(debug_state* DebugState){
    render_group* RenderGroup = &DebugState->RenderGroup;

    layout* Layout = &DebugState->MouseTextLayout;

    for(uint32 ToolTipIndex = 0;
        ToolTipIndex < DebugState->ToolTipCount;
        ToolTipIndex++)
    {
        char* Text = DebugState->ToolTipIndex[ToolTipIndex];

        rectangle2 TextBounds = GetTextSize(DebugState, Text);
        vec2 Dim = {GetDim(TextBounds).x, Layout->LineAdvance};

        layout_element Element = BeginElementRectangle(Layout, &Dim);
        EndElement(&Element);

        PushRectangle(&DebugState->RenderGroup,
            AddRadiusTo(Element.Bounds, Vec2(4.0f, 4.0f)),
            0.0f, Vec4(0.0f, 0.0f, 0.0f, 0.75f));

        TextOutAt(DebugState, Vec2(GetMinCorner(Element.Bounds).x,
            GetMaxCorner(Element.Bounds).yDebugState->FontScale * GetStartingBaselineY(DebugState->DebugFontInfo)), 
            Text, Vec4(1, 1, 1, 1), 4000.0f);
    }
}