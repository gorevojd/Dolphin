#ifndef WIN32_DOLPHIN_H
#define WIN32_DOLPHIN_H

#include "dolphin.h"

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
#endif