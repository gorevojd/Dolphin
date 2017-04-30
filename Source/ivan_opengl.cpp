#include "ivan_render_group.h"

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

#define GL_DEBUG_SEVERITY_HIGH            0x9146
#define GL_DEBUG_SEVERITY_MEDIUM          0x9147
#define GL_DEBUG_SEVERITY_LOW             0x9148
#define GL_DEBUG_TYPE_MARKER              0x8268
#define GL_DEBUG_TYPE_PUSH_GROUP          0x8269
#define GL_DEBUG_TYPE_POP_GROUP           0x826A
#define GL_DEBUG_SEVERITY_NOTIFICATION    0x826B

#define GL_DEBUG_OUTPUT_SYNCHRONOUS       0x8242
#define GL_ARRAY_BUFFER                   0x8892
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
// NOTE(casey): Windows-specific
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

INTERNAL_FUNCTION void 
OpenGLBindFramebuffer(uint32 TargetIndex, rectangle2i DrawRegion){
    uint32 WindowWidth = GetWidth(DrawRegion);
    uint32 WindowHeight = GetHeight(DrawRegion);

    glBindFramebuffer(GL_FRAMEBUFFER, GlobalFramebufferHandles[TargetIndex]);
    glViewport(0, 0, WindowWidth, WindowHeight);
}

inline void OpenGLRenderRectangle(
    vec3 MinP,
    vec3 MaxP,
    vec4 PremultColor,
    vec2 MinUV = Vec2(0.0f, 0.0f),
    vec2 MaxUV = Vec2(1.0f, 1.0f))
{
    GL_DEBUG_MARKER();
    
    glBegin(GL_TRIANGLES);

#if 1
    

    glColor4f(PremultColor.r, PremultColor.g, PremultColor.b, PremultColor.a);

    /*Upper triangle*/
    glTexCoord2f(MinUV.x, MaxUV.y);
    glVertex3f(MinP.x, MaxP.y, MinP.z);
    glTexCoord2f(MaxUV.x, MaxUV.y);
    glVertex3f(MaxP.x, MaxP.y, MinP.z);
    glTexCoord2f(MaxUV.x, MinUV.y);
    glVertex3f(MaxP.x, MinP.y, MinP.z);

    /*Lower triangle*/
    glTexCoord2f(MinUV.x, MaxUV.y);
    glVertex3f(MinP.x, MaxP.y, MinP.z);
    glTexCoord2f(MaxUV.x, MinUV.y);
    glVertex3f(MaxP.x, MinP.y, MinP.z);
    glTexCoord2f(MinUV.x, MinUV.y);
    glVertex3f(MinP.x, MinP.y, MinP.z);
#else
    glColor4f(PremultColor.r, PremultColor.g, PremultColor.b, PremultColor.a);

    // NOTE(casey): Lower triangle
    glTexCoord2f(MinUV.x, MinUV.y);
    glVertex3f(MinP.x, MinP.y, MinP.z);

    glTexCoord2f(MaxUV.x, MinUV.y);
    glVertex3f(MaxP.x, MinP.y, MinP.z);

    glTexCoord2f(MaxUV.x, MaxUV.y);
    glVertex3f(MaxP.x, MaxP.y, MinP.z);

    // NOTE(casey): Upper triangle
    glTexCoord2f(MinUV.x, MinUV.y);
    glVertex3f(MinP.x, MinP.y, MinP.z);

    glTexCoord2f(MaxUV.x, MaxUV.y);
    glVertex3f(MaxP.x, MaxP.y, MinP.z);

    glTexCoord2f(MinUV.x, MaxUV.y);
    glVertex3f(MinP.x, MaxP.y, MinP.z);
#endif

    glEnd();

    GL_DEBUG_MARKER();
}

inline void OpenGLRenderBitmap(
    int Width, 
    int Height,
    void* Memory,
    rectangle2i DrawRegion,
    vec4 ClearColor,
    GLuint BlitTexture)
{
    GL_DEBUG_MARKER();

    OpenGLBindFramebuffer(0, DrawRegion);
    //glViewport(0, 0, Width, Height);

    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);

    GL_DEBUG_MARKER();

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

    GL_DEBUG_MARKER();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    GL_DEBUG_MARKER();

    glEnable(GL_TEXTURE_2D);

    GL_DEBUG_MARKER();

    glClearColor(ClearColor.r, ClearColor.g, ClearColor.b, ClearColor.a);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    GL_DEBUG_MARKER();

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

    GL_DEBUG_MARKER();
	
    vec3 MinP = {0, 0, 0};
    vec3 MaxP = {(float)Width, (float)Height, 0.0f};
    vec4 Color = {1.0f, 1.0f, 1.0f, 1.0f};

    OpenGLRenderRectangle(MinP, MaxP, Color);
    
    GL_DEBUG_MARKER();

    glBindTexture(GL_TEXTURE_2D, 0);
    glEnable(GL_BLEND);

    GL_DEBUG_MARKER();
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

INTERNAL_FUNCTION void* OpenGLAllocateTexture(uint32 Width, uint32 Height, void* Data){
    
    GL_DEBUG_MARKER();

    GLuint Handle;
    glGenTextures(1, &Handle);
    glBindTexture(GL_TEXTURE_2D, Handle);

    GL_DEBUG_MARKER();

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

    GL_DEBUG_MARKER();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

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
                Op->Allocate.Data);
        }
        else{
            GLuint Handle = UINT32_FROM_POINTER(Op->Deallocate.Handle);
            glDeleteTextures(1, &Handle);
        }
    }
}

INTERNAL_FUNCTION void 
OpenGLInit(opengl_info Info, bool32 FramebufferSupportsSRGB){

    GL_DEBUG_MARKER();

    OpenGL.ShaderSimTexReadSRGB = true;
    OpenGL.ShaderSimTexWriteSRGB = true;

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
    OpenGL.WhiteBitmap.TextureHandle = OpenGLAllocateTexture(1, 1, &OpenGL.White);

    GL_DEBUG_MARKER();
}