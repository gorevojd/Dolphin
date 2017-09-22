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