#include <string.h>
#include <stdio.h>
#include <time.h>

#ifdef ARM9
#include "arm9/source/common.h"
#include "arm9/source/hid.h"
#include "screeninit.h"
#else
#include <3ds.h>
#endif

#include "config.h"
#include "draw.h"
#include "menu.h"
#include "utility.h"


static int pass_size = 0;
static int pass_fail = -1;

void keyLeft(int index) {

    switch (index) {
        case 0:
            if (config->timeout > -1)
                config->timeout--;
            break;
        case 1:
            if (config->index > 0)
                config->index--;
            else
                config->index = config->count - 1;
            break;
        case 2:
            if (config->recovery > 0)
                config->recovery--;
            else
                config->recovery = MAX_KEY;
            break;
        case 3:
        #ifdef ARM9
            if (config->brightness < BRIGHTNESS_COUNT-1)
                config->brightness++;
        #else
            if (config->autobootfix > 0)
                config->autobootfix--;
        #endif
            break;
        case 4:
            if ( config->passMaxAttempt > 0 )
                config->passMaxAttempt--;
            break;
        case 5:
            if (pass_fail > -1)
                pass_fail--;
            break;
        case 6:
            if (pass_size > 0)
                pass_size--;
            break;
    }
}

void keyRight(int index) {

    switch (index) {
        case 0:
            config->timeout++;
            break;
        case 1:
            if (config->index < config->count - 1)
                config->index++;
            else
                config->index = 0;
            break;
        case 2:
            if (config->recovery < MAX_KEY)
                config->recovery++;
            else
                config->recovery = 0;
            break;
        case 3:
        #ifdef ARM9
            if (config->brightness > 0)
                config->brightness--;
        #else
            config->autobootfix++;
        #endif
            break;
        case 4:
            config->passMaxAttempt++;
            break;
        case 5:
        #ifdef ARM9
            if (pass_fail < config->count+1)
        #else
            if (pass_fail < config->count+2)
        #endif
                pass_fail++;
            break;
        case 6:
            if (pass_size < PASSWORD_MAX_SIZE)
                pass_size++;
            break;
    }
}

void keyIn(int index) {
    
    if ( index != 7 )
        return; // Currently, only password is managed
    
    static const u32 checkedKeys = 0x0FFF;
    config->passSize = pass_size;
    
    hidScanInput();
    u32 kDownPrev = (hidKeysDown() & checkedKeys);
    int curKey = 0;

    while (aptMainLoop() && curKey < pass_size)
    {
        hidScanInput();
        u32 kDown = (hidKeysDown() & checkedKeys);
        
        if ( 0 == kDownPrev && 0 != kDown )
        {
            kDownPrev = kDown;
            int value = 0;
            while ( (kDown >> (value+1)) != 0 )
                value++;
            
            config->password[curKey] = value;
            curKey++;
        }
        kDownPrev = kDown;
        
        drawBg();
        drawTitle("*** Password setup ***");
        
        char msg[256];
        int size = 0;
        size += snprintf(msg, 256, "Enter the new password (%i keys):\n", pass_size);
        for ( int i = 0 ; i < curKey ; i++ )
        {
            size += snprintf(msg+size, 256, get_button(config->password[i]));
            if ( i < curKey-1 )
                size += snprintf(msg+size, 256, (i%4 == 3) ? "\n" : ", ");
        }
        msg[size] = '\0';

        drawTextf(GFX_TOP, GFX_LEFT, &fontDefault, MENU_MIN_X + 64, MENU_MIN_Y + 64, msg);
        if ( IS3DACTIVE )
            drawTextf(GFX_TOP, GFX_RIGHT, &fontDefault, MENU_MIN_X + 64, MENU_MIN_Y + 64, msg);

        swapFrameBuffers();
    }
}

int menu_config() {

    pass_size = config->passSize;
    pass_fail = -1;

    if ( strcasecmp(config->failPath, "reboot") == 0 )
        pass_fail = config->count;
    else if (strcasecmp(config->failPath, "shutdown") == 0)
        pass_fail = config->count+1;
#ifndef ARM9
    else if (strcasecmp(config->failPath, "homemenu") == 0)
        pass_fail = config->count+2;
#endif
    else
    {
        for ( int i = 0 ; i < config->count ; i++ )
        {
            boot_entry_s* entry = &config->entries[i];
            if ( 0 == entry->patchesCount && strcasecmp(config->failPath, entry->path) == 0
              && config->failOffset == entry->offset )
            {
                pass_fail = i;
                break;
            }
        }
    }

    bool pass_fail_found = ( pass_fail >= 0 );
    int menu_count = 8, menu_index = 0;

    // key repeat timer
    time_t t_start = 0, t_end = 0, t_elapsed = 0;

    while (aptMainLoop()) {

        hidScanInput();
        u32 kHeld = hidKeysHeld();
        u32 kDown = hidKeysDown();

#ifndef ARM9
        if (hidKeysUp()) {
            time(&t_start); // reset held timer
        }
#endif
        if (START_DRIVE_RO)
        {
            if (kDown & KEY_B)
                return 0;
            
            drawBg();
            drawTitle("*** Informations ***");
            
            char* msg = "CtrBootManager has been started from SysNAND.\n\
For safety reasons, the configuration file cannot be saved.\n\n\
Please manually create a configuration file named\n\
\"CtrBootManager.cfg\" and write it to SysNAND root.\n\n\
I decline all responsability for any harm on your 3DS system!\n\
Do it at your own risk!!!";

            drawTextf(GFX_TOP, GFX_LEFT, &fontDefault, MENU_MIN_X + 32, MENU_MIN_Y + 32, msg);
            if ( IS3DACTIVE )
                drawTextf(GFX_TOP, GFX_RIGHT, &fontDefault, MENU_MIN_X + 32, MENU_MIN_Y + 32, msg);
        }
        else
        {
            if (kDown & KEY_DOWN) {
                menu_index++;
                if (menu_index >= menu_count)
                    menu_index = 0;
            }
            else if (kDown & KEY_UP) {
                menu_index--;
                if (menu_index < 0)
                    menu_index = menu_count - 1;
            }

            if (kDown & KEY_LEFT) {
                keyLeft(menu_index);
                time(&t_start);
            } else if (kHeld & KEY_LEFT) {
                time(&t_end);
                t_elapsed = t_end - t_start;
                if (t_elapsed > 0) {
                    keyLeft(menu_index);
                    svcSleep(100);
                }
            }

            if (kDown & KEY_RIGHT) {
                keyRight(menu_index);
                time(&t_start);
            } else if (kHeld & KEY_RIGHT) {
                time(&t_end);
                t_elapsed = t_end - t_start;
                if (t_elapsed > 0) {
                    keyRight(menu_index);
                    svcSleep(100);
                }
            }
            
            if (kDown & KEY_A) {
                keyIn(menu_index);
            }

            if (kDown & KEY_B) {

                // Apply password failed behavior
                int size = 0;
                if ( pass_fail == config->count )
                    size = snprintf(config->failPath, 256, "reboot");
                else if ( pass_fail == config->count+1 )
                    size = snprintf(config->failPath, 256, "shutdown");
        #ifndef ARM9
                else if ( pass_fail == config->count+2 )
                    size = snprintf(config->failPath, 256, "homemenu");
        #endif
                else if ( pass_fail >= 0 )
                {
                    boot_entry_s* entry = &config->entries[pass_fail];
                    size = snprintf(config->failPath, 256, entry->path);
                    config->failOffset = entry->offset;
                }
                if ( pass_fail >= 0 || pass_fail_found )
                    config->failPath[size] = '\0';
                
                configSave();
                return 0;
            }

            drawBg();
            drawTitle("*** Boot configuration ***");

            if ( config->timeout >= 0 )
                drawItem(menu_index == 0, 0, "Timeout:  %i", config->timeout);
            else
                drawItem(menu_index == 0, 0, "Timeout:  Disabled");

            drawItem(menu_index == 1, 16, "Default:  %s", config->entries[config->index].title);
            drawItem(menu_index == 2, 32, "Recovery key:  %s", get_button(config->recovery));
            
            if ( config->passMaxAttempt > 0 )
                drawItem(menu_index == 4, 80, "Password allowed attempts:  %i", config->passMaxAttempt);
            else
                drawItem(menu_index == 4, 80, "Password allowed attempts:  Infinite");

            char msg[256];
            int size = 0;
            if ( pass_fail < 0 )
            {
                if (config->failPath[0] == '\0' || pass_fail_found)
                {
                    if (config->failSplashTop[0] != '\0' || config->failSplashBot[0] != '\0')
                        size = snprintf(msg, 256, "Freeze on failure images");
                    else if (config->splashTopDef[0] != '\0' || config->splashBotDef[0] != '\0')
                        size = snprintf(msg, 256, "Freeze on default splash screen");
                    else
                        size = snprintf(msg, 256, "Freeze on last rendered image");
                }
                else
                    size = snprintf(msg, 256, "Load \"%s\"", config->failPath);
            }
            else if ( pass_fail == config->count )
                size = snprintf(msg, 256, "Reboot");
            else if ( pass_fail == config->count+1 )
                size = snprintf(msg, 256, "Power off");
        #ifndef ARM9
            else if ( pass_fail == config->count+2 )
                size = snprintf(msg, 256, "Return to home menu");
        #endif
            else
                size = snprintf(msg, 256, "Load %s", config->entries[pass_fail].title);
            msg[size] = '\0';

            drawItem(menu_index == 5, 96, "Password fail behavior:  %s", msg);
            drawItem(menu_index == 6, 112, "Password size:  %i", pass_size);
            drawItem(menu_index == 7, 128, "Password:");
            
            size = 0;
            if ( config->passSize > 0 )
            {
                for ( int i = 0 ; i < config->passSize ; i++ )
                {
                    size += snprintf(msg+size, 256, get_button(config->password[i]));
                    if ( i < config->passSize-1 )
                        size += snprintf(msg+size, 256, (i%4 == 3) ? "\n" : ", ");
                }
            }
            else
                size = snprintf(msg, 256, "None");
            msg[size] = '\0';
            
            drawTextf(GFX_TOP, GFX_LEFT, &fontDefault, MENU_MIN_X + 64, MENU_MIN_Y + 128, msg);
            if ( IS3DACTIVE )
                drawTextf(GFX_TOP, GFX_RIGHT, &fontDefault, MENU_MIN_X + 64, MENU_MIN_Y + 128, msg);
            
        #ifdef ARM9
            drawItem(menu_index == 3, 48, "Screen initialization brightness:  %i", BRIGHTNESS_COUNT-config->brightness);  
        #else
            drawItem(menu_index == 3, 48, "Bootfix:  %i", config->autobootfix);
        #endif
        }
        
#ifdef ARM9
        drawInfo("CtrBootManager9 v2.5.0");    
#else
        drawInfo("CtrBootManager v2.5.0");
#endif

        swapFrameBuffers();
    }
    return -1;
}
