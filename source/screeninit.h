/*
*   screeninit.h
*
*   Screen init code by dark_samus, bil1s, Normmatt, delebile and others.
*   Screen deinit code by tiniVi.
*/

#pragma once

#ifdef ARM9

#include "arm9/source/common.h"

#define BRIGHTNESS_COUNT   0x4
#define PDN_GPU_CNT        (*(vu8 *)0x10141200)

void deinitScreens(void);
void initScreens(u8 brightnessLevel);
//void updateBrightness(u8 brightnessLevel);

#endif
