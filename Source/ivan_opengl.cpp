#include "ivan_render_group.h"

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


inline void RenderOpenGLRectangle(
    vec2 MinP,
    vec2 MaxP,
    vec4 PremultColor,
    vec2 MinUV = Vec2(0.0f, 0.0f),
    vec2 MaxUV = Vec2(1.0f, 1.0f))
{
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

PLATFORM_ALLOCATE_TEXTURE(AllocateTexture){
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

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glBindTexture(GL_TEXTURE_2D, 0);

    glFlush();

    Assert(sizeof(Handle) <= sizeof(void*));
    return(POINTER_FROM_UINT32(void, Handle));
}

PLATFORM_DEALLOCATE_TEXTURE(DeallocateTexture){
    GLuint Handle = UINT32_FROM_POINTER(Texture);
    glDeleteTextures(1, &Handle);
}

INTERNAL_FUNCTION void 
OpenGLInit(opengl_info Info, bool32 FramebufferSupportsSRGB){
    OpenGL.ShaderSimTexReadSRGB = true;
    OpenGL.ShaderSimTexWriteSRGB = true;

    glGenTextures(1, &OpenGL.ReservedBlitTexture);

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

    if(FramebufferSupportsSRGB && Info.GL_EXT_framebuffer_sRGB){
        GLuint TestTexture;
        glGenTextures(1, &TestTexture);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, TestTexture);
        glGetError();
        glTexImage2DMultisample(
            GL_TEXTURE_2D_MULTISAMPLE, 
            OpenGL.MaxMultiSampleCount,
            GL_SRGB8_ALPHA8,
            1920, 1080,
            GL_FALSE);

        if(glGetError() == GL_NO_ERROR){
            OpenGL.DefaultFramebufferTextureFormat = GL_SRGB8_ALPHA8;
            glEnable(GL_FRAMEBUFFER_SRGB);
            OpenGL.ShaderSimTexWriteSRGB = false;
        }

        glDeleteTextures(1, &TestTexture);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
    }

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
    OpenGL.WhiteBitmap.TextureHandle = AllocateTexture(1, 1, & OpenGL.White);
}