#ifdef ARM9

#include "arm9/source/common.h"
#include "arm9/source/hid.h"
#include "screeninit.h"

#else
#include <3ds.h>
#include <time.h>
#endif

#include "loader.h"
#include "draw.h"
#include "utility.h"
#include "menu.h"
#include "config.h"
#include "anim.h"

static bool timer = true;

static void draw(int boot_index, time_t elapsed);

int key_override(int index) {

    hidScanInput();
    u32 k = hidKeysHeld();
    if (k) {
        int i = 0;
        for (i = 0; i < config->count; i++) {
            if (k & BIT(config->entries[i].key)) {
                return i;
            }
        }
    }
    return index;
}

int boot(int index) {

#ifndef ARM9
    int delay = config->autobootfix;
    while (aptMainLoop() && delay > 0) {
        swapFrameBuffers();
        delay--;
    }
#endif
    boot_entry_s* entry = &config->entries[index];
    return load(entry->path, entry->offset, entry->patches, entry->patchesCount,
                splashScreen(entry->splashTop, config->splashTopDef),
                splashScreen(entry->splashBot, config->splashBotDef));
}

int menu_boot() {

#ifdef ARM9
    time_t elapsed = 0;
    vu32 now = 0;
#else
    time_t start, end, elapsed = 0;
#endif
    int boot_index = config->index;

    hidScanInput();
    u32 key = hidKeysHeld();
    if (key & BIT(config->recovery) || config->timeout < 0) { // disable autoboot
        timer = false;
    } else if (config->timeout == 0 || key_override(boot_index) != boot_index) { // autoboot
    #ifdef ARM9
        if (config->directBootScreenInit)
            initScreens(config->brightness);
        else
        {
            // Screen was not initialize, we must absolutly avoid any splash screen!
            boot_entry_s* entry = &config->entries[key_override(boot_index)];
            entry->splashTop[0] = '\0';
            entry->splashBot[0] = '\0';
            config->splashTopDef[0] = '\0';
            config->splashBotDef[0] = '\0';
        }
    #endif
        return boot(key_override(boot_index));
    }

#ifndef ARM9
    time(&start);
#else
    initScreens(config->brightness);
#endif

    static const u32 checkedKeys = 0x0FFF;
    
    // Trash first key
    hidScanInput();
    hidKeysDown();
    
    while (aptMainLoop()) {

        hidScanInput();
        u32 kDown = hidKeysDown();

        if (timer) {
#ifdef ARM9
            now++;
            elapsed = (int)(now/25); // fake/crappy timer
#else
            time(&end);
            elapsed = end - start;
#endif
            if (elapsed >= config->timeout && config->count > boot_index) {
                int index = key_override(boot_index);
                return boot(index);
            }

            if (kDown & checkedKeys)
                timer = false;
        }

        if (!timer)
        {
            if ( menu_password() )
                kDown = 0;

            if (kDown & KEY_DOWN) {
                boot_index++;
                if (boot_index > config->count)
                    boot_index = 0;
            }
            else if (kDown & KEY_UP) {
                boot_index--;
                if (boot_index < 0)
                    boot_index = config->count;
            }
            if (kDown & KEY_RIGHT) {
                boot_index = (boot_index == config->count) ? 0 : config->count;
            }
            else if (kDown & KEY_LEFT) {
                boot_index = (boot_index == 0) ? config->count : 0;
            }
            else if (kDown & KEY_A) {
                if (boot_index == config->count) {
                    if(menu_more() == 0) {
                        break;
                    }
                } else if (boot(boot_index) == 0) {
                    break;
                }
            }
            else if (kDown & KEY_X) {
                if (boot_index != config->count) {
                    if (confirm(3, "Delete boot entry: \"%s\" ?\n", config->entries[boot_index].title)) {
                        configRemoveEntry(boot_index);
                        boot_index = config->index;
                    }
                }
            }
        }

        draw(boot_index, elapsed);
    }

    return 0;
}

void drawThumbnail(gfxScreen_t screen, gfx3dSide_t side, thumbnail_s* thumbnail)
{
    if ( NULL == thumbnail->imgBuff )
        return;
    if ( 0 == thumbnail->loaded )
    {
        // Lazy image load
        size_t size = fileSize(thumbnail->path);
        if ( size == -1 || size == 0 ) {
            thumbnail->loaded = -1;
            return;
        }

        size_t maxsize = thumbnail->sizeX * thumbnail->sizeY * (thumbnail->isRGBA?4:3);
        if (size > maxsize)
            size = maxsize;
        
        if (fileRead(thumbnail->path, thumbnail->imgBuff, size) != 0) {
            thumbnail->loaded = -1;
            return;
        }
        
        thumbnail->loaded = 1;
    }
    if ( thumbnail->loaded <= 0 )
        return;

    drawImage(screen, side, thumbnail->imgBuff, thumbnail->isRGBA, thumbnail->sizeX, thumbnail->sizeY, thumbnail->posX, thumbnail->posY, anim->thumbFade);
}

static void draw(int boot_index, time_t elapsed) {

    int i = 0;

    drawBg();
    
    if ( boot_index < config->count )
    {
        boot_entry_s* selEntry = &config->entries[boot_index];
        drawThumbnail(GFX_TOP, GFX_LEFT, &selEntry->thumbTop);
        if ( IS3DACTIVE )
            drawThumbnail(GFX_TOP, GFX_RIGHT, &selEntry->thumbTop3D);
        drawThumbnail(GFX_BOTTOM, GFX_LEFT, &selEntry->thumbBot);
    }

    if (!timer) {
        drawTitle("*** Select a boot entry ***");
    } else {
        drawTitle("*** Booting %s in %i ***", config->entries[boot_index].title, config->timeout - elapsed);
    }

    int entriesStart = ENTRIES_COUNT_INC * (boot_index / ENTRIES_COUNT_INC);
    int entriesEnd = entriesStart + ENTRIES_COUNT_INC;
    if ( entriesEnd > config->count )
        entriesEnd = config->count;
    int displayedEntries = entriesEnd - entriesStart;
    
    for (i = 0; i < displayedEntries; i++) {
        boot_entry_s* curEntry = &config->entries[entriesStart+i];
        bool selected = (entriesStart+i == boot_index);
        drawItem(selected, 16 * i, curEntry->title);
        if (selected) {
        #ifdef ARM9
            if ( curEntry->patchesCount > 0 )
            {
                drawInfo("Name: %s\nPath: %s\nOffset: 0x%lx\nPatches count: %d\n\nPress (A) to launch\nPress (X) to remove entry\n",
                        curEntry->title,
                        curEntry->path,
                        curEntry->offset,
                        curEntry->patchesCount);
            }
            else
        #endif
            {
                drawInfo("Name: %s\nPath: %s\nOffset: 0x%lx\n\n\nPress (A) to launch\nPress (X) to remove entry\n",
                        curEntry->title,
                        curEntry->path,
                        curEntry->offset);
            }
        }
    }
    
    if ( i < ENTRIES_COUNT_INC )
    {
        bool selected = (boot_index == config->count);
        drawItem(selected, 16 * i, "More...");
        if (selected) {
            drawInfo("Show more options ...");
        }
    }

    swapFrameBuffers();
}
