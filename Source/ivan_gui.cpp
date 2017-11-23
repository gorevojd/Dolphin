#include "ivan_gui.h"

INTERNAL_FUNCTION rectangle2 GUITextOut(
    gui_state* GUIState, 
    gui_text_op Op,
    vec2 P, 
    char* String, 
    vec4 Color = Vec4(1, 1, 1, 1), 
    float AtZ = 0.0f)
{
	rectangle2 Result = InvertedInfinityRectangle();

	if(GUIState && GUIState->GUIFont){
		render_group* RenderGroup = GUIState->RenderGroup;
	    loaded_font* Font = PushFont(RenderGroup, GUIState->FontID);
	    dda_font* Info = GUIState->GUIFontInfo;

        float CharScale = GUIState->FontScale;
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
                CharScale = GUIState->FontScale * IVAN_MATH_CLAMP01(ScaleScale * (float)(Ptr[2] - '0'));
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
                    
                    if(Op == GUITextOp_DrawText){
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
                    	Assert(Op == GUITextOp_SizeText);
                    	
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
	gui_state* GUIState, 
	vec2 P, 
	char* String, 
	vec4 Color = Vec4(1, 1, 1, 1),
	float AtZ = 0.0f)
{
	GUITextOut(GUIState, GUITextOp_DrawText, P, String, Color, AtZ);
}

inline rectangle2
GetTextSize(gui_state* GUIState, char* String){
	rectangle2 Result = GUITextOut(GUIState, GUITextOp_SizeText, Vec2(0, 0), String);

	return(Result);
}

inline float 
GetLineAdvance(gui_state* GUIState){
	float Result = GetLineAdvanceFor(GUIState->GUIFontInfo) * GUIState->FontScale;
}

inline float 
GetBaseline(gui_state* GUIState){
	float Result = GUIState->FontScale * GetStartingBaselineY(GUIState->GUIFontInfo);
	return(Result);
}

INTERNAL_FUNCTION gui_state* InitGUI(memory_arena* Arena){
	gui_state* Result = PushStruct(Arena, gui_state);

	return(Result);
}

INTERNAL_FUNCTION void BeginGUI(
	gui_state* GUIState,
	game_assets* Assets,
	render_group* RenderGroup)
{
 	asset_vector MatchVector = {};
    asset_vector WeightVector = {};
    MatchVector.Data[Tag_FontType] = (float)FontType_Debug;
    WeightVector.Data[Tag_FontType] = 1.0f;
    
    GUIState->FontID = GetBestMatchFontFrom(Assets, Asset_Font, &MatchVector, &WeightVector);
	GUIState->FontScale = 1.0f;

	GUIState->RenderGroup = RenderGroup;

	GUIState->GUIFont = PushFont(GUIState->RenderGroup, GUIState->FontID);
	GUIState->GUIFontInfo = GetFontInfo(GUIState->RenderGroup->Assets, GUIState->FontID);
}

INTERNAL_FUNCTION void EndGUI(gui_state* GUIState)
{

}
