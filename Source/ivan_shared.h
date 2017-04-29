#ifndef IVAN_SHARED_H
#define IVAN_SHARED_H

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
StringsAreEqual(uintptr ALength, char* A, char* B){
	bool32 Result = false;

	if(B){
		char* At = B;
		for(uintptr Index = 0;
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

#endif