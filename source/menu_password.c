#include <stdio.h>

#ifdef ARM9

#include "arm9/source/common.h"
#include "arm9/source/hid.h"

#else

#include <3ds.h>
#include "menu_netloaderarm9.h"

#endif

#include "anim.h"
#include "config.h"
#include "draw.h"
#include "utility.h"
#include "loader.h"
#include "menu.h"

static bool passSuccess = false;
static int attempt = 0;
static int keyIndex = 0;

static void draw();

static u32 kDownPrev = 0;
static u32 kDown = 0;

int menu_password() {

    if ( passSuccess )
        return 0;

    if ( config->passSize <= 0 )
    {
        passSuccess = true;
        return 0;
    }
    
    static const u32 checkedKeys = 0x0FFF;

    hidScanInput();
    kDownPrev = (hidKeysDown() & checkedKeys);
    int validCount = 0;

    while (aptMainLoop()) {

        hidScanInput();
        kDown = (hidKeysDown() & checkedKeys);

        if ( 0 == kDownPrev && 0 != kDown )
        {
            if ( kDown & BIT(config->password[keyIndex]) )
                validCount++;
            keyIndex++;
            
            if ( keyIndex >= config->passSize )
            {
                if ( validCount >= config->passSize )
                {
                    passSuccess = true;
                    break;
                }
                
                keyIndex = 0;
                validCount = 0;
                attempt++;
                
                if ( config->passMaxAttempt > 0 && attempt >= config->passMaxAttempt )
                {
                    // Fail!
                    char* splashTop = splashScreen(config->failSplashTop, config->splashTopDef);
                    char* splashBot = splashScreen(config->failSplashBot, config->splashBotDef);
                    load(config->failPath, config->failOffset, NULL, 0, splashTop, splashBot);
                    while (aptMainLoop()) // Loop of death
                        screensBehavior(splashTop, splashBot);
                    return -1;
                }
            }
        }
        kDownPrev = kDown;

        draw();
    }
    return 1;
}

static void draw() {

    drawBg();
    drawTitle("*** Password required ***", kDownPrev, kDown);

    char passStars[PASSWORD_MAX_SIZE+1];
    int i = 0;
    for ( ; i < keyIndex ; i++ )
        passStars[i] = 'X';
    for ( ; i < config->passSize ; i++ )
        passStars[i] = '_';
    passStars[i] = '\0';
    
    char* invalidMsg = ( 0 == keyIndex && attempt > 0 ) ? "\nInvalid password, try again!\n" : "";
    char msg[256];
    if ( config->passMaxAttempt > 0 )
        sprintf(msg, "Password:  %s\n\nAttempt %d on %d\n%s", passStars, attempt+1, config->passMaxAttempt, invalidMsg);
    else
        sprintf(msg, "Password:  %s\n%s", passStars, invalidMsg);    

    drawTextf(GFX_TOP, GFX_LEFT, &fontDefault, MENU_MIN_X + 96, MENU_MIN_Y + 48, msg);
    if ( IS3DACTIVE )
        drawTextf(GFX_TOP, GFX_RIGHT, &fontDefault, MENU_MIN_X + 96, MENU_MIN_Y + 48, msg);

    drawInfo("Please enter the password for menu access...\n");
    
    swapFrameBuffers();
}
