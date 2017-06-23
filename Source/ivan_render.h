#ifndef IVAN_RENDER_H
#define IVAN_RENDER_H

enum allocate_texture_flags{
    AllocateTexture_FilterNearest = 0x1,
};


struct texture_op_allocate{
	uint32 Width;
	uint32 Height;
	uint32 Flags;
	void* Data;

	void** ResultHandle;
};

struct texture_op_deallocate{
	void* Handle;
};

struct texture_op{
	texture_op* Next;
	bool32 IsAllocate;
	union{
		texture_op_allocate Allocate;
		texture_op_deallocate Deallocate;
	};
};

#endif