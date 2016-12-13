#ifndef DOLPHIN_INTRINSICS
#define DOLPHIN_INTRINSICS

#include "math.h"
#include <intrin.h>

inline int32 RoundReal32ToInt32(real32 Value){
	int32 Result = (int32)roundf(Value);
	return(Result);
}

inline uint32
RoundReal32ToUInt32(real32 Value){
	uint32 Result = (uint32)roundf(Value);
	return(Result);
}

inline int32
FloorReal32ToInt32(real32 Value){
	int32 Result = (int32)floorf(Value);
	return(Result);
}

inline int32
CeilReal32ToInt32(real32 Value){
	int32 Result = (int32)ceilf(Value);
	return(Result);
}

inline int32
TruncateReal32ToInt32(real32 Value){
	int32 Result = (int32)Value;
	return(Result);
}

inline real32
Sin(real32 Angle){
	real32 Result = sinf(Angle);
	return(Result);
}

inline real32
Cos(real32 Angle){
	real32 Result = cosf(Angle);
	return(Result);
}

inline real32
ATan2(real32 Y, real32 X){
	real32 Result = atan2f(Y, X);
	return(Result);
}

struct bit_scan_result{
	bool32 Found;
	uint32 Index;
};

inline bit_scan_result
FindLeastSignificantSetBit(uint32 Mask){
	bit_scan_result Result = {};
#if GDA_COMPILER_MSVC
	Result.Found = _BitScanForward((unsigned long*)&Result.Index, Mask);
#else
	for (uint32 i = 0; i < 32; i++){
		if (Mask & (1 << i)){
			Result.Index = i;
			Result.Found = true;
			break;
		}
	}
#endif
	return(Result);
}

#endif