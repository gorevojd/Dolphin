#ifndef IVAN_OPENGL_H
#define IVAN_OPENGL_H

struct opengl_program_common{
    GLuint ProgramHandle;

    GLuint VertPID;
    GLuint VertNID;
    GLuint VertUVID;
};

struct opengl_info{
    bool32 ModernContext;

    char* Vendor;
    char* Renderer;
    char* Version;
    char* ShadingLanguageVersion;
    char* Extensions;

    bool32 GL_EXT_texture_sRGB;
    bool32 GL_EXT_framebuffer_sRGB;
    bool32 GL_ARB_framebuffer_object;
};

struct open_gl{

    bool32 ShaderSimTexReadSRGB;
    bool32 ShaderSimTexWriteSRGB;

	GLint MaxMultiSampleCount;
	bool32 SupportsSRGBFramebuffer;

	GLuint DefaultSpriteTextureFormat;
	GLuint DefaultFramebufferTextureFormat;

	GLuint ReservedBlitTexture;

	loaded_bitmap WhiteBitmap;
	uint32 White[4][4];
};

#endif