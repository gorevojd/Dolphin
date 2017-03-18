#ifndef DOLHPIN_RENDER_MATH_H
#define DOLHPIN_RENDER_MATH_H

#define MY_TEMP_SQUARE(value) ((value) * (value))
inline vec4 SRGB255ToLinear1(vec4 v){
	vec4 Result;
	real32 OneOver255 = 1.0f / 255.0f;

	Result.x = MY_TEMP_SQUARE(OneOver255 * v.x);
	Result.y = MY_TEMP_SQUARE(OneOver255 * v.y);
	Result.z = MY_TEMP_SQUARE(OneOver255 * v.z);
	Result.w = OneOver255 * v.w;

	return(Result);
}

inline vec4 Linear1ToSRGB255(vec4 v){
	vec4 Result;

	Result.x = 255.0f * Sqrt(v.x);
	Result.y = 255.0f * Sqrt(v.y);
	Result.z = 255.0f * Sqrt(v.z);
	Result.w = 255.0f * v.w;

	return(Result);
}

inline float GetFloatRepresentOfColor(vec3 Color){
	float Result = 0;

	vec4 TempV = Linear1ToSRGB255(Vec4(Color, 0.0f));

	uint32 Temp = 
		((uint32)(TempV.r) << 16) 	|
		((uint32)(TempV.g) << 8) 	|
		((uint32)(TempV.b) << 0);

	Result = (float)Temp;

	return(Result);
}

#endif