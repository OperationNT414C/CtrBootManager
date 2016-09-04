#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef ARM9
#include "arm9/source/common.h"
#include "memory.h"
#else
#include <3ds.h>
#endif

#include <stdarg.h>
#include "text.h"
#include "anim.h"

#ifdef ARM9

u8 *gfxGetFramebuffer(gfxScreen_t screen, gfx3dSide_t side, u16 *width, u16 *height) {
    //WARNING: width and height must stay inverted to simulate 3DSX version behavior!
    if (screen == GFX_TOP) {
        if (width) *width = 240;
        if (height) *height = 400;
        return PTR_TOP_SCREEN_BUF;
    } else {
        if (width) *width = 240;
        if (height) *height = 320;
        return PTR_BOT_SCREEN_BUF;
    }
}

#endif

void drawPixel(int x, int y, char r, char g, char b, u8 *screen) {
    int height = 240;

    u32 v = (height - 1 - y + x * height) * 3;
    screen[v] = b;
    screen[v + 1] = g;
    screen[v + 2] = r;
}

void drawPixelAlpha(int x, int y, char r, char g, char b, char a, u8 *screen) {
    int height = 240;

    u32 v = (height - 1 - y + x * height) * 3;

    int alpha = a;
    int one_minus_alpha = 255-alpha;
    
    screen[v + 0] = (u8)((alpha*b+one_minus_alpha*screen[v + 0])/255);
    screen[v + 1] = (u8)((alpha*g+one_minus_alpha*screen[v + 1])/255);
    screen[v + 2] = (u8)((alpha*a+one_minus_alpha*screen[v + 2])/255);
}

void drawLine(gfxScreen_t screen, gfx3dSide_t side, int x1, int y1, int x2, int y2, char r, char g, char b, char a) {
    u16 fbWidth, fbHeight;
    u8 *fbAdr = gfxGetFramebuffer(screen, side, &fbWidth, &fbHeight);

    int x, y;
    if ( a == 0xFF )
    {
        if (x1 == x2) {
            if (y1 < y2) for (y = y1; y < y2; y++) drawPixel(x1, y, r, g, b, fbAdr);
            else for (y = y2; y < y1; y++) drawPixel(x1, y, r, g, b, fbAdr);
        } else {
            if (x1 < x2) for (x = x1; x < x2; x++) drawPixel(x, y1, r, g, b, fbAdr);
            else for (x = x2; x < x1; x++) drawPixel(x, y1, r, g, b, fbAdr);
        }
    }
    else if ( a > 0 )
    {
        if (x1 == x2) {
            if (y1 < y2) for (y = y1; y < y2; y++) drawPixelAlpha(x1, y, r, g, b, a, fbAdr);
            else for (y = y2; y < y1; y++) drawPixelAlpha(x1, y, r, g, b, a, fbAdr);
        } else {
            if (x1 < x2) for (x = x1; x < x2; x++) drawPixelAlpha(x, y1, r, g, b, a, fbAdr);
            else for (x = x2; x < x1; x++) drawPixelAlpha(x, y1, r, g, b, a, fbAdr);
        }
    }
}

void drawRect(gfxScreen_t screen, gfx3dSide_t side, int x1, int y1, int x2, int y2, char r, char g, char b, char a) {
    drawLine(screen, side, x1+1, y1, x2, y1, r, g, b, a);
    drawLine(screen, side, x2, y1, x2, y2, r, g, b, a);
    drawLine(screen, side, x1, y2, x2+1, y2, r, g, b, a);
    drawLine(screen, side, x1, y1, x1, y2, r, g, b, a);
}

void drawRectColor(gfxScreen_t screen, gfx3dSide_t side, int x1, int y1, int x2, int y2, u8 *rgbaColor) {
    drawRect(screen, side, x1, y1, x2, y2, rgbaColor[0], rgbaColor[1], rgbaColor[2], rgbaColor[3]);
}

void drawTextf(gfxScreen_t screen, gfx3dSide_t side, font_s *f, s16 x, s16 y, const char *fmt, ...) {
    char s[512];
    memset(s, 0, 512);
    va_list args;
    va_start(args, fmt);
    int len = vsprintf(s, fmt, args);
    va_end(args);
    if (len)
        drawText(screen, side, f, s, x, y);
}

void drawText(gfxScreen_t screen, gfx3dSide_t side, font_s *f, char *str, s16 x, s16 y) {
    if (!str)return;
    if (!f)f = &fontDefault;

    u16 fbWidth, fbHeight;
    u8 *fbAdr = gfxGetFramebuffer(screen, side, &fbWidth, &fbHeight);

    drawString(fbAdr, f, str, x, 240 - y, fbHeight, fbWidth);
}

void drawTextN(gfxScreen_t screen, gfx3dSide_t side, font_s *f, char *str, u16 length, s16 x, s16 y) {
    if (!str)return;
    if (!f)f = &fontDefault;

    u16 fbWidth, fbHeight;
    u8 *fbAdr = gfxGetFramebuffer(screen, side, &fbWidth, &fbHeight);

    drawStringN(fbAdr, f, str, length, x, 240 - y, fbHeight, fbWidth);
}

void drawImage(gfxScreen_t screen, gfx3dSide_t side, u8* img, bool rgba, u16 w, u16 h, s16 x, s16 y, char a)
{
    u16 fbWidth, fbHeight;
    u8 *fbAdr = gfxGetFramebuffer(screen, side, &fbWidth, &fbHeight);
    
    int imax = w;
    if (imax+x > fbHeight)
        imax = fbHeight-x;

    int jmax = h;
    if (jmax+y > fbWidth)
        jmax = fbWidth-y;

    int i, j;
    if ( rgba )
    {
        for (i = 0; i < imax; i++)
        {
            u8* imgAdr = img + 4*i*h;
            u8* rowFbAdr = fbAdr + 3*((x+i)*fbWidth + y);
            for (j = 0; j < jmax; j++)
            {
                int alpha = ((int)a*imgAdr[0]) / 255;
                int one_minus_alpha = 255-alpha;
                rowFbAdr[0] = (u8)((alpha*imgAdr[1]+one_minus_alpha*rowFbAdr[0])/255);
                rowFbAdr[1] = (u8)((alpha*imgAdr[2]+one_minus_alpha*rowFbAdr[1])/255);
                rowFbAdr[2] = (u8)((alpha*imgAdr[3]+one_minus_alpha*rowFbAdr[2])/255);
                imgAdr += 4;
                rowFbAdr += 3;
            }
        }
    }
    else
    {
        int alpha = a;
        int one_minus_alpha = 255-a;
        for (i = 0; i < imax; i++)
        {
            u8* imgAdr = img + 3*i*h;
            u8* rowFbAdr = fbAdr + 3*((x+i)*fbWidth + y);
            for (j = 0; j < jmax; j++)
            {
                rowFbAdr[0] = (u8)((alpha*imgAdr[0]+one_minus_alpha*rowFbAdr[0])/255);
                rowFbAdr[1] = (u8)((alpha*imgAdr[1]+one_minus_alpha*rowFbAdr[1])/255);
                rowFbAdr[2] = (u8)((alpha*imgAdr[2]+one_minus_alpha*rowFbAdr[2])/255);
                imgAdr += 3;
                rowFbAdr += 3;
            }
        }
    }
}

void fillColor(gfxScreen_t screen, gfx3dSide_t side, u8 rgbColor[3]) {
    u16 fbWidth, fbHeight;
    u8 *fbAdr = gfxGetFramebuffer(screen, side, &fbWidth, &fbHeight);

    //TODO : optimize; use GX command ?
    int i;
    for (i = 0; i < fbWidth * fbHeight; i++) {
        *(fbAdr++) = rgbColor[2];
        *(fbAdr++) = rgbColor[1];
        *(fbAdr++) = rgbColor[0];
    }
}

void fillColorGradient(gfxScreen_t screen, gfx3dSide_t side, u8 rgbColorStart[3], u8 rgbColorEnd[3]) {
    u16 fbWidth, fbHeight;
    u8 *fbAdr = gfxGetFramebuffer(screen, side, &fbWidth, &fbHeight);
    u8 colorLine[720]; // Avoid dynamic "fbWidth*3", illogical crash on 3DSX version with top right buffer

    //TODO : optimize; use GX command ?
    // Avoid float operations, illogical crash on 3DSX version with top right buffer
    int i;
    int width = fbWidth;
    for (i = 0; i < fbWidth; i++) {
        int minus = width-i-1;
        colorLine[i * 3 + 0] = (minus*rgbColorStart[2] + i*rgbColorEnd[2]) / fbWidth;
        colorLine[i * 3 + 1] = (minus*rgbColorStart[1] + i*rgbColorEnd[1]) / fbWidth;
        colorLine[i * 3 + 2] = (minus*rgbColorStart[0] + i*rgbColorEnd[0]) / fbWidth;
    }

    for (i = 0; i < fbHeight; i++) {
        memcpy(fbAdr, colorLine, fbWidth * 3);
        fbAdr += fbWidth * 3;
    }
}

void _drawRectangle(gfxScreen_t screen, gfx3dSide_t side, u8 rgbaColor[4], s16 x, s16 y, u16 width, u16 height) {
    u16 fbWidth, fbHeight;
    u8 *fbAdr = gfxGetFramebuffer(screen, side, &fbWidth, &fbHeight);

    if (x + width < 0 || x >= fbWidth)return;
    if (y + height < 0 || y >= fbHeight)return;

    if (x < 0) {
        width += x;
        x = 0;
    }
    if (y < 0) {
        height += y;
        y = 0;
    }
    if (x + width >= fbWidth)width = fbWidth - x;
    if (y + height >= fbHeight)height = fbHeight - y;

	if ( rgbaColor[3] == 0xFF )
	{
		u8 colorLine[width * 3];

		int j;
		for (j = 0; j < width; j++) {
			colorLine[j * 3 + 0] = rgbaColor[2];
			colorLine[j * 3 + 1] = rgbaColor[1];
			colorLine[j * 3 + 2] = rgbaColor[0];
		}

		fbAdr += fbWidth * 3 * y;
		for (j = 0; j < height; j++) {
			memcpy(&fbAdr[x * 3], colorLine, width * 3);
			fbAdr += fbWidth * 3;
		}
	}
	else if ( rgbaColor[3] > 0 )
	{
        int alpha = rgbaColor[3];
        int one_minus_alpha = 255-alpha;

		int i, j;
		fbAdr += fbWidth * 3 * y;
		for (j = 0; j < height; j++)
		{
			for (i = 0; i < width; i++)
			{
                fbAdr[3*(i+x)+0] = (u8)((alpha*rgbaColor[2]+one_minus_alpha*fbAdr[3*(i+x)+0])/255);
                fbAdr[3*(i+x)+1] = (u8)((alpha*rgbaColor[1]+one_minus_alpha*fbAdr[3*(i+x)+1])/255);
                fbAdr[3*(i+x)+2] = (u8)((alpha*rgbaColor[0]+one_minus_alpha*fbAdr[3*(i+x)+2])/255);
			}
			fbAdr += fbWidth * 3;
		}
	}
}

void drawRectangle(gfxScreen_t screen, gfx3dSide_t side, u8 rgbColor[4], s16 x, s16 y, u16 width, u16 height) {
    _drawRectangle(screen, side, rgbColor, 240 - y, x, height, width);
}

void clearTop(u8 top1[3], u8 top2[3]) {
    fillColorGradient(GFX_TOP, GFX_LEFT, top1, top2);
    if( IS3DACTIVE )
        fillColorGradient(GFX_TOP, GFX_RIGHT, top1, top2);
}

void clearBot(u8 bot[8]) {
    fillColor(GFX_BOTTOM, GFX_LEFT, bot);
}

void swapFrameBuffers() {
#ifdef ARM9
    memcpy(PTR_TOP_SCREEN, PTR_TOP_SCREEN_BUF, TOP_SCREEN_SIZE);
    memcpy(PTR_BOT_SCREEN, PTR_BOT_SCREEN_BUF, BOT_SCREEN_SIZE);
#else
    gfxFlushBuffers();
    gfxSwapBuffers();
    gspWaitForVBlank();
#endif
    incrementAnimTime();
}
