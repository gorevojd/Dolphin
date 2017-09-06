#include "ivan_platform.h"

#include <Windows.h>
#include <stdio.h>
#include <malloc.h>
#include <dsound.h>
#include <Xinput.h>
#include <gl/GL.h>

#include "win32_ivan.h"

#define WGL_DRAW_TO_WINDOW_ARB                  0x2001
#define WGL_ACCELERATION_ARB                    0x2003
#define WGL_SUPPORT_OPENGL_ARB                  0x2010
#define WGL_DOUBLE_BUFFER_ARB                   0x2011
#define WGL_PIXEL_TYPE_ARB                      0x2013

#define WGL_TYPE_RGBA_ARB                       0x202B
#define WGL_FULL_ACCELERATION_ARB               0x2027

#define WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB        0x20A9

#define WGL_RED_BITS_ARB                        0x2015
#define WGL_GREEN_BITS_ARB                      0x2017
#define WGL_BLUE_BITS_ARB                       0x2019
#define WGL_ALPHA_BITS_ARB                      0x201B
#define WGL_DEPTH_BITS_ARB                      0x2022

typedef HGLRC WINAPI wgl_create_context_attribs_arb(
    HDC hdc,
    HGLRC hShareContext, 
    const int *attribList);

typedef BOOL WINAPI wgl_get_pixel_format_attrib_iv_arb(
    HDC hdc,
    int iPixelFormat,
    int iLayerPlane,
    UINT nAttributes,
    const int *piAttributes,
    int *piValues);

typedef BOOL WINAPI wgl_get_pixel_format_attrib_fv_arb(
    HDC hdc,
    int iPixelFormat,
    int iLayerPlane,
    UINT nAttributes,
    const int *piAttributes,
    FLOAT *pfValues);

typedef BOOL WINAPI wgl_choose_pixel_format_arb(HDC hdc,
    const int *piAttribIList,
    const FLOAT *pfAttribFList,
    UINT nMaxFormats,
    int *piFormats,
    UINT *nNumFormats);

typedef BOOL WINAPI wgl_swap_interval_ext(int interval);
typedef const char* WINAPI wgl_get_extensions_string_ext(void);

GLOBAL_VARIABLE wgl_create_context_attribs_arb* wglCreateContextAttribsARB;
GLOBAL_VARIABLE wgl_choose_pixel_format_arb* wglChoosePixelFormatARB;
GLOBAL_VARIABLE wgl_swap_interval_ext* wglSwapIntervalEXT;
GLOBAL_VARIABLE wgl_get_extensions_string_ext* wglGetExtensionsStringEXT;

typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;

typedef void WINAPI type_glTexImage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);
typedef void WINAPI type_glBindFramebuffer(GLenum target, GLuint framebuffer);
typedef void WINAPI type_glGenFramebuffers(GLsizei n, GLuint *framebuffers);
typedef void WINAPI type_glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef GLenum WINAPI type_glCheckFramebufferStatus(GLenum target);
typedef void WINAPI type_glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
typedef void WINAPI type_glAttachShader(GLuint program, GLuint shader);
typedef void WINAPI type_glCompileShader(GLuint shader);
typedef GLuint WINAPI type_glCreateProgram(void);
typedef GLuint WINAPI type_glCreateShader(GLenum type);
typedef void WINAPI type_glLinkProgram(GLuint program);
typedef void WINAPI type_glShaderSource(GLuint shader, GLsizei count, GLchar **string, GLint *length);
typedef void WINAPI type_glUseProgram(GLuint program);
typedef void WINAPI type_glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void WINAPI type_glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void WINAPI type_glValidateProgram(GLuint program);
typedef void WINAPI type_glGetProgramiv(GLuint program, GLenum pname, GLint *params);
typedef void WINAPI type_glGetShaderiv(GLuint shader, GLenum pname, GLint *params);
typedef GLint WINAPI type_glGetUniformLocation (GLuint program, const GLchar *name);
typedef void WINAPI type_glUniform4fv(GLint location, GLsizei count, const GLfloat *value);
typedef void WINAPI type_glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void WINAPI type_glUniform1i(GLint location, GLint v0);
typedef void WINAPI type_glUniform1f(GLint location, GLfloat v0);
typedef void WINAPI type_glUniform2fv(GLint location, GLsizei count, const GLfloat *value);
typedef void WINAPI type_glUniform3fv(GLint location, GLsizei count, const GLfloat *value);
typedef void WINAPI type_glEnableVertexAttribArray(GLuint index);
typedef void WINAPI type_glDisableVertexAttribArray(GLuint index);
typedef GLint WINAPI type_glGetAttribLocation(GLuint program, const GLchar *name);
typedef void WINAPI type_glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
typedef void WINAPI type_glBindVertexArray(GLuint array);
typedef void WINAPI type_glGenVertexArrays(GLsizei n, GLuint *arrays);
typedef void WINAPI type_glBindBuffer (GLenum target, GLuint buffer);
typedef void WINAPI type_glGenBuffers (GLsizei n, GLuint *buffers);
typedef void WINAPI type_glBufferData (GLenum target, GLsizeiptr size, const void *data, GLenum usage);
typedef void WINAPI type_glActiveTexture(GLenum texture);
typedef void WINAPI type_glDeleteShader(GLuint shader);
typedef void WINAPI type_glDeleteProgram(GLuint program);
typedef void WINAPI type_glDeleteFramebuffers(GLsizei n, const GLuint* framebuffers);
typedef const GLubyte * WINAPI type_glGetStringi(GLenum name, GLuint index);
typedef void WINAPI type_glUniform2f(GLint location, GLfloat v0, GLfloat v1);
typedef void WINAPI type_glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void WINAPI type_glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void WINAPI type_glDeleteBuffers(GLsizei n, const GLuint * buffers);
typedef void WINAPI type_glDeleteVertexArrays(GLsizei n, const GLuint *arrays);
typedef void WINAPI type_glGenerateMipmap(GLenum target);
typedef void WINAPI type_glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid * data);

#define GL_DEBUG_CALLBACK(Name) void WINAPI Name(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,const void *userParam)
typedef GL_DEBUG_CALLBACK(GLDEBUGPROC);
typedef void WINAPI type_glDebugMessageCallbackARB(GLDEBUGPROC *callback, const void *userParam);

#define OPENGL_GLOBAL_FUNCTION(Name) GLOBAL_VARIABLE type_##Name* Name;
OPENGL_GLOBAL_FUNCTION(glUniform1f);
OPENGL_GLOBAL_FUNCTION(glUniform2fv);
OPENGL_GLOBAL_FUNCTION(glUniform3fv);
OPENGL_GLOBAL_FUNCTION(glEnableVertexAttribArray);
OPENGL_GLOBAL_FUNCTION(glDisableVertexAttribArray);
OPENGL_GLOBAL_FUNCTION(glGetAttribLocation);
OPENGL_GLOBAL_FUNCTION(glVertexAttribPointer);
OPENGL_GLOBAL_FUNCTION(glDebugMessageCallbackARB);
OPENGL_GLOBAL_FUNCTION(glBindVertexArray);
OPENGL_GLOBAL_FUNCTION(glGenVertexArrays);
OPENGL_GLOBAL_FUNCTION(glBindBuffer);
OPENGL_GLOBAL_FUNCTION(glGenBuffers);
OPENGL_GLOBAL_FUNCTION(glBufferData);
OPENGL_GLOBAL_FUNCTION(glGetStringi);
OPENGL_GLOBAL_FUNCTION(glActiveTexture);
OPENGL_GLOBAL_FUNCTION(glDeleteProgram);
OPENGL_GLOBAL_FUNCTION(glDeleteShader);
OPENGL_GLOBAL_FUNCTION(glDeleteFramebuffers);
OPENGL_GLOBAL_FUNCTION(glTexImage2DMultisample);
OPENGL_GLOBAL_FUNCTION(glBindFramebuffer);
OPENGL_GLOBAL_FUNCTION(glGenFramebuffers);
OPENGL_GLOBAL_FUNCTION(glFramebufferTexture2D);
OPENGL_GLOBAL_FUNCTION(glCheckFramebufferStatus);
OPENGL_GLOBAL_FUNCTION(glBlitFramebuffer);
OPENGL_GLOBAL_FUNCTION(glAttachShader);
OPENGL_GLOBAL_FUNCTION(glCompileShader);
OPENGL_GLOBAL_FUNCTION(glCreateProgram);
OPENGL_GLOBAL_FUNCTION(glCreateShader);
OPENGL_GLOBAL_FUNCTION(glLinkProgram);
OPENGL_GLOBAL_FUNCTION(glShaderSource);
OPENGL_GLOBAL_FUNCTION(glUseProgram);
OPENGL_GLOBAL_FUNCTION(glGetProgramInfoLog);
OPENGL_GLOBAL_FUNCTION(glGetShaderInfoLog);
OPENGL_GLOBAL_FUNCTION(glValidateProgram);
OPENGL_GLOBAL_FUNCTION(glGetProgramiv);
OPENGL_GLOBAL_FUNCTION(glGetShaderiv);
OPENGL_GLOBAL_FUNCTION(glGetUniformLocation);
OPENGL_GLOBAL_FUNCTION(glUniform4fv);
OPENGL_GLOBAL_FUNCTION(glUniformMatrix4fv);
OPENGL_GLOBAL_FUNCTION(glUniform1i);
OPENGL_GLOBAL_FUNCTION(glUniform2f);
OPENGL_GLOBAL_FUNCTION(glUniform3f);
OPENGL_GLOBAL_FUNCTION(glUniform4f);
OPENGL_GLOBAL_FUNCTION(glDeleteBuffers);
OPENGL_GLOBAL_FUNCTION(glDeleteVertexArrays);
OPENGL_GLOBAL_FUNCTION(glGenerateMipmap);
OPENGL_GLOBAL_FUNCTION(glBufferSubData);

#include "ivan_opengl.h"
GLOBAL_VARIABLE open_gl OpenGL;

#include "ivan_opengl.cpp"

#define XINPUT_GET_STATE(name) DWORD WINAPI name(DWORD, XINPUT_STATE*)
typedef XINPUT_GET_STATE(xinput_get_state);
XINPUT_GET_STATE(XInputGetStateStub){
    return 0;
}
GLOBAL_VARIABLE xinput_get_state* XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define XINPUT_SET_STATE(name) DWORD WINAPI name(DWORD, XINPUT_VIBRATION*)
typedef XINPUT_SET_STATE(xinput_set_state);
XINPUT_SET_STATE(XInputSetStateStub){
    return 0;
}
GLOBAL_VARIABLE xinput_set_state* XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPGUID, LPDIRECTSOUND*, LPUNKNOWN)
typedef DIRECT_SOUND_CREATE(direct_sound_create);
DIRECT_SOUND_CREATE(DirectSoundCreateStub){
    return 0;
}
GLOBAL_VARIABLE direct_sound_create* DirectSoundCreate_ = DirectSoundCreateStub;
#define DirectSoundCreate DirectSoundCreate_

#define DIRECT_SOUND_CAPTURE_CREATE(name) HRESULT WINAPI name(LPGUID, LPDIRECTSOUNDCAPTURE*, LPUNKNOWN)
typedef DIRECT_SOUND_CAPTURE_CREATE(direct_sound_capture_create);
DIRECT_SOUND_CAPTURE_CREATE(DirectSoundCaptureCreateStub){
    return 0;
}
GLOBAL_VARIABLE direct_sound_capture_create* DirectSoundCaptureCreate_ = DirectSoundCaptureCreateStub;
#define DirectSoundCaptureCreate DirectSoundCaptureCreate_

GLOBAL_VARIABLE bool32 GlobalRunning;
GLOBAL_VARIABLE win32_offscreen_buffer GlobalScreen;
GLOBAL_VARIABLE WINDOWPLACEMENT GlobalWindowPlacement;
GLOBAL_VARIABLE LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
GLOBAL_VARIABLE int64 GlobalPerfomanceCounterFrequency;
GLOBAL_VARIABLE bool32 GlobalCapturingMouse;
GLOBAL_VARIABLE game_input GlobalGameInput;


DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory){
    if (Memory != 0){
        VirtualFree(Memory, 0, MEM_RELEASE);
    }
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile){
    debug_read_file_result Res = {};

    HANDLE FileHandle = CreateFileA(
        FileName,
        GENERIC_READ,
        FILE_SHARE_READ,
        0,
        OPEN_EXISTING,
        0, 0);
    if (FileHandle != INVALID_HANDLE_VALUE){
        LARGE_INTEGER FileSize;
        if (GetFileSizeEx(FileHandle, &FileSize)){
            uint32 FileSize32 = SafeTruncateUInt64(FileSize.QuadPart);
            Res.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if (Res.Contents){
                DWORD BytesRead;
                if (ReadFile(FileHandle, Res.Contents, FileSize32, &BytesRead, 0) && (FileSize32 == BytesRead)){
                    Res.ContentsSize = FileSize32;
                }
                else{
                    DEBUGPlatformFreeFileMemory(Res.Contents);
                    Res.Contents = 0;
                }
            }
            else{
                //TODO(Dima): Logging
            }
        }
        else{
            //TODO(Dima): Logging
        }
    }
    else{
        //TODO(Dima): Logging
    }
    return(Res);
}

DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile){
    bool32 Result = false;

    HANDLE FileHandle = CreateFileA(FileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if (FileHandle != INVALID_HANDLE_VALUE){
        DWORD BytesWritten;
        if (WriteFile(FileHandle, Contents, ContentsSize, &BytesWritten, 0)){
            Result = (BytesWritten == ContentsSize);
        }
        else{
            //TODO(Dima): Logging
        }
    }
    else{
        //TODO(Dima): Logging
    }

    return(Result);
}

INTERNAL_FUNCTION LPDIRECTSOUNDBUFFER
Win32InitDirectSound(HWND Window, win32_sound_output* SOutput){
    HINSTANCE DSoundLib = LoadLibraryA("dsound.dll");
    direct_sound_create* DirectSoundCreate = (direct_sound_create*)GetProcAddress(DSoundLib, "DirectSoundCreate");

    LPDIRECTSOUNDBUFFER result = 0;

    LPDIRECTSOUND DirectSound;
    if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0))){
        WAVEFORMATEX WaveFormat;
        WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
        WaveFormat.nChannels = SOutput->Channels;
        WaveFormat.nSamplesPerSec = SOutput->SamplesPerSecond;
        WaveFormat.wBitsPerSample = SOutput->BytesPerSample * 8;
        WaveFormat.nBlockAlign = SOutput->Channels * SOutput->BytesPerSample; //2 * 16 / 8
        WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
        WaveFormat.cbSize = sizeof(WAVEFORMATEX);

        if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY))){
            DSBUFFERDESC Description;
            memset(&Description, 0, sizeof(DSBUFFERDESC));
            Description.dwSize = sizeof(DSBUFFERDESC);
            Description.dwFlags = DSBCAPS_PRIMARYBUFFER;
            Description.dwBufferBytes = 0;
            Description.dwReserved = 0;
            Description.guid3DAlgorithm = GUID_NULL;
            Description.lpwfxFormat = 0;

            LPDIRECTSOUNDBUFFER PrimaryBuffer;
            if (SUCCEEDED(DirectSound->CreateSoundBuffer(&Description, &PrimaryBuffer, 0))){
                PrimaryBuffer->SetFormat(&WaveFormat);
            }
            else{
                //TODO(Dima): Logging
            }
        }
        else{
            //TODO(Dima): Logging
        }

        DSBUFFERDESC Desc = {};
        Desc.dwSize = sizeof(DSBUFFERDESC);
        Desc.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
        Desc.dwBufferBytes = WaveFormat.nAvgBytesPerSec; // 1 second length buffer
        Desc.lpwfxFormat = &WaveFormat;
        Desc.dwReserved = 0;
        Desc.guid3DAlgorithm = GUID_NULL;

        LPDIRECTSOUNDBUFFER SecondaryBuffer;
        if (SUCCEEDED(DirectSound->CreateSoundBuffer(&Desc, &SecondaryBuffer, 0))){
            result = SecondaryBuffer;
        }
        else{
            //TODO(Dima): Logging
        }
    }
    else{
        //TODO(Dima): Logging
    }

    return(result);
}

INTERNAL_FUNCTION win32_sound_input
Win32InitDirectCoundCapture()
{
    HINSTANCE DSoundLib = LoadLibraryA("dsound.dll");
    direct_sound_capture_create* DirectSoundCaptureCreate =
        (direct_sound_capture_create*)GetProcAddress(DSoundLib, "DirectSoundCaptureCreate");

    win32_sound_input Result = {};

    LPDIRECTSOUNDCAPTURE DirectSoundCapture;
    HRESULT CaptureCreateResult = DirectSoundCaptureCreate(0, &DirectSoundCapture, 0);
    if (DirectSoundCaptureCreate && SUCCEEDED(CaptureCreateResult))
    {
        uint32 DiscreteFrequency = 44100;
        WAVEFORMATEX WaveFormat = {};
        WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
        WaveFormat.nChannels = 2;
        WaveFormat.nSamplesPerSec = DiscreteFrequency;
        WaveFormat.wBitsPerSample = 16;
        WaveFormat.nBlockAlign = 4;
        WaveFormat.nAvgBytesPerSec = DiscreteFrequency * WaveFormat.nBlockAlign;
        WaveFormat.cbSize = 0;

        DSCBUFFERDESC CaptureBufferDescription = {};
        CaptureBufferDescription.dwSize = sizeof(DSCBUFFERDESC);
        CaptureBufferDescription.dwFlags = 0;
        CaptureBufferDescription.dwBufferBytes = WaveFormat.nAvgBytesPerSec * 4;
        CaptureBufferDescription.dwReserved = 0;
        CaptureBufferDescription.lpwfxFormat = &WaveFormat;
        CaptureBufferDescription.dwFXCount = 0;
        CaptureBufferDescription.lpDSCFXDesc = NULL;

        LPDIRECTSOUNDCAPTUREBUFFER SecondaryBuffer;
        if (SUCCEEDED(DirectSoundCapture->CreateCaptureBuffer(
            &CaptureBufferDescription,
            &SecondaryBuffer, 0)))
        {

            /*
                DWORD BufferByteSize;
                DWORD BytesPerSecond;
                WORD SamplesPerSecond;
                WORD Channels;
                WORD BytesPerSample;
                WORD BlockSize;
                DWORD RunningSample;*/

            Result.Buffer = SecondaryBuffer;
            Result.BufferByteSize = WaveFormat.nAvgBytesPerSec; // 1 second lenght
            Result.BytesPerSecond = WaveFormat.nAvgBytesPerSec;
            Result.SamplesPerSecond = WaveFormat.nSamplesPerSec;
            Result.Channels = WaveFormat.nChannels;
            Result.BytesPerSample = WaveFormat.wBitsPerSample / 8;
            Result.BlockSize = WaveFormat.nBlockAlign;
        }
    }
    else{
        //TODO(Dima): Logging
    }

    return(Result);
}

INTERNAL_FUNCTION void
Win32FillSoundBuffer(
    win32_sound_output* Buffer,
    DWORD ByteToLock,
    DWORD BytesToLock,
    game_sound_output_buffer* SourceBuffer)
{

    LPVOID Region1Ptr;
    DWORD Region1Size;
    LPVOID Region2Ptr;
    DWORD Region2Size;

    HRESULT LockResult =
        GlobalSecondaryBuffer->Lock(
        ByteToLock,
        BytesToLock,
        &Region1Ptr, &Region1Size,
        &Region2Ptr, &Region2Size,
        0);
    if (SUCCEEDED(LockResult)){
        DWORD Region1SampleCount = Region1Size / (Buffer->BlockSize);
        DWORD Region2SampleCount = Region2Size / (Buffer->BlockSize);

        //FROM ABSTRACTION LAYER
        int16* SourceSample = (int16*)SourceBuffer->Samples;
        int16* DestSample = (int16*)Region1Ptr;

        for (DWORD SampleIndex = 0;
            SampleIndex < Region1SampleCount;
            SampleIndex++)
        {
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;

            Buffer->RunningSample++;
        }

        DestSample = (int16*)Region2Ptr;
        for (DWORD SampleIndex = 0;
            SampleIndex < Region2SampleCount;
            SampleIndex++)
        {
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;

            Buffer->RunningSample++;
        }

        HRESULT UnlockResult = GlobalSecondaryBuffer->Unlock(
            Region1Ptr, Region1Size,
            Region2Ptr, Region2Size);
        if (SUCCEEDED(UnlockResult)){
            //NOTE(Dima): Function complete succesfully
            
        }
        else{
            //TODO(Dima): Logging
        }
    }
    else{
        //TODO(Dima): Logging
    }
}

INTERNAL_FUNCTION void
    Win32ClearSoundBuffer(win32_sound_output* Buffer)
{
    LPVOID Region1Ptr;
    DWORD Region1Size;
    LPVOID Region2Ptr;
    DWORD Region2Size;

    HRESULT LockResult =
        GlobalSecondaryBuffer->Lock(
        0,
        Buffer->SecondaryBufferByteSize,
        &Region1Ptr, &Region1Size,
        &Region2Ptr, &Region2Size,
        0);
    if (SUCCEEDED(LockResult)){

        uint8* ByteToClear = (uint8*)Region1Ptr;
        for (DWORD ByteIndex = 0;
            ByteIndex < Region1Size;
            ByteIndex++)
        {
            *ByteToClear++ = 0;
        }

        ByteToClear = (uint8*)Region2Ptr;
        for (DWORD ByteIndex = 0;
            ByteIndex < Region2Size;
            ByteIndex++)
        {
            *ByteToClear++ = 0;
        }

        HRESULT UnlockResult = GlobalSecondaryBuffer->Unlock(
            Region1Ptr, Region1Size,
            Region2Ptr, Region2Size);
        if (SUCCEEDED(UnlockResult)){
            //NOTE(Dima): Function complete succesfully
        }
        else{
            //TODO(Dima): Logging
        }
    }
}

struct win32_game_code{
    HMODULE GameCodeLib;
    FILETIME LastWriteTimeDLL;
    game_update_and_render* GameUpdateAndRender;
    game_get_sound_samples* GameGetSoundSamples;
    bool32 IsValid;
};

inline FILETIME
Win32GetLastFileWriteTime(char* FileName){
    FILETIME LastWriteTime = {};

    WIN32_FIND_DATA FindData;
    HANDLE FindHandle = FindFirstFileA(FileName, &FindData);
    if(FindHandle != INVALID_HANDLE_VALUE){
        LastWriteTime = FindData.ftLastWriteTime;
        FindClose(FindHandle);
    }

    return(LastWriteTime);
}

INTERNAL_FUNCTION win32_game_code 
Win32LoadGameCode(char* SourceFileName, char* TempFileName){
    win32_game_code GameCode = {};

    GameCode.LastWriteTimeDLL = Win32GetLastFileWriteTime(SourceFileName);
    CopyFile(SourceFileName, TempFileName, FALSE);
    GameCode.GameCodeLib = LoadLibraryA(TempFileName);
    if(GameCode.GameCodeLib){
        GameCode.GameUpdateAndRender = (game_update_and_render*)GetProcAddress(GameCode.GameCodeLib, "GameUpdateAndRender");
        GameCode.GameGetSoundSamples = (game_get_sound_samples*)GetProcAddress(GameCode.GameCodeLib, "GameGetSoundSamples");             
        
        GameCode.IsValid = GameCode.GameUpdateAndRender && GameCode.GameGetSoundSamples;
    }

    if(!GameCode.IsValid){
        GameCode.GameUpdateAndRender = GameUpdateAndRenderStub;
        GameCode.GameGetSoundSamples = GameGetSoundSamplesStub;
    }

    return(GameCode);
}

INTERNAL_FUNCTION void 
Win32UnloadGameCode(win32_game_code* GameCode){
    if(GameCode->GameCodeLib){
        FreeLibrary(GameCode->GameCodeLib);
        GameCode->GameCodeLib = 0;
    }
    GameCode->GameUpdateAndRender = GameUpdateAndRenderStub;
    GameCode->GameGetSoundSamples = GameGetSoundSamplesStub;
    GameCode->IsValid = false;
}


void BuildPathForFileInEXEDir(char* TargetFileName, char* FileName){
    char BufferForModulePath[MAX_PATH];
    DWORD RecomendedFilePathForModule = GetModuleFileName(0, BufferForModulePath, MAX_PATH);

    int PathLen = 0;
    for(int i = strlen(BufferForModulePath) - 1;
        i >= 0;
        i--)
    {
        if(BufferForModulePath[i] == '\\'){
            PathLen = i + 1;
            break;
        }
    }

    int FileNameStrLen = strlen(FileName);
    int FinalPathLen = PathLen + FileNameStrLen;
    
    int Count = 0;
    for(;Count < PathLen; Count++){
        TargetFileName[Count] = BufferForModulePath[Count];
    }
    for(int i = 0;Count < FinalPathLen; Count++, i++){
        TargetFileName[Count] = FileName[i];
    }

    TargetFileName[Count] = 0;
}


INTERNAL_FUNCTION void
Win32InitXInput(){
    HINSTANCE XInputLib = LoadLibraryA("xinput1_3.dll");
    if (!XInputLib){
        XInputLib = LoadLibraryA("XInput1_4.dll");
        if (!XInputLib){
            XInputLib = LoadLibraryA("XInput9_1_0.dll");
            if (!XInputLib){
                XInputLib = LoadLibraryA("xinput1_2.dll");
                if (!XInputLib){
                    XInputLib = LoadLibraryA("xinput1_1.dll");

                }
            }
        }
    }

    if (XInputLib){
        XInputGetState = (xinput_get_state*)GetProcAddress(XInputLib, "XInputGetState");
        XInputSetState = (xinput_set_state*)GetProcAddress(XInputLib, "XInputSetState");
    }
}

INTERNAL_FUNCTION void
Win32ProcessXInputDigitalButton(
    DWORD ButtonState,
    game_button_state* OldState,
    DWORD ButtonBit,
    game_button_state* NewState)
{
    NewState->EndedDown = (ButtonState & ButtonBit);
    NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}

INTERNAL_FUNCTION real32
Win32ProcessXInputStick(int16 StickValue, int16 DeadzoneValue)
{
    real32 Result = 0;
    if (StickValue < -DeadzoneValue){
        Result = (real32)StickValue / 32768.0f;
    }
    else if (StickValue > DeadzoneValue){
        Result = (real32)StickValue / 32767.0f;
    }
    return Result;
}

INTERNAL_FUNCTION void
Win32ProcessKeyboardMessage(game_button_state* State, bool32 IsDown, bool32 WasDown){
    State->EndedDown = IsDown;
    State->HalfTransitionCount = (WasDown != IsDown) ? 1 : 0;
}


INTERNAL_FUNCTION void
Win32ToggleFullscreen(HWND Window)
{
    DWORD Style = GetWindowLong(Window, GWL_STYLE);
    if (Style & WS_OVERLAPPEDWINDOW)
    {
        MONITORINFO MonitorInfo = { sizeof(MonitorInfo) };
        if (GetWindowPlacement(Window, &GlobalWindowPlacement) &&
            GetMonitorInfo(MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo))
        {
            SetWindowLong(Window, GWL_STYLE, Style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(Window, HWND_TOP,
                MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
                MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
                MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
                SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else
    {
        SetWindowLong(Window, GWL_STYLE, Style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(Window, &GlobalWindowPlacement);
        SetWindowPos(Window, 0, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
            SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}


INTERNAL_FUNCTION win32_window_dimension
Win32GetMonitorDimension(HWND Window){
    win32_window_dimension Res;
    RECT Rect;
    HMONITOR Monitor = MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO MonitorInfo;
    GetMonitorInfo(Monitor, &MonitorInfo);
    Rect = MonitorInfo.rcMonitor;
    Res.Width = Rect.right - Rect.left;
    Res.Height = Rect.bottom - Rect.top;
    return Res;
}

INTERNAL_FUNCTION
win32_window_dimension Win32GetWindowDimension(HWND Window){

    win32_window_dimension Res;
    RECT Rect;
    GetClientRect(Window, &Rect);
    Res.Width = Rect.right - Rect.left;
    Res.Height = Rect.bottom - Rect.top;
    return(Res);
}

INTERNAL_FUNCTION void
Win32InitScreenBuffer(win32_offscreen_buffer* Buffer, int Width, int Height){
    if (Buffer->Memory){
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->BitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    Buffer->BitmapInfo.bmiHeader.biWidth = Width;
    Buffer->BitmapInfo.bmiHeader.biHeight = -Height;
    Buffer->BitmapInfo.bmiHeader.biPlanes = 1;
    Buffer->BitmapInfo.bmiHeader.biBitCount = 32;
    Buffer->BitmapInfo.bmiHeader.biCompression = BI_RGB;
    Buffer->BitmapInfo.bmiHeader.biSizeImage = 0;

    int BytesPerPixel = 4;
    DWORD BufferSize = Width * Height * BytesPerPixel;
    Buffer->Memory = VirtualAlloc(0, BufferSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    Buffer->Width = Width;
    Buffer->Height = Height;
    Buffer->Pitch = Width;
    Buffer->BytesPerPixel = Buffer->BitmapInfo.bmiHeader.biBitCount / 8;
}

INTERNAL_FUNCTION void
Win32DisplayScreenBufferToWindow(
    game_render_commands* Commands,
    HDC hdc,
    win32_offscreen_buffer* Buffer,
    int TargetWidth,
    int TargetHeight)
{
#if 0
#if 0
    StretchDIBits(
        hdc,
        0, 0, TargetWidth, TargetHeight,
        0, 0, Buffer->Width, Buffer->Height,
        Buffer->Memory, &Buffer->BitmapInfo,
        DIB_RGB_COLORS, SRCCOPY);
#else
    if ((TargetWidth >= Buffer->Width * 2) &&
        (TargetHeight >= Buffer->Height * 2))
    {
        StretchDIBits(
            hdc,
            0, 0, 2 * Buffer->Width, 2 * Buffer->Height,
            0, 0, Buffer->Width, Buffer->Height,
            Buffer->Memory, &Buffer->BitmapInfo,
            DIB_RGB_COLORS, SRCCOPY);
    }
    else{
        int OffsetX = 0;
        int OffsetY = 0;
        PatBlt(hdc, 0, 0, TargetWidth, OffsetY, BLACKNESS);
        PatBlt(hdc, 0, OffsetY + Buffer->Height, TargetWidth, TargetHeight, BLACKNESS);
        PatBlt(hdc, 0, 0, OffsetX, TargetHeight, BLACKNESS);
        PatBlt(hdc, OffsetX + Buffer->Width, 0, TargetWidth, TargetHeight, BLACKNESS);


        StretchDIBits(
            hdc,
            OffsetX, OffsetY, Buffer->Width, Buffer->Height,
            0, 0, Buffer->Width, Buffer->Height,
            Buffer->Memory,
            &Buffer->BitmapInfo,
            DIB_RGB_COLORS,
            SRCCOPY);
    }
#endif
#else
    loaded_bitmap OutputTarget;
    OutputTarget.Memory = Buffer->Memory;
    OutputTarget.Width = Buffer->Width;
    OutputTarget.Height = Buffer->Height;

    win32_window_dimension WindowDim = Win32GetWindowDimension(GlobalScreen.Window);

    rectangle2i DrawRegion;
    DrawRegion.MinX = 0;
    DrawRegion.MinY = 0;
    DrawRegion.MaxX = WindowDim.Width;
    DrawRegion.MaxY = WindowDim.Height;

#if 0
    OpenGLRenderBitmap(
        OutputTarget.Width, 
        OutputTarget.Height, 
        OutputTarget.Memory,
        DrawRegion, Vec4(1.0f, 1.0f, 1.0f, 1.0f),
        OpenGL.ReservedBlitTexture);
#else
    OpenGLGroupToOutput(Commands, DrawRegion, Buffer->Width, Buffer->Height);
#endif

    SwapBuffers(hdc);
#endif
}


INTERNAL_FUNCTION void
Win32ProcessPendingMessages(game_controller_input* Controller){
    MSG Message;
    while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE)){
        switch (Message.message){
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                uint32 vkey = (uint32)Message.wParam;
                bool32 WasDown = ((Message.lParam & (1 << 30)) != 0);
                bool32 IsDown = ((Message.lParam & (1 << 31)) == 0);
                bool32 AltKeyWasDown = ((Message.lParam & (1 << 29)) != 0);

                if (WasDown != IsDown){
                    if (vkey == 'W'){
                        Win32ProcessKeyboardMessage(&Controller->MoveUp, IsDown, WasDown);
                    }
                    else if (vkey == 'A'){
                        Win32ProcessKeyboardMessage(&Controller->MoveLeft, IsDown, WasDown);
                    }
                    else if (vkey == 'S'){
                        Win32ProcessKeyboardMessage(&Controller->MoveDown, IsDown, WasDown);
                    }
                    else if (vkey == 'D'){
                        Win32ProcessKeyboardMessage(&Controller->MoveRight, IsDown, WasDown);
                    }
                    else if (vkey == 'Q'){
                        Win32ProcessKeyboardMessage(&Controller->LeftShoulder, IsDown, WasDown);
                    }
                    else if (vkey == 'E'){
                        Win32ProcessKeyboardMessage(&Controller->RightShoulder, IsDown, WasDown);
                    }
                    else if (vkey == VK_UP){
                        Win32ProcessKeyboardMessage(&Controller->MoveUp, IsDown, WasDown);
                    }
                    else if (vkey == VK_LEFT){
                        Win32ProcessKeyboardMessage(&Controller->MoveLeft, IsDown, WasDown);
                    }
                    else if (vkey == VK_DOWN){
                        Win32ProcessKeyboardMessage(&Controller->MoveDown, IsDown, WasDown);
                    }
                    else if (vkey == VK_RIGHT){
                        Win32ProcessKeyboardMessage(&Controller->MoveRight, IsDown, WasDown);
                    }
                    else if (vkey == VK_ESCAPE){
                        Win32ProcessKeyboardMessage(&Controller->Esc, IsDown, WasDown);
                    }
                    else if (vkey == VK_CONTROL){
                        Win32ProcessKeyboardMessage(&Controller->Ctrl, IsDown, WasDown);
                    }
                    else if (vkey == VK_SPACE){
                        Win32ProcessKeyboardMessage(&Controller->Space, IsDown, WasDown);
                    }
                    else if (vkey == VK_HOME){
                        Win32ProcessKeyboardMessage(&Controller->Home, IsDown, WasDown);
                    }
                    else if (vkey == VK_END){
                        Win32ProcessKeyboardMessage(&Controller->End, IsDown, WasDown);
                    }
                    else if (vkey == VK_BACK){
                        Win32ProcessKeyboardMessage(&Controller->Backspace, IsDown, WasDown);
                    }
                    else if (vkey == VK_TAB){
                        Win32ProcessKeyboardMessage(&Controller->Tab, IsDown, WasDown);
                    }
                    else if (vkey == VK_F4){
                        Win32ProcessKeyboardMessage(&Controller->F4, IsDown, WasDown);
                    }

                    if(IsDown){
                        if (vkey == VK_ESCAPE){
                            GlobalGameInput.CapturingMouse = !GlobalGameInput.CapturingMouse;
                        }
                    }
                }

                if (IsDown){
                    if (AltKeyWasDown == (bool32)true && vkey == VK_F4){
                        GlobalRunning = false;
                    }
                    if (AltKeyWasDown == (bool32)true && vkey == VK_RETURN){
                        Win32ToggleFullscreen(GlobalScreen.Window);
                    }
                }
            }break;

            default:{
                TranslateMessage(&Message);
                DispatchMessage(&Message);
            }break;
        }
    }
}

int Win32OpenGLAttribs[] = 
{
    WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
    WGL_CONTEXT_MINOR_VERSION_ARB, 3,

    //NOTE(Dima): WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB can remove all deprecated features.
    //So I should never actually use it here. LOL. 
    WGL_CONTEXT_FLAGS_ARB, 0
#ifdef INTERNAL_BUILD
    |WGL_CONTEXT_DEBUG_BIT_ARB
#endif
    ,
    WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB | WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
    0,
};

INTERNAL_FUNCTION void
Win32SetPixelFormat(HDC WindowDC){
    
    int SuggestedPixelFormatIndex = 0;
    GLuint ExtendedPick = 0;

    if(wglChoosePixelFormatARB){
        int IntAttribList[] = {
            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
            WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
            WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
            WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,
            0
        };
        /*
            WGL_COLOR_BITS_ARB, 32,
            WGL_DEPTH_BITS_ARB, 24,
            WGL_STENCIL_BITS_ARB, 8,
            WGL_SAMPLE_BUFFERS_ARB, 1, //Number of buffers (must be 1 at time of writing)
            WGL_SAMPLES_ARB, numberOfSamples,        //Number of samples
        */

        if(!OpenGL.SupportsSRGBFramebuffer){
            IntAttribList[10] = 0;
        }

        bool32 PFValid = wglChoosePixelFormatARB(
            WindowDC, 
            IntAttribList, 
            0, 
            1,
            &SuggestedPixelFormatIndex,
            &ExtendedPick);
    }

    if(!ExtendedPick){
        PIXELFORMATDESCRIPTOR DesiredPixelFormat = {};
        DesiredPixelFormat.nSize = sizeof(DesiredPixelFormat);
        DesiredPixelFormat.nVersion = 1;
        DesiredPixelFormat.iPixelType = PFD_TYPE_RGBA;
        DesiredPixelFormat.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        DesiredPixelFormat.cColorBits = 32;
        DesiredPixelFormat.cAlphaBits = 8;
        DesiredPixelFormat.cDepthBits = 24;
        DesiredPixelFormat.iLayerType = PFD_MAIN_PLANE;

        SuggestedPixelFormatIndex = ChoosePixelFormat(WindowDC, &DesiredPixelFormat);
    }

    PIXELFORMATDESCRIPTOR SuggestedPixelFormat;
    DescribePixelFormat(
        WindowDC, 
        SuggestedPixelFormatIndex,
        sizeof(SuggestedPixelFormat),
        &SuggestedPixelFormat);
    SetPixelFormat(
        WindowDC,
        SuggestedPixelFormatIndex, 
        &SuggestedPixelFormat);
}

INTERNAL_FUNCTION void
Win32LoadWGLExtensions(){
    WNDCLASSA WindowClass = {};
    
    WindowClass.lpfnWndProc = DefWindowProcA;
    WindowClass.hInstance = GetModuleHandle(0);
    WindowClass.lpszClassName = "IvanWGLLoader";

    if(RegisterClassA(&WindowClass)){
        HWND Window = CreateWindowExA(
            0,
            WindowClass.lpszClassName,
            "IvanEngineWGLLoader",
            0,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            WindowClass.hInstance,
            0);

        HDC WindowDC = GetDC(Window);
        Win32SetPixelFormat(WindowDC);

        HGLRC OpenGLRC = wglCreateContext(WindowDC);
        
        if(wglMakeCurrent(WindowDC, OpenGLRC)){
        
            wglChoosePixelFormatARB = (wgl_choose_pixel_format_arb*)wglGetProcAddress("wglChoosePixelFormatARB");
            wglCreateContextAttribsARB = (wgl_create_context_attribs_arb*)wglGetProcAddress("wglCreateContextAttribsARB");
            wglGetExtensionsStringEXT = (wgl_get_extensions_string_ext*)wglGetProcAddress("wglGetExtensionsStringEXT");

            if(wglGetExtensionsStringEXT){
                char* Extensions = (char*)wglGetExtensionsStringEXT();
                char* At = Extensions;

                while(*At){
                    while(IsWhitespace(*At)){
                        At++;
                    }

                    char* End = At;
                    while(*End && !IsWhitespace(*End)){
                        End++;
                    }

                    uintptr Count = End - At;

                    if(StringsAreEqual(Count, At, "WGL_EXT_framebuffer_sRGB")){
                        OpenGL.SupportsSRGBFramebuffer = true;
                    }
                    else if(StringsAreEqual(Count, At, "WGL_ARB_framebuffer_sRGB")){
                        OpenGL.SupportsSRGBFramebuffer = true;
                    }

                    At = End;
                }
            }

            wglMakeCurrent(0, 0);
        }

        wglDeleteContext(OpenGLRC);
        ReleaseDC(Window, WindowDC);
        DestroyWindow(Window);
    }
}

INTERNAL_FUNCTION HGLRC Win32InitOpenGL(HDC WindowDC){

    Win32LoadWGLExtensions();

    Win32SetPixelFormat(WindowDC);

    bool32 ModernContext = true;
    HGLRC OpenGLRC = 0;
    if(wglCreateContextAttribsARB){
        OpenGLRC = wglCreateContextAttribsARB(WindowDC, 0, Win32OpenGLAttribs);
    }

    if(!OpenGLRC){
        ModernContext = false;
        OpenGLRC = wglCreateContext(WindowDC);
    }

    if(wglMakeCurrent(WindowDC, OpenGLRC)){

#define WIN32_GET_OPENGL_FUNCTION(Name) Name = (type_##Name*)wglGetProcAddress(#Name)

        WIN32_GET_OPENGL_FUNCTION(glEnableVertexAttribArray);
        WIN32_GET_OPENGL_FUNCTION(glDisableVertexAttribArray);
        WIN32_GET_OPENGL_FUNCTION(glGetAttribLocation);
        WIN32_GET_OPENGL_FUNCTION(glVertexAttribPointer);
        WIN32_GET_OPENGL_FUNCTION(glDebugMessageCallbackARB);
        WIN32_GET_OPENGL_FUNCTION(glBindVertexArray);
        WIN32_GET_OPENGL_FUNCTION(glGenVertexArrays);
        WIN32_GET_OPENGL_FUNCTION(glBindBuffer);
        WIN32_GET_OPENGL_FUNCTION(glGenBuffers);
        WIN32_GET_OPENGL_FUNCTION(glBufferData);
        WIN32_GET_OPENGL_FUNCTION(glGetStringi);
        WIN32_GET_OPENGL_FUNCTION(glActiveTexture);
        WIN32_GET_OPENGL_FUNCTION(glDeleteShader);
        WIN32_GET_OPENGL_FUNCTION(glDeleteProgram);
        WIN32_GET_OPENGL_FUNCTION(glDeleteFramebuffers);
        WIN32_GET_OPENGL_FUNCTION(glTexImage2DMultisample);
        WIN32_GET_OPENGL_FUNCTION(glBlitFramebuffer);
        WIN32_GET_OPENGL_FUNCTION(glAttachShader);
        WIN32_GET_OPENGL_FUNCTION(glCompileShader);
        WIN32_GET_OPENGL_FUNCTION(glCreateProgram);
        WIN32_GET_OPENGL_FUNCTION(glCreateShader);
        WIN32_GET_OPENGL_FUNCTION(glLinkProgram);
        WIN32_GET_OPENGL_FUNCTION(glShaderSource);
        WIN32_GET_OPENGL_FUNCTION(glUseProgram);
        WIN32_GET_OPENGL_FUNCTION(glGetProgramInfoLog);
        WIN32_GET_OPENGL_FUNCTION(glGetShaderInfoLog);
        WIN32_GET_OPENGL_FUNCTION(glValidateProgram);
        WIN32_GET_OPENGL_FUNCTION(glGetProgramiv);
        WIN32_GET_OPENGL_FUNCTION(glGetShaderiv);
        WIN32_GET_OPENGL_FUNCTION(glGetUniformLocation);
        WIN32_GET_OPENGL_FUNCTION(glUniform4fv);
        WIN32_GET_OPENGL_FUNCTION(glUniformMatrix4fv);
        WIN32_GET_OPENGL_FUNCTION(glUniform1i);
        WIN32_GET_OPENGL_FUNCTION(glUniform1f);
        WIN32_GET_OPENGL_FUNCTION(glUniform2fv);
        WIN32_GET_OPENGL_FUNCTION(glUniform3fv);
        WIN32_GET_OPENGL_FUNCTION(glUniform2f);
        WIN32_GET_OPENGL_FUNCTION(glUniform3f);
        WIN32_GET_OPENGL_FUNCTION(glUniform4f);
        WIN32_GET_OPENGL_FUNCTION(glDeleteBuffers);
        WIN32_GET_OPENGL_FUNCTION(glDeleteVertexArrays);
        WIN32_GET_OPENGL_FUNCTION(glGenerateMipmap);
        WIN32_GET_OPENGL_FUNCTION(glBufferSubData);

        opengl_info Info = OpenGLGetInfo(ModernContext);

        if(Info.GL_ARB_framebuffer_object){
            WIN32_GET_OPENGL_FUNCTION(glBindFramebuffer);
            WIN32_GET_OPENGL_FUNCTION(glGenFramebuffers);
            WIN32_GET_OPENGL_FUNCTION(glFramebufferTexture2D);
            WIN32_GET_OPENGL_FUNCTION(glCheckFramebufferStatus);
        }
        
        wgl_swap_interval_ext* wglSwapIntervalEXT = (wgl_swap_interval_ext*)wglGetProcAddress("wglSwapIntervalEXT");

        if(wglSwapIntervalEXT){
            wglSwapIntervalEXT(0);
        }

        OpenGLInit(Info, OpenGL.SupportsSRGBFramebuffer, 
            DEBUGPlatformReadEntireFile, 
            DEBUGPlatformFreeFileMemory);
    }

    return(OpenGLRC);
}

INTERNAL_FUNCTION void Win32AddEntry(platform_work_queue* Queue, platform_work_queue_callback* Callback, void* Data){
    int NewNextEntryToWrite = (Queue->NextEntryToWrite + 1) % ArrayCount(Queue->Entries);
    Assert(NewNextEntryToWrite != Queue->NextEntryToRead);
    platform_work_queue_entry* Entry = Queue->Entries + Queue->NextEntryToWrite;
    Entry->Callback = Callback;
    Entry->Data = Data;
    ++Queue->EntryCount;
    _mm_sfence();
    _WriteBarrier();
    Queue->NextEntryToWrite = NewNextEntryToWrite;
    ReleaseSemaphore(Queue->SemaphoreHandle, 1, 0);
}

INTERNAL_FUNCTION bool32 
Win32DoNextWork(platform_work_queue* Queue){
    bool32 ShouldSleep = false;

    _ReadBarrier();

    int OriginalNextEntryToRead = Queue->NextEntryToRead;
    int NewNextEntryToRead = (OriginalNextEntryToRead + 1) % ArrayCount(Queue->Entries);
    if (OriginalNextEntryToRead != Queue->NextEntryToWrite){
        int Index = InterlockedCompareExchange((LONG volatile *)&Queue->NextEntryToRead,
            NewNextEntryToRead,
            OriginalNextEntryToRead);
        if (Index == OriginalNextEntryToRead){
            platform_work_queue_entry Entry = Queue->Entries[Index];
            Entry.Callback(Queue, Entry.Data);
            InterlockedIncrement((LONG volatile *)&Queue->FinishedEntries);
        }
    }
    else{
        ShouldSleep = true;
    }

    return(ShouldSleep);
}

INTERNAL_FUNCTION void 
Win32CompleteAllWork(platform_work_queue* Queue){
    while (Queue->EntryCount != Queue->FinishedEntries){
        Win32DoNextWork(Queue);
    }
    Queue->FinishedEntries = 0;
    Queue->EntryCount = 0;
}

DWORD WINAPI 
ThreadProcedure(LPVOID Param){
    win32_thread_startup* Thread = (win32_thread_startup*)Param;
    platform_work_queue* Queue = Thread->Queue;

    for (;;){
        if (Win32DoNextWork(Queue)){
            WaitForSingleObjectEx(Queue->SemaphoreHandle, INFINITE, FALSE);
        }
    }                   

    return(0);
}

INTERNAL_FUNCTION void 
Win32MakeQueue(platform_work_queue* Queue, int ThreadCount, win32_thread_startup* Startups){
    Queue->EntryCount = 0;
    Queue->FinishedEntries = 0;

    Queue->NextEntryToWrite = 0;
    Queue->NextEntryToRead = 0;

    Queue->SemaphoreHandle = CreateSemaphoreEx(
        0,
        0,
        ThreadCount,
        0,
        0,
        SEMAPHORE_ALL_ACCESS);

    for (int i = 0; i < ThreadCount; i++){
        win32_thread_startup* Startup = Startups + i;
        Startup->Queue = Queue;

        DWORD ThreadID;
        HANDLE ThreadHandle = CreateThread(0, 0, ThreadProcedure, Startup, 0, &ThreadID);
        CloseHandle(ThreadHandle);
    }
}

#if 0
INTERNAL_FUNCTION PLATFORM_WORK_QUEUE_CALLBACK(DoWorkerWork){
    char Buffer[256];
    sprintf(Buffer, "Thread %u: %s\n", GetCurrentThreadId(), (char*)Data);
    OutputDebugStringA(Buffer);
    Sleep(100);
}
#endif

struct win32_platform_file_handle{
    platform_file_handle D;
    HANDLE Win32Handle;
};

struct win32_platform_file_group{
    platform_file_group D;
    HANDLE FindHandle;
    WIN32_FIND_DATAA FindData;
};

INTERNAL_FUNCTION PLATFORM_GET_ALL_FILES_OF_TYPE_BEGIN(Win32GetAllFilesOfTypeBegin){
    win32_platform_file_group* Win32FileGroup = (win32_platform_file_group*)VirtualAlloc(
        0,
        sizeof(win32_platform_file_group),
        MEM_RESERVE | MEM_COMMIT,
        PAGE_READWRITE);

    char* TypeAt = Type;
    char WildCard[32] = "../Data/*.";
    for(uint32 WildCardIndex = strlen(WildCard);
        WildCardIndex < sizeof(WildCard);
        WildCardIndex++)
    {
        WildCard[WildCardIndex] = *TypeAt;
        if(*TypeAt == 0){
            break;
        }

        TypeAt++;
    }
    WildCard[sizeof(WildCard) - 1] = 0;

    Win32FileGroup->D.FileCount = 0;

    WIN32_FIND_DATAA FindData;
    HANDLE FindHandle = FindFirstFileA(WildCard, &FindData);
    while(FindHandle != INVALID_HANDLE_VALUE){
        Win32FileGroup->D.FileCount++;

        if(!FindNextFileA(FindHandle, &FindData)){
            break;
        }
    }
    FindClose(FindHandle);

    Win32FileGroup->FindHandle = FindFirstFileA(WildCard, &Win32FileGroup->FindData);

    return((platform_file_group*)Win32FileGroup);
}

INTERNAL_FUNCTION PLATFORM_GET_ALL_FILES_OF_TYPE_END(Win32GetAllFilesOfTypeEnd){
    win32_platform_file_group* Win32FileGroup = (win32_platform_file_group*)FileGroup;
    if(Win32FileGroup){
        FindClose(Win32FileGroup->FindHandle);

        VirtualFree(Win32FileGroup, 0, MEM_RELEASE);
    }
}

INTERNAL_FUNCTION PLATFORM_OPEN_FILE(Win32OpenNextFile){

    win32_platform_file_group* Win32FileGroup = (win32_platform_file_group*)FileGroup;
    win32_platform_file_handle* Result = 0;

    if(Win32FileGroup->FindHandle != INVALID_HANDLE_VALUE){
        Result = (win32_platform_file_handle*)VirtualAlloc(
            0,
            sizeof(win32_platform_file_handle),
            MEM_RESERVE | MEM_COMMIT,
            PAGE_READWRITE);

        if(Result){
            char* FileName = Win32FileGroup->FindData.cFileName;
            char TargetFileName[256];
            char TargetDir[] = "../Data/";
            int SourceLen = strlen(FileName);
            int DirLen = strlen(TargetDir);
            int it = 0;
            for(it; it < DirLen; it++){
                TargetFileName[it] = TargetDir[it];
            }
            for(it; it < DirLen + SourceLen; it++){
                TargetFileName[it] = FileName[it - DirLen];
            }
            *(TargetFileName + it) = 0;

            Result->Win32Handle = CreateFileA(TargetFileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
            Result->D.NoErrors = (Result->Win32Handle != INVALID_HANDLE_VALUE);
        }

        if(!FindNextFileA(Win32FileGroup->FindHandle, &Win32FileGroup->FindData)){
            FindClose(Win32FileGroup->FindHandle);
            Win32FileGroup->FindHandle = INVALID_HANDLE_VALUE;
        }
    }

    return((platform_file_handle*)Result);
}

INTERNAL_FUNCTION PLATFORM_FILE_ERROR(Win32FileError){

    Handle->NoErrors = false;
}

INTERNAL_FUNCTION PLATFORM_READ_DATA_FROM_FILE(Win32ReadDataFromFile){
    if(PlatformNoFileErrors(Source)){
        win32_platform_file_handle* Handle = (win32_platform_file_handle*)Source;
        OVERLAPPED Overlapped = {};
        Overlapped.Offset = (uint32)((Offset >> 0) & 0xFFFFFFFF);
        Overlapped.OffsetHigh = (uint32)((Offset >> 32) & 0xFFFFFFFF);

        uint32 FileSize32 = SafeTruncateUInt64(Size);

        DWORD BytesRead;
		BOOL FileHasBeenRead = ReadFile(Handle->Win32Handle, Dest, FileSize32, &BytesRead, &Overlapped);
        if(FileHasBeenRead && (FileSize32 == BytesRead))
        {

        }
        else{
            #if 1
            DWORD LastError = GetLastError();
            LARGE_INTEGER FileSize;
            GetFileSizeEx(Handle->Win32Handle, &FileSize);
            #endif
            Win32FileError(&Handle->D, "Error while reading the file");
        }
    }
}

INTERNAL_FUNCTION PLATFORM_ALLOCATE_MEMORY(Win32AllocateMemory){
    void* Result = VirtualAlloc(
        0, Size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    return(Result);
}

INTERNAL_FUNCTION PLATFORM_DEALLOCATE_MEMORY(Win32DeallocateMemory){
    if(Memory){
        VirtualFree(Memory, 0, MEM_RELEASE);
    }
}

LRESULT CALLBACK
Win32WindowProcessing(
    HWND Window,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam)
{
    switch (Message){
        case WM_CLOSE:{
            GlobalRunning = false;
        }
        case WM_DESTROY:{
            GlobalRunning = false;
        }break;
        case WM_QUIT:{
            GlobalRunning = false;
        }break;
        case WM_SIZE:{

        }break;

        case WM_PAINT:{
            PAINTSTRUCT p;
            HDC PaintHdc = BeginPaint(Window, &p);
            //PatBlt(PaintHdc, 0, 0, GlobalScreen.Width, GlobalScreen.Height, BLACKNESS);
            //Win32DisplayScreenBufferToWindow(PaintHdc, &GlobalScreen, GlobalScreen.Width, GlobalScreen.Height);
            EndPaint(Window, &p);
        }break;
        default:{
            return DefWindowProc(Window, Message, WParam, LParam);
        }break;
    }
    return 0;
}

int WINAPI WinMain(
    HINSTANCE Instance,
    HINSTANCE PrevInstance,
    LPSTR ComandLine,
    int ComandShow)
{

    Win32InitXInput();

#if 1
    Win32InitScreenBuffer(&GlobalScreen, 1366, 768);
#else
    Win32InitScreenBuffer(&GlobalScreen, 960, 540);
#endif


    WNDCLASSEX wcex = {};
    wcex.hInstance = Instance;
    wcex.lpfnWndProc = Win32WindowProcessing;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wcex.lpszClassName = "WindowClassName";

    RegisterClassEx(&wcex);
	


    GlobalScreen.Window = CreateWindowEx(
        0,
        wcex.lpszClassName,
        "IvanEngine",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        10,
        10,
        GlobalScreen.Width,
        GlobalScreen.Height,
        0,
        0,
        Instance,
        0);

    HDC OpenGLDC = GetDC(GlobalScreen.Window);
    HGLRC OpenGLRC = 0;
    OpenGLRC = Win32InitOpenGL(OpenGLDC);
    //InitOpenGL2(&GlobalScreen);

    win32_thread_startup HighPriStartups[4] = {};
    platform_work_queue HighPriorityQueue = {};
    Win32MakeQueue(&HighPriorityQueue, ArrayCount(HighPriStartups), HighPriStartups);

    win32_thread_startup LowPriStartups[2] = {};
    platform_work_queue LowPriorityQueue = {};
    Win32MakeQueue(&LowPriorityQueue, ArrayCount(LowPriStartups), LowPriStartups);

    win32_thread_startup VoxelMeshStartups[4] = {};
    platform_work_queue VoxelMeshQueue = {};
    Win32MakeQueue(&VoxelMeshQueue, ArrayCount(VoxelMeshStartups), VoxelMeshStartups);

    game_memory GameMemory = {};
    GameMemory.PermanentStorageSize = GD_MEGABYTES(200);
    GameMemory.TransientStorageSize = GD_MEGABYTES(2000);
    
    uint32 MemoryBlockSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
    void* MemoryBlock = VirtualAlloc(0, MemoryBlockSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    GameMemory.PermanentStorage = MemoryBlock;
    GameMemory.TransientStorage = (uint8*)MemoryBlock + GameMemory.PermanentStorageSize;
    
    GameMemory.PlatformAPI.DEBUGFreeFileMemory = DEBUGPlatformFreeFileMemory;
    GameMemory.PlatformAPI.DEBUGReadEntireFile = DEBUGPlatformReadEntireFile;
    GameMemory.PlatformAPI.DEBUGWriteEntireFile = DEBUGPlatformWriteEntireFile;
    
    GameMemory.PlatformAPI.AddEntry = Win32AddEntry;
    GameMemory.PlatformAPI.CompleteAllWork = Win32CompleteAllWork;
    
    GameMemory.HighPriorityQueue = &HighPriorityQueue;
    GameMemory.LowPriorityQueue = &LowPriorityQueue;
    GameMemory.VoxelMeshQueue = &VoxelMeshQueue;

    GameMemory.PlatformAPI.GetAllFilesOfTypeBegin = Win32GetAllFilesOfTypeBegin;
    GameMemory.PlatformAPI.GetAllFilesOfTypeEnd = Win32GetAllFilesOfTypeEnd;
    GameMemory.PlatformAPI.OpenNextFile = Win32OpenNextFile;
    GameMemory.PlatformAPI.ReadDataFromFile = Win32ReadDataFromFile;
    GameMemory.PlatformAPI.FileError = Win32FileError;

    GameMemory.PlatformAPI.AllocateMemory = Win32AllocateMemory;
    GameMemory.PlatformAPI.DeallocateMemory = Win32DeallocateMemory;

    uint32 TextureOpCount = 1024;
    platform_texture_op_queue* TextureOpQueue = &GameMemory.TextureOpQueue;
    TextureOpQueue->FirstFree = (texture_op*)VirtualAlloc(0, sizeof(texture_op) * TextureOpCount,
                                                            MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    for(uint32 TextureOpIndex = 0;
        TextureOpIndex < (TextureOpCount - 1);
        TextureOpIndex++)
    {
        texture_op* Op = TextureOpQueue->FirstFree + TextureOpIndex;
        Op->Next = TextureOpQueue->FirstFree + TextureOpIndex + 1;
    }

    uint32 PushBufferSize = GD_MEGABYTES(5);
    uint8* PushBuffer = (uint8*)VirtualAlloc(0, PushBufferSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    game_render_commands RenderCommands = DefaultRenderCommands(
        PushBufferSize,
        PushBuffer,
        (uint32)GlobalScreen.Width,
        (uint32)GlobalScreen.Height);


    char* SourceDLLName = "ivan.dll";
    char* TempDLLName = "ivan_temp.dll";

    char SourceDLLFullPath[MAX_PATH];
    BuildPathForFileInEXEDir(SourceDLLFullPath, SourceDLLName);

	char TempDLLFullPath[MAX_PATH];
    BuildPathForFileInEXEDir(TempDLLFullPath, TempDLLName);

    win32_game_code Game = Win32LoadGameCode(SourceDLLFullPath, TempDLLFullPath);

    DWORD SampsPerSec = 48000;
    WORD Channs = 2;
    WORD BytesPerSample = 2;
    win32_sound_output SoundOutput = {};
    SoundOutput.BytesPerSample = BytesPerSample;
    SoundOutput.Channels = Channs;
    SoundOutput.BlockSize = BytesPerSample * Channs;
    SoundOutput.SamplesPerSecond = (WORD)SampsPerSec;
    SoundOutput.SecondaryBufferByteSize = SampsPerSec * Channs * BytesPerSample;
    //SoundOutput.LatencyCycleCount = SampsPerSec / 15;
    GlobalSecondaryBuffer = Win32InitDirectSound(GlobalScreen.Window, &SoundOutput);
    Win32ClearSoundBuffer(&SoundOutput);
    GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

    int16* Samples = (int16*)VirtualAlloc(0, SoundOutput.SecondaryBufferByteSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    GlobalGameInput.CapturingMouse = 1;

    LARGE_INTEGER PerfomanceCounterFreq;
    QueryPerformanceFrequency(&PerfomanceCounterFreq);
    GlobalPerfomanceCounterFrequency = PerfomanceCounterFreq.QuadPart;
    real32 TargetMSPerFrame = 1000.0f / 30.0f;

    LARGE_INTEGER LastCounter;
    QueryPerformanceCounter(&LastCounter);
    uint64 LastCycleCount = __rdtsc();
    real32 DeltaTime = 1.0f;

    GlobalRunning = true;
    while (GlobalRunning){

        FILETIME NewDLLWriteFileTime = Win32GetLastFileWriteTime(SourceDLLFullPath);
        if(CompareFileTime(&NewDLLWriteFileTime, &Game.LastWriteTimeDLL) != 0){

            Win32CompleteAllWork(&HighPriorityQueue);
            Win32CompleteAllWork(&LowPriorityQueue);

            Win32UnloadGameCode(&Game);
            Game = Win32LoadGameCode(SourceDLLName, TempDLLFullPath);
        }

        game_controller_input* KeyboardController = GetController(&GlobalGameInput, 0);
        Win32ProcessPendingMessages(KeyboardController);

        GlobalGameInput.DeltaTime = DeltaTime;

        float ScreenCenterX = (float)GlobalScreen.Width / 2.0f;
        float ScreenCenterY = (float)GlobalScreen.Height / 2.0f;
        
        if(GlobalGameInput.CapturingMouse){
        
            POINT MouseP;
            GetCursorPos(&MouseP);
            //ScreenToClient(GlobalScreen.Window, &MouseP);
            
            GlobalGameInput.MouseP.x = MouseP.x;
            GlobalGameInput.MouseP.y = MouseP.y;
            GlobalGameInput.MouseP.z = 0;
            
            GlobalGameInput.DeltaMouseP.x = -(float)MouseP.x + ScreenCenterX;
            GlobalGameInput.DeltaMouseP.y = (float)MouseP.y - ScreenCenterY;
        
            SetCursorPos(ScreenCenterX, ScreenCenterY);
        }
        else{
            GlobalGameInput.DeltaMouseP.x = 0;
            GlobalGameInput.DeltaMouseP.y = 0;
        }

        int16 MouseLButton = GetKeyState(VK_LBUTTON);
        int16 MouseRButton = GetKeyState(VK_RBUTTON);
        int16 MouseMButton = GetKeyState(VK_MBUTTON);
        int16 MouseX1Button = GetKeyState(VK_XBUTTON1);
        int16 MouseX2Button = GetKeyState(VK_XBUTTON2);

        Win32ProcessKeyboardMessage(
            &GlobalGameInput.MouseButtons[0],
            MouseLButton & (1 << 15),
            MouseLButton & (1 << 0));
        Win32ProcessKeyboardMessage(
            &GlobalGameInput.MouseButtons[1],
            MouseRButton & (1 << 15),
            MouseRButton & (1 << 0));
        Win32ProcessKeyboardMessage(
            &GlobalGameInput.MouseButtons[2],
            MouseMButton & (1 << 15),
            MouseMButton & (1 << 0));
        Win32ProcessKeyboardMessage(
            &GlobalGameInput.MouseButtons[3],
            MouseX1Button & (1 << 15),
            MouseX1Button & (1 << 0));
        Win32ProcessKeyboardMessage(
            &GlobalGameInput.MouseButtons[4],
            MouseX2Button & (1 << 15),
            MouseX2Button & (1 << 0));

        //PROCESS XINPUT
        int MaxContrllerCount = XUSER_MAX_COUNT;
        if (MaxContrllerCount > ArrayCount(GlobalGameInput.Controllers)){
            MaxContrllerCount = ArrayCount(GlobalGameInput.Controllers);
        }
        DWORD dwResult = 0;
        for (DWORD ControllerIndex = 0;
            ControllerIndex < XUSER_MAX_COUNT;
            ControllerIndex++)
        {
            game_controller_input* NewController = GetController(&GlobalGameInput, ControllerIndex + 1);
            game_controller_input* OldController = GetController(&GlobalGameInput, ControllerIndex + 1);

            XINPUT_STATE XInputState;
            ZeroMemory(&XInputState, sizeof(XINPUT_STATE));
            if (XInputGetState(ControllerIndex, &XInputState) == ERROR_SUCCESS){
                //NOTE(Dima): Controller is connected
                XINPUT_GAMEPAD Gamepad = XInputState.Gamepad;

                NewController->IsConnected = true;
                NewController->IsAnalog = true;
                NewController->StickAverageX = Win32ProcessXInputStick(Gamepad.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                NewController->StickAverageY = Win32ProcessXInputStick(Gamepad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

                if (NewController->StickAverageX != 0 ||
                    NewController->StickAverageY != 0)
                {
                    NewController->IsAnalog = true;
                }

                if (Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP){
                    NewController->StickAverageY = 1.0f;
                    NewController->IsAnalog = false;
                }
                if (Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN){
                    NewController->StickAverageY = -1.0f;
                    NewController->IsAnalog = false;
                }
                if (Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) {
                    NewController->StickAverageX = -1.0f;
                    NewController->IsAnalog = false;
                }
                if (Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT){
                    NewController->StickAverageX = 1.0f;
                    NewController->IsAnalog = false;
                }

                real32 Threshold = 0.5f;
                Win32ProcessXInputDigitalButton(
                    NewController->StickAverageX > Threshold ? 1 : 0,
                    &OldController->MoveRight, 1,
                    &NewController->MoveRight);
                Win32ProcessXInputDigitalButton(
                    NewController->StickAverageX < -Threshold ? 1 : 0,
                    &OldController->MoveLeft, 1,
                    &NewController->MoveLeft);
                Win32ProcessXInputDigitalButton(
                    NewController->StickAverageY > Threshold ? 1 : 0,
                    &OldController->MoveUp, 1,
                    &NewController->MoveUp);
                Win32ProcessXInputDigitalButton(
                    NewController->StickAverageY < -Threshold ? 1 : 0,
                    &OldController->MoveDown, 1,
                    &NewController->MoveDown);


                Win32ProcessXInputDigitalButton(
                    Gamepad.wButtons,
                    &OldController->ActionDown, XINPUT_GAMEPAD_A,
                    &NewController->ActionDown);
                Win32ProcessXInputDigitalButton(
                    Gamepad.wButtons,
                    &OldController->ActionRight, XINPUT_GAMEPAD_B,
                    &NewController->ActionRight);
                Win32ProcessXInputDigitalButton(
                    Gamepad.wButtons,
                    &OldController->ActionLeft, XINPUT_GAMEPAD_X,
                    &NewController->ActionLeft);
                Win32ProcessXInputDigitalButton(
                    Gamepad.wButtons,
                    &OldController->ActionUp, XINPUT_GAMEPAD_Y,
                    &NewController->ActionUp);
                Win32ProcessXInputDigitalButton(
                    Gamepad.wButtons,
                    &OldController->LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER,
                    &NewController->LeftShoulder);
                Win32ProcessXInputDigitalButton(
                    Gamepad.wButtons,
                    &OldController->RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER,
                    &NewController->RightShoulder);
                Win32ProcessXInputDigitalButton(
                    Gamepad.wButtons,
                    &OldController->Start, XINPUT_GAMEPAD_START,
                    &NewController->Start);
                Win32ProcessXInputDigitalButton(
                    Gamepad.wButtons,
                    &OldController->Back, XINPUT_GAMEPAD_BACK,
                    &NewController->Back);

                int16 StickX = Gamepad.sThumbLX;
                int16 StickY = Gamepad.sThumbLY;
            }
            else{
                //TODO(Dima): Logging
                NewController->IsConnected = false;
            }
        }

        bool32 SoundIsValid = false;
        DWORD ByteToLock = 0;
        DWORD BytesToLock = 0;
        DWORD WriteCursor;
        DWORD PlayCursor;
        DWORD TargetPlayCursor;

        if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor))){
            ByteToLock = (SoundOutput.RunningSample * SoundOutput.BlockSize) % SoundOutput.SecondaryBufferByteSize;
            //TargetPlayCursor = (PlayCursor + (SoundOutput.LatencyCycleCount * SoundOutput.BlockSize)) % SoundOutput.SecondaryBufferByteSize;
            TargetPlayCursor = (PlayCursor) % SoundOutput.SecondaryBufferByteSize;

            if (TargetPlayCursor == ByteToLock){
                BytesToLock = 0;
                //BytesToLock = SoundOutput.SecondaryBufferByteSize;
            }
            else{
                if (TargetPlayCursor < ByteToLock){
                    BytesToLock = SoundOutput.SecondaryBufferByteSize - (ByteToLock - TargetPlayCursor);
                }
                else{
                    BytesToLock = TargetPlayCursor - ByteToLock;
                }
            }
            SoundIsValid = true;
        }
        else{
            //TODO(Dima): Logging
        }

        game_sound_output_buffer GameSoundOutputBuffer = {};
        GameSoundOutputBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
        GameSoundOutputBuffer.SampleCount = BytesToLock / SoundOutput.BlockSize;
        GameSoundOutputBuffer.Samples = Samples;

        win32_window_dimension WindowDim = Win32GetWindowDimension(GlobalScreen.Window);
        game_offscreen_buffer GameBuffer = {};
        GameBuffer.Memory = GlobalScreen.Memory;
        GameBuffer.Width = GlobalScreen.Width;
        GameBuffer.Height = GlobalScreen.Height;

        if(Game.GameUpdateAndRender){
            Game.GameUpdateAndRender(&GameMemory, &GlobalGameInput, &GameBuffer, &RenderCommands);
        }
        if(Game.GameGetSoundSamples){
            Game.GameGetSoundSamples(&GameMemory, &GameSoundOutputBuffer);
        }

        if (SoundIsValid){
            Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToLock, &GameSoundOutputBuffer);
        }

        BeginTicketMutex(&TextureOpQueue->Mutex);
        texture_op* FirstTextureOp = TextureOpQueue->First;
        texture_op* LastTextureOp = TextureOpQueue->Last;
        TextureOpQueue->Last = TextureOpQueue->First = 0;
        EndTicketMutex(&TextureOpQueue->Mutex);

        if(FirstTextureOp){
            Assert(LastTextureOp);
            OpenGLManageTextures(FirstTextureOp);
            BeginTicketMutex(&TextureOpQueue->Mutex);
            LastTextureOp->Next = TextureOpQueue->FirstFree;
            TextureOpQueue->FirstFree = FirstTextureOp;
            EndTicketMutex(&TextureOpQueue->Mutex);
        }

        HDC hdc = GetDC(GlobalScreen.Window);
        Win32DisplayScreenBufferToWindow(&RenderCommands, hdc, &GlobalScreen, WindowDim.Width, WindowDim.Height);
        ReleaseDC(GlobalScreen.Window, hdc);

        RenderCommands.PushBufferDataAt = RenderCommands.PushBufferBase;

        //PROCESS COUNTERS AND CYCLES
        uint64 EndCycleCount = __rdtsc();
        LARGE_INTEGER EndCounter;
        QueryPerformanceCounter(&EndCounter);
        
        uint64 CyclesElapsed = EndCycleCount - LastCycleCount;
        int64 CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
        real32 SPF = (real32)((real32)CounterElapsed / (real32)GlobalPerfomanceCounterFrequency); // seconds per frame OR delta time
        real32 MSPerFrame = SPF * 1000.0f; // mili seconds per frame
        real32 FPS = (real32)GlobalPerfomanceCounterFrequency / (real32)CounterElapsed;
        real32 MCPF = ((real32)CyclesElapsed / (1000.0f * 1000.0f)); //mili cycles per frame
        DeltaTime = SPF;

        char OutputStr[64];
        sprintf_s(OutputStr, "MSPerFrame: %.2fms. FPS: %.2f\n", MSPerFrame, FPS);
        //sprintf_s(OutputStr, "FPS: %.2f\n", FPS);
        OutputDebugStringA(OutputStr);

        LastCounter = EndCounter;
        LastCycleCount = EndCycleCount;
    }

    VirtualFree(Samples, 0, MEM_RELEASE);

    return 0;
}