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
#define GL_TEXTURE4                       0x84C4
#define GL_TEXTURE5                       0x84C5
#define GL_TEXTURE6                       0x84C6
#define GL_TEXTURE7                       0x84C7

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
#define GL_COLOR_ATTACHMENT1              0x8CE1
#define GL_COLOR_ATTACHMENT2              0x8CE2
#define GL_COLOR_ATTACHMENT3              0x8CE3
#define GL_COLOR_ATTACHMENT4              0x8CE4
#define GL_COLOR_ATTACHMENT5              0x8CE5
#define GL_COLOR_ATTACHMENT6              0x8CE6
#define GL_COLOR_ATTACHMENT7              0x8CE7
#define GL_COLOR_ATTACHMENT8              0x8CE8
#define GL_COLOR_ATTACHMENT9              0x8CE9
#define GL_COLOR_ATTACHMENT10             0x8CEA
#define GL_COLOR_ATTACHMENT11             0x8CEB
#define GL_COLOR_ATTACHMENT12             0x8CEC
#define GL_COLOR_ATTACHMENT13             0x8CED
#define GL_COLOR_ATTACHMENT14             0x8CEE
#define GL_COLOR_ATTACHMENT15             0x8CEF
#define GL_COLOR_ATTACHMENT16             0x8CF0
#define GL_COLOR_ATTACHMENT17             0x8CF1
#define GL_COLOR_ATTACHMENT18             0x8CF2
#define GL_COLOR_ATTACHMENT19             0x8CF3
#define GL_COLOR_ATTACHMENT20             0x8CF4
#define GL_COLOR_ATTACHMENT21             0x8CF5
#define GL_COLOR_ATTACHMENT22             0x8CF6
#define GL_COLOR_ATTACHMENT23             0x8CF7
#define GL_COLOR_ATTACHMENT24             0x8CF8
#define GL_COLOR_ATTACHMENT25             0x8CF9
#define GL_COLOR_ATTACHMENT26             0x8CFA
#define GL_COLOR_ATTACHMENT27             0x8CFB
#define GL_COLOR_ATTACHMENT28             0x8CFC
#define GL_COLOR_ATTACHMENT29             0x8CFD
#define GL_COLOR_ATTACHMENT30             0x8CFE
#define GL_COLOR_ATTACHMENT31             0x8CFF
#define GL_DEPTH_ATTACHMENT               0x8D00
#define GL_FRAMEBUFFER_COMPLETE           0x8CD5

#define GL_DEPTH_COMPONENT16              0x81A5
#define GL_DEPTH_COMPONENT24              0x81A6
#define GL_DEPTH_COMPONENT32              0x81A7
#define GL_DEPTH_COMPONENT32F             0x8CAC

#define GL_RGBA32F                        0x8814
#define GL_RGB32F                         0x8815
#define GL_RGBA16F                        0x881A
#define GL_RGB16F                         0x881B
#define GL_R8                             0x8229
#define GL_R16                            0x822A
#define GL_RG8                            0x822B
#define GL_RG16                           0x822C
#define GL_R16F                           0x822D
#define GL_R32F                           0x822E
#define GL_RG16F                          0x822F
#define GL_RG32F                          0x8230
#define GL_R8I                            0x8231
#define GL_R8UI                           0x8232
#define GL_R16I                           0x8233
#define GL_R16UI                          0x8234
#define GL_R32I                           0x8235
#define GL_R32UI                          0x8236
#define GL_RG8I                           0x8237
#define GL_RG8UI                          0x8238
#define GL_RG16I                          0x8239
#define GL_RG16UI                         0x823A
#define GL_RG32I                          0x823B
#define GL_RG32UI                         0x823C

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
    Common->VertPID = glGetAttribLocation(ProgramID, "P");
    Common->VertNID = glGetAttribLocation(ProgramID, "N");
    Common->VertUVID = glGetAttribLocation(ProgramID, "UV");
    Common->PUVNID = glGetAttribLocation(ProgramID, "VertexData");

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
    Result->OneTextureWidthLoc = glGetUniformLocation(Prog, "OneTextureWidth");

    Result->ViewPositionLoc = glGetUniformLocation(Prog, "ViewPosition");
    Result->DiffuseMapLoc = glGetUniformLocation(Prog, "DiffuseMap");
    Result->DirLightDirectionLoc = glGetUniformLocation(Prog, "DirLight.Direction");
    Result->DirLightDiffuseLoc = glGetUniformLocation(Prog, "DirLight.Diffuse");
    Result->DirLightAmbientLoc = glGetUniformLocation(Prog, "DirLight.Ambient");
}

INTERNAL_FUNCTION void
OpenGLCompileMeshShaderProgram(mesh_shader_program* Result){
    GLuint Prog = OpenGLLoadProgram(
        "../Data/Shaders/mesh.vert",
        "../Data/Shaders/mesh.frag",
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
}

INTERNAL_FUNCTION void 
UseProgramEnd(opengl_program_common* Program){
    glUseProgram(0);
}

INTERNAL_FUNCTION void 
UseProgramBegin(
    voxel_shader_program* Program, 
    render_setup* Setup, 
    mat4 ModelTransform, 
    uint32 AtlasTextureGLId,
    uint32 OneTextureWidth)
{
    UseProgramBegin(&Program->Common);

    glUniformMatrix4fv(Program->ProjectionLoc, 1, GL_TRUE, Setup->Projection.E);
    glUniformMatrix4fv(Program->ViewLoc, 1, GL_TRUE, Setup->CameraTransform.E);
    glUniformMatrix4fv(Program->ModelLoc, 1, GL_TRUE, ModelTransform.E);
    glUniform1i(Program->OneTextureWidthLoc, OneTextureWidth);

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
UseProgramBegin(
    mesh_shader_program* Program, 
    render_setup* Setup, 
    mat4 ModelTransform) 
{
    UseProgramBegin(&Program->Common);

    glUniformMatrix4fv(Program->ProjectionLoc, 1, GL_TRUE, Setup->Projection.E);
    glUniformMatrix4fv(Program->ViewLoc, 1, GL_TRUE, Setup->CameraTransform.E);
    glUniformMatrix4fv(Program->ModelLoc, 1, GL_TRUE, ModelTransform.E);

    //glActiveTexture(GL_TEXTURE0);
    //glBindTexture(GL_TEXTURE_2D, AtlasTextureGLId);
    //glUniform1i(Program->DiffuseMapLoc, 0);
    glUniform3fv(Program->ViewPositionLoc, 1, Setup->CameraP.E);
    glUniform3fv(Program->DirLightDirectionLoc, 1, Setup->DirLightDirection.E);
    glUniform3fv(Program->DirLightDiffuseLoc, 1, Setup->DirLightDiffuse.E);
    glUniform3fv(Program->DirLightAmbientLoc, 1, Setup->DirLightAmbient.E);
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
        Width, 
        Height,
        0, 
        GL_BGRA_EXT, 
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

#define ALLOW_GPU_SRGB 1

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

#if ALLOW_GPU_SRGB
    if(Info.GL_EXT_texture_sRGB){
        OpenGL.DefaultSpriteTextureFormat = GL_SRGB8_ALPHA8;
        OpenGL.ShaderSimTexReadSRGB = false;
    }

    GL_DEBUG_MARKER();
    if(FramebufferSupportsSRGB && Info.GL_EXT_framebuffer_sRGB){
#if 0
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

        if(glGetError() == GL_NO_ERROR)
#else
        {
            OpenGL.DefaultFramebufferTextureFormat = GL_SRGB8_ALPHA8;
            glEnable(GL_FRAMEBUFFER_SRGB);
            OpenGL.ShaderSimTexWriteSRGB = false;
        }
#endif

#if 0
        glDeleteTextures(1, &TestTexture);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
#endif
    }
#endif

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

    OpenGLFreeProgram(&OpenGL.MeshShaderProgram.Common);
    OpenGLCompileMeshShaderProgram(&OpenGL.MeshShaderProgram);

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

            case RenderGroupEntry_render_entry_mesh:{
#if 1
                render_entry_mesh* EntryMesh = (render_entry_mesh*)EntryData;

                mesh_shader_program* Program = &OpenGL.MeshShaderProgram;
                opengl_program_common* Common = &Program->Common;
                loaded_mesh* Mesh = EntryMesh->Mesh;
                render_setup* Setup = &EntryMesh->Setup;

                mat4 ModelTransform = Translate(Identity(), EntryMesh->P);
                

                GLfloat CubeVertices[] = {
                    /*P N UV*/
                    //NOTE(Dima): Front side
                    -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
                    0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
                    0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
                    -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                    //NOTE(Dima): Top side
                    -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
                    0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
                    0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
                    -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
                    //NOTE(Dima): Right side
                    0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
                    0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
                    0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                    0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                    //NOTE(Dima): Left side
                    -0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
                    -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
                    -0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                    -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                    //NOTE(Dima): Back side
                    0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,
                    -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
                    -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,
                    0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
                    //NOTE(Dima): Down side
                    -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,
                    0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,
                    0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
                    -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f
                };

                GLuint CubeIndices[] = {
                    0, 1, 2,
                    0, 2, 3,

                    4, 5, 6,
                    4, 6, 7,

                    8, 9, 10,
                    8, 10, 11,

                    12, 13, 14,
                    12, 14, 15,

                    16, 17, 18,
                    16, 18, 19,

                    20, 21, 22,
                    20, 22, 23
                };

                GLuint VAO;
                GLuint VBO;
                GLuint EBO;

                glGenVertexArrays(1, &VAO);
                glGenBuffers(1, &VBO);
                glGenBuffers(1, &EBO);

                glBindVertexArray(VAO);

                glBindBuffer(GL_ARRAY_BUFFER, VBO);
                glBufferData(GL_ARRAY_BUFFER, sizeof(CubeVertices), CubeVertices, GL_DYNAMIC_DRAW);

                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(CubeIndices), CubeIndices, GL_DYNAMIC_DRAW);

                if(IsValidArray(Common->VertPID)){
                    glEnableVertexAttribArray(Common->VertPID);
                    //TODO(Dima): Change last param for actual mesh
                    glVertexAttribPointer(Common->VertPID, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);
                }

                if(IsValidArray(Common->VertUVID)){
                    glEnableVertexAttribArray(Common->VertUVID);
                    glVertexAttribPointer(Common->VertUVID, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (GLvoid*)(6 * sizeof(GLfloat)));
                }

                if(IsValidArray(Common->VertNID)){
                    glEnableVertexAttribArray(Common->VertNID);
                    glVertexAttribPointer(Common->VertNID, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (GLvoid*)(3 * sizeof(GLfloat)));
                }
                
                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glBindVertexArray(0);

                UseProgramBegin(Program, Setup, ModelTransform);
                glBindVertexArray(VAO);

                //TODO(Dima): Change indices count;
                glDrawArrays(GL_TRIANGLES, 0, ArrayCount(CubeIndices));

                glBindVertexArray(0);
                UseProgramEnd(&Program->Common);

                glDeleteVertexArrays(1, &VAO);
                glDeleteBuffers(1, &VBO);
                glDeleteBuffers(1, &EBO);
#endif
            }break;

            case RenderGroupEntry_render_entry_voxel_mesh:{
                render_entry_voxel_mesh* EntryVoxelMesh = (render_entry_voxel_mesh*)EntryData;

                voxel_shader_program* Program = &OpenGL.VoxelShaderProgram;
                opengl_program_common* Common = &Program->Common;
                voxel_chunk_mesh* Mesh = EntryVoxelMesh->Mesh;

                render_setup* Setup = &EntryVoxelMesh->Setup;
                int32 OneTextureWidth = EntryVoxelMesh->OneTextureWidth;
                mat4 ModelTransform = Translate(Identity(), EntryVoxelMesh->P);
                
                uint32 TextureToBind = 0;
                if(EntryVoxelMesh->Bitmap){
                    TextureToBind = UINT32_FROM_POINTER(EntryVoxelMesh->Bitmap->TextureHandle);
                }

                GL_DEBUG_MARKER();

                GLuint MeshVAO;
                GLuint MeshVBO;

                glGenVertexArrays(1, &MeshVAO);
                glGenBuffers(1, &MeshVBO);

                glBindVertexArray(MeshVAO);

                glBindBuffer(GL_ARRAY_BUFFER, MeshVBO);
                glBufferData(GL_ARRAY_BUFFER,
                    Mesh->VerticesCount * sizeof(uint32),
                    Mesh->PUVN, GL_DYNAMIC_DRAW);

                if(IsValidArray(Common->PUVNID)){
                    glEnableVertexAttribArray(Common->PUVNID);
                    glVertexAttribPointer(Common->PUVNID, 1, GL_FLOAT, GL_FALSE, 4, 0);
                }

                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glBindVertexArray(0);

                GL_DEBUG_MARKER();

                UseProgramBegin(Program, Setup, ModelTransform, TextureToBind, OneTextureWidth);
                
                glBindVertexArray(MeshVAO);

                glDrawArrays(GL_TRIANGLES, 0, Mesh->VerticesCount); 

                UseProgramEnd(&Program->Common);

                glDeleteVertexArrays(1, &MeshVAO);
                glDeleteBuffers(1, &MeshVBO);

                glBindVertexArray(0);

                HeaderAt += sizeof(*EntryVoxelMesh);
            }break;
            
            INVALID_DEFAULT_CASE;
        }
    }
}