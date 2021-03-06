#ifndef ARM9

#include <3ds.h>
#include <string.h>
#include "brahma.h"

#else

#include "arm9/source/common.h"
#include "arm9/source/fatfs/ff.h"
#include "stage2_bin.h"
#include "memory.h"
#include "arm9/source/firm.h"
#include "arm9/source/screen.h"

#endif

#include <strings.h>
#include "draw.h"
#include "loader.h"
#include "utility.h"

int screenBehavior(gfxScreen_t screen, int screenSize, char* splash)
{
    if ( NULL == splash || splash[0] == '\0' )
        return 0;

    u8* fb = gfxGetFramebuffer(screen, GFX_LEFT, NULL, NULL);
    if ( splash[0] == '/' )
        fileRead(splash, fb, screenSize);
    else if ( strcasecmp(splash, "clear") == 0 )
        memset(fb, 0, screenSize);

    return 1;
}

void screensBehavior(char* splashTop, char* splashBot)
{
#ifndef ARM9
    gfxSet3D(false);
#endif

    int needSwap = screenBehavior(GFX_TOP, TOP_SCREEN_SIZE, splashTop);
    needSwap += screenBehavior(GFX_BOTTOM, BOT_SCREEN_SIZE, splashBot);
    
    if (needSwap)
        swapFrameBuffers();
}

#ifdef ARM9

unsigned char *memsearch(unsigned char *startPos, const void *pattern, int size, int patternSize) // From Luma3DS' pathchanger.c
{
    const unsigned char *patternc = (const unsigned char *)pattern;

    //Preprocessing
    int table[256];

    int i;
    for(i = 0; i < 256; ++i)
        table[i] = patternSize + 1;
    for(i = 0; i < patternSize; ++i)
        table[patternc[i]] = patternSize - i;

    //Searching
    int j = 0;

    while(j <= size - patternSize)
    {
        if(memcmp(patternc, startPos + j, patternSize) == 0)
            return startPos + j;
        j += table[startPos[j + patternSize]];
    }

    return NULL;
}

void applyPatches(unsigned char*  binaryData, size_t binarySize, long offset, binary_patch* patches, int patchesCount)
{
    for (int i = 0 ; i < patchesCount ; i++)
    {
        unsigned char* binFound = binaryData;
        int curOccurence = 0;
        while ( NULL != (binFound = memsearch(binFound, patches[i].memToSearch, binarySize-offset+binaryData-binFound, patches[i].memToSearchSize)) )
        {
            curOccurence++;
            if ( patches[i].occurence == 0 || curOccurence == patches[i].occurence )
            {
                memcpy(binFound, patches[i].memOverwrite, patches[i].memOverwriteSize);
                
                if ( patches[i].occurence > 0 )
                    break;
                binFound += patches[i].memOverwriteSize;
            }
            else
                binFound += patches[i].memToSearchSize;
        }
    }
}

int load_bin(char *path, long offset, binary_patch* patches, int patchesCount, char* splashTop, char* splashBot) {

    // Load binary
    size_t payloadSize = fileSize(path);
    if (!payloadSize) {
        return -1;
    }

    if (fileReadOffset(path, (void *) PTR_PAYLOAD_MAIN_DATA, PTR_PAYLOAD_SIZE_MAX, offset) != 0) {
        return -1;
    }

    applyPatches((unsigned char*)PTR_PAYLOAD_MAIN_DATA, payloadSize, offset, patches, patchesCount);
    screensBehavior(splashTop, splashBot);

    memcpy((void *) PTR_PAYLOAD_STAGE2, stage2_bin, stage2_bin_size);
    ((void (*)()) PTR_PAYLOAD_STAGE2)();

    return 0;
}

static Firm *const firm = (Firm *const)PTR_PAYLOAD_FIRM_DATA;
extern struct fb fbs[2];

int load_firm(char *path, binary_patch* patches, int patchesCount, char* splashTop, char* splashBot)
{
    u32 *loaderAddress = (u32 *)PTR_PAYLOAD_FIRM;
    u32 maxPayloadSize = (u32)((u8 *)loaderAddress - (u8 *)firm);

    size_t payloadSize = fileSize(path);
    if (!payloadSize || payloadSize <= 0x200 || payloadSize > maxPayloadSize) {
        return -1;
    }

    if (fileRead(path, firm, payloadSize) != 0) {
        return -1;
    }

    if(!checkFirmPayload(firm, payloadSize)) {
        return -1;
    }

    applyPatches((unsigned char*)firm, payloadSize, sizeof(Firm), patches, patchesCount);
    screensBehavior(splashTop, splashBot);

    // Luma3DS loading support
    char absPath[256];
    char *argv[2] = {computeFullPath(path, absPath), (char *)fbs};

    launchFirm((firm->reserved2[0] & 1) ? 2 : 1, argv);
    return 0;
}

#else
char boot_app[512];
bool boot_app_enabled;

int load_3dsx(char *path, char* splashTop, char* splashBot) {
    
    screensBehavior(splashTop, splashBot);
    memset(boot_app, 0, 512);
    strncpy(boot_app, path, 512);
    boot_app_enabled = true;
    return 0;
}

int load_bin(char *path, long offset, binary_patch* patches, int patchesCount, char* splashTop, char* splashBot) {

    screensBehavior(splashTop, splashBot);
    gfxExit();

    if (brahma_init()) {
        if (load_arm9_payload_offset(path, (u32) offset, 0x10000) != 1) {
            debug("Err: Couldn't load arm9 payload...\n");
            return -1;
        }
        firm_reboot();
        brahma_exit();

    } else {
        debug("Err: Couldn't init brahma...\n");
        return -1;
    }

    return 0;
}

#endif

int load(char *path, long offset, binary_patch* patches, int patchesCount, char* splashTop, char* splashBot) {
    // check for reboot/poweroff
    if (strcasecmp(path, "reboot") == 0) {
        reboot();
    } else if (strcasecmp(path, "shutdown") == 0) {
        poweroff();
#ifndef ARM9
    } else if (strcasecmp(path, "homemenu") == 0) {
        return load_homemenu();
#endif
    } else if (NULL != path && path[0] != '\0') {
        
        const char *ext = get_filename_ext(path);
        if (strcasecmp(ext, "bin") == 0 || strcasecmp(ext, "dat") == 0) {
            return load_bin(path, offset, patches, patchesCount, splashTop, splashBot);
#ifdef ARM9
        } else if (strcasecmp(ext, "firm") == 0) {
            return load_firm(path, patches, patchesCount, splashTop, splashBot);
#else
        } else if (strcasecmp(ext, "3dsx") == 0) {
            return load_3dsx(path, splashTop, splashBot);
#endif
        } else {
            debug("Invalid file: %s\n", path);
            return -1;
        }
    }
    return 0;
}
