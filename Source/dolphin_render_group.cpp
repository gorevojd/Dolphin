#include "dolphin_render_group.h"

#define MY_TEMP_SQUARE(value) ((value) * (value))

inline gdVec4 SRGB255ToLinear1(gdVec4 v){
	gdVec4 Result;
	real32 OneOver255 = 1.0f / 255.0f;

	Result.x = MY_TEMP_SQUARE(OneOver255 * v.x);
	Result.y = MY_TEMP_SQUARE(OneOver255 * v.y);
	Result.z = MY_TEMP_SQUARE(OneOver255 * v.z);
	Result.w = OneOver255 * v.w;
	
	return(Result);
}

inline gdVec4 Linear1ToSRGB255(gdVec4 v){
	gdVec4 Result;

	Result.x = 255.0f * gd_sqrt(v.x);
	Result.y = 255.0f * gd_sqrt(v.y);
	Result.z = 255.0f * gd_sqrt(v.z);
	Result.w = 255.0f * v.w;

	return(Result);
}

inline void RenderNonScaledAABitmapQuickly(
	loaded_bitmap* Buffer,
	loaded_bitmap* Bitmap,
	gdVec2 Pos,
	gdVec2 Align = {})
{
	
	int32 MinX = Pos.x - Align.x;
	int32 MinY = Pos.y - Align.y;
	int32 MaxX = Pos.x - Align.x + Bitmap->Width;
	int32 MaxY = Pos.y - Align.y + Bitmap->Height;

	int SourceOffsetX = 0;
	int SourceOffsetY = 0;

	if (MinX < 0){
		SourceOffsetX = -MinX;
		MinX = 0;
	}
	if (MinY < 0){
		SourceOffsetY = -MinY;
		MinY = 0;
	}
	if (MaxX > Buffer->Width){
		MaxX = Buffer->Width;
	}
	if (MaxY > Buffer->Height){
		MaxY = Buffer->Height;
	}

	__m128i mMaskFF = _mm_set1_epi32(0xFF);
	__m128 mOneOver255 = _mm_set1_ps(1.0f / 255.0f);
	__m128 mOne = _mm_set1_ps(1.0f);

	uint32* SourceRow = (uint32*)Bitmap->Memory + (-SourceOffsetY + Bitmap->Height - 1) * Bitmap->Width + SourceOffsetX;
	uint32* DestRow = (uint32*)Buffer->Memory + MinY * Buffer->Width + MinX;

	for (int32 j = MinY; j < MaxY; j++){
		uint32* DestPtr = DestRow;
		uint32* SourcePtr = SourceRow;
		for (int32 i = MinX; i < MaxX; i+=4){

			__m128i mSource = _mm_loadu_si128((__m128i*)SourcePtr);
			__m128i mDest = _mm_loadu_si128((__m128i*)DestPtr);

			__m128i mSource_a_epi32 = _mm_and_si128(_mm_srli_epi32(mSource, 24), mMaskFF);
			__m128i mDest_a_epi32 = _mm_and_si128(_mm_srli_epi32(mDest, 24), mMaskFF);

			//__m128i mWriteMask = _mm_and_si128();
			
			/*
			//If Source Alpha is Fully is fully transparent, then draw only destination 
			if (SourceA == 0.0f){
				*DestPtr++ = *DestPtr;
				SourcePtr++;
				continue;
			}

			//If Source Alpha is fully opaque, then we render without intepolating
			if (SourceA == 255.0){
				*DestPtr++ = *SourcePtr++;
				continue;
			}
			*/
			__m128 mSource_a = _mm_cvtepi32_ps(mSource_a_epi32);
			__m128 mSource_r = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mSource, 16), mMaskFF));
			__m128 mSource_g = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mSource, 8), mMaskFF));
			__m128 mSource_b = _mm_cvtepi32_ps(_mm_and_si128(mSource, mMaskFF));

			__m128 mDest_a = _mm_cvtepi32_ps(mDest_a_epi32);
			__m128 mDest_r = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mDest, 16), mMaskFF));
			__m128 mDest_g = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mDest, 8), mMaskFF));
			__m128 mDest_b = _mm_cvtepi32_ps(_mm_and_si128(mDest, mMaskFF));

			__m128 mInvSrcAlpha = _mm_mul_ps(mSource_a, mOneOver255);
			__m128 mOneMinusInvSrcAlpha = _mm_sub_ps(mOne, mInvSrcAlpha);

			__m128 mResultColor_a = _mm_add_ps(mSource_a, _mm_mul_ps(mOneMinusInvSrcAlpha, mDest_a));
			__m128 mResultColor_r = _mm_add_ps(mSource_r, _mm_mul_ps(mOneMinusInvSrcAlpha, mDest_r));
			__m128 mResultColor_g = _mm_add_ps(mSource_g, _mm_mul_ps(mOneMinusInvSrcAlpha, mDest_g));
			__m128 mResultColor_b = _mm_add_ps(mSource_b, _mm_mul_ps(mOneMinusInvSrcAlpha, mDest_b));

			__m128i mResultColor_a_epi32 = _mm_cvttps_epi32(mResultColor_a);
			__m128i mResultColor_r_epi32 = _mm_cvttps_epi32(mResultColor_r);
			__m128i mResultColor_g_epi32 = _mm_cvttps_epi32(mResultColor_g);
			__m128i mResultColor_b_epi32 = _mm_cvttps_epi32(mResultColor_b);

			__m128i mOutColor = _mm_or_si128(
				_mm_or_si128(mResultColor_b_epi32, _mm_slli_epi32(mResultColor_g_epi32, 8)),
				_mm_or_si128(_mm_slli_epi32(mResultColor_r_epi32, 16), _mm_slli_epi32(mResultColor_r_epi32, 24)));

			_mm_storeu_si128((__m128i*)DestPtr, mOutColor);

			DestPtr += 4;
			SourcePtr += 4;
		}
		DestRow += Buffer->Width;
		SourceRow -= Bitmap->Width;
	}
}

inline void RenderBitmap(
	loaded_bitmap* Buffer,
	loaded_bitmap* Bitmap,
	gdVec2 Pos,
	gdVec2 Align = {})
{

	int32 MinX = Pos.x - Align.x;
	int32 MinY = Pos.y - Align.y;
	int32 MaxX = Pos.x - Align.x + Bitmap->Width;
	int32 MaxY = Pos.y - Align.y + Bitmap->Height;

	int SourceOffsetX = 0;
	int SourceOffsetY = 0;

	if (MinX < 0){
		SourceOffsetX = -MinX;
		MinX = 0;
	}
	if (MinY < 0){
		SourceOffsetY = -MinY;
		MinY = 0;
	}
	if (MaxX > Buffer->Width){
		MaxX = Buffer->Width;
	}
	if (MaxY > Buffer->Height){
		MaxY = Buffer->Height;
	}

	uint32 AlphaShift = 24;
	uint32 RedShift = 16;
	uint32 GreenShift = 8;
	uint32 BlueShift = 0;

	uint32* SourceRow = (uint32*)Bitmap->Memory + (-SourceOffsetY + Bitmap->Height - 1) * Bitmap->Width + SourceOffsetX;
	uint32* DestRow = (uint32*)Buffer->Memory + MinY * Buffer->Width + MinX;

	for (int32 j = MinY; j < MaxY; j++){
		uint32* DestPtr = DestRow;
		uint32* SourcePtr = SourceRow;
		for (int32 i = MinX; i < MaxX; i++){

#if 1
			
			
			
#if 0
			gdVec4 Source = {
				(real32)((*SourcePtr >> RedShift) & 0xFF),
				(real32)((*SourcePtr >> GreenShift) & 0xFF),
				(real32)((*SourcePtr >> BlueShift) & 0xFF),
				(real32)((*SourcePtr >> AlphaShift) & 0xFF) };
			Source = SRGB255ToLinear1(Source);
			
			gdVec4 Dest = {
				(real32)((*DestPtr >> 16) & 0xFF),
				(real32)((*DestPtr >> 8) & 0xFF),
				(real32)((*DestPtr >> 0) & 0xFF),
				(real32)((*DestPtr >> 24) & 0xFF) };
			Dest = SRGB255ToLinear1(Dest);

			gdVec4 Result = Source + (1.0f - Source.a) * Dest;

			Result = Linear1ToSRGB255(Result);
			*DestPtr++ =
				((uint32)(Result.a + 0.5f) << 24) |
				((uint32)(Result.r + 0.5f) << 16) |
				((uint32)(Result.g + 0.5f) << 8) |
				((uint32)(Result.b + 0.5f) << 0);
#else
			real32 SourceA = (real32)((*SourcePtr >> AlphaShift) & 0xFF);
			real32 DestA = (real32)((*DestPtr >> 24) & 0xFF);

			//If Source Alpha is Fully is fully transparent, then draw only destination 
			if (SourceA == 0.0f){
				*DestPtr++ = *DestPtr;
				SourcePtr++;
				continue;
			}

			//If Source Alpha is fully opaque, then we render without intepolating
			if (SourceA == 255.0){
				*DestPtr++ = *SourcePtr++;
				continue;
			}

			real32 SourceR = (real32)((*SourcePtr >> RedShift) & 0xFF);
			real32 SourceG = (real32)((*SourcePtr >> GreenShift) & 0xFF);
			real32 SourceB = (real32)((*SourcePtr >> BlueShift) & 0xFF);

			real32 DestR = (real32)((*DestPtr >> 16) & 0xFF);
			real32 DestG = (real32)((*DestPtr >> 8) & 0xFF);
			real32 DestB = (real32)((*DestPtr >> 0) & 0xFF);

			real32 InvA = SourceA / 255.0f;
			real32 ResultR = SourceR + (1.0f - InvA) * DestR;
			real32 ResultG = SourceG + (1.0f - InvA) * DestG;
			real32 ResultB = SourceB + (1.0f - InvA) * DestB;
			real32 ResultA = SourceA + (1.0f - InvA) * DestA;

			*DestPtr++ =
				((uint32)(ResultA + 0.5f) << 24) |
				((uint32)(ResultR + 0.5f) << 16) |
				((uint32)(ResultG + 0.5f) << 8) |
				((uint32)(ResultB + 0.5f) << 0);
#endif


			SourcePtr++;
#else
			*DestPtr++ = *SourcePtr++;
#endif
		}
		DestRow += Buffer->Width;
		SourceRow -= Bitmap->Width;
	}
}


inline void RenderRectangle(
	loaded_bitmap* Buffer,
	gdVec2 MinP,
	gdVec2 MaxP,
	gdVec4 Color, 
	gdRect2i ClipRect)
{
	int32 MinX = RoundReal32ToInt32(MinP.x);
	int32 MinY = RoundReal32ToInt32(MinP.y);
	int32 MaxX = RoundReal32ToInt32(MaxP.x);
	int32 MaxY = RoundReal32ToInt32(MaxP.y);

	if (MinX < 0){
		MinX = 0;
	}
	if (MinY < 0){
		MinY = 0;
	}
	if (MaxX > Buffer->Width){
		MaxX = Buffer->Width;
	}
	if (MaxY > Buffer->Height){
		MaxY = Buffer->Height;
	}

	int ClipMinX = ClipRect.MinX;
	int ClipMinY = ClipRect.MinY;
	int ClipMaxX = ClipRect.MaxX;
	int ClipMaxY = ClipRect.MaxY;

	if (MinX < ClipMinX){ MinX = ClipMinX; }
	if (MinY < ClipMinY){ MinY = ClipMinY; }
	if (MaxX > ClipMaxX){ MaxX = ClipMaxX; }
	if (MaxY > ClipMaxY){ MaxY = ClipMaxY; }

	uint32 DestColor = (
		(RoundReal32ToUInt32(255.0f * Color.a) << 24)|
		(RoundReal32ToUInt32(255.0f * Color.x) << 16)|
		(RoundReal32ToUInt32(255.0f * Color.y) << 8	)|
		(RoundReal32ToUInt32(255.0f * Color.z) << 0	));

	uint32* DestRow = (uint32*)Buffer->Memory + (MinY * Buffer->Width) + MinX;
	BEGIN_TIMED_BLOCK(RenderRectangle);
	for (int j = MinY; j < MaxY; j++){
		uint32* Pixel = (uint32*)DestRow;
		for (int i = MinX; i < MaxX; i++){
			*Pixel++ = DestColor;
		}
		DestRow += Buffer->Width;
	}
	END_TIMED_BLOCK_COUNTED(RenderRectangle, ((MaxY - MinY) * (MaxX - MinX)));
}

inline float DotProduct(gdVec2 v1, gdVec2 v2){
	return v1.x * v2.x + v1.y * v2.y;
}

#ifndef SPLIT_INTO_SCANLINES

inline void RenderRectangleQuickly(
	loaded_bitmap* Buffer,
	gdVec2 Origin,
	gdVec2 XAxis,
	gdVec2 YAxis,
	loaded_bitmap* Texture,
	gdVec4 Color,
	gdRect2i ClipRect)
{
	BEGIN_TIMED_BLOCK(RenderRectangleQuickly)


	Color.rgb *= Color.a;

	real32 InvXAxisLengthSq = 1.0f / gd_vec2_squared_mag(XAxis);
	real32 InvYAxisLengthSq = 1.0f / gd_vec2_squared_mag(YAxis);

	int MinX = Buffer->Width;
	int MinY = Buffer->Height;
	int MaxX = 0;
	int MaxY = 0;
	
	int WidthMax = Buffer->Width;
	int HeightMax = Buffer->Height;
	int RowAdvance = Buffer->Width;

	real32 InvWidthMax = 1.0f / (real32)WidthMax;
	real32 InvHeightMax = 1.0f / (real32)HeightMax;

	gdVec2 Points[] = { Origin, Origin + XAxis, Origin + YAxis, Origin + XAxis + YAxis };
	for (int i = 0; i < ArrayCount(Points); i++){
		if (Points[i].x < MinX){ MinX = Points[i].x; }
		if (Points[i].y < MinY){ MinY = Points[i].y; }
		if (Points[i].x > MaxX){ MaxX = Points[i].x; }
		if (Points[i].y > MaxY){ MaxY = Points[i].y; }
	}

	if (MinX < 0){
		MinX = 0;
	}
	if (MaxX > WidthMax){
		MaxX = WidthMax;
	}
	if (MinY < 0){
		MinY = 0;
	}
	if (MaxY > HeightMax){
		MaxY = HeightMax;
	}

	int ClipMinX = ClipRect.MinX;
	int ClipMinY = ClipRect.MinY;
	int ClipMaxX = ClipRect.MaxX;
	int ClipMaxY = ClipRect.MaxY;

	if (MinX < ClipMinX){ MinX = ClipMinX; }
	if (MinY < ClipMinY){ MinY = ClipMinY; }
	if (MaxX > ClipMaxX){ MaxX = ClipMaxX; }
	if (MaxY > ClipMaxY){ MaxY = ClipMaxY; }

	real32 OneOver255 = 1.0f / 255.0f;
	real32 PremultipliedXAxis_x = InvXAxisLengthSq * XAxis.x;
	real32 PremultipliedXAxis_y = InvXAxisLengthSq * XAxis.y;
	real32 PremultipliedYAxis_x = InvYAxisLengthSq * YAxis.x;
	real32 PremultipliedYAxis_y = InvYAxisLengthSq * YAxis.y;

	__m128 mOneOver255 = _mm_set1_ps(1.0f / 255.0f);
	__m128 mOne = _mm_set1_ps(1.0f);
	__m128 mOne255 = _mm_set1_ps(255.0f);
	__m128 mZero = _mm_set1_ps(0.0f);
	__m128 mHalf = _mm_set1_ps(0.5f);
	
	__m128 mPremultXAxis_x = _mm_set1_ps(PremultipliedXAxis_x);
	__m128 mPremultXAxis_y = _mm_set1_ps(PremultipliedXAxis_y);
	__m128 mPremultYAxis_x = _mm_set1_ps(PremultipliedYAxis_x);
	__m128 mPremultYAxis_y = _mm_set1_ps(PremultipliedYAxis_y);

	__m128 mXAxis_x = _mm_set1_ps(XAxis.x);
	__m128 mXAxis_y = _mm_set1_ps(XAxis.y);
	__m128 mYAxis_x = _mm_set1_ps(YAxis.x);
	__m128 mYAxis_y = _mm_set1_ps(YAxis.y);
	__m128 mOrigin_x = _mm_set1_ps(Origin.x);
	__m128 mOrigin_y = _mm_set1_ps(Origin.y);

	__m128 mColor_r = _mm_set1_ps(Color.r);
	__m128 mColor_g = _mm_set1_ps(Color.g);
	__m128 mColor_b = _mm_set1_ps(Color.b);
	__m128 mColor_a = _mm_set1_ps(Color.a);

	__m128 mWidth = _mm_set1_ps((real32)Texture->Width);
	__m128i mWidthI = _mm_set1_epi32(Texture->Width);
	__m128 mHeight = _mm_set1_ps((real32)Texture->Height);
	__m128 mScreenWidth = _mm_set1_ps((real32)Buffer->Width);

	__m128i mMaskFF = _mm_set1_epi32(0xFF);

	uint32* Row = (uint32*)Buffer->Memory + MinY * Buffer->Width + MinX;

#define mmSquare(value) _mm_mul_ps(value, value)
#define M(a, i) ((float*)(&a))[i]
#define Mi(a, i) ((int*)(&a))[i]

	BEGIN_TIMED_BLOCK(RenderRectangleQuicklyCounted);
	for (int Y = MinY; Y < MaxY; Y++){
		uint32* DestPixel = (uint32*)Row;
		__m128 mPixelPosition_y = _mm_set1_ps((real32)Y);
		for (int X = MinX; X < MaxX; X+=4){

			__m128 mPixelPosition_x = _mm_set_ps((real32)(X + 3), (real32)(X + 2), (real32)(X + 1), (real32)(X));

			__m128 mDiff_x = _mm_sub_ps(mPixelPosition_x, mOrigin_x);
			__m128 mDiff_y = _mm_sub_ps(mPixelPosition_y, mOrigin_y);

			/*Calculating U and V texture coordinates*/
			__m128 mU = _mm_add_ps(_mm_mul_ps(mPremultXAxis_x, mDiff_x), _mm_mul_ps(mPremultXAxis_y, mDiff_y));
			__m128 mV = _mm_add_ps(_mm_mul_ps(mPremultYAxis_x, mDiff_x), _mm_mul_ps(mPremultYAxis_y, mDiff_y));

			/*Clamping U and V to range between 0 and 1*/
			mU = _mm_min_ps(_mm_max_ps(mU, mZero), mOne);
			mV = _mm_min_ps(_mm_max_ps(mV, mZero), mOne);

			/*Calculating fractional part for bilinear texture blend*/
			__m128 mTextureSpaceXReal = _mm_mul_ps(mU, mWidth);
			__m128 mTextureSpaceYReal = _mm_mul_ps(mV, mHeight);

			__m128i mTextureSpaceX = _mm_cvttps_epi32(mTextureSpaceXReal);
			__m128i mTextureSpaceY = _mm_cvttps_epi32(mTextureSpaceYReal);

			__m128 mFractionalX = _mm_sub_ps(mTextureSpaceXReal, _mm_cvtepi32_ps(mTextureSpaceX));
			__m128 mFractionalY = _mm_sub_ps(mTextureSpaceYReal, _mm_cvtepi32_ps(mTextureSpaceY));

			/*Calculating mask of pixels that are about to be filled*/
			__m128 mWriteMaskTemp = _mm_and_ps(
				_mm_and_ps(
					_mm_cmpge_ps(mU, mZero),
					_mm_cmple_ps(mU, mOne)),
				_mm_and_ps(
					_mm_cmpge_ps(mV, mZero),
					_mm_cmple_ps(mV, mOne)));

			__m128 mEndScreenMask = _mm_cmplt_ps(mPixelPosition_x, _mm_set1_ps(MaxX));
			mWriteMaskTemp = _mm_and_ps(mEndScreenMask, mWriteMaskTemp);

			__m128i mWriteMask = _mm_castps_si128(mWriteMaskTemp);

			__m128i mTexelA = _mm_set1_epi32(0.0f);
			__m128i mTexelB = _mm_set1_epi32(0.0f);
			__m128i mTexelC = _mm_set1_epi32(0.0f);
			__m128i mTexelD = _mm_set1_epi32(0.0f);

			mTextureSpaceY = _mm_or_si128(
				_mm_mullo_epi16(mTextureSpaceY, mWidthI),
				_mm_slli_epi32(_mm_mulhi_epi16(mTextureSpaceY, mWidthI), 16));
			__m128i mTextureFetch = _mm_add_epi32(mTextureSpaceY, mTextureSpaceX);

			int32 Fetch0 = Mi(mTextureFetch, 0);
			int32 Fetch1 = Mi(mTextureFetch, 1);
			int32 Fetch2 = Mi(mTextureFetch, 2);
			int32 Fetch3 = Mi(mTextureFetch, 3);
			
			/*Loading texels*/
			uint32* Texel0Ptr = (uint32*)Texture->Memory + Fetch0;
			uint32* Texel1Ptr = (uint32*)Texture->Memory + Fetch1;
			uint32* Texel2Ptr = (uint32*)Texture->Memory + Fetch2;
			uint32* Texel3Ptr = (uint32*)Texture->Memory + Fetch3;

			mTexelA = _mm_setr_epi32(
				*Texel0Ptr,
				*Texel1Ptr,
				*Texel2Ptr,
				*Texel3Ptr);

			mTexelB = _mm_setr_epi32(
				*(Texel0Ptr + 1),
				*(Texel1Ptr + 1),
				*(Texel2Ptr + 1),
				*(Texel3Ptr + 1));

			mTexelC = _mm_setr_epi32(
				*(Texel0Ptr + Texture->Width),
				*(Texel1Ptr + Texture->Width),
				*(Texel2Ptr + Texture->Width),
				*(Texel3Ptr + Texture->Width));

			mTexelD = _mm_setr_epi32(
				*(Texel0Ptr + Texture->Width + 1),
				*(Texel1Ptr + Texture->Width + 1),
				*(Texel2Ptr + Texture->Width + 1),
				*(Texel3Ptr + Texture->Width + 1));

			/*Unpacking texels*/
			__m128 mTexelA_b = _mm_cvtepi32_ps(_mm_and_si128(mTexelA, mMaskFF));
			__m128 mTexelA_g = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mTexelA, 8), mMaskFF));
			__m128 mTexelA_r = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mTexelA, 16), mMaskFF));
			__m128 mTexelA_a = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mTexelA, 24), mMaskFF));

			__m128 mTexelB_b = _mm_cvtepi32_ps(_mm_and_si128(mTexelB, mMaskFF));
			__m128 mTexelB_g = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mTexelB, 8), mMaskFF));
			__m128 mTexelB_r = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mTexelB, 16), mMaskFF));
			__m128 mTexelB_a = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mTexelB, 24), mMaskFF));

			__m128 mTexelC_b = _mm_cvtepi32_ps(_mm_and_si128(mTexelC, mMaskFF));
			__m128 mTexelC_g = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mTexelC, 8), mMaskFF));
			__m128 mTexelC_r = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mTexelC, 16), mMaskFF));
			__m128 mTexelC_a = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mTexelC, 24), mMaskFF));
			
			__m128 mTexelD_b = _mm_cvtepi32_ps(_mm_and_si128(mTexelD, mMaskFF));
			__m128 mTexelD_g = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mTexelD, 8), mMaskFF));
			__m128 mTexelD_r = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mTexelD, 16), mMaskFF));
			__m128 mTexelD_a = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mTexelD, 24), mMaskFF));

			/*Converting texels from sRGB 255 space to linear 0-1 space*/
			mTexelA_r = mmSquare(_mm_mul_ps(mOneOver255, mTexelA_r));
			mTexelA_g = mmSquare(_mm_mul_ps(mOneOver255, mTexelA_g));
			mTexelA_b = mmSquare(_mm_mul_ps(mOneOver255, mTexelA_b));
			mTexelA_a = _mm_mul_ps(mOneOver255, mTexelA_a);

			mTexelB_r = mmSquare(_mm_mul_ps(mOneOver255, mTexelB_r));
			mTexelB_g = mmSquare(_mm_mul_ps(mOneOver255, mTexelB_g));
			mTexelB_b = mmSquare(_mm_mul_ps(mOneOver255, mTexelB_b));
			mTexelB_a = _mm_mul_ps(mOneOver255, mTexelB_a);

			mTexelC_r = mmSquare(_mm_mul_ps(mOneOver255, mTexelC_r));
			mTexelC_g = mmSquare(_mm_mul_ps(mOneOver255, mTexelC_g));
			mTexelC_b = mmSquare(_mm_mul_ps(mOneOver255, mTexelC_b));
			mTexelC_a = _mm_mul_ps(mOneOver255, mTexelC_a);

			mTexelD_r = mmSquare(_mm_mul_ps(mOneOver255, mTexelD_r));
			mTexelD_g = mmSquare(_mm_mul_ps(mOneOver255, mTexelD_g));
			mTexelD_b = mmSquare(_mm_mul_ps(mOneOver255, mTexelD_b));
			mTexelD_a = _mm_mul_ps(mOneOver255, mTexelD_a);

			/*Bilinear texture blend*/
			__m128 mInvFractionalX = _mm_sub_ps(mOne, mFractionalX);
			__m128 mInvFractionalY = _mm_sub_ps(mOne, mFractionalY);

			__m128 mTempBlend1_r = _mm_add_ps(_mm_mul_ps(mTexelA_r, mInvFractionalX), _mm_mul_ps(mTexelB_r, mFractionalX));
			__m128 mTempBlend1_g = _mm_add_ps(_mm_mul_ps(mTexelA_g, mInvFractionalX), _mm_mul_ps(mTexelB_g, mFractionalX));
			__m128 mTempBlend1_b = _mm_add_ps(_mm_mul_ps(mTexelA_b, mInvFractionalX), _mm_mul_ps(mTexelB_b, mFractionalX));
			__m128 mTempBlend1_a = _mm_add_ps(_mm_mul_ps(mTexelA_a, mInvFractionalX), _mm_mul_ps(mTexelB_a, mFractionalX));

			__m128 mTempBlend2_r = _mm_add_ps(_mm_mul_ps(mTexelC_r, mInvFractionalX), _mm_mul_ps(mTexelD_r, mFractionalX));
			__m128 mTempBlend2_g = _mm_add_ps(_mm_mul_ps(mTexelC_g, mInvFractionalX), _mm_mul_ps(mTexelD_g, mFractionalX));
			__m128 mTempBlend2_b = _mm_add_ps(_mm_mul_ps(mTexelC_b, mInvFractionalX), _mm_mul_ps(mTexelD_b, mFractionalX));
			__m128 mTempBlend2_a = _mm_add_ps(_mm_mul_ps(mTexelC_a, mInvFractionalX), _mm_mul_ps(mTexelD_a, mFractionalX));

			__m128 mSampledResult_r = _mm_add_ps(_mm_mul_ps(mTempBlend1_r, mInvFractionalY), _mm_mul_ps(mTempBlend2_r, mFractionalY));
			__m128 mSampledResult_g = _mm_add_ps(_mm_mul_ps(mTempBlend1_g, mInvFractionalY), _mm_mul_ps(mTempBlend2_g, mFractionalY));
			__m128 mSampledResult_b = _mm_add_ps(_mm_mul_ps(mTempBlend1_b, mInvFractionalY), _mm_mul_ps(mTempBlend2_b, mFractionalY));
			__m128 mSampledResult_a = _mm_add_ps(_mm_mul_ps(mTempBlend1_a, mInvFractionalY), _mm_mul_ps(mTempBlend2_a, mFractionalY));

			/*Multiplying sampled color with our incoming color*/
			mSampledResult_r = _mm_mul_ps(mSampledResult_r, mColor_r);
			mSampledResult_g = _mm_mul_ps(mSampledResult_g, mColor_g);
			mSampledResult_b = _mm_mul_ps(mSampledResult_b, mColor_b);
			mSampledResult_a = _mm_mul_ps(mSampledResult_a, mColor_a);

			/*Clamping resulted color between 0 and 1*/
			mSampledResult_r = _mm_min_ps(_mm_max_ps(mSampledResult_r, mZero), mOne);
			mSampledResult_g = _mm_min_ps(_mm_max_ps(mSampledResult_g, mZero), mOne);
			mSampledResult_b = _mm_min_ps(_mm_max_ps(mSampledResult_b, mZero), mOne);

			/*Loading destination color*/
			__m128i mOriginalDest = _mm_loadu_si128((__m128i*)DestPixel);;
			__m128 mDest_b = _mm_cvtepi32_ps(_mm_and_si128(mOriginalDest, mMaskFF));
			__m128 mDest_g = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mOriginalDest, 8), mMaskFF));
			__m128 mDest_r = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mOriginalDest, 16), mMaskFF));
			__m128 mDest_a = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mOriginalDest, 24), mMaskFF));

			/*Converting dest value from sRGB 255 space to linear 0-1 space*/
			__m128 mTempDest_r = _mm_mul_ps(mOneOver255, mDest_r);
			__m128 mTempDest_g = _mm_mul_ps(mOneOver255, mDest_g);
			__m128 mTempDest_b = _mm_mul_ps(mOneOver255, mDest_b);
			mDest_r = mmSquare(mTempDest_r);
			mDest_g = mmSquare(mTempDest_g);
			mDest_b = mmSquare(mTempDest_b);
			mDest_a = _mm_mul_ps(mOneOver255, mDest_a);

			/*Calculating result color*/
			__m128 mInvDelta = _mm_sub_ps(mOne, mSampledResult_a);
			__m128 mResultColor_r = _mm_add_ps(_mm_mul_ps(mDest_r, mInvDelta), mSampledResult_r);
			__m128 mResultColor_g = _mm_add_ps(_mm_mul_ps(mDest_g, mInvDelta), mSampledResult_g);
			__m128 mResultColor_b = _mm_add_ps(_mm_mul_ps(mDest_b, mInvDelta), mSampledResult_b);
			__m128 mResultColor_a = _mm_sub_ps(_mm_add_ps(mSampledResult_a, mDest_a), _mm_mul_ps(mSampledResult_a, mDest_a));

			/*Converting result color from linear 0-1 space to sRGB space*/
			mResultColor_r = _mm_mul_ps(mOne255, _mm_sqrt_ps(mResultColor_r));
			mResultColor_g = _mm_mul_ps(mOne255, _mm_sqrt_ps(mResultColor_g));
			mResultColor_b = _mm_mul_ps(mOne255, _mm_sqrt_ps(mResultColor_b));
			mResultColor_a = _mm_mul_ps(mOne255, mResultColor_a);

			/*Shifting, preparing for packing*/
			__m128i mResultColor_r_epi32 = _mm_cvtps_epi32(mResultColor_r);
			__m128i mResultColor_g_epi32 = _mm_cvtps_epi32(mResultColor_g);
			__m128i mResultColor_b_epi32 = _mm_cvtps_epi32(mResultColor_b);
			__m128i mResultColor_a_epi32 = _mm_cvtps_epi32(mResultColor_a);

			__m128i mResColorShifted_r = _mm_slli_epi32(mResultColor_r_epi32, 16);
			__m128i mResColorShifted_g = _mm_slli_epi32(mResultColor_g_epi32, 8);
			__m128i mResColorShifted_b = mResultColor_b_epi32;
			__m128i mResColorShifted_a = _mm_slli_epi32(mResultColor_a_epi32, 24);

			/*Packing result color*/
			__m128i mOut = _mm_or_si128(
				_mm_or_si128(mResColorShifted_a, mResColorShifted_r),
				_mm_or_si128(mResColorShifted_g, mResColorShifted_b));

			/*Applying mWriteMask to the result set*/
			__m128i mOutMasked = _mm_or_si128(
				_mm_and_si128(mOut, mWriteMask),
				_mm_andnot_si128(mWriteMask, mOriginalDest));

			/*Storing the value*/
			_mm_storeu_si128((__m128i*)DestPixel, mOutMasked);

			DestPixel += 4;
		}

		Row += RowAdvance;
	}
	END_TIMED_BLOCK_COUNTED(RenderRectangleQuicklyCounted, ((MaxX - MinX)*((MaxY - MinY))));
	END_TIMED_BLOCK(RenderRectangleQuickly)
}

#else

inline void RenderRectangleQuicklySplit(
	loaded_bitmap* Buffer,
	gdVec2 Origin,
	gdVec2 XAxis,
	gdVec2 YAxis,
	loaded_bitmap* Texture,
	gdVec4 Color,
	gdRect2i ClipRect,
	bool32 EvenRow)
{
	Color.rgb *= Color.a;

	real32 InvXAxisLengthSq = 1.0f / gd_vec2_squared_mag(XAxis);
	real32 InvYAxisLengthSq = 1.0f / gd_vec2_squared_mag(YAxis);

	int MinX = Buffer->Width;
	int MinY = Buffer->Height;
	int MaxX = 0;
	int MaxY = 0;

	int WidthMax = Buffer->Width;
	int HeightMax = Buffer->Height;
	int RowAdvance = Buffer->Width * 2;

	real32 InvWidthMax = 1.0f / (real32)WidthMax;
	real32 InvHeightMax = 1.0f / (real32)HeightMax;

	gdVec2 Points[] = { Origin, Origin + XAxis, Origin + YAxis, Origin + XAxis + YAxis };
	for (int i = 0; i < ArrayCount(Points); i++){
		if (Points[i].x < MinX){ MinX = Points[i].x; }
		if (Points[i].y < MinY){ MinY = Points[i].y; }
		if (Points[i].x > MaxX){ MaxX = Points[i].x; }
		if (Points[i].y > MaxY){ MaxY = Points[i].y; }
	}

	if (MinX < 0){
		MinX = 0;
	}
	if (MaxX > WidthMax){
		MaxX = WidthMax;
	}
	if (MinY < 0){
		MinY = 0;
	}
	if (MaxY > HeightMax){
		MaxY = HeightMax;
	}

	int ClipMinX = ClipRect.MinX;
	int ClipMinY = ClipRect.MinY;
	int ClipMaxX = ClipRect.MaxX;
	int ClipMaxY = ClipRect.MaxY;

	if (MinX < ClipMinX){ MinX = ClipMinX; }
	if (MinY < ClipMinY){ MinY = ClipMinY; }
	if (MaxX > ClipMaxX){ MaxX = ClipMaxX; }
	if (MaxY > ClipMaxY){ MaxY = ClipMaxY; }

	real32 OneOver255 = 1.0f / 255.0f;
	real32 PremultipliedXAxis_x = InvXAxisLengthSq * XAxis.x;
	real32 PremultipliedXAxis_y = InvXAxisLengthSq * XAxis.y;
	real32 PremultipliedYAxis_x = InvYAxisLengthSq * YAxis.x;
	real32 PremultipliedYAxis_y = InvYAxisLengthSq * YAxis.y;

	__m128 mOneOver255 = _mm_set1_ps(1.0f / 255.0f);
	__m128 mOne = _mm_set1_ps(1.0f);
	__m128 mOne255 = _mm_set1_ps(255.0f);
	__m128 mZero = _mm_set1_ps(0.0f);
	__m128 mHalf = _mm_set1_ps(0.5f);

	__m128 mPremultXAxis_x = _mm_set1_ps(PremultipliedXAxis_x);
	__m128 mPremultXAxis_y = _mm_set1_ps(PremultipliedXAxis_y);
	__m128 mPremultYAxis_x = _mm_set1_ps(PremultipliedYAxis_x);
	__m128 mPremultYAxis_y = _mm_set1_ps(PremultipliedYAxis_y);

	__m128 mXAxis_x = _mm_set1_ps(XAxis.x);
	__m128 mXAxis_y = _mm_set1_ps(XAxis.y);
	__m128 mYAxis_x = _mm_set1_ps(YAxis.x);
	__m128 mYAxis_y = _mm_set1_ps(YAxis.y);
	__m128 mOrigin_x = _mm_set1_ps(Origin.x);
	__m128 mOrigin_y = _mm_set1_ps(Origin.y);

	__m128 mColor_r = _mm_set1_ps(Color.r);
	__m128 mColor_g = _mm_set1_ps(Color.g);
	__m128 mColor_b = _mm_set1_ps(Color.b);
	__m128 mColor_a = _mm_set1_ps(Color.a);

	__m128 mWidth = _mm_set1_ps((real32)Texture->Width);
	__m128i mWidthI = _mm_set1_epi32(Texture->Width);
	__m128 mHeight = _mm_set1_ps((real32)Texture->Height);
	__m128 mScreenWidth = _mm_set1_ps((real32)Buffer->Width);

	__m128i mMaskFF = _mm_set1_epi32(0xFF);

	if (!EvenRow == (MinY & 1)){
		MinY = MinY + 1;
	}

#define mmSquare(value) _mm_mul_ps(value, value)
#define M(a, i) ((float*)(&a))[i]
#define Mi(a, i) ((int*)(&a))[i]

	uint32* Row = (uint32*)Buffer->Memory + MinY * Buffer->Width + MinX;

	BEGIN_TIMED_BLOCK(RenderRectangleQuicklyCounted);
	for (int Y = MinY; Y < MaxY; Y += 2){
		uint32* DestPixel = (uint32*)Row;
		__m128 mPixelPosition_y = _mm_set1_ps((real32)Y);
		for (int X = MinX; X < MaxX; X += 4){

			__m128 mPixelPosition_x = _mm_set_ps((real32)(X + 3), (real32)(X + 2), (real32)(X + 1), (real32)(X));

			__m128 mDiff_x = _mm_sub_ps(mPixelPosition_x, mOrigin_x);
			__m128 mDiff_y = _mm_sub_ps(mPixelPosition_y, mOrigin_y);

			/*Calculating U and V texture coordinates*/
			__m128 mU = _mm_add_ps(_mm_mul_ps(mPremultXAxis_x, mDiff_x), _mm_mul_ps(mPremultXAxis_y, mDiff_y));
			__m128 mV = _mm_add_ps(_mm_mul_ps(mPremultYAxis_x, mDiff_x), _mm_mul_ps(mPremultYAxis_y, mDiff_y));

			/*Clamping U and V to range between 0 and 1*/
			mU = _mm_min_ps(_mm_max_ps(mU, mZero), mOne);
			mV = _mm_min_ps(_mm_max_ps(mV, mZero), mOne);

			/*Calculating fractional part for bilinear texture blend*/
			__m128 mTextureSpaceXReal = _mm_mul_ps(mU, mWidth);
			__m128 mTextureSpaceYReal = _mm_mul_ps(mV, mHeight);

			__m128i mTextureSpaceX = _mm_cvttps_epi32(mTextureSpaceXReal);
			__m128i mTextureSpaceY = _mm_cvttps_epi32(mTextureSpaceYReal);

			__m128 mFractionalX = _mm_sub_ps(mTextureSpaceXReal, _mm_cvtepi32_ps(mTextureSpaceX));
			__m128 mFractionalY = _mm_sub_ps(mTextureSpaceYReal, _mm_cvtepi32_ps(mTextureSpaceY));

			/*Calculating mask of pixels that are about to be filled*/
			__m128 mWriteMaskTemp = _mm_and_ps(
				_mm_and_ps(
				_mm_cmpge_ps(mU, mZero),
				_mm_cmple_ps(mU, mOne)),
				_mm_and_ps(
				_mm_cmpge_ps(mV, mZero),
				_mm_cmple_ps(mV, mOne)));

			__m128 mEndScreenMask = _mm_cmplt_ps(mPixelPosition_x, _mm_set1_ps(MaxX));
			mWriteMaskTemp = _mm_and_ps(mEndScreenMask, mWriteMaskTemp);

			__m128i mWriteMask = _mm_castps_si128(mWriteMaskTemp);

			__m128i mTexelA = _mm_set1_epi32(0.0f);
			__m128i mTexelB = _mm_set1_epi32(0.0f);
			__m128i mTexelC = _mm_set1_epi32(0.0f);
			__m128i mTexelD = _mm_set1_epi32(0.0f);

			mTextureSpaceY = _mm_or_si128(
				_mm_mullo_epi16(mTextureSpaceY, mWidthI),
				_mm_slli_epi32(_mm_mulhi_epi16(mTextureSpaceY, mWidthI), 16));
			__m128i mTextureFetch = _mm_add_epi32(mTextureSpaceY, mTextureSpaceX);

			int32 Fetch0 = Mi(mTextureFetch, 0);
			int32 Fetch1 = Mi(mTextureFetch, 1);
			int32 Fetch2 = Mi(mTextureFetch, 2);
			int32 Fetch3 = Mi(mTextureFetch, 3);

			/*Loading texels*/
			uint32* Texel0Ptr = (uint32*)Texture->Memory + Fetch0;
			uint32* Texel1Ptr = (uint32*)Texture->Memory + Fetch1;
			uint32* Texel2Ptr = (uint32*)Texture->Memory + Fetch2;
			uint32* Texel3Ptr = (uint32*)Texture->Memory + Fetch3;

			mTexelA = _mm_setr_epi32(
				*Texel0Ptr,
				*Texel1Ptr,
				*Texel2Ptr,
				*Texel3Ptr);

			mTexelB = _mm_setr_epi32(
				*(Texel0Ptr + 1),
				*(Texel1Ptr + 1),
				*(Texel2Ptr + 1),
				*(Texel3Ptr + 1));

			mTexelC = _mm_setr_epi32(
				*(Texel0Ptr + Texture->Width),
				*(Texel1Ptr + Texture->Width),
				*(Texel2Ptr + Texture->Width),
				*(Texel3Ptr + Texture->Width));

			mTexelD = _mm_setr_epi32(
				*(Texel0Ptr + Texture->Width + 1),
				*(Texel1Ptr + Texture->Width + 1),
				*(Texel2Ptr + Texture->Width + 1),
				*(Texel3Ptr + Texture->Width + 1));

			/*Unpacking texels*/
			__m128 mTexelA_b = _mm_cvtepi32_ps(_mm_and_si128(mTexelA, mMaskFF));
			__m128 mTexelA_g = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mTexelA, 8), mMaskFF));
			__m128 mTexelA_r = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mTexelA, 16), mMaskFF));
			__m128 mTexelA_a = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mTexelA, 24), mMaskFF));

			__m128 mTexelB_b = _mm_cvtepi32_ps(_mm_and_si128(mTexelB, mMaskFF));
			__m128 mTexelB_g = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mTexelB, 8), mMaskFF));
			__m128 mTexelB_r = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mTexelB, 16), mMaskFF));
			__m128 mTexelB_a = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mTexelB, 24), mMaskFF));

			__m128 mTexelC_b = _mm_cvtepi32_ps(_mm_and_si128(mTexelC, mMaskFF));
			__m128 mTexelC_g = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mTexelC, 8), mMaskFF));
			__m128 mTexelC_r = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mTexelC, 16), mMaskFF));
			__m128 mTexelC_a = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mTexelC, 24), mMaskFF));

			__m128 mTexelD_b = _mm_cvtepi32_ps(_mm_and_si128(mTexelD, mMaskFF));
			__m128 mTexelD_g = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mTexelD, 8), mMaskFF));
			__m128 mTexelD_r = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mTexelD, 16), mMaskFF));
			__m128 mTexelD_a = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mTexelD, 24), mMaskFF));

			/*Converting texels from sRGB 255 space to linear 0-1 space*/
			mTexelA_r = mmSquare(_mm_mul_ps(mOneOver255, mTexelA_r));
			mTexelA_g = mmSquare(_mm_mul_ps(mOneOver255, mTexelA_g));
			mTexelA_b = mmSquare(_mm_mul_ps(mOneOver255, mTexelA_b));
			mTexelA_a = _mm_mul_ps(mOneOver255, mTexelA_a);

			mTexelB_r = mmSquare(_mm_mul_ps(mOneOver255, mTexelB_r));
			mTexelB_g = mmSquare(_mm_mul_ps(mOneOver255, mTexelB_g));
			mTexelB_b = mmSquare(_mm_mul_ps(mOneOver255, mTexelB_b));
			mTexelB_a = _mm_mul_ps(mOneOver255, mTexelB_a);

			mTexelC_r = mmSquare(_mm_mul_ps(mOneOver255, mTexelC_r));
			mTexelC_g = mmSquare(_mm_mul_ps(mOneOver255, mTexelC_g));
			mTexelC_b = mmSquare(_mm_mul_ps(mOneOver255, mTexelC_b));
			mTexelC_a = _mm_mul_ps(mOneOver255, mTexelC_a);

			mTexelD_r = mmSquare(_mm_mul_ps(mOneOver255, mTexelD_r));
			mTexelD_g = mmSquare(_mm_mul_ps(mOneOver255, mTexelD_g));
			mTexelD_b = mmSquare(_mm_mul_ps(mOneOver255, mTexelD_b));
			mTexelD_a = _mm_mul_ps(mOneOver255, mTexelD_a);

			/*Bilinear texture blend*/
			__m128 mInvFractionalX = _mm_sub_ps(mOne, mFractionalX);
			__m128 mInvFractionalY = _mm_sub_ps(mOne, mFractionalY);

			__m128 mTempBlend1_r = _mm_add_ps(_mm_mul_ps(mTexelA_r, mInvFractionalX), _mm_mul_ps(mTexelB_r, mFractionalX));
			__m128 mTempBlend1_g = _mm_add_ps(_mm_mul_ps(mTexelA_g, mInvFractionalX), _mm_mul_ps(mTexelB_g, mFractionalX));
			__m128 mTempBlend1_b = _mm_add_ps(_mm_mul_ps(mTexelA_b, mInvFractionalX), _mm_mul_ps(mTexelB_b, mFractionalX));
			__m128 mTempBlend1_a = _mm_add_ps(_mm_mul_ps(mTexelA_a, mInvFractionalX), _mm_mul_ps(mTexelB_a, mFractionalX));

			__m128 mTempBlend2_r = _mm_add_ps(_mm_mul_ps(mTexelC_r, mInvFractionalX), _mm_mul_ps(mTexelD_r, mFractionalX));
			__m128 mTempBlend2_g = _mm_add_ps(_mm_mul_ps(mTexelC_g, mInvFractionalX), _mm_mul_ps(mTexelD_g, mFractionalX));
			__m128 mTempBlend2_b = _mm_add_ps(_mm_mul_ps(mTexelC_b, mInvFractionalX), _mm_mul_ps(mTexelD_b, mFractionalX));
			__m128 mTempBlend2_a = _mm_add_ps(_mm_mul_ps(mTexelC_a, mInvFractionalX), _mm_mul_ps(mTexelD_a, mFractionalX));

			__m128 mSampledResult_r = _mm_add_ps(_mm_mul_ps(mTempBlend1_r, mInvFractionalY), _mm_mul_ps(mTempBlend2_r, mFractionalY));
			__m128 mSampledResult_g = _mm_add_ps(_mm_mul_ps(mTempBlend1_g, mInvFractionalY), _mm_mul_ps(mTempBlend2_g, mFractionalY));
			__m128 mSampledResult_b = _mm_add_ps(_mm_mul_ps(mTempBlend1_b, mInvFractionalY), _mm_mul_ps(mTempBlend2_b, mFractionalY));
			__m128 mSampledResult_a = _mm_add_ps(_mm_mul_ps(mTempBlend1_a, mInvFractionalY), _mm_mul_ps(mTempBlend2_a, mFractionalY));

			/*Multiplying sampled color with our incoming color*/
			mSampledResult_r = _mm_mul_ps(mSampledResult_r, mColor_r);
			mSampledResult_g = _mm_mul_ps(mSampledResult_g, mColor_g);
			mSampledResult_b = _mm_mul_ps(mSampledResult_b, mColor_b);
			mSampledResult_a = _mm_mul_ps(mSampledResult_a, mColor_a);

			/*Clamping resulted color between 0 and 1*/
			mSampledResult_r = _mm_min_ps(_mm_max_ps(mSampledResult_r, mZero), mOne);
			mSampledResult_g = _mm_min_ps(_mm_max_ps(mSampledResult_g, mZero), mOne);
			mSampledResult_b = _mm_min_ps(_mm_max_ps(mSampledResult_b, mZero), mOne);

			/*Loading destination color*/
			__m128i mOriginalDest = _mm_loadu_si128((__m128i*)DestPixel);;
			__m128 mDest_b = _mm_cvtepi32_ps(_mm_and_si128(mOriginalDest, mMaskFF));
			__m128 mDest_g = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mOriginalDest, 8), mMaskFF));
			__m128 mDest_r = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mOriginalDest, 16), mMaskFF));
			__m128 mDest_a = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mOriginalDest, 24), mMaskFF));

			/*Converting dest value from sRGB 255 space to linear 0-1 space*/
			__m128 mTempDest_r = _mm_mul_ps(mOneOver255, mDest_r);
			__m128 mTempDest_g = _mm_mul_ps(mOneOver255, mDest_g);
			__m128 mTempDest_b = _mm_mul_ps(mOneOver255, mDest_b);
			mDest_r = mmSquare(mTempDest_r);
			mDest_g = mmSquare(mTempDest_g);
			mDest_b = mmSquare(mTempDest_b);
			mDest_a = _mm_mul_ps(mOneOver255, mDest_a);

			/*Calculating result color*/
			__m128 mInvDelta = _mm_sub_ps(mOne, mSampledResult_a);
			__m128 mResultColor_r = _mm_add_ps(_mm_mul_ps(mDest_r, mInvDelta), mSampledResult_r);
			__m128 mResultColor_g = _mm_add_ps(_mm_mul_ps(mDest_g, mInvDelta), mSampledResult_g);
			__m128 mResultColor_b = _mm_add_ps(_mm_mul_ps(mDest_b, mInvDelta), mSampledResult_b);
			__m128 mResultColor_a = _mm_sub_ps(_mm_add_ps(mSampledResult_a, mDest_a), _mm_mul_ps(mSampledResult_a, mDest_a));

			/*Converting result color from linear 0-1 space to sRGB space*/
			mResultColor_r = _mm_mul_ps(mOne255, _mm_sqrt_ps(mResultColor_r));
			mResultColor_g = _mm_mul_ps(mOne255, _mm_sqrt_ps(mResultColor_g));
			mResultColor_b = _mm_mul_ps(mOne255, _mm_sqrt_ps(mResultColor_b));
			mResultColor_a = _mm_mul_ps(mOne255, mResultColor_a);

			/*Shifting, preparing for packing*/
			__m128i mResultColor_r_epi32 = _mm_cvtps_epi32(mResultColor_r);
			__m128i mResultColor_g_epi32 = _mm_cvtps_epi32(mResultColor_g);
			__m128i mResultColor_b_epi32 = _mm_cvtps_epi32(mResultColor_b);
			__m128i mResultColor_a_epi32 = _mm_cvtps_epi32(mResultColor_a);

			__m128i mResColorShifted_r = _mm_slli_epi32(mResultColor_r_epi32, 16);
			__m128i mResColorShifted_g = _mm_slli_epi32(mResultColor_g_epi32, 8);
			__m128i mResColorShifted_b = mResultColor_b_epi32;
			__m128i mResColorShifted_a = _mm_slli_epi32(mResultColor_a_epi32, 24);

			/*Packing result color*/
			__m128i mOut = _mm_or_si128(
				_mm_or_si128(mResColorShifted_a, mResColorShifted_r),
				_mm_or_si128(mResColorShifted_g, mResColorShifted_b));

			/*Applying mWriteMask to the result set*/
			__m128i mOutMasked = _mm_or_si128(
				_mm_and_si128(mOut, mWriteMask),
				_mm_andnot_si128(mWriteMask, mOriginalDest));

			/*Storing the value*/
			_mm_storeu_si128((__m128i*)DestPixel, mOutMasked);

			DestPixel += 4;
		}

		Row += RowAdvance;
	}
	END_TIMED_BLOCK_COUNTED(RenderRectangleQuicklyCounted, ((MaxX - MinX)*((MaxY - MinY) / 2)))
}
#endif


inline void RenderRectangleSlowly(
	loaded_bitmap* Buffer,
	gdVec2 Origin,
	gdVec2 XAxis,
	gdVec2 YAxis,
	loaded_bitmap* Texture,
	gdVec4 Color)
{
	Color.rgb *= Color.a;

	real32 InvXAxisLengthSq = 1.0f / gd_vec2_squared_mag(XAxis);
	real32 InvYAxisLengthSq = 1.0f / gd_vec2_squared_mag(YAxis);

	int MinX = Buffer->Width, MinY = Buffer->Height, MaxX = 0, MaxY = 0;
	
	int WidthMax = Buffer->Width;
	int HeightMax = Buffer->Height;

	real32 InvWidthMax = 1.0f / (real32)WidthMax;
	real32 InvHeightMax = 1.0f / (real32)HeightMax;

	gdVec2 Points[] = { Origin, Origin + XAxis, Origin + YAxis, Origin + XAxis + YAxis };
	for (int i = 0; i < ArrayCount(Points); i++){
		
#if 0
		int FloorX = FloorReal32ToInt32(Points[i].x);
		int CeilX = CeilReal32ToInt32(Points[i].x);
		int FloorY = FloorReal32ToInt32(Points[i].y);
		int CeilY = CeilReal32ToInt32(Points[i].y);
		
		if (MinX > FloorX){ MinX = FloorX; }
		if (MinY > FloorY){ MinY = FloorY; }
		if (MaxX < CeilX){ MaxX = CeilX; }
		if (MaxY < CeilY){ MaxY = CeilY; }
#else
		if (Points[i].x < MinX){ MinX = Points[i].x; }
		if (Points[i].y < MinY){ MinY = Points[i].y; }
		if (Points[i].x > MaxX){ MaxX = Points[i].x; }
		if (Points[i].y > MaxY){ MaxY = Points[i].y; }
#endif
	}
	if (MinX < 0){
		MinX = 0;
	}
	if (MaxX > WidthMax){
		MaxX = WidthMax;
	}
	if (MinY < 0){
		MinY = 0;
	}
	if (MaxY > HeightMax){
		MaxY = HeightMax;
	}

	uint32 AlphaShift = 24;
	uint32 RedShift = 16;
	uint32 GreenShift = 8;
	uint32 BlueShift = 0;

	uint32 DestColor = (
		(RoundReal32ToUInt32(255.0f * Color.a) << 24) |
		(RoundReal32ToUInt32(255.0f * Color.x) << 16) |
		(RoundReal32ToUInt32(255.0f * Color.y) << 8) |
		(RoundReal32ToUInt32(255.0f * Color.z) << 0));

	BEGIN_TIMED_BLOCK(RenderRectangleSlowlyCounted)
	for (int Y = MinY; Y < MaxY; Y++){
		for (int X = MinX; X < MaxX; X++){
			uint32* DestPixel = (uint32*)Buffer->Memory + Y * Buffer->Width + X;
			gdVec2 P = gd_vec2(X, Y);
			
			bool32 EdgeTest0 = DotProduct(P - (Origin), YAxis) > 0.0f;
			bool32 EdgeTest1 = DotProduct(P - (Origin + XAxis), -XAxis) > 0.0f;
			bool32 EdgeTest2 = DotProduct(P - (Origin + XAxis + YAxis), -YAxis) > 0.0f;
			bool32 EdgeTest3 = DotProduct(P - (Origin + YAxis), XAxis) > 0.0f;

			if (EdgeTest0 &&
				EdgeTest1 &&
				EdgeTest2 &&
				EdgeTest3)
			{
				gdVec2 ScreenSpaceUV = { InvWidthMax * (real32)X, InvHeightMax * (real32)Y };

				gdVec2 Diff = P - Origin;
				real32 U = InvXAxisLengthSq * DotProduct(Diff, XAxis);
				real32 V = InvYAxisLengthSq * DotProduct(Diff, YAxis);

				real32 TextureSpaceXReal = U * Texture->Width;
				real32 TextureSpaceYReal = V * Texture->Height;

				int32 TextureSpaceX = (int32)TextureSpaceXReal;
				int32 TextureSpaceY = (int32)TextureSpaceYReal;

				real32 FractionalX = TextureSpaceXReal - (real32)TextureSpaceX;
				real32 FractionalY = TextureSpaceYReal - (real32)TextureSpaceY;

				uint32* Source = (uint32*)Texture->Memory + TextureSpaceY * Texture->Width + TextureSpaceX;
#if 1
				/*With texture filtering - bilinear sample*/
	
				uint32* TexelAPtr = Source;
				uint32* TexelBPtr = Source + 1;
				uint32* TexelCPtr = Source + Texture->Width;
				uint32* TexelDPtr = Source + Texture->Width + 1;

				gdVec4 TexelA = {
					(real32)((*TexelAPtr >> RedShift) & 0xFF),
					(real32)((*TexelAPtr >> GreenShift) & 0xFF),
					(real32)((*TexelAPtr >> BlueShift) & 0xFF),
					(real32)((*TexelAPtr >> AlphaShift) & 0xFF)};
				
				gdVec4 TexelB = {
					(real32)((*TexelBPtr >> RedShift) & 0xFF),
					(real32)((*TexelBPtr >> GreenShift) & 0xFF),
					(real32)((*TexelBPtr >> BlueShift) & 0xFF), 
					(real32)((*TexelBPtr >> AlphaShift) & 0xFF)};

				gdVec4 TexelC = {
					(real32)((*TexelCPtr >> RedShift) & 0xFF),
					(real32)((*TexelCPtr >> GreenShift) & 0xFF),
					(real32)((*TexelCPtr >> BlueShift) & 0xFF),
					(real32)((*TexelCPtr >> AlphaShift) & 0xFF) };

				gdVec4 TexelD = {
					(real32)((*TexelDPtr >> RedShift) & 0xFF),
					(real32)((*TexelDPtr >> GreenShift) & 0xFF),
					(real32)((*TexelDPtr >> BlueShift) & 0xFF),
					(real32)((*TexelDPtr >> AlphaShift) & 0xFF) };

				TexelA = SRGB255ToLinear1(TexelA);
				TexelB = SRGB255ToLinear1(TexelB);
				TexelC = SRGB255ToLinear1(TexelC);
				TexelD = SRGB255ToLinear1(TexelD);

				gdVec4 SampledResult = gd_vec4_lerp(
					gd_vec4_lerp(TexelA, TexelB, FractionalX),
					gd_vec4_lerp(TexelC, TexelD, FractionalX),
					FractionalY);

#else
				/*Without texture filtering*/
				gdVec4 SampledResult = {
					(real32)((*Source >> RedShift) & 0xFF),
					(real32)((*Source >> GreenShift) & 0xFF),
					(real32)((*Source >> BlueShift) & 0xFF),
					(real32)((*Source >> AlphaShift) & 0xFF)};
				SampledResult = SRGB255ToLinear1(SampledResult);
#endif
				gdVec4 Dest = {
					(real32)((*DestPixel >> 16) & 0xFF),
					(real32)((*DestPixel >> 8) & 0xFF),
					(real32)((*DestPixel >> 0) & 0xFF),
					(real32)((*DestPixel >> 24) & 0xFF)};

				Dest = SRGB255ToLinear1(Dest);

				SampledResult.r *= Color.r;
				SampledResult.g *= Color.g;
				SampledResult.b *= Color.b;
				SampledResult.a *= Color.a;
				real32 InvDelta = 1.0f - SampledResult.a;

				//Source##C calculated with premultiplied alpha
				gdVec4 ResultColor;
				ResultColor.x = SampledResult.x + InvDelta * Dest.x;
				ResultColor.y = SampledResult.y + InvDelta * Dest.y;
				ResultColor.z = SampledResult.z + InvDelta * Dest.z;
				ResultColor.w = (SampledResult.a + Dest.a - SampledResult.a * Dest.a);

				ResultColor = Linear1ToSRGB255(ResultColor);

				*DestPixel = ((uint32)(ResultColor.w + 0.5f) << 24) |
						 ((uint32)(ResultColor.x + 0.5f) << 16) |
						 ((uint32)(ResultColor.y + 0.5f) << 8) |
						 ((uint32)(ResultColor.z + 0.5f) << 0);
			}
		}
	}
	END_TIMED_BLOCK_COUNTED(RenderRectangleSlowlyCounted, ((MaxX - MinX)*((MaxY - MinY))))
}

inline void RenderGroupToOutput(render_group* RenderGroup, loaded_bitmap* OutputTarget, gdRect2i ClipRect){

	for (int Base = 0; Base < RenderGroup->PushBufferSize;){
		render_group_entry_header* Header = (render_group_entry_header*)(RenderGroup->PushBufferBase + Base);
		void* EntryData = (uint8*)Header + sizeof(render_group_entry_header);
		Base += sizeof(render_group_entry_header);
		switch (Header->Type){
			case RenderGroupEntry_render_entry_clear:{
				render_entry_clear* EntryClear = (render_entry_clear*)EntryData;
				RenderRectangle(
					OutputTarget,
					gd_vec2_zero(),
					gd_vec2(OutputTarget->Width, OutputTarget->Height),
					EntryClear->Color,
					ClipRect);
				Base += sizeof(*EntryClear);
			}break;
			case RenderGroupEntry_render_entry_rectangle:{
				render_entry_rectangle* EntryRect = (render_entry_rectangle*)EntryData;

				RenderRectangle(
					OutputTarget,
					EntryRect->EntityBasis.Offset.xy,
					EntryRect->EntityBasis.Offset.xy + EntryRect->Dim,
					EntryRect->Color,
					ClipRect);
					
				Base += sizeof(*EntryRect);
			}break;
			case RenderGroupEntry_render_entry_bitmap:{
				render_entry_bitmap* EntryBitmap = (render_entry_bitmap*)EntryData;
				
#if 0
				RenderBitmap(
					OutputTarget,
					EntryBitmap->Bitmap,
					EntryBitmap->EntityBasis.Offset.xy);
#else
				gdVec2 XAxis = { 1.0f, 0.0f };
				gdVec2 YAxis = { 0.0f, 1.0f };


#if defined(SPLIT_INTO_SCANLINES)
				RenderRectangleQuicklySplit(
					OutputTarget,
					EntryBitmap->EntityBasis.Offset.xy,
					XAxis * EntryBitmap->Bitmap->Width,
					YAxis * EntryBitmap->Bitmap->Height,
					EntryBitmap->Bitmap,
					EntryBitmap->Color,
					ClipRect,
					true);

				RenderRectangleQuicklySplit(
					OutputTarget,
					EntryBitmap->EntityBasis.Offset.xy,
					XAxis * EntryBitmap->Bitmap->Width,
					YAxis * EntryBitmap->Bitmap->Height,
					EntryBitmap->Bitmap,
					EntryBitmap->Color,
					ClipRect,
					false);
#else
				RenderRectangleQuickly(
					OutputTarget,
					EntryBitmap->EntityBasis.Offset.xy,
					XAxis * EntryBitmap->Bitmap->Width,
					YAxis * EntryBitmap->Bitmap->Height,
					EntryBitmap->Bitmap,
					EntryBitmap->Color,
					ClipRect);
#endif
#endif


				Base += sizeof(*EntryBitmap);
			}break;
			
			INVALID_DEFAULT_CASE;
		}
	}
}

inline void TiledRenderGroupToOutput(render_group* RenderGroup, loaded_bitmap* OutputTarget){
	
	int TileCountX = 4;
	int TileCountY = 4;

	int TileWidth = OutputTarget->Width / TileCountX;
	int TileHeight = OutputTarget->Height / TileCountY;

	for (int TileY = 0; TileY < TileCountY; TileY++){
		for (int TileX = 0; TileX < TileCountX; TileX++){
			gdRect2i ClipRect;
			
			ClipRect.MinX = TileX * TileWidth;
			ClipRect.MinY = TileY * TileHeight;
			ClipRect.MaxX = TileX * TileWidth + TileWidth;
			ClipRect.MaxY = TileY * TileHeight + TileHeight;

			if (TileX == (TileCountX - 1)){
				ClipRect.MaxX = OutputTarget->Width;
			}
			if (TileY == (TileCountY - 1)){
				ClipRect.MaxY = OutputTarget->Height;
			}

			RenderGroupToOutput(RenderGroup, OutputTarget, ClipRect);
		}
	}
}

INTERNAL_FUNCTION render_group*
AllocateRenderGroup(memory_arena* Arena, uint32 MaxPushBufferSize, real32 MetersToPixels){ 
	render_group* Result = PushStruct(Arena, render_group);
	Result->PushBufferBase = (uint8*)PushSize(Arena, MaxPushBufferSize);

	Result->MetersToPixels = MetersToPixels;
	Result->MaxPushBufferSize = MaxPushBufferSize;
	Result->PushBufferSize = 0;

	return(Result);
}

#define PUSH_RENDER_ELEMENT(RenderGroup, type) (type*)PushRenderElement_(RenderGroup, sizeof(type), RenderGroupEntry_##type)
inline void* PushRenderElement_(
	render_group* RenderGroup,
	uint32 Size,
	render_group_entry_type Type)
{
	void* Result = 0;
	if (RenderGroup->PushBufferSize + Size < RenderGroup->MaxPushBufferSize){
		render_group_entry_header* Header = (render_group_entry_header*)(RenderGroup->PushBufferBase + RenderGroup->PushBufferSize);
		Header->Type = Type;
		RenderGroup->PushBufferSize += (Size  + sizeof(render_group_entry_header));
		Result = (uint8*)Header + sizeof(render_group_entry_header);
	}
	else{
		INVALID_CODE_PATH;
	}
	return(Result);
}

inline void PushBitmap(
	render_group* RenderGroup,
	loaded_bitmap* Bitmap,
	gdVec3 Offset,
	gdVec2 Align = gd_vec2(0.0, 0.0f),
	gdVec4 Color = gd_vec4(1.0f, 1.0f, 1.0f, 1.0f))
{
	render_entry_bitmap* PushedBitmap = PUSH_RENDER_ELEMENT(RenderGroup, render_entry_bitmap);
	if (PushedBitmap){
		PushedBitmap->Bitmap = Bitmap;
		//PushedBitmap->EntityBasis.Offset = RenderGroup->MetersToPixels * gd_vec2(Offset.x, -Offset.y) - Align;
		gdVec2 Temp = gd_vec2(Offset.x, Offset.y) - Align;

		PushedBitmap->EntityBasis.Offset = gd_vec3(Temp.x, Temp.y, Offset.z);
		PushedBitmap->Color = Color;
	}
}

inline void PushRectangle(
	render_group* RenderGroup,
	gdVec3 Offset,
	gdVec2 Dim,
	gdVec4 Color)
{
	render_entry_rectangle* PushedRect =  PUSH_RENDER_ELEMENT(RenderGroup, render_entry_rectangle);
	if (PushedRect){
		//gdVec2 HalfOfDimension = 0.5f * gd_vec2_mul(Dim, RenderGroup->MetersToPixels);

		PushedRect->Color = Color;
		//PushedRect->Dim = gd_vec2_mul(Dim, RenderGroup->MetersToPixels);
		//PushedRect->EntityBasis.Offset = RenderGroup->MetersToPixels * gd_vec2(Offset.x, -Offset.y) - HalfOfDimension;
		PushedRect->Dim = Dim;
		gdVec2 Temp = gd_vec2(Offset.x, Offset.y);
		PushedRect->EntityBasis.Offset = gd_vec3(Temp.x, Temp.y, Offset.z);
	}
}

inline void PushRectangleOutline(
	render_group* RenderGroup,
	gdVec3 Offset,
	gdVec2 Dim,
	gdVec4 Color = gd_vec4(0.0f, 0.0f, 0.0f, 1.0f),
	real32 Thickness = 4)
{
	PushRectangle(RenderGroup, Offset - gd_vec3(Thickness, 0.0f, 0.0f), gd_vec2(Thickness, Dim.y + Thickness), Color);
	PushRectangle(RenderGroup, Offset - gd_vec3(Thickness, Thickness, 0.0f), gd_vec2(Dim.x + 2.0f * Thickness, Thickness), Color);
	PushRectangle(RenderGroup, Offset + gd_vec3(0.0f, Dim.y, 0.0f), gd_vec2(Dim.x + Thickness, Thickness), Color);
	PushRectangle(RenderGroup, Offset + gd_vec3(Dim.x, 0.0f, 0.0f), gd_vec2(Thickness, Dim.y), Color);
}

inline void PushClear(render_group* RenderGroup, gdVec4 Color){
	render_entry_clear* PushedClear = PUSH_RENDER_ELEMENT(RenderGroup, render_entry_clear);
	if (PushedClear){
		PushedClear->Color = Color;
	}
}

inline void PushCoordinateSystem(render_group* RenderGroup, gdVec2 Origin, gdVec2 XAxis, gdVec2 YAxis, gdVec4 Color, loaded_bitmap* Texture){
	render_entry_coordinate_system* PushedCS = PUSH_RENDER_ELEMENT(RenderGroup, render_entry_coordinate_system);
	if (PushedCS){
		PushedCS->Origin = Origin;
		PushedCS->XAxis = XAxis;
		PushedCS->YAxis = YAxis;
		PushedCS->Color = Color;
		PushedCS->Texture = Texture;
		int TmpCount = 0;
		for (float j = 0; j < 1.0f; j += 0.25f){
			for (float i = 0; i < 1.0f; i += 0.25f){
				PushedCS->Points[TmpCount] = Origin + i * XAxis + j * YAxis;
				TmpCount++;
			}
		}
	}
}