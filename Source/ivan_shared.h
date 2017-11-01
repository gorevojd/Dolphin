#ifndef IVAN_SHARED_H
#define IVAN_SHARED_H

#include <stdarg.h>

inline bool32 
IsEndOfLine(char C){
	bool32 Result = 
		((C == '\n') || 
		(C == '\r'));
	
	return(Result);
}

inline bool32 
IsWhitespace(char C){
	bool32 Result = 
		((C == ' ') ||
		(C == '\t') || 
		(C == '\v') ||
		(C == '\f') || 
		IsEndOfLine(C));

	return(Result);
}

inline uint32 StringLength(char* String){
	uint32 Result = 0;
	if(String){
		while(*String){
			++Result;

			*String++;
		}
	}

	return(Result);
}

inline bool32
StringsAreEqual(char* A, char* B){
	bool32 Result = (A == B);

	if(A && B){
		while(*A && (*A == *B)){
			++A;
			++B;
		}

		Result = ((*A == 0) && (*B == 0));
	}

	return(Result);
}

inline bool32
StringsAreEqual(umm ALength, char* A, char* B){
	bool32 Result = false;

	if(B){
		char* At = B;
		for(umm Index = 0;
			Index < ALength;
			++Index, ++At)
		{
			if((*At == 0) || (A[Index] != *At)){
				return(false);
			}
		}

		Result = (*At == 0);
	}
	else{
		Result = (ALength == 0);
	}

	return(Result);
}

INTERNAL_FUNCTION int
IntFromStringInternal(char** AtInit){
	int Result = 0;

	char* At = *AtInit;
	while((*At >= '0') &&
		(*At <= '9'))
	{
		Result *= 10;
		Result += (*At - '0');
		++At;
	}

	*AtInit = At;

	return(Result);
}

INTERNAL_FUNCTION int 
IntFromString(char* At){
	char* Ignored = At;
	int Result = IntFromStringInternal(&At);
	return(Result);
}

struct dest_format_context {
	umm Size;
	char* At;
};

inline void OutputChar(dest_format_context* Context, char ToOutput) {
	if (Context->Size) {
		--Context->Size;
		*Context->At++ = ToOutput;
	}
}

INTERNAL_FUNCTION void OutputChars(dest_format_context* Context, char* ToOutput, umm Count){
	char* At = ToOutput;
	
	for (int Index = 0;
		Index < Count;
		Index++)
	{
		OutputChar(Context, *At);
		++At;
	}
}

INTERNAL_FUNCTION void OutputChars(dest_format_context* Context, char* ToOutput) {
	char* At = ToOutput;
	while (*At) {
		OutputChar(Context, *At);
		++At;
	}
}

char DecimalChars[] = "0123456789";
char LowerHexChars[] = "0123456789abcdef";
char UpperHexChars[] = "0123456789ABCDEF";

INTERNAL_FUNCTION void U64ToASCII(dest_format_context* Context, u64 Value, u32 Base, char* Digits) {
	Assert(Base != 0);

	char* Start = Context->At;
	do {
		u64 DigitIndex = (Value % Base);
		char Digit = Digits[DigitIndex];
		OutputChar(Context, Digit);

		Value /= Base;
	} while (Value != 0);

	char* End = Context->At;

	while (Start < End) {
		--End;
		char Temp = *End;
		*End = *Start;
		*Start = Temp;
		++Start;
	}
}

INTERNAL_FUNCTION void F64ToASCII(dest_format_context* Context, f64 Value, int Precision) {
	if (Value < 0) {
		OutputChar(Context, '-');
		Value = -Value;
	}

	u64 IntegerPart = (u64)Value;
	Value -= (f64)IntegerPart;
	U64ToASCII(Context, IntegerPart, 10, DecimalChars);

	OutputChar(Context, '.');

	for (u32 PrecisionIndex = 0;
		PrecisionIndex < Precision;
		++PrecisionIndex)
	{
		Value *= 10.0f;
		u32 IntPart = (u32)Value;
		Value -= (f32)IntPart;
		OutputChar(Context, DecimalChars[IntPart]);
	}
}

#define READ_VARARG_UNSIGNED_INTEGER(Length, ArgList) ((Length) == 8) ? va_arg(ArgList, u64) : (u64)va_arg(ArgList, u32)
#define READ_VARARG_SIGNED_INTEGER(Length, ArgList) ((Length) == 8) ? va_arg(ArgList, s64) : (s64)va_arg(ArgList, s32)
#define READ_VARARG_FLOAT(Length, ArgList) va_arg(ArgList, f64)


INTERNAL_FUNCTION umm PrintFormattedArgList(umm DestSize, char* Dest, char* FormatString, va_list ArgList) {
	dest_format_context FormatContext = { DestSize, Dest };

	if (FormatContext.Size) {
		char* At = FormatString;

		while (*At) {
			if (*At == '%') {
				++At;

				b32 LeftJustify = false;
				b32 NeedsSign = false;
				b32 BlankSpaceSign = false;
				b32 PadWithZeros = false;
				b32 SharpSymbolFlag = false;

				/*Flags handling*/
				b32 Parsing = true;
#define FORMATTO_HANDLE_FLAGS_IN_LOOP 0
#if FORMATTO_HANDLE_FLAGS_IN_LOOP
				while (Parsing) {
#endif
					switch (*At) {
						case '-': {
							LeftJustify = true;
						}break;

						case '+': {
							NeedsSign = true;
						}break;

						case ' ': {
							BlankSpaceSign = true;
						}break;

						case '#': {
							SharpSymbolFlag = true;
						}break;

						case '0': {
							PadWithZeros = true;
						}break;

						default: {
							Parsing = false;
						}break;
					}
					if (Parsing) {
						++At;
					}
#if FORMATTO_HANDLE_FLAGS_IN_LOOP
				}
#endif
				
				/*Width handling*/
				b32 WidthSpecified = false;
				int Width = 0;
				if (*At >= '0' && *At <= '9') {
					Width = IntFromStringInternal(&At);
					WidthSpecified = true;
				}
				else if (*At == '*') {
					Width = va_arg(ArgList, int);
					WidthSpecified = true;
					++At;
				}

				/*Precision handling*/
				b32 PrecisionSpecified = false;
				//NOTE(DIMA): Set precesion to 3 by default
				int Precision = 6;
				if (*At == '.') {
					++At;

					if (*At >= '0' && *At <= '9') {
						Precision = IntFromStringInternal(&At);
						PrecisionSpecified = true;
					}
					else if (*At == '*') {
						Precision = va_arg(ArgList, int);
						PrecisionSpecified = true;
						++At;
					}
					else {
						Assert(!"Unhandled precision specifier");
					}
				}

				/*Length handling*/
				int IntegerLen = 4;
				int FloatLen = 8;
				int CharLen = 4;
				int SLen = sizeof(char*);
				int PLen = sizeof(void*);
				int NLen = sizeof(int*);

				size_t PointerSize = sizeof(void*);
				size_t CharSize = sizeof(signed char);
				size_t ShortSize = sizeof(signed short);
				size_t IntSize = sizeof(signed int);
				size_t LongSize = sizeof(long int);
				size_t LongLongIntSize = sizeof(long long int);
				size_t IntMaxTSize = sizeof(long long);
				size_t SizeTSize = sizeof(size_t);
				size_t PtrdiffSize = sizeof(ptrdiff_t);

				if (At[0] == 'h' && At[1] == 'h') {
					IntegerLen = ShortSize;
					At += 2;
				}
				else if(At[0] == 'h'){
					IntegerLen = ShortSize;
					++At;
				}
				else if (At[0] == 'l') {
					IntegerLen = LongSize;
					CharLen = ShortSize;
					++At;
				}
				else if (At[0] == 'l' && At[1] == 'l') {
					IntegerLen = LongLongIntSize;
					At += 2;
				}
				else if (At[0] == 'j') {
					IntegerLen = IntMaxTSize;
					++At;
				}
				else if (At[0] == 'z') {
					IntegerLen = SizeTSize;
					++At;
				}
				else if (At[0] == 't') {
					IntegerLen = PtrdiffSize;
					++At;
				}
				else if (At[0] == 'L') {
					FloatLen = 8;
					++At;
				}

				char TempBuffer[64];
				char* Temp = TempBuffer;
				dest_format_context TempContext = { ArrayCount(TempBuffer), Temp };
				b32 IsFloat = false;
				char* Prefix = "";


				switch (*At) {
					case 'd':
					case 'i':{
						s64 Value = READ_VARARG_SIGNED_INTEGER(IntegerLen, ArgList);
						b32 IsNegative = (Value < 0);
						if (IsNegative) {
							Value = -Value;
						}
						U64ToASCII(&TempContext, (u64)Value, 10, DecimalChars);

						if (IsNegative) {
							Prefix = "-";
						}
						else if (NeedsSign) {
							Assert(!BlankSpaceSign);
							Prefix = "+";
						}
						else if (BlankSpaceSign) {
							Prefix = " ";
						}
					}break;

					case 'u':{
						u64 Value = READ_VARARG_UNSIGNED_INTEGER(IntegerLen, ArgList);
						U64ToASCII(&TempContext, Value, 10, DecimalChars);
					}break;

					case 'o':{
						u64 Value = READ_VARARG_UNSIGNED_INTEGER(IntegerLen, ArgList);
						U64ToASCII(&TempContext, Value, 8, DecimalChars);
						if (SharpSymbolFlag && (Value != 0)) {
							Prefix = "0";
						}
					}break;

					case 'x':{
						u64 Value = READ_VARARG_UNSIGNED_INTEGER(IntegerLen, ArgList);
						U64ToASCII(&TempContext, Value, 16, LowerHexChars);
						if (SharpSymbolFlag && (Value != 0)) {
							Prefix = "0x";
						}
					}break;
					
					case 'X':{
						u64 Value = READ_VARARG_UNSIGNED_INTEGER(IntegerLen, ArgList);
						U64ToASCII(&TempContext, Value, 16, UpperHexChars);
						if (SharpSymbolFlag && (Value != 0)) {
							Prefix = "0x";
						}
					}break;
					
					case 'f':
					case 'F':
					case 'e':
					case 'E':
					case 'g':
					case 'G':
					case 'a':
					case 'A':{
						f64 Value = READ_VARARG_FLOAT(FloatLen, ArgList);
						F64ToASCII(&TempContext, Value, Precision);
						IsFloat = true;
					}break;

					case 'c':{
						int Value = va_arg(ArgList, int);
						OutputChar(&TempContext, (char)Value);
					}break;

					case 's':{
						char* StringParam = va_arg(ArgList, char*);

						Temp = StringParam;
						if (PrecisionSpecified) {
							TempContext.Size = 0;
							for (char* Cursor = StringParam;
								*Cursor && (TempContext.Size < Precision);
								++Cursor)
							{
								++TempContext.Size;
							}
						}
						else {
							TempContext.Size = StringLength(StringParam);
						}

						TempContext.At = StringParam + TempContext.Size;
					}break;
					
					case 'p':{
						void* Value = va_arg(ArgList, void*);
						U64ToASCII(&TempContext, *(umm*)&Value, 16, LowerHexChars);
					}break;
					
					case 'n':{
						int* TabDest = va_arg(ArgList, int*);
						*TabDest = (int)(FormatContext.At - Dest);
					}break;
					case '%': {
						OutputChar(&FormatContext, '%');
					}break;

					default: {
						Assert(!"Unknown format specifier");
					}break;
				}

				if (Temp < TempContext.At) {					
					smm UsePrecision = Precision;
					if (IsFloat || !PrecisionSpecified) {
						UsePrecision = (TempContext.At - Temp);
					}

					smm PrefixLen = StringLength(Prefix);
					smm UseWidth = Width;
					smm ComputedWidth = UsePrecision + PrefixLen;
					if (UseWidth < ComputedWidth) {
						UseWidth = ComputedWidth;
					}

					if (PadWithZeros) {
						Assert(!LeftJustify);
						LeftJustify = false;
					}

					if (!LeftJustify) {
						while (UseWidth > (UsePrecision + PrefixLen)) {
							OutputChar(&FormatContext, PadWithZeros ? '0' : ' ');
							--UseWidth;
						}
					}

					for (char* Pre = Prefix;
						*Pre && UseWidth;
						++Pre) 
					{
						OutputChar(&FormatContext, *Pre);
						--UseWidth;
					}

					if (UsePrecision > UseWidth) {
						UsePrecision = UseWidth;
					}

					while (UsePrecision > (TempContext.At - Temp)) {
						OutputChar(&FormatContext, '0');
						--UsePrecision;
						--UseWidth;
					}
					while (UsePrecision && (TempContext.At != Temp)) {
						OutputChar(&FormatContext, *Temp++);
						--UsePrecision;
						--UseWidth;
					}
					
					if (LeftJustify) {
						while (UseWidth) {
							OutputChar(&FormatContext, ' ');
							--UseWidth;
						}
					}
				}

				if (*At) {
					++At;
				}
			}
			else {
				OutputChar(&FormatContext, *At++);
			}
		}

		if (FormatContext.Size) {
			FormatContext.At[0] = 0;
		}
		else {
			FormatContext.At[-1] = 0;
		}
	}

	umm Result = FormatContext.At - Dest;
	return(Result);
}

INTERNAL_FUNCTION umm  PrintFormatto(umm DestSize, char* Dest, char* FormatString, ...) 
{
	va_list ArgList;

	va_start(ArgList, FormatString);
	umm Result = PrintFormattedArgList(DestSize, Dest, FormatString, ArgList);
	va_end(ArgList);

	return(Result);
}

#endif