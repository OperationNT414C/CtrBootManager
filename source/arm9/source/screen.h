/*
*   screeninit.h
*
*   Screen init code by dark_samus, bil1s, Normmatt, delebile and others.
*   Screen deinit code by tiniVi.
*/

#pragma once

#include "common.h"

#define BRIGHTNESS_COUNT   0x4
#define PDN_GPU_CNT        (*(vu8 *)0x10141200)

struct fb {
     u8 *top_left;
     u8 *top_right;
     u8 *bottom;
}  __attribute__((packed));

extern struct fb fbs[2];

//void deinitScreens(void);
void initScreens(u8 brightnessLevel);
//void updateBrightness(u8 brightnessLevel);
