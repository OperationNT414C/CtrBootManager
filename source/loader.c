#ifndef ARM9

#include <string.h>
#include <3ds.h>
#include "brahma.h"

#else

#include "arm9/source/common.h"
#include "arm9/source/fatfs/ff.h"
#include "stage2_bin.h"
#include "memory.h"

#endif

#include "draw.h"
#include "loader.h"
#include "utility.h"

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

int load_bin(char *path, long offset, binary_patch* patches, int patchesCount) {

    // Load binary
    size_t size = fileSize(path);
    if (!size) {
        return -1;
    }

    if (fileReadOffset(path, (void *) PTR_PAYLOAD_MAIN_DATA, PTR_PAYLOAD_SIZE_MAX, offset) != 0) {
        return -1;
    }
    
    // Apply patches
    unsigned char* oriBinary = (unsigned char*)PTR_PAYLOAD_MAIN_DATA;
    for (int i = 0 ; i < patchesCount ; i++)
    {
        unsigned char* binFound = oriBinary;
        int curOccurence = 0;
        while ( NULL != (binFound = memsearch(binFound, patches[i].memToSearch, size-offset+oriBinary-binFound, patches[i].memToSearchSize)) )
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

    // Start binary
    clearFrameBuffers();

    memcpy((void *) PTR_PAYLOAD_STAGE2, stage2_bin, stage2_bin_size);
    ((void (*)()) PTR_PAYLOAD_STAGE2)();

    return 0;
}

#else
char boot_app[512];
bool boot_app_enabled;

int load_3dsx(char *path) {
    memset(boot_app, 0, 512);
    strncpy(boot_app, path, 512);
    boot_app_enabled = true;
    return 0;
}

int load_bin(char *path, long offset, binary_patch* patches, int patchesCount) {

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

int load(char *path, long offset, binary_patch* patches, int patchesCount) {
    // check for reboot/poweroff
    if (strcasecmp(path, "reboot") == 0) {
        reboot();
    } else if (strcasecmp(path, "shutdown") == 0) {
        poweroff();
#ifndef ARM9
    } else if (strcasecmp(path, "homemenu") == 0) {
        return load_homemenu();
#endif
    } else {
        const char *ext = get_filename_ext(path);
        if (strcasecmp(ext, "bin") == 0
            || strcasecmp(ext, "dat") == 0) {
            return load_bin(path, offset, patches, patchesCount);
#ifndef ARM9
        } else if (strcasecmp(ext, "3dsx") == 0) {
            return load_3dsx(path);
#endif
        } else {
            debug("Invalid file: %s\n", path);
            return -1;
        }
    }
    return 0;
}
