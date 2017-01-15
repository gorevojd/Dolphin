#include "dolphin_render_group.h"

#if 0
inline void RenderOpenGLRectangle(
    gdVec2 MinP,
    gdVec2 MaxP,
    gdVec4 PremultColor,
    gdVec2 MinUV = gd_vec2(0.0f, 0.0f),
    gdVec2 MaxUV = gd_vec2(1.0f, 1.0f))
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
#endif
