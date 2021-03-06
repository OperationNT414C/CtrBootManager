#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "anim.h"
#include "utility.h"
#include "menu.h"

#ifdef ARM9

#include "memory.h"
#include "arm9/source/common.h"
#include "arm9/source/hid.h"
#include "arm9/source/fatfs/ff.h"
#include "arm9/source/screen.h"

#else

#include <3ds.h>
#include "hb_menu/netloader.h"

extern char boot_app[512];
extern bool boot_app_enabled;

extern void scanMenuEntry(menuEntry_s *me);

int bootApp(char *executablePath, executableMetadata_s *em, char *arg);

void __appInit() {
    srvInit();
    aptInit();
    fsInit();
    sdmcInit();
    openSDArchive();
    hidInit();
    acInit();
    ptmuInit();
    amInit();
    gfxInitDefault();
    gfxSet3D(false);
}

void __appExit() {
    gfxExit();
    netloader_exit();
    configExit();
    animExit();
    amExit();
    ptmuExit();
    acExit();
    hidExit();
    closeSDArchive();
    sdmcExit();
    fsExit();
    aptExit();
    srvExit();
}
#endif

#ifdef ARM9
int startCtrBootManager(int argc, char **argv, u32 magicWord) {
#else
int main() {
#endif

#ifdef ARM9
    *(u32 *) 0x10000020 = 0;
    *(u32 *) 0x10000020 = 0x340;

    // Setup framebuffers:
    // May be corrupted if screen were not initialized or if framebuffers were pushed on arguments
    fbs[0].top_left = (u8 *)*PTR_TOP_SCREEN;
    fbs[1].top_left = (u8 *)*PTR_TOP_SCREEN;
    fbs[0].top_right = (u8 *)*PTR_TOP_RIGHT_SCREEN;
    fbs[1].top_right = (u8 *)*PTR_TOP_RIGHT_SCREEN;
    fbs[0].bottom = (u8 *)*PTR_BOT_SCREEN;
    fbs[1].bottom = (u8 *)*PTR_BOT_SCREEN;

    // Code inspired from Luma3DS
    u16 launchedPath[4];
    if ((magicWord & 0xFFFF) == 0xBEEF && argc >= 1)
    {
        u32 i;
        for(i = 0; i < 4 && argv[0][i] != 0; i++)
            launchedPath[i] = argv[0][i];
        
        // Framebuffers management
        if(argc >= 2)
        {
            u8 **fb = (u8 **)(void *)argv[1];
            fbs[0].top_left = fb[0];
            fbs[0].top_right = fb[1];
            fbs[0].bottom = fb[2];

            if (argc >= 4)
            {
                fbs[1].top_left = fb[3];
                fbs[1].top_right = fb[4];
                fbs[1].bottom = fb[5];
            }
            else
            {
                fbs[1].top_left = fbs[0].top_left;
                fbs[1].top_right = fbs[0].top_right;
                fbs[1].bottom = fbs[0].bottom;
            }

            *PTR_TOP_SCREEN = fbs[0].top_left;
            *PTR_TOP_RIGHT_SCREEN = fbs[0].top_right;
            *PTR_BOT_SCREEN = fbs[0].bottom;
        }
    }
    else if (magicWord == 0xBABE && argc == 2)
    {
        u32 i;
        u16 *p = (u16 *)argv[0];
        for(i = 0; i < 4 && p[i] != 0; i++)
            launchedPath[i] = p[i];
    }
    else
        launchedPath[0] = 0;

    bool loadedFromSD = (memcmp(launchedPath, "n\0a\0n\0d\0", 8) != 0);
    initFileSystems(loadedFromSD);
#else
    if (netloader_init() != 0) {
        // fix SOC_Initialize
        strcpy(boot_app, "/boot.3dsx");
        boot_app_enabled = true;
    }
    osSetSpeedupEnable(true);

    // offset potential issues caused by homebrew that just ran (from hb_menu)
    //aptOpenSession();
    APT_SetAppCpuTimeLimit(0);
    //aptCloseSession();

    if (!boot_app_enabled) { // fix SOC_Initialize
#endif
    animInit();
    if (configInit() != 0 || config->count <= 0) { // recovery
    #ifdef ARM9
    	initScreens(config->brightness);
    #endif
        animSetup();
        while (aptMainLoop()) {
            if (menu_more() == 0)
                break;
        }
    } else {
        animSetup();
        while (aptMainLoop()) {
            if (menu_boot() == 0)
                break;
        }
    }
#ifndef ARM9
    }

    menuEntry_s *me = malloc(sizeof(menuEntry_s));
    strncpy(me->executablePath, boot_app, 128);
    initDescriptor(&me->descriptor);
    static char xmlPath[128];
    snprintf(xmlPath, 128, "%s", boot_app);
    int l = strlen(xmlPath);
    xmlPath[l - 1] = 0;
    xmlPath[l - 2] = 'l';
    xmlPath[l - 3] = 'm';
    xmlPath[l - 4] = 'x';
    if (fileExists(xmlPath))
        loadDescriptor(&me->descriptor, xmlPath);
    scanMenuEntry(me);

    return bootApp(me->executablePath, &me->descriptor.executableMetadata, me->arg);
#else
    return 0;
#endif
}
