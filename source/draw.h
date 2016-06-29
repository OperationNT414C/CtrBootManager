#pragma once

#ifdef ARM9

#include "arm9/source/common.h"

typedef enum
{
    GFX_TOP = 0,   ///< Top screen
    GFX_BOTTOM = 1 ///< Bottom screen
}gfxScreen_t;

typedef enum
{
    GFX_LEFT = 0, ///< Left eye framebuffer
    GFX_RIGHT = 1,///< Right eye framebuffer
}gfx3dSide_t;

u8 *gfxGetFramebuffer(gfxScreen_t screen, gfx3dSide_t side, u16 *width, u16 *height);

#define TOP_SCREEN_SIZE (400*240*3)
#define BOT_SCREEN_SIZE (320*240*3)
#define IS3DACTIVE false

#else

#include <3ds.h>
#define IS3DACTIVE gfxIs3D()

#endif

#include "font.h"

//rendering stuff
void drawLine(gfxScreen_t screen, gfx3dSide_t side, int x1, int y1, int x2, int y2, char r, char g, char b, char a);

void drawRectColor(gfxScreen_t screen, gfx3dSide_t side, int x1, int y1, int x2, int y2, u8 *rgbaColor);

void drawText(gfxScreen_t screen, gfx3dSide_t side, font_s *f, char *str, s16 x, s16 y);

void drawTextN(gfxScreen_t screen, gfx3dSide_t side, font_s *f, char *str, u16 length, s16 x, s16 y);

void drawTextf(gfxScreen_t screen, gfx3dSide_t side, font_s *f, s16 x, s16 y, const char *fmt, ...);

void fillColor(gfxScreen_t screen, gfx3dSide_t side, u8 rgbColor[3]);

void fillColorGradient(gfxScreen_t screen, gfx3dSide_t side, u8 rgbColorStart[3], u8 rgbColorEnd[3]);

void drawRectangle(gfxScreen_t screen, gfx3dSide_t side, u8 rgbaColor[4], s16 x, s16 y, u16 width, u16 height);

void clearTop(u8 top1[3], u8 top2[3]);

void clearBot(u8 bot[8]);

void clearFrameBuffers();

void swapFrameBuffers();
