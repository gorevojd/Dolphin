#ifndef DOLHPIN_RENDER_MATH_H
#define DOLHPIN_RENDER_MATH_H

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

#endif