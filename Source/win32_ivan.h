#ifndef WIN32_IVAN_H
#define WIN32_IVAN_H

#include "ivan.h"

struct win32_offscreen_buffer{
	void* Memory;
	int Width;
	int Height;
	int Pitch;
	BITMAPINFO BitmapInfo;
	HWND Window;
	bool32 IsFullscreen;
	int BytesPerPixel;
};

struct win32_window_dimension{
	int Width;
	int Height;
};

struct win32_sound_output{
	DWORD SecondaryBufferByteSize;
	DWORD BytesPerSecond;
	WORD SamplesPerSecond;
	WORD Channels;
	WORD BytesPerSample;
	WORD BlockSize;
	DWORD RunningSample;
	DWORD LatencyCycleCount;

	int32 ToneHz;
	int32 ToneVolume;
};

struct win32_sound_input{
	LPDIRECTSOUNDCAPTUREBUFFER Buffer;
	DWORD BufferByteSize;
	DWORD BytesPerSecond;
	WORD SamplesPerSecond;
	WORD Channels;
	WORD BytesPerSample;
	WORD BlockSize;
	DWORD RunningSample;
};

struct platform_work_queue_entry{
    platform_work_queue_callback* Callback;
    void* Data;
};

struct platform_work_queue{
    uint32 volatile EntryCount;
    uint32 volatile FinishedEntries;
    uint32 volatile NextEntryToWrite;
    uint32 volatile NextEntryToRead;
    HANDLE SemaphoreHandle;
    platform_work_queue_entry Entries[256];
};

struct win32_thread_startup{
	platform_work_queue* Queue;
};

#endif