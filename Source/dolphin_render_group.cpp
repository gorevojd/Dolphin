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
	gdVec4 Color)
{
	int32 MinX = RoundReal32ToInt32(MinP.x);
	int32 MinY = RoundReal32ToInt32(MinP.y);
	int32 MaxX = RoundReal32ToInt32(MaxP.x);
	int32 MaxY = RoundReal32ToInt32(MaxP.y);

	//int32 MinX = MinP.x;
	//int32 MinY = MinP.y;
	//int32 MaxX = MaxP.x;
	//int32 MaxY = MaxP.y;

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


	uint32 DestColor = (
		(RoundReal32ToUInt32(255.0f * Color.a) << 24)|
		(RoundReal32ToUInt32(255.0f * Color.x) << 16)|
		(RoundReal32ToUInt32(255.0f * Color.y) << 8	)|
		(RoundReal32ToUInt32(255.0f * Color.z) << 0	));


	uint32* DestRow = (uint32*)Buffer->Memory + (MinY * Buffer->Width) + MinX;
	for (int j = MinY; j < MaxY; j++){
		uint32* Pixel = (uint32*)DestRow;
		for (int i = MinX; i < MaxX; i++){
			*Pixel++ = DestColor;
		}
		DestRow += Buffer->Width;
	}
}

inline float DotProduct(gdVec2 v1, gdVec2 v2){
	return v1.x * v2.x + v1.y * v2.y;
}

inline void RenderRectangleQuickly(
	loaded_bitmap* Buffer,
	gdVec2 Origin,
	gdVec2 XAxis,
	gdVec2 YAxis,
	loaded_bitmap* Texture,
	gdVec4 Color)
{
	Color.rgb *= Color.a;
	//Color.r = Color.r * Color.a;
	//Color.g = Color.g * Color.a;
	//Color.b = Color.b * Color.a;

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

	real32 OneOver255 = 1.0f / 255.0f;
	real32 PremultipliedXAxis_x = InvXAxisLengthSq * XAxis.x;
	real32 PremultipliedXAxis_y = InvXAxisLengthSq * XAxis.y;
	real32 PremultipliedYAxis_x = InvYAxisLengthSq * YAxis.x;
	real32 PremultipliedYAxis_y = InvYAxisLengthSq * YAxis.y;

	__m128i mAlphaShift = _mm_set1_epi32(24);
	__m128i mRedShift = _mm_set1_epi32(16);
	__m128i mGreenShift = _mm_set1_epi32(8);
	__m128i mBlueShift = _mm_set1_epi32(0);

	__m128 mOneOver255 = _mm_set1_ps(1.0f / 255.0f);
	__m128 mOne = _mm_set1_ps(1.0f);
	__m128 mOne255 = _mm_set1_ps(255.0f);
	__m128 mZero = _mm_set1_ps(0.0f);
	
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

	__m128i mMaskFF = _mm_set1_epi32(0xFF);

	bool ShouldFill[4];

#define mmSquare(value) _mm_mul_ps(value, value)
#define M(a, i) ((float*)(&a))[i]

	uint32* Row = (uint32*)Buffer->Memory + MinY * Buffer->Width + MinX;

	for (int Y = MinY; Y < MaxY; Y++){
		uint32* DestPixel = (uint32*)Row;
		for (int X = MinX; X < MaxX; X+=4){

			__m128 mTexelA_r = _mm_set1_ps(0.0f);
			__m128 mTexelA_g = _mm_set1_ps(0.0f);
			__m128 mTexelA_b = _mm_set1_ps(0.0f);
			__m128 mTexelA_a = _mm_set1_ps(0.0f);

			__m128 mTexelB_r = _mm_set1_ps(0.0f);
			__m128 mTexelB_g = _mm_set1_ps(0.0f);
			__m128 mTexelB_b = _mm_set1_ps(0.0f);
			__m128 mTexelB_a = _mm_set1_ps(0.0f);

			__m128 mTexelC_r = _mm_set1_ps(0.0f);
			__m128 mTexelC_g = _mm_set1_ps(0.0f);
			__m128 mTexelC_b = _mm_set1_ps(0.0f);
			__m128 mTexelC_a = _mm_set1_ps(0.0f);

			__m128 mTexelD_r = _mm_set1_ps(0.0f);
			__m128 mTexelD_g = _mm_set1_ps(0.0f);
			__m128 mTexelD_b = _mm_set1_ps(0.0f);
			__m128 mTexelD_a = _mm_set1_ps(0.0f);

			__m128 mDest_r = _mm_set1_ps(0.0f);
			__m128 mDest_g = _mm_set1_ps(0.0f);
			__m128 mDest_b = _mm_set1_ps(0.0f);
			__m128 mDest_a = _mm_set1_ps(0.0f);

			__m128 mSampledResult_r = _mm_set1_ps(0.0f);
			__m128 mSampledResult_g = _mm_set1_ps(0.0f);
			__m128 mSampledResult_b = _mm_set1_ps(0.0f);
			__m128 mSampledResult_a = _mm_set1_ps(0.0f);

#if 0
			real32 P_x = X;
			real32 P_y = Y;

			real32 Diff_x = P_x - Origin.x;
			real32 Diff_y = P_y - Origin.y;

			real32 U = Diff_x * PremultipliedXAxis_x + Diff_y * PremultipliedXAxis_y;
			real32 V = Diff_x * PremultipliedYAxis_x + Diff_y * PremultipliedYAxis_y;
#else
			__m128 mPixelPosition_x = _mm_set_ps((real32)(X + 3), (real32)(X + 2), (real32)(X + 1), (real32)(X));
			//__m128 mPixelPosition_x = _mm_set_ps((real32)X, (real32)(X + 1), (real32)(X + 2), (real32)(X + 3));
			__m128 mPixelPosition_y = _mm_set1_ps((real32)Y);

			__m128 mDiff_x = _mm_sub_ps(mPixelPosition_x, mOrigin_x);
			__m128 mDiff_y = _mm_sub_ps(mPixelPosition_y, mOrigin_y);

			__m128 mU = _mm_add_ps(_mm_mul_ps(mPremultXAxis_x, mDiff_x), _mm_mul_ps(mPremultXAxis_y, mDiff_y));
			__m128 mV = _mm_add_ps(_mm_mul_ps(mPremultYAxis_x, mDiff_x), _mm_mul_ps(mPremultYAxis_y, mDiff_y));
#endif

			__m128 mFractionalX = _mm_set1_ps(0.0f);
			__m128 mFractionalY = _mm_set1_ps(0.0f);


			for (int i = 0; i < 4; i++){

				ShouldFill[i] =
					M(mU, i) >= 0.0f &&
					M(mU, i) <= 1.0f &&
					M(mV, i) >= 0.0f &&
					M(mV, i) <= 1.0f;

				if (ShouldFill[i])
				{
					real32 TextureSpaceXReal = M(mU, i) * (real32)(Texture->Width);
					real32 TextureSpaceYReal = M(mV, i) * (real32)(Texture->Height);

					int32 TextureSpaceX = (int32)TextureSpaceXReal;
					int32 TextureSpaceY = (int32)TextureSpaceYReal;

					M(mFractionalX, i) = TextureSpaceXReal - (real32)TextureSpaceX;
					M(mFractionalY, i) = TextureSpaceYReal - (real32)TextureSpaceY;

					uint32* Source = (uint32*)Texture->Memory + TextureSpaceY * Texture->Width + TextureSpaceX;

					/*Loading texels*/
					uint32* TexelAPtr = Source;
					uint32* TexelBPtr = Source + 1;
					uint32* TexelCPtr = Source + Texture->Width;
					uint32* TexelDPtr = Source + Texture->Width + 1;

					M(mTexelA_r, i) = (real32)((*TexelAPtr >> RedShift) & 0xFF);
					M(mTexelA_g, i) = (real32)((*TexelAPtr >> GreenShift) & 0xFF);
					M(mTexelA_b, i) = (real32)((*TexelAPtr >> BlueShift) & 0xFF);
					M(mTexelA_a, i) = (real32)((*TexelAPtr >> AlphaShift) & 0xFF);

					M(mTexelB_r, i) = (real32)((*TexelBPtr >> RedShift) & 0xFF);
					M(mTexelB_g, i) = (real32)((*TexelBPtr >> GreenShift) & 0xFF);
					M(mTexelB_b, i) = (real32)((*TexelBPtr >> BlueShift) & 0xFF);
					M(mTexelB_a, i) = (real32)((*TexelBPtr >> AlphaShift) & 0xFF);

					M(mTexelC_r, i) = (real32)((*TexelCPtr >> RedShift) & 0xFF);
					M(mTexelC_g, i) = (real32)((*TexelCPtr >> GreenShift) & 0xFF);
					M(mTexelC_b, i) = (real32)((*TexelCPtr >> BlueShift) & 0xFF);
					M(mTexelC_a, i) = (real32)((*TexelCPtr >> AlphaShift) & 0xFF);

					M(mTexelD_r, i) = (real32)((*TexelDPtr >> RedShift) & 0xFF);
					M(mTexelD_g, i) = (real32)((*TexelDPtr >> GreenShift) & 0xFF);
					M(mTexelD_b, i) = (real32)((*TexelDPtr >> BlueShift) & 0xFF);
					M(mTexelD_a, i) = (real32)((*TexelDPtr >> AlphaShift) & 0xFF);

					/*Destination loading*/
					M(mDest_r, i) = (real32)((*(DestPixel + i) >> 16) & 0xFF);
					M(mDest_g, i) = (real32)((*(DestPixel + i) >> 8) & 0xFF);
					M(mDest_b, i) = (real32)((*(DestPixel + i) >> 0) & 0xFF);
					M(mDest_a, i) = (real32)((*(DestPixel + i) >> 24) & 0xFF);
				}
			}

			/*Converting texels from sRGB 255 space to linear 0-1 space*/
			//TODO(Dima): Do those without Temp
			__m128 mTempTexelA_r = _mm_mul_ps(mOneOver255, mTexelA_r);
			__m128 mTempTexelA_g = _mm_mul_ps(mOneOver255, mTexelA_g);
			__m128 mTempTexelA_b = _mm_mul_ps(mOneOver255, mTexelA_b);
			mTexelA_r = mmSquare(mTempTexelA_r);
			mTexelA_g = mmSquare(mTempTexelA_g);
			mTexelA_b = mmSquare(mTempTexelA_b);
			mTexelA_a = _mm_mul_ps(mOneOver255, mTexelA_a);

			__m128 mTempTexelB_r = _mm_mul_ps(mOneOver255, mTexelB_r);
			__m128 mTempTexelB_g = _mm_mul_ps(mOneOver255, mTexelB_g);
			__m128 mTempTexelB_b = _mm_mul_ps(mOneOver255, mTexelB_b);
			mTexelB_r = mmSquare(mTempTexelB_r);
			mTexelB_g = mmSquare(mTempTexelB_g);
			mTexelB_b = mmSquare(mTempTexelB_b);
			mTexelB_a = _mm_mul_ps(mOneOver255, mTexelB_a);

			__m128 mTempTexelC_r = _mm_mul_ps(mOneOver255, mTexelC_r);
			__m128 mTempTexelC_g = _mm_mul_ps(mOneOver255, mTexelC_g);
			__m128 mTempTexelC_b = _mm_mul_ps(mOneOver255, mTexelC_b);
			mTexelC_r = mmSquare(mTempTexelC_r);
			mTexelC_g = mmSquare(mTempTexelC_g);
			mTexelC_b = mmSquare(mTempTexelC_b);
			mTexelC_a = _mm_mul_ps(mOneOver255, mTexelC_a);

			__m128 mTempTexelD_r = _mm_mul_ps(mOneOver255, mTexelD_r);
			__m128 mTempTexelD_g = _mm_mul_ps(mOneOver255, mTexelD_g);
			__m128 mTempTexelD_b = _mm_mul_ps(mOneOver255, mTexelD_b);
			mTexelD_r = mmSquare(mTempTexelD_r);
			mTexelD_g = mmSquare(mTempTexelD_g);
			mTexelD_b = mmSquare(mTempTexelD_b);
			mTexelD_a = _mm_mul_ps(mOneOver255, mTexelD_a);

			/*Bilinear texture blend*/
#if 1
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

			mSampledResult_r = _mm_add_ps(_mm_mul_ps(mTempBlend1_r, mInvFractionalY), _mm_mul_ps(mTempBlend2_r, mFractionalY));
			mSampledResult_g = _mm_add_ps(_mm_mul_ps(mTempBlend1_g, mInvFractionalY), _mm_mul_ps(mTempBlend2_g, mFractionalY));
			mSampledResult_b = _mm_add_ps(_mm_mul_ps(mTempBlend1_b, mInvFractionalY), _mm_mul_ps(mTempBlend2_b, mFractionalY));
			mSampledResult_a = _mm_add_ps(_mm_mul_ps(mTempBlend1_a, mInvFractionalY), _mm_mul_ps(mTempBlend2_a, mFractionalY));
#else
			__m128 mTempBlend1_r = _mm_add_ps(mTexelA_r, _mm_mul_ps(_mm_sub_ps(mTexelB_r, mTexelA_r), mFractionalX));
			__m128 mTempBlend1_g = _mm_add_ps(mTexelA_g, _mm_mul_ps(_mm_sub_ps(mTexelB_g, mTexelA_g), mFractionalX));
			__m128 mTempBlend1_b = _mm_add_ps(mTexelA_b, _mm_mul_ps(_mm_sub_ps(mTexelB_b, mTexelA_b), mFractionalX));
			__m128 mTempBlend1_a = _mm_add_ps(mTexelA_a, _mm_mul_ps(_mm_sub_ps(mTexelB_a, mTexelA_a), mFractionalX));

			__m128 mTempBlend2_r = _mm_add_ps(mTexelC_r, _mm_mul_ps(_mm_sub_ps(mTexelD_r, mTexelC_r), mFractionalX));
			__m128 mTempBlend2_g = _mm_add_ps(mTexelC_g, _mm_mul_ps(_mm_sub_ps(mTexelD_g, mTexelC_g), mFractionalX));
			__m128 mTempBlend2_b = _mm_add_ps(mTexelC_b, _mm_mul_ps(_mm_sub_ps(mTexelD_b, mTexelC_b), mFractionalX));
			__m128 mTempBlend2_a = _mm_add_ps(mTexelC_a, _mm_mul_ps(_mm_sub_ps(mTexelD_a, mTexelC_a), mFractionalX));

			mSampledResult_r = _mm_add_ps(mTempBlend1_r, _mm_mul_ps(_mm_sub_ps(mTempBlend2_r, mTempBlend1_r), mFractionalY));
			mSampledResult_g = _mm_add_ps(mTempBlend1_g, _mm_mul_ps(_mm_sub_ps(mTempBlend2_g, mTempBlend1_g), mFractionalY));
			mSampledResult_b = _mm_add_ps(mTempBlend1_b, _mm_mul_ps(_mm_sub_ps(mTempBlend2_b, mTempBlend1_b), mFractionalY));
			mSampledResult_a = _mm_add_ps(mTempBlend1_a, _mm_mul_ps(_mm_sub_ps(mTempBlend2_a, mTempBlend1_a), mFractionalY));
#endif

			/*Multiplying sampled color with our incoming color*/
			mSampledResult_r = _mm_mul_ps(mSampledResult_r, mColor_r);
			mSampledResult_g = _mm_mul_ps(mSampledResult_g, mColor_g);
			mSampledResult_b = _mm_mul_ps(mSampledResult_b, mColor_b);
			mSampledResult_a = _mm_mul_ps(mSampledResult_a, mColor_a);

			/*Clamping resulted color between 0 and 1*/
			mSampledResult_r = _mm_min_ps(_mm_max_ps(mSampledResult_r, mZero), mOne);
			mSampledResult_g = _mm_min_ps(_mm_max_ps(mSampledResult_g, mZero), mOne);
			mSampledResult_b = _mm_min_ps(_mm_max_ps(mSampledResult_b, mZero), mOne);

			// /*Loading destination color*/
			// __m128i mOriginalDest = _mm_load_si128((__m128i*)DestPixel);;
			// __m128 mDest_r = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mOriginalDest, 16), mMaskFF));
			// __m128 mDest_g = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mOriginalDest, 8), mMaskFF));
			// __m128 mDest_b = _mm_cvtepi32_ps(_mm_and_si128(mOriginalDest, mMaskFF));
			// __m128 mDest_a = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mOriginalDest, 24), mMaskFF));

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


#if 0
			/*Storing the result value*/
			for (int i = 0; i < 4; i++){
				if (ShouldFill[i]){
					*(DestPixel + i) = ((uint32)(M(mResultColor_a, i) + 0.5f) << 24) |
						((uint32)(M(mResultColor_r, i) + 0.5f) << 16) |
						((uint32)(M(mResultColor_g, i) + 0.5f) << 8) |
						((uint32)(M(mResultColor_b, i) + 0.5f) << 0);
				}
			}
#else
			__m128i mResultColor_r_epi32 = _mm_cvtps_epi32(mResultColor_r);
			__m128i mResultColor_g_epi32 = _mm_cvtps_epi32(mResultColor_g);
			__m128i mResultColor_b_epi32 = _mm_cvtps_epi32(mResultColor_b);
			__m128i mResultColor_a_epi32 = _mm_cvtps_epi32(mResultColor_a);

			__m128i mResColorShifted_r = _mm_slli_epi32(mResultColor_r_epi32, 16);
			__m128i mResColorShifted_g = _mm_slli_epi32(mResultColor_g_epi32, 8);
			__m128i mResColorShifted_b = mResultColor_b_epi32;
			__m128i mResColorShifted_a = _mm_slli_epi32(mResultColor_a_epi32, 24);

			__m128i mOut = _mm_or_si128(
				_mm_or_si128(mResColorShifted_a, mResColorShifted_r),
				_mm_or_si128(mResColorShifted_g, mResColorShifted_b));

			_mm_storeu_si128((__m128i*)DestPixel, mOut);
#endif
			DestPixel += 4;
		}
		Row += Buffer->Width;
	}
}


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
}

inline void RenderGroupToOutput(render_group* RenderGroup, loaded_bitmap* OutputTarget){

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
					EntryClear->Color);
				Base += sizeof(*EntryClear);
			}break;
			case RenderGroupEntry_render_entry_rectangle:{
				render_entry_rectangle* EntryRect = (render_entry_rectangle*)EntryData;

				RenderRectangle(
					OutputTarget,
					EntryRect->EntityBasis.Offset.xy,
					EntryRect->EntityBasis.Offset.xy + EntryRect->Dim,
					EntryRect->Color);
					
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

				RenderRectangleSlowly(
					OutputTarget,
					EntryBitmap->EntityBasis.Offset.xy,
					XAxis * EntryBitmap->Bitmap->Width,
					YAxis * EntryBitmap->Bitmap->Height,
					EntryBitmap->Bitmap,
					EntryBitmap->Color);
#endif


				Base += sizeof(*EntryBitmap);
			}break;
			case RenderGroupEntry_render_entry_coordinate_system:{
				render_entry_coordinate_system* EntryCS = (render_entry_coordinate_system*)EntryData;

				gdVec2 RectDim = { 2, 2 };
				RenderRectangleQuickly(OutputTarget, EntryCS->Origin, EntryCS->XAxis, EntryCS->YAxis, EntryCS->Texture, EntryCS->Color);
				gdVec2 P = EntryCS->Origin;
				RenderRectangle(OutputTarget, P - RectDim, P + RectDim, gd_vec4(1.0f, 1.0f, 0.0f, 1.0f));
				P = EntryCS->Origin + EntryCS->XAxis;
				RenderRectangle(OutputTarget, P - RectDim, P + RectDim, gd_vec4(1.0f, 1.0f, 0.0f, 1.0f));
				P = EntryCS->Origin + EntryCS->YAxis;
				RenderRectangle(OutputTarget, P - RectDim, P + RectDim, gd_vec4(1.0f, 1.0f, 0.0f, 1.0f));
				P = EntryCS->Origin + EntryCS->YAxis + EntryCS->XAxis;
				RenderRectangle(OutputTarget, P - RectDim, P + RectDim, gd_vec4(1.0f, 1.0f, 0.0f, 1.0f));

				Base += sizeof(*EntryCS);
			}break;
			
			INVALID_DEFAULT_CASE;
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
	gdVec2 Align,
	gdVec4 Color)
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
	gdVec4 Color = gd_vec4(1.0f, 0.0f, 0.0f, 1.0f),
	real32 Thickness = 4)
{
	PushRectangle(RenderGroup, Offset - gd_vec3(Thickness, 0.0f, 0.0f), gd_vec2(Thickness, Dim.y), Color);
	PushRectangle(RenderGroup, Offset - gd_vec3(0.0f, Thickness, 0.0f), gd_vec2(Dim.x, Thickness), Color);
	PushRectangle(RenderGroup, Offset + gd_vec3(0.0f, Dim.y, 0.0f), gd_vec2(Dim.x, Thickness), Color);
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
