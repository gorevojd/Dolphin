#include "ivan_render_group.h"
#include "ivan_voxel_mesh.h"

/*
    WHAT I HAPPENDED TO LEARN:

        1) OpenGL can optimize out uniform variables and
        set glGetUniformLocation result to -1 if it has not
        been used;
*/

#define GL_TOSTR__(x) #x
#define GL_TOSTR_(x) GL_TOSTR__(x)
#define GL_ERR_LOC(ErrName) #ErrName ":"GL_TOSTR_(__LINE__)":" __FUNCTION__ "\n"


#define GL_DEBUG_MARKER() {									    \
    GLenum ErrCode = glGetError();								\
    if(ErrCode != GL_NO_ERROR){								    \
        switch(ErrCode){										\
            case(GL_INVALID_ENUM):{								\
                OutputDebugStringA(GL_ERR_LOC(GL_INVALID_ENUM));    \
            }break;												\
            case(GL_INVALID_VALUE):{							\
                OutputDebugStringA(GL_ERR_LOC(GL_INVALID_VALUE));			\
            }break;												\
            case(GL_INVALID_OPERATION):{						\
                OutputDebugStringA(GL_ERR_LOC(GL_INVALID_OPERATION));				\
            }break;												\
            case(GL_STACK_OVERFLOW):{							\
                OutputDebugStringA(GL_ERR_LOC(GL_STACK_OVERFLOW));				\
            }break;												\
            case(GL_STACK_UNDERFLOW):{							\
                OutputDebugStringA(GL_ERR_LOC(GL_STACK_UNDERFLOW));				\
            }break;												\
            case(GL_OUT_OF_MEMORY):{							\
                OutputDebugStringA(GL_ERR_LOC(GL_OUT_OF_MEMORY));				\
            }break;												\
        }														\
    }															\
}

/*Windows non-specific*/
#define GL_NUM_EXTENSIONS                 0x821D

#define GL_TEXTURE0                       0x84C0
#define GL_TEXTURE1                       0x84C1
#define GL_TEXTURE2                       0x84C2
#define GL_TEXTURE3                       0x84C3

#define GL_DEBUG_SEVERITY_HIGH            0x9146
#define GL_DEBUG_SEVERITY_MEDIUM          0x9147
#define GL_DEBUG_SEVERITY_LOW             0x9148
#define GL_DEBUG_TYPE_MARKER              0x8268
#define GL_DEBUG_TYPE_PUSH_GROUP          0x8269
#define GL_DEBUG_TYPE_POP_GROUP           0x826A
#define GL_DEBUG_SEVERITY_NOTIFICATION    0x826B

#define GL_DEBUG_OUTPUT_SYNCHRONOUS       0x8242
#define GL_ARRAY_BUFFER                   0x8892
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_STREAM_DRAW                    0x88E0
#define GL_STREAM_READ                    0x88E1
#define GL_STREAM_COPY                    0x88E2
#define GL_STATIC_DRAW                    0x88E4
#define GL_STATIC_READ                    0x88E5
#define GL_STATIC_COPY                    0x88E6
#define GL_DYNAMIC_DRAW                   0x88E8
#define GL_DYNAMIC_READ                   0x88E9
#define GL_DYNAMIC_COPY                   0x88EA

#define GL_CLAMP_TO_EDGE                  0x812F

#define GL_FRAMEBUFFER_SRGB               0x8DB9
#define GL_SRGB8_ALPHA8                   0x8C43

#define GL_SHADING_LANGUAGE_VERSION       0x8B8C
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82
#define GL_VALIDATE_STATUS                0x8B83

#define GL_FRAMEBUFFER                    0x8D40
#define GL_READ_FRAMEBUFFER               0x8CA8
#define GL_DRAW_FRAMEBUFFER               0x8CA9
#define GL_COLOR_ATTACHMENT0              0x8CE0
#define GL_DEPTH_ATTACHMENT               0x8D00
#define GL_FRAMEBUFFER_COMPLETE           0x8CD5

#define GL_DEPTH_COMPONENT16              0x81A5
#define GL_DEPTH_COMPONENT24              0x81A6
#define GL_DEPTH_COMPONENT32              0x81A7
#define GL_DEPTH_COMPONENT32F             0x8CAC

#define GL_MULTISAMPLE                    0x809D
#define GL_SAMPLE_ALPHA_TO_COVERAGE       0x809E
#define GL_SAMPLE_ALPHA_TO_ONE            0x809F
#define GL_SAMPLE_COVERAGE                0x80A0
#define GL_SAMPLE_BUFFERS                 0x80A8
#define GL_SAMPLES                        0x80A9
#define GL_SAMPLE_COVERAGE_VALUE          0x80AA
#define GL_SAMPLE_COVERAGE_INVERT         0x80AB
#define GL_TEXTURE_2D_MULTISAMPLE         0x9100
#define GL_MAX_SAMPLES                    0x8D57
#define GL_MAX_COLOR_TEXTURE_SAMPLES      0x910E
#define GL_MAX_DEPTH_TEXTURE_SAMPLES      0x910F


/*Windows specific*/
#define WGL_CONTEXT_MAJOR_VERSION_ARB           0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB           0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB             0x2093
#define WGL_CONTEXT_FLAGS_ARB                   0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB            0x9126

#define WGL_CONTEXT_DEBUG_BIT_ARB               0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB  0x0002

#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB        0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002

GLOBAL_VARIABLE uint32 GlobalFramebufferCount = 0;
GLOBAL_VARIABLE GLuint GlobalFramebufferHandles[256] = {0};
GLOBAL_VARIABLE GLuint GlobalFramebufferTextures[256] = {0};

INTERNAL_FUNCTION GLuint
OpenGLCreateProgram(char* VertexCode, char* FragmentCode, opengl_program_common* Common){

    GLint VertexShaderCompiled = 0;
    GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLchar* VertexShaderCode[] = {
        VertexCode,
    };
    glShaderSource(VertexShaderID, ArrayCount(VertexShaderCode), VertexShaderCode, 0);
    glCompileShader(VertexShaderID);
    glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &VertexShaderCompiled);
    if(!VertexShaderCompiled){
        GLsizei Ignored;
        char VertexErrors[4096];
        glGetShaderInfoLog(VertexShaderID, sizeof(VertexErrors), &Ignored, VertexErrors);
        
        INVALID_CODE_PATH;
    }

    GLint FragmentShaderCompiled = 0;
    GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
    GLchar* FragmentShaderCode[] = {
        FragmentCode,
    };
    glShaderSource(FragmentShaderID, ArrayCount(FragmentShaderCode), FragmentShaderCode, 0);
    glCompileShader(FragmentShaderID);
    glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &FragmentShaderCompiled);
    if(!FragmentShaderCompiled){
        GLsizei Ignored;
        char FragmentErrors[4096];
        glGetShaderInfoLog(FragmentShaderID, sizeof(FragmentErrors), &Ignored, FragmentErrors);

        INVALID_CODE_PATH;
    }

    GLuint ProgramID = glCreateProgram();
    glAttachShader(ProgramID, VertexShaderID);
    glAttachShader(ProgramID, FragmentShaderID);
    glLinkProgram(ProgramID);

    //glValidateProgram(ProgramID);
    GLint ProgramLinked = false;
    glGetProgramiv(ProgramID, GL_LINK_STATUS, &ProgramLinked);
    if(!ProgramLinked){
        GLsizei Ignored;
        char ProgramErrors[4096];
        glGetShaderInfoLog(ProgramID, sizeof(ProgramErrors), &Ignored, ProgramErrors);

        INVALID_CODE_PATH;
    }

    Common->ProgramHandle = ProgramID;
    Common->VertPID = glGetAttribLocation(ProgramID, "VertPos");
    Common->VertNID = glGetAttribLocation(ProgramID, "VertNorm");
    Common->VertUVID = glGetAttribLocation(ProgramID, "VertUV");

    glDeleteShader(VertexShaderID);
    glDeleteShader(FragmentShaderID);

    return(ProgramID);
}

INTERNAL_FUNCTION void
OpenGLFreeProgram(opengl_program_common* Common){
    glDeleteProgram(Common->ProgramHandle);
    Common->ProgramHandle = 0;
}

INTERNAL_FUNCTION GLuint 
OpenGLLoadProgram(
    char* VertexPath, 
    char* FragmentPath, 
    opengl_program_common* Common)
{
    debug_read_file_result VertResult = OpenGL.DEBUGReadEntireFile(VertexPath);
    debug_read_file_result FragResult = OpenGL.DEBUGReadEntireFile(FragmentPath);

    if(VertResult.ContentsSize == 0){        
        INVALID_CODE_PATH;
    }

    if(FragResult.ContentsSize == 0){
        INVALID_CODE_PATH;
    }

    OpenGLCreateProgram(
        (char*)VertResult.Contents, 
        (char*)FragResult.Contents,
        Common);

    OpenGL.DEBUGFreeFileMemory(VertResult.Contents);
    OpenGL.DEBUGFreeFileMemory(FragResult.Contents);

    return(Common->ProgramHandle);
}

INTERNAL_FUNCTION void
OpenGLCompileVoxelShaderProgram(voxel_shader_program* Result)
{
    GLuint Prog = OpenGLLoadProgram(
        "../Data/Shaders/voxel_shader.vert",
        "../Data/Shaders/voxel_shader.frag",
        &Result->Common);

    Result->ProjectionLoc = glGetUniformLocation(Prog, "Projection");
    Result->ViewLoc = glGetUniformLocation(Prog, "View");
    Result->ModelLoc = glGetUniformLocation(Prog, "Model");

    Result->ViewPositionLoc = glGetUniformLocation(Prog, "ViewPosition");
    Result->DiffuseMapLoc = glGetUniformLocation(Prog, "DiffuseMap");
    Result->DirLightDirectionLoc = glGetUniformLocation(Prog, "DirLight.Direction");
    Result->DirLightDiffuseLoc = glGetUniformLocation(Prog, "DirLight.Diffuse");
    Result->DirLightAmbientLoc = glGetUniformLocation(Prog, "DirLight.Ambient");
}

INTERNAL_FUNCTION bool32 
IsValidArray(GLuint ArrayID){
    bool32 Result = false;

    if(ArrayID != -1){
        Result = true;
    }

    return(Result);
}

INTERNAL_FUNCTION void 
UseProgramBegin(opengl_program_common* Program){
    glUseProgram(Program->ProgramHandle);

#if 0
    GLuint PArray = Program->VertPID;
    GLuint NArray = Program->VertNID;
    GLuint UVArray = Program->VertUVID;

    if(IsValidArray(PArray)){
        glEnableVertexAttribArray(PArray);
        glVertexAttribPointer(
            PArray, 
            3, 
            GL_FLOAT, 
            false, 
            sizeof(textured_vertex),
            (void*)OffsetOf(textured_vertex, P));        
    }

    if(IsValidArray(NArray)){
        glEnableVertexAttribArray(NArray);
        glVertexAttribPointer(
            NArray,
            3,
            GL_FLOAT,
            false,
            sizeof(textured_vertex),
            (void*)OffsetOf(textured_vertex, N));
    }

    if(IsValidArray(UVArray)){
        glEnableVertexAttribArray(UVArray);
        glVertexAttribPointer(
            UVArray, 
            2,
            GL_FLOAT,
            false,
            sizeof(textured_vertex),
            (void*)OffsetOf(textured_vertex, UV));
    }
#endif
}

INTERNAL_FUNCTION void 
UseProgramEnd(opengl_program_common* Program){
    glUseProgram(0);

#if 0
    GLuint PArray = Program->VertPID;
    GLuint NArray = Program->VertNID;
    GLuint UVArray = Program->VertUVID;

    if(IsValidArray(PArray)){
        glDisableVertexAttribArray(PArray);
    }

    if(IsValidArray(NArray)){
        glDisableVertexAttribArray(NArray);
    }

    if(IsValidArray(UVArray)){
        glDisableVertexAttribArray(UVArray);
    }
#endif
}

INTERNAL_FUNCTION void 
UseProgramBegin(voxel_shader_program* Program, render_setup* Setup, mat4 ModelTransform, uint32 AtlasTextureGLId)
{
    UseProgramBegin(&Program->Common);

    glUniformMatrix4fv(Program->ProjectionLoc, 1, GL_TRUE, Setup->Projection.E);
    glUniformMatrix4fv(Program->ViewLoc, 1, GL_TRUE, Setup->CameraTransform.E);
    glUniformMatrix4fv(Program->ModelLoc, 1, GL_TRUE, ModelTransform.E);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, AtlasTextureGLId);
    glUniform1i(Program->DiffuseMapLoc, 0);
    glUniform3fv(Program->ViewPositionLoc, 1, Setup->CameraP.E);
    glUniform3fv(Program->DirLightDirectionLoc, 1, Setup->DirLightDirection.E);
    glUniform3fv(Program->DirLightDiffuseLoc, 1, Setup->DirLightDiffuse.E);
    glUniform3fv(Program->DirLightAmbientLoc, 1, Setup->DirLightAmbient.E);
}

INTERNAL_FUNCTION void
UseProgramEnd(voxel_shader_program* Program){
    UseProgramEnd(&Program->Common);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

INTERNAL_FUNCTION void 
OpenGLBindFramebuffer(uint32 TargetIndex, uint32 RenderWidth, uint32 RenderHeight){
    glBindFramebuffer(GL_FRAMEBUFFER, GlobalFramebufferHandles[TargetIndex]);
    glViewport(0, 0, RenderWidth, RenderHeight);
}

inline void OpenGLRenderRectangle(
    vec2 MinP,
    vec2 MaxP,
    vec4 PremultColor,
    vec2 MinUV = Vec2(0.0f, 0.0f),
    vec2 MaxUV = Vec2(1.0f, 1.0f))
{
    GL_DEBUG_MARKER();
    
    glBegin(GL_TRIANGLES);

    glColor4f(PremultColor.r, PremultColor.g, PremultColor.b, PremultColor.a);

    /*Upper triangle*/
    glTexCoord2f(MinUV.x, MaxUV.y);
    glVertex2f(MinP.x, MaxP.y);
    glTexCoord2f(MaxUV.x, MaxUV.y);
    glVertex2f(MaxP.x, MaxP.y);
    glTexCoord2f(MaxUV.x, MinUV.y);
    glVertex2f(MaxP.x, MinP.y);

    /*Lower triangle*/
    glTexCoord2f(MinUV.x, MaxUV.y);
    glVertex2f(MinP.x, MaxP.y);
    glTexCoord2f(MaxUV.x, MinUV.y);
    glVertex2f(MaxP.x, MinP.y);
    glTexCoord2f(MinUV.x, MinUV.y);
    glVertex2f(MinP.x, MinP.y);

    glEnd();

    GL_DEBUG_MARKER();
}

inline void OpenGLSetScreenSpace(int Width, int Height){
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);

    float a = SafeRatio1(2.0f, (float)Width);
    float b = SafeRatio1(2.0f, (float)Height);
    
    float ProjectionMatrix[] =
    {
         a,  0,  0,  0,
         0,  -b,  0,  0,
         0,  0,  1,  0,
        -1, 1,  0,  1,
    };
    glLoadMatrixf(ProjectionMatrix);
}

inline void OpenGLRenderBitmap(
    int Width, 
    int Height,
    void* Memory,
    rectangle2i DrawRegion,
    vec4 ClearColor,
    GLuint BlitTexture)
{
    OpenGLBindFramebuffer(0, GetWidth(DrawRegion), GetHeight(DrawRegion));
    //glViewport(0, 0, Width, Height);

    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);

    glBindTexture(GL_TEXTURE_2D, BlitTexture);

    glTexImage2D(
        GL_TEXTURE_2D, 
        0, 
        GL_SRGB8_ALPHA8, 
        //GL_RGBA8,
        Width, 
        Height,
        0, 
        GL_BGRA_EXT, 
        //GL_RGBA,
        GL_UNSIGNED_BYTE, 
        Memory);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glEnable(GL_TEXTURE_2D);

    glClearColor(ClearColor.r, ClearColor.g, ClearColor.b, ClearColor.a);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);

    float a = SafeRatio1(2.0f, (float)Width);
    float b = SafeRatio1(2.0f, (float)Height);
    
    float ProjectionMatrix[] =
    {
         a,  0,  0,  0,
         0,  -b,  0,  0,
         0,  0,  1,  0,
        -1, 1,  0,  1,
    };
    glLoadMatrixf(ProjectionMatrix);

    vec2 MinP = {0, 0};
    vec2 MaxP = {(float)Width, (float)Height};
    vec4 Color = {1.0f, 1.0f, 1.0f, 1.0f};

    OpenGLRenderRectangle(MinP, MaxP, Color);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    glEnable(GL_BLEND);
}

INTERNAL_FUNCTION opengl_info OpenGLGetInfo(bool32 ModernContext){
    opengl_info Result = {};

    Result.ModernContext = ModernContext;
    Result.Vendor = (char*)glGetString(GL_VENDOR);
    Result.Renderer = (char*)glGetString(GL_RENDERER);
    Result.Version = (char*)glGetString(GL_VERSION);

    if(Result.ModernContext){
        Result.ShadingLanguageVersion = (char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
    }
    else{
        Result.ShadingLanguageVersion = "(none)";
    }
    //Result.Extensions = (char*)glGetString(GL_EXTENSIONS);

    if(glGetStringi){
        GLint ExtensionCount = 0;
        glGetIntegerv(GL_NUM_EXTENSIONS, &ExtensionCount);
        for(GLint ExtensionIndex = 0;
            ExtensionIndex < ExtensionCount;
            ExtensionIndex++)
        {
            char* ExtName = (char*)glGetStringi(GL_EXTENSIONS, ExtensionIndex);

            if(0){}
            else if(StringsAreEqual(ExtName, "GL_EXT_texture_sRGB")){Result.GL_EXT_texture_sRGB = true;}
            else if(StringsAreEqual(ExtName, "GL_EXT_framebuffer_sRGB")){Result.GL_EXT_framebuffer_sRGB = true;}
            else if(StringsAreEqual(ExtName, "GL_ARB_framebuffer_sRGB")){Result.GL_EXT_framebuffer_sRGB = true;}
            else if(StringsAreEqual(ExtName, "GL_ARB_framebuffer_object")){Result.GL_ARB_framebuffer_object = true;}
            else if(StringsAreEqual(ExtName, "GL_ARB_compatibility")){
                int a = 1;
            }
        }
    }

    char* MajorAt = Result.Version;
    char* MinorAt = 0;

    for(char* At = Result.Version;
        *At;
        At++)
    {
        if(At[0] == '.'){
            MinorAt = At + 1;
            break;
        }
    }

    int Major = 1;
    int Minor = 0;
    if(MinorAt){
        Major = IntFromString(MajorAt);
        Minor = IntFromString(MinorAt);
    }

    if((Major > 2) || ((Major == 2) && (Minor >= 1))){
        Result.GL_EXT_texture_sRGB = true;
    }

    return(Result);
}


INTERNAL_FUNCTION void* OpenGLAllocateTexture(uint32 Width, uint32 Height, void* Data, uint32 Flags)
{
    GLuint FilterMode = GL_LINEAR;
    if(Flags & AllocateTexture_FilterNearest){
        FilterMode = GL_NEAREST;
    }

    GLuint Handle;
    glGenTextures(1, &Handle);
    glBindTexture(GL_TEXTURE_2D, Handle);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        OpenGL.DefaultSpriteTextureFormat,
        Width, 
        Height,
        0,
        GL_BGRA_EXT, 
        GL_UNSIGNED_BYTE, 
        Data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, FilterMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, FilterMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    glFlush();

    Assert(sizeof(Handle) <= sizeof(void*));
    return(POINTER_FROM_UINT32(void, Handle));
}

INTERNAL_FUNCTION void
OpenGLManageTextures(texture_op* First){
    for(texture_op* Op = First; Op; Op = Op->Next){
        if(Op->IsAllocate){
            *Op->Allocate.ResultHandle = OpenGLAllocateTexture(
                Op->Allocate.Width,
                Op->Allocate.Height,
                Op->Allocate.Data,
                Op->Allocate.Flags);
        }
        else{
            GLuint Handle = UINT32_FROM_POINTER(Op->Deallocate.Handle);
            glDeleteTextures(1, &Handle);
        }
    }
}

INTERNAL_FUNCTION void 
OpenGLInit(
    opengl_info Info, 
    bool32 FramebufferSupportsSRGB,
    debug_platform_read_entire_file* DEBUGReadEntireFile,
    debug_platform_free_file_memory* DEBUGFreeFileMemory)
{

    GL_DEBUG_MARKER();

    OpenGL.ShaderSimTexReadSRGB = true;
    OpenGL.ShaderSimTexWriteSRGB = true;

    OpenGL.DEBUGReadEntireFile = DEBUGReadEntireFile;
    OpenGL.DEBUGFreeFileMemory = DEBUGFreeFileMemory;

    glGenTextures(1, &OpenGL.ReservedBlitTexture);

    GL_DEBUG_MARKER();

    glGetIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &OpenGL.MaxMultiSampleCount);
    if(OpenGL.MaxMultiSampleCount > 16){
        OpenGL.MaxMultiSampleCount = 16;
    }

    OpenGL.DefaultSpriteTextureFormat = GL_RGBA8;
    OpenGL.DefaultFramebufferTextureFormat = GL_RGBA8;

    if(Info.GL_EXT_texture_sRGB){
        OpenGL.DefaultSpriteTextureFormat = GL_SRGB8_ALPHA8;
        OpenGL.ShaderSimTexReadSRGB = false;
    }

    GL_DEBUG_MARKER();
    if(FramebufferSupportsSRGB && Info.GL_EXT_framebuffer_sRGB){
        GLuint TestTexture;
        glGenTextures(1, &TestTexture);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, TestTexture);
        
        GL_DEBUG_MARKER();

        glTexImage2DMultisample(
            GL_TEXTURE_2D_MULTISAMPLE, 
            OpenGL.MaxMultiSampleCount,
            GL_SRGB8_ALPHA8,
            1920, 1080,
            GL_FALSE);

        GL_DEBUG_MARKER();

        if(glGetError() == GL_NO_ERROR){
            OpenGL.DefaultFramebufferTextureFormat = GL_SRGB8_ALPHA8;
            glEnable(GL_FRAMEBUFFER_SRGB);
            OpenGL.ShaderSimTexWriteSRGB = false;
        }

        glDeleteTextures(1, &TestTexture);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
    }

    GL_DEBUG_MARKER();

    for(int j = 0; j < 4; j++){
        for(int i = 0; i < 4; i++){
            OpenGL.White[j][i] = 0xFFFFFFFF;
        }
    }

    OpenGL.WhiteBitmap.Memory = OpenGL.White;
    OpenGL.WhiteBitmap.AlignPercentage = Vec2(0.5f, 0.5f);
    OpenGL.WhiteBitmap.WidthOverHeight = 1.0f;
    OpenGL.WhiteBitmap.Width = 4;
    OpenGL.WhiteBitmap.Height = 4;
    OpenGL.WhiteBitmap.TextureHandle = OpenGLAllocateTexture(1, 1, &OpenGL.White, 0);

    OpenGLFreeProgram(&OpenGL.VoxelShaderProgram.Common);
    OpenGLCompileVoxelShaderProgram(&OpenGL.VoxelShaderProgram);

    GL_DEBUG_MARKER();
}

INTERNAL_FUNCTION void
OpenGLGroupToOutput(
    game_render_commands* Commands, 
    rectangle2i DrawRegion, 
    int32 Width, int32 Height)
{
    glViewport(0, 0, GetWidth(DrawRegion), GetHeight(DrawRegion));

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    OpenGLSetScreenSpace(Width, Height);

    for(uint8* HeaderAt = Commands->PushBufferBase;
        HeaderAt < Commands->PushBufferDataAt;
        HeaderAt += sizeof(render_group_entry_header))
    {
        render_group_entry_header* Header = (render_group_entry_header*)HeaderAt;
        void* EntryData = (uint8*)Header + sizeof(*Header);

        switch(Header->Type){
            case RenderGroupEntry_render_entry_clear:{
                render_entry_clear* EntryClear = (render_entry_clear*)EntryData;

                /*Check if it is premultiplied and correct for our framebuffer*/
                glClearColor(
                    EntryClear->Color.x,
                    EntryClear->Color.y,
                    EntryClear->Color.z,
                    EntryClear->Color.w);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                HeaderAt += sizeof(*EntryClear);
            }break;
            case RenderGroupEntry_render_entry_rectangle:{
                render_entry_rectangle* EntryRect = (render_entry_rectangle*)EntryData;

                glDisable(GL_DEPTH_TEST);
                glDisable(GL_TEXTURE_2D);
                OpenGLRenderRectangle(
                    EntryRect->P, 
                    EntryRect->P + EntryRect->Dim,
                    EntryRect->Color);
                glEnable(GL_TEXTURE_2D);
                glEnable(GL_DEPTH_TEST);

                HeaderAt += sizeof(*EntryRect);
            }break;
            case RenderGroupEntry_render_entry_bitmap:{
                render_entry_bitmap* EntryBitmap = (render_entry_bitmap*)EntryData;

                glDisable(GL_DEPTH_TEST);
                if(EntryBitmap->Bitmap->Width && EntryBitmap->Bitmap->Height){
                    
                    vec2 XAxis = { 1.0f, 0.0f };
                    vec2 YAxis = { 0.0f, 1.0f };

                    XAxis *= EntryBitmap->Size.x;
                    YAxis *= EntryBitmap->Size.y;

                    vec2 MinXMinY = EntryBitmap->P;
                    vec2 MinXMaxY = EntryBitmap->P + YAxis;
                    vec2 MaxXMinY = EntryBitmap->P + XAxis;
                    vec2 MaxXMaxY = EntryBitmap->P + YAxis + XAxis;

                    GLuint TextureToBind = (GLuint)UINT32_FROM_POINTER(EntryBitmap->Bitmap->TextureHandle);
                    glBindTexture(GL_TEXTURE_2D, TextureToBind);

                    float TexelDeltaU = 1.0f / (float)EntryBitmap->Bitmap->Width;
                    float TexelDeltaV = 1.0f / (float)EntryBitmap->Bitmap->Height;

                    vec2 MinUV = Vec2(TexelDeltaU, TexelDeltaV);
                    vec2 MaxUV = Vec2(1.0f - TexelDeltaU, 1.0f - TexelDeltaV);

                    glBegin(GL_TRIANGLES);
                    glColor4fv(EntryBitmap->Color.E);

                    glTexCoord2f(MinUV.x, MaxUV.y);
                    glVertex2fv(MinXMaxY.E);
                    glTexCoord2f(MaxUV.x, MaxUV.y);
                    glVertex2fv(MaxXMaxY.E);
                    glTexCoord2f(MaxUV.x, MinUV.y);
                    glVertex2fv(MaxXMinY.E);

                    glTexCoord2f(MinUV.x, MaxUV.y);
                    glVertex2fv(MinXMaxY.E);
                    glTexCoord2f(MaxUV.x, MinUV.y);
                    glVertex2fv(MaxXMinY.E);
                    glTexCoord2f(MinUV.x, MinUV.y);
                    glVertex2fv(MinXMinY.E);

                    glEnd();
                }
                glEnable(GL_DEPTH_TEST);

                HeaderAt += sizeof(*EntryBitmap);
            }break;

            case RenderGroupEntry_render_entry_coordinate_system:{
                render_entry_coordinate_system* EntryCS = (render_entry_coordinate_system*)EntryData;

                HeaderAt += sizeof(*EntryCS);
            }break;

            case RenderGroupEntry_render_entry_voxel_mesh:{
                render_entry_voxel_mesh* EntryVoxelMesh = (render_entry_voxel_mesh*)EntryData;

#if 1
                voxel_shader_program* Program = &OpenGL.VoxelShaderProgram;
                opengl_program_common* Common = &Program->Common;

                render_setup* Setup = &EntryVoxelMesh->Setup;
                mat4 ModelTransform = Translate(Identity(), EntryVoxelMesh->P);
                
                uint32 TextureToBind = 0;
                if(EntryVoxelMesh->Bitmap){
                    TextureToBind = UINT32_FROM_POINTER(EntryVoxelMesh->Bitmap->TextureHandle);
                }

                GL_DEBUG_MARKER();
#if 1
                GLuint MeshVAO;
                GLuint MeshVertsVBO;
                GLuint MeshNormsVBO;
                GLuint MeshUVsVBO;
                GLuint MeshEBO;

                glGenVertexArrays(1, &MeshVAO);
                glGenBuffers(1, &MeshEBO);
                glGenBuffers(1, &MeshVertsVBO);
                glGenBuffers(1, &MeshNormsVBO);
                glGenBuffers(1, &MeshUVsVBO);

                glBindVertexArray(MeshVAO);

                glBindBuffer(GL_ARRAY_BUFFER, MeshVertsVBO);
                glBufferData(GL_ARRAY_BUFFER, 
                    EntryVoxelMesh->Mesh->VerticesCount * sizeof(vec3), 
                    EntryVoxelMesh->Mesh->Positions, GL_DYNAMIC_DRAW);

                glBindBuffer(GL_ARRAY_BUFFER, MeshNormsVBO);
                glBufferData(GL_ARRAY_BUFFER, 
                    EntryVoxelMesh->Mesh->VerticesCount * sizeof(vec3), 
                    EntryVoxelMesh->Mesh->Normals, GL_DYNAMIC_DRAW);

                glBindBuffer(GL_ARRAY_BUFFER, MeshUVsVBO);
                glBufferData(GL_ARRAY_BUFFER, 
                    EntryVoxelMesh->Mesh->VerticesCount * sizeof(vec2), 
                    EntryVoxelMesh->Mesh->TexCoords, GL_DYNAMIC_DRAW);
                
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, MeshEBO);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
                    EntryVoxelMesh->Mesh->IndicesCount * sizeof(uint32_t), 
                    EntryVoxelMesh->Mesh->Indices, GL_DYNAMIC_DRAW);

                GL_DEBUG_MARKER();

                if(IsValidArray(Common->VertPID)){
                    glBindBuffer(GL_ARRAY_BUFFER, MeshVertsVBO);
                    glEnableVertexAttribArray(Common->VertPID);
                    glVertexAttribPointer(Common->VertPID, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GL_FLOAT), 0);
                }

                if(IsValidArray(Common->VertNID)){
                    glBindBuffer(GL_ARRAY_BUFFER, MeshNormsVBO);
                    glEnableVertexAttribArray(Common->VertNID);
                    glVertexAttribPointer(Common->VertNID, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GL_FLOAT), 0);
                }

                if(IsValidArray(Common->VertUVID)){
                    glBindBuffer(GL_ARRAY_BUFFER, MeshUVsVBO);
                    glEnableVertexAttribArray(Common->VertUVID);
                    glVertexAttribPointer(Common->VertUVID, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GL_FLOAT), 0);  
                }

                GL_DEBUG_MARKER();
#else
                GLuint MeshVAO;
                GLuint MeshEBO;
                GLuint MeshVBO;

                glGenVertexArrays(1, &MeshVAO);
                glGenBuffers(1, &MeshEBO);
                glGenBuffers(1, &MeshVBO);

                glBindBuffer(GL_ARRAY_BUFFER, MeshVertsVBO);

                glEnableVertexAttribArray(Program.VertPID);
                glVertexAttribPointer(Program.VertPID, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GL_FLOAT), OffsetOf(textured_vertex, P));

                glEnableVertexAttribArray(Program.VertNID);
                glVertexAttribPointer(Program.VertNID, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GL_FLOAT), OffsetOf(textured_vertex, N));

                glEnableVertexAttribArray(Program.VertUVID);
                glVertexAttribPointer(Program.VertUVID, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GL_FLOAT), OffsetOf(textured_vertex, UV));
#endif

                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glBindVertexArray(0);

                UseProgramBegin(Program, Setup, ModelTransform, TextureToBind);
                
                glBindVertexArray(MeshVAO);

                voxel_chunk_mesh* Mesh = EntryVoxelMesh->Mesh;
                glDrawElements(
                    GL_TRIANGLES, 
                    Mesh->IndicesCount, 
                    GL_UNSIGNED_INT, 
                    0);

                UseProgramEnd(&Program->Common);

                glDeleteVertexArrays(1, &MeshVAO);
                glDeleteBuffers(1, &MeshVertsVBO);
                glDeleteBuffers(1, &MeshNormsVBO);
                glDeleteBuffers(1, &MeshUVsVBO);
                glDeleteBuffers(1, &MeshEBO);

                glBindVertexArray(0);
#endif

                HeaderAt += sizeof(*EntryVoxelMesh);
            }break;
            
            INVALID_DEFAULT_CASE;
        }
    }
}