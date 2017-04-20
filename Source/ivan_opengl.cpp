#include "ivan_render_group.h"

#define GL_SHADING_LANGUAGE_VERSION 0x8B8C

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

struct opengl_info{
    bool32 ModernContext;

    char* Vendor;
    char* Renderer;
    char* Version;
    char* ShadingLanguageVersion;
    char* Extensions;
};

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
    Result.Extensions = (char*)glGetString(GL_EXTENSIONS);

    return(Result);
}

PLATFORM_ALLOCATE_TEXTURE(AllocateTexture){
    GLuint Handle;
    glGenTextures(1, &Handle);
    glBindTexture(GL_TEXTURE_2D, Handle);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        OPENGL_DEFAULT_INTERNAL_TEXTURE_FORMAT,
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