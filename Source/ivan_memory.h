#ifndef IVAN_MEMORY_H
#define IVAN_MEMORY_H

struct memory_arena{
	size_t MemoryUsed;
	void* BaseAddress;
	size_t MemorySize;
	int TempCount;
};

struct temporary_memory{
	memory_arena* Arena;
	size_t Used;
};

inline temporary_memory
BeginTemporaryMemory(memory_arena* Arena){
	temporary_memory Result;

	Result.Arena = Arena;
	Result.Used = Arena->MemoryUsed;
	++Arena->TempCount;

	return(Result);
}

inline void EndTemporaryMemory(temporary_memory TempMem){
	memory_arena* Arena = TempMem.Arena;
	Assert(Arena->MemoryUsed >= TempMem.Used);
	Arena->MemoryUsed = TempMem.Used;
	Assert(Arena->TempCount > 0);
	--Arena->TempCount;
}


inline size_t
GetAlignmentOffset(memory_arena* Arena, size_t Alignment){
	size_t AlignmentOffset = 0;
	size_t ResultPointer = (size_t)Arena->BaseAddress + Arena->MemoryUsed;
	size_t AlignmentMask = Alignment - 1;
	if (ResultPointer & AlignmentMask){
		AlignmentOffset = Alignment - (ResultPointer & AlignmentMask);
	}
	return(AlignmentOffset);
}

#define PushStruct(Arena, Type, ...) (Type*)PushSize_(Arena, sizeof(Type), ## __VA_ARGS__)
#define PushArray(Arena, Count , Type, ...) (Type*)PushSize_(Arena, (Count)*sizeof(Type), ## __VA_ARGS__)
#define PushSize(Arena, Size, ...) PushSize_(Arena, Size, ## __VA_ARGS__)

inline void* PushSize_(memory_arena* Arena, size_t SizeInit, size_t Alignment = 4){
	size_t Size = SizeInit;
	size_t AlignmentOffset = GetAlignmentOffset(Arena, Alignment);
	Size += AlignmentOffset;

	Assert(Arena->MemoryUsed + Size <= Arena->MemorySize);
	void* Result = (uint8*)Arena->BaseAddress + Arena->MemoryUsed + AlignmentOffset;
	Arena->MemoryUsed += Size;

	Assert(Size >= SizeInit);

	return(Result);
}

inline char* 
PushString(memory_arena* Arena, char* Source){
    uint32 BufferSize = 1;
    for(char* It = Source;
        *It;
        It++)
    {
        BufferSize++;
    }

    char* Dest = (char*)PushSize_(Arena, BufferSize);
    for(uint32 CharIndex = 0;
        CharIndex < BufferSize;
        CharIndex++)
    {
        Dest[CharIndex] = Source[CharIndex];
    }

    return(Dest);
}

#define ZeroStruct(Loc) ZeroSize(&(Loc), sizeof(Loc))
inline void ZeroSize(void* Memory, size_t Size){
	uint8* Ptr = (uint8*)Memory;
	for (int i = 0; i < Size; i++){
		*Ptr++ = 0;
	}
}


inline void SubArena(memory_arena* Result, memory_arena* Arena, size_t Size, size_t Alignment = 16){
	Result->MemorySize = Size;
	Result->BaseAddress = (uint8*)PushSize_(Arena, Size, Alignment);
	Result->MemoryUsed = 0;
	Result->TempCount = 0;
}

inline void InitializeMemoryArena(memory_arena* Arena, size_t Size, void* BaseAddress){
	Arena->BaseAddress = BaseAddress;
	Arena->MemoryUsed = 0;
	Arena->MemorySize = Size;
}

#endif`