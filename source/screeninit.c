/*
*   screeninit.c
*
*   Screen init code by dark_samus, bil1s, Normmatt, delebile and others.
*   Screen deinit code by tiniVi.
*/

#ifdef ARM9

#include "screeninit.h"

#include <stdio.h>
#include "arm9/source/i2c.h"
#include "../build/screeninit.h"

vu32 *arm11Entry = (u32 *)0x1FFFFFF8;

void deinitScreens(void)
{
    void __attribute__((naked)) ARM11(void)
    {
        //Disable interrupts
        __asm(".word 0xF10C01C0");

        //Clear ARM11 entry offset
        *arm11Entry = 0;

        //Shutdown LCDs
        *(vu32 *)0x10202A44 = 0;
        *(vu32 *)0x10202244 = 0;
        *(vu32 *)0x10202014 = 0;

        //Wait for the entry to be set
        while(!*arm11Entry);

        //Jump to it
        ((void (*)())*arm11Entry)();
    }

    if(PDN_GPU_CNT != 1)
    {
        *arm11Entry = (u32)ARM11;
        while(*arm11Entry);
    }
}

const u32 brightness[BRIGHTNESS_COUNT] = {0x5F, 0x4C, 0x39, 0x26};

void initScreens(u8 brightnessLevel)
{
    if(PDN_GPU_CNT == 1)
    {
        u32 *const screenInitAddress = (u32 *)0x24FFFC00;
        memcpy(screenInitAddress, screeninit, screeninit_size);

        //Write brightness level for the stub to pick up
        screenInitAddress[2] = brightnessLevel%BRIGHTNESS_COUNT;
        
        *arm11Entry = (u32)screenInitAddress;
        while(*arm11Entry);

        //Turn on backlight
        i2cWriteRegister(I2C_DEV_MCU, 0x22, 0x2A);
    }
}

/*void updateBrightness(u8 brightnessLevel)
{
    static int brightnessValue;
    brightnessValue = brightness[brightnessLevel%BRIGHTNESS_COUNT];
    
    void __attribute__((naked)) ARM11(void)
    {
        //Disable interrupts
        __asm(".word 0xF10C01C0");      
        
        //Clear ARM11 entry offset
        *arm11Entry = 0;

        //Change brightness
        *(vu32 *)0x10202240 = brightnessValue;
        *(vu32 *)0x10202A40 = brightnessValue;

        //Wait for the entry to be set
        while(!*arm11Entry);

        //Jump to it
        ((void (*)())*arm11Entry)();
    }

    *arm11Entry = (u32)ARM11;
    while(*arm11Entry);
}*/

#endif
