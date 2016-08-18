//
// Created by cpasjuste on 25/01/16.
//

#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "draw.h"
#include "anim.h"
#include "config.h"
#include "menu.h"

void drawBg() {
    
    u16 fbWidth, fbHeight;
    u8* fbTop = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &fbWidth, &fbHeight);
    if (readMovie(fbTop, &anim->topMovie) < 0)
    {
        if (!config->imgError && 0xFF == anim->imgTopFade && !config->imgTopIsRGBA) {
            memcpy(fbTop, config->bgImgTopBuff, (size_t) config->bgImgTopSize);
        } else {
            clearTop(anim->bgTop1, anim->bgTop2);
            if (!config->imgError && anim->imgTopFade)
                drawImage(GFX_TOP, GFX_LEFT, config->bgImgTopBuff, config->imgTopIsRGBA, fbHeight, fbWidth, 0, 0, anim->imgTopFade);
        }
    }
    u8* fbBot = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, &fbWidth, &fbHeight);
    if (readMovie(fbBot, &anim->botMovie) < 0)
    {
        if (!config->imgErrorBot && 0xFF == anim->imgBotFade && !config->imgBotIsRGBA) {
            memcpy(fbBot, config->bgImgBotBuff, (size_t) config->bgImgBotSize);
        } else {
            clearBot(anim->bgBot);
            if (!config->imgErrorBot && anim->imgBotFade)
                drawImage(GFX_BOTTOM, GFX_LEFT, config->bgImgBotBuff, config->imgBotIsRGBA, fbHeight, fbWidth, 0, 0, anim->imgBotFade);
        }
    }
    drawRectColor(GFX_TOP, GFX_LEFT, MENU_MIN_X, MENU_MIN_Y - 20, MENU_MAX_X, MENU_MAX_Y, anim->borders);

    u8* fbTop3D = IS3DACTIVE ? gfxGetFramebuffer(GFX_TOP, GFX_RIGHT, &fbWidth, &fbHeight) : NULL;
    if (fbTop3D)
    {
        if (readMovie(fbTop3D, &anim->top3DMovie) < 0)
        {
            if (!config->imgError3D && 0xFF == anim->imgTopFade && !config->imgTop3DIsRGBA) {
                memcpy(fbTop3D, config->bgImgTop3DBuff, (size_t) config->bgImgTop3DSize);
            } else {
                if (!config->imgError3D && anim->imgTopFade)
                    drawImage(GFX_TOP, GFX_RIGHT, config->bgImgTop3DBuff, config->imgTop3DIsRGBA, fbHeight, fbWidth, 0, 0, anim->imgTopFade);
            }
        }

        drawRectColor(GFX_TOP, GFX_RIGHT, MENU_MIN_X, MENU_MIN_Y - 20, MENU_MAX_X, MENU_MAX_Y, anim->borders);
    }
}

void drawTitle(const char *format, ...) {

    char msg[512];
    va_list argp;
    va_start(argp, format);
    vsnprintf(msg, 512, format, argp);
    va_end(argp);

    drawText(GFX_TOP, GFX_LEFT, &fontDefault, msg, 140, 25);
    if( IS3DACTIVE )
        drawText(GFX_TOP, GFX_RIGHT, &fontDefault, msg, 140, 25);
}

void drawItem(bool selected, int y, const char *format, ...) {

    char msg[512];
    va_list argp;
    va_start(argp, format);
    vsnprintf(msg, 512, format, argp);
    va_end(argp);

    if (selected) {
        drawRectangle(GFX_TOP, GFX_LEFT, anim->highlight, (s16) (MENU_MIN_X + 4), (s16) (y + MENU_MIN_Y), 361, 15);
        if( IS3DACTIVE )
            drawRectangle(GFX_TOP, GFX_RIGHT, anim->highlight, (s16) (MENU_MIN_X + 4), (s16) (y + MENU_MIN_Y), 361, 15);
    }
    memcpy(fontDefault.color, selected ? anim->fntSel : anim->fntDef, sizeof(u8[4]));

    drawText(GFX_TOP, GFX_LEFT, &fontDefault, msg, (s16) (MENU_MIN_X + 6), (s16) y + (s16) MENU_MIN_Y);
    if( IS3DACTIVE )
        drawText(GFX_TOP, GFX_RIGHT, &fontDefault, msg, (s16) (MENU_MIN_X + 6), (s16) y + (s16) MENU_MIN_Y);

    memcpy(fontDefault.color, anim->fntDef, sizeof(u8[4]));
}

void drawItemN(bool selected, int maxChar, int y, const char *format, ...) {

    char msg[512];
    va_list argp;
    va_start(argp, format);
    vsnprintf(msg, 512, format, argp);
    va_end(argp);

    if (selected) {
        drawRectangle(GFX_TOP, GFX_LEFT, anim->highlight, (s16) (MENU_MIN_X + 4), (s16) (y + MENU_MIN_Y), 361, 15);
        if( IS3DACTIVE )
            drawRectangle(GFX_TOP, GFX_RIGHT, anim->highlight, (s16) (MENU_MIN_X + 4), (s16) (y + MENU_MIN_Y), 361, 15);
    }
    memcpy(fontDefault.color, selected ? anim->fntSel : anim->fntDef, sizeof(u8[4]));
    drawTextN(GFX_TOP, GFX_LEFT, &fontDefault, msg, maxChar, (s16) (MENU_MIN_X + 6), (s16) y + (s16) MENU_MIN_Y);
    if( IS3DACTIVE )
        drawTextN(GFX_TOP, GFX_RIGHT, &fontDefault, msg, maxChar, (s16) (MENU_MIN_X + 6), (s16) y + (s16) MENU_MIN_Y);

    memcpy(fontDefault.color, anim->fntDef, sizeof(u8[4]));
}

void drawInfo(const char *format, ...) {

    char msg[512];
    va_list argp;
    va_start(argp, format);
    vsnprintf(msg, 512, format, argp);
    va_end(argp);

    memcpy(fontDefault.color, config->fntBotActive ? anim->fntBot : anim->fntDef, sizeof(u8[4]));
    drawText(GFX_BOTTOM, GFX_LEFT, &fontDefault, "Informations", (s16) (MENU_MIN_X + 6), 40);
    drawText(GFX_BOTTOM, GFX_LEFT, &fontDefault, msg, (s16) (MENU_MIN_X + 12), 80);
    
    memcpy(fontDefault.color, anim->fntDef, sizeof(u8[4]));
}
