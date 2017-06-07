#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#ifdef ARM9

#include "arm9/source/common.h"
#include "arm9/source/hid.h"
#include "arm9/source/i2c.h"
#include "arm9/source/fatfs/ff.h"
#include "memory.h"

#else
    
#include <3ds.h>
#include <CakeBrah/source/libkhax/khax.h>
#include <sys/unistd.h>

#endif

#include "draw.h"
#include "menu.h"
#include "utility.h"

#ifdef ARM9

void waitcycles(uint32_t val);

#define SD_CARD_NAME "0:"
#define SD_CARD_PATH "sdmc:"
#define SD_CARD_PATH_SIZE 5
#define SYSNAND_NAME "1:"
#define SYSNAND_PATH "nand:"
#define SYSNAND_PATH_SIZE 5

typedef struct
{
    char* driveNames[MANAGED_DRIVES_COUNT];
    char* drivePathes[MANAGED_DRIVES_COUNT];
    int drivePathSizes[MANAGED_DRIVES_COUNT];
    bool driveWritable[MANAGED_DRIVES_COUNT];
    FATFS driveFS[MANAGED_DRIVES_COUNT];
    int mountedDrives[MANAGED_DRIVES_COUNT];
    int curDrive;
    int defaultDrive;
} file_systems_s;

file_systems_s *fsConfig;

void initFileSystems(bool isLoadedFromSD)
{
    fsConfig = (file_systems_s*)PTR_FILE_SYSTEMS;
    
    fsConfig->driveNames[SD_CARD_ID] = SD_CARD_NAME;
    fsConfig->driveNames[SYSNAND_ID] = SYSNAND_NAME;
    
    fsConfig->drivePathes[SD_CARD_ID] = SD_CARD_PATH;
    fsConfig->drivePathes[SYSNAND_ID] = SYSNAND_PATH;

    fsConfig->drivePathSizes[SD_CARD_ID] = SD_CARD_PATH_SIZE;
    fsConfig->drivePathSizes[SYSNAND_ID] = SYSNAND_PATH_SIZE;

    fsConfig->driveWritable[SD_CARD_ID] = true;
    fsConfig->driveWritable[SYSNAND_ID] = false;

    fsConfig->mountedDrives[SD_CARD_ID] = -1;
    fsConfig->mountedDrives[SYSNAND_ID] = -1;

    fsConfig->defaultDrive = (isLoadedFromSD ? SD_CARD_ID : SYSNAND_ID);
    fsConfig->curDrive = fsConfig->defaultDrive;
    fsConfig->mountedDrives[fsConfig->curDrive] =
            (f_mount(&fsConfig->driveFS[fsConfig->curDrive], fsConfig->driveNames[fsConfig->curDrive], 0) == FR_OK
            && f_chdrive(fsConfig->driveNames[fsConfig->curDrive]) == FR_OK) ? 1 : 0;
}

const char* prepareFileSystemForPath(const char* path)
{
    if (NULL == path)
        return NULL;

    const char* relatPath = NULL;
    if (path[0] == '/')
    {
        if (fsConfig->defaultDrive != fsConfig->curDrive)
        {
            // We assume default drive is always mounted.
            fsConfig->curDrive = fsConfig->defaultDrive;
            f_chdrive(fsConfig->driveNames[fsConfig->curDrive]);
        }
        relatPath = path;
    }
    else
    {
        int driveSize = 0;
        while (path[driveSize] != '\0' && path[driveSize] != '/')
            driveSize++;
        if (path[driveSize] == '\0')
            return NULL;

        for (int selDrive = 0 ; selDrive < MANAGED_DRIVES_COUNT ; selDrive++)
        {
            if (driveSize == fsConfig->drivePathSizes[selDrive] && memcmp(path, fsConfig->drivePathes[selDrive], driveSize) == 0)
            {
                relatPath = &path[driveSize];
                fsConfig->curDrive = selDrive;
                if (fsConfig->mountedDrives[selDrive] < 0)
                {
                    fsConfig->mountedDrives[selDrive] =
                            (f_mount(&fsConfig->driveFS[selDrive], fsConfig->driveNames[selDrive], 0) == FR_OK) ? 1 : 0;
                }
                f_chdrive(fsConfig->driveNames[selDrive]);
                break;
            }
        }
    }
    return (1 == fsConfig->mountedDrives[fsConfig->curDrive]) ? relatPath : NULL;
}

bool isDefaultDriveReadOnly()
{
    return !fsConfig->driveWritable[fsConfig->defaultDrive];
}

bool switchCurrentDrive(unsigned int driveID)
{
    if (driveID >= MANAGED_DRIVES_COUNT)
        return false;
    if (fsConfig->curDrive == driveID)
        return true;
    
    if (fsConfig->mountedDrives[driveID] < 0)
    {
        fsConfig->mountedDrives[driveID] =
                (f_mount(&fsConfig->driveFS[driveID], fsConfig->driveNames[driveID], 0) == FR_OK) ? 1 : 0;
    }
    if (fsConfig->mountedDrives[fsConfig->curDrive] < 1)
        return false;

    fsConfig->curDrive = driveID;
    f_chdrive(fsConfig->driveNames[driveID]);
    return true;
}

void setCurrentDriveAsDefault()
{
    switchCurrentDrive(fsConfig->defaultDrive);
}

int getDefaultDrive()
{
    return fsConfig->defaultDrive;
}

char* computeFullPath(const char* relative, char* absolute)
{
    if (NULL == relative || NULL == absolute)
        return NULL;
    if (relative[0] == '/')
        sprintf(absolute, "%s%s", fsConfig->drivePathes[fsConfig->curDrive], relative);
    else
        sprintf(absolute, "%s", relative);
    return absolute;
}

#else

FS_Archive sdmcArchive;
extern void __appExit();

void openSDArchive() {
    FSUSER_OpenArchive(&sdmcArchive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));
}

void closeSDArchive() {
    FSUSER_CloseArchive(sdmcArchive);
}

#endif

void svcSleep(u32 millis) {
    u64 nano = millis * 1000000;
#ifndef ARM9
    svcSleepThread(nano);
#else
    waitcycles((u32) nano);
#endif
}

bool end_with(const char *str, const char c) {
    return (str && *str && str[strlen(str) - 1] == c) ? true : false;
}

const char *get_filename_ext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return "";
    return dot + 1;
}

char *get_button(int button) {

    switch (BIT(button)) {
        case KEY_A:
            return "A";

        case KEY_B:
            return "B";

        case KEY_SELECT:
            return "SELECT";

        case KEY_START:
            return "START";

        case KEY_DRIGHT:
            return "D-PAD RIGHT";

        case KEY_DLEFT:
            return "D-PAD LEFT";

        case KEY_DUP:
            return "D-PAD UP";

        case KEY_DDOWN:
            return "D-PAD DOWN";

        case KEY_R:
            return "R";

        case KEY_L:
            return "L";

        case KEY_X:
            return "X";

        case KEY_Y:
            return "Y";

        default:
            return "Invalid button";
    }
}

void debug(const char *fmt, ...) {
    char s[512];
    memset(s, 0, 512);
    va_list args;
    va_start(args, fmt);
    vsprintf(s, fmt, args);
    va_end(args);

    while (aptMainLoop()) {

        hidScanInput();
        if (hidKeysDown() & KEY_A)
            break;

        drawBg();
        drawText(GFX_TOP, GFX_LEFT, &fontDefault, s, MENU_MIN_X + 16, MENU_MIN_Y + 16);
        drawText(GFX_TOP, GFX_LEFT, &fontDefault, "Press (A) key to continue...", MENU_MIN_X + 16, MENU_MIN_Y + 64);
        if ( IS3DACTIVE )
        {
            drawText(GFX_TOP, GFX_RIGHT, &fontDefault, s, MENU_MIN_X + 16, MENU_MIN_Y + 16);
            drawText(GFX_TOP, GFX_RIGHT, &fontDefault, "Press (A) key to continue...", MENU_MIN_X + 16, MENU_MIN_Y + 64);
        }
        swapFrameBuffers();
    }
}

bool confirm(int confirmButton, const char *fmt, ...) {
    char s[512];
    memset(s, 0, 512);
    va_list args;
    va_start(args, fmt);
    vsprintf(s, fmt, args);
    va_end(args);

    while (aptMainLoop()) {

        hidScanInput();
        u32 key = hidKeysDown();
        if (key & BIT(confirmButton)) {
            return true;
        } else if (key & KEY_B) {
            return false;
        }

        drawBg();
        drawText(GFX_TOP, GFX_LEFT, &fontDefault, s, MENU_MIN_X + 16, MENU_MIN_Y + 16);
        drawText(GFX_TOP, GFX_LEFT, &fontDefault, "Press (B) key to cancel...", MENU_MIN_X + 16, MENU_MIN_Y + 64);
        drawTextf(GFX_TOP, GFX_LEFT, &fontDefault, MENU_MIN_X + 16, MENU_MIN_Y + 84, "Press (%s) to confirm...",
                     get_button(confirmButton));
        if ( IS3DACTIVE )
        {
            drawText(GFX_TOP, GFX_RIGHT, &fontDefault, s, MENU_MIN_X + 16, MENU_MIN_Y + 16);
            drawText(GFX_TOP, GFX_RIGHT, &fontDefault, "Press (B) key to cancel...", MENU_MIN_X + 16, MENU_MIN_Y + 64);
            drawTextf(GFX_TOP, GFX_RIGHT, &fontDefault, MENU_MIN_X + 16, MENU_MIN_Y + 84, "Press (%s) to confirm...",
                         get_button(confirmButton));
        }
        swapFrameBuffers();
    }
    return false;
}

bool fileExists(const char *path) {
#ifdef ARM9
    path = prepareFileSystemForPath(path);
    FIL file;
    if (f_open(&file, path, FA_READ) != FR_OK) {
        return false;
    }
    f_close(&file);
#else
    if (!path)return false;

    Result ret;
    Handle fileHandle;

    ret = FSUSER_OpenFile(&fileHandle, sdmcArchive, fsMakePath(PATH_ASCII, path),
                          FS_OPEN_READ, 0);
    if (ret != 0) return false;

    ret = FSFILE_Close(fileHandle);
    if (ret != 0) return false;
#endif
    return true;
}

size_t fileSize(const char *path) {
    size_t size = -1;
#ifdef ARM9
    path = prepareFileSystemForPath(path);
    FIL file;
    if (f_open(&file, path, FA_READ | FA_OPEN_EXISTING) != FR_OK) {
        return size;
    }
    size = f_size(&file);
    f_close(&file);
#else
    FILE *file;
    if((file = fopen(path, "rt")) == NULL) {
        return size;
    }
    fseek(file, 0, SEEK_END);
    size = ftell(file);
    fclose(file);
#endif
    return size;
}

int fileReadOffset(const char *path, void *data, size_t size, u32 offset) {

#ifdef ARM9
    path = prepareFileSystemForPath(path);
    FIL file;
    UINT bytes_read = 0;
    if (f_open(&file, path, FA_READ | FA_OPEN_EXISTING) != FR_OK) {
        return -1;
    }
    f_lseek(&file, offset);
    u32 newSize = size - offset;
    if (f_read(&file, data, newSize, &bytes_read) != FR_OK) {
        f_close(&file);
        return -1;
    }
    f_close(&file);
#else
    FILE *file;
    if((file = fopen(path, "rt")) == NULL) {
        return -1;
    }
    fseek( file, offset, SEEK_SET );
    u32 newSize = size - offset;
    if(!fread(data, newSize, 1, file)) {
        fclose(file);
        return -1;
    }
    fclose(file);
#endif
    return 0;
}

int fileRead(const char *path, void *data, size_t size) {
    return fileReadOffset(path, data, size, 0);
}

int fileWrite(const char *path, void *data, size_t size) {
#ifdef ARM9
    path = prepareFileSystemForPath(path);
    if (!fsConfig->driveWritable[fsConfig->curDrive])
        return -1;

    FIL file;
    unsigned int br = 0;
    f_unlink(path);
    if(f_open(&file, path, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) {
        return -1;
    }
    f_write(&file, data, size, &br);
    f_close(&file);
#else
    unlink(path);
    FILE *file = fopen(path, "w");
    fwrite(data, 1 , size, file);
    fclose(file);
#endif
    return size;
}

int getFileHandleSize()
{
#ifdef ARM9
    return sizeof(FIL);
#else
    return sizeof(FILE*);
#endif
}

int fileHandleOpen(void* handlePtr, const char *filePath)
{
#ifdef ARM9
    filePath = prepareFileSystemForPath(filePath);
    return (f_open((FIL*)handlePtr, filePath, FA_READ | FA_OPEN_EXISTING) == FR_OK) ? 0 : -1;
#else
    *(FILE**)handlePtr = fopen(filePath, "rb");
    return ( NULL != *(FILE**)handlePtr ) ? 0 : -1;
#endif
}

size_t fileHandleSize(void* handlePtr)
{
#ifdef ARM9
    return f_size((FIL*)handlePtr);
#else
    fseek(*(FILE**)handlePtr, 0, SEEK_END);
    return ftell(*(FILE**)handlePtr);
#endif
}

int fileHandleRead(void* handlePtr, void *data, size_t size, u32 offset)
{
#ifdef ARM9
    UINT bytes_read = 0;
    f_lseek((FIL*)handlePtr, offset);
    if (f_read((FIL*)handlePtr, data, size, &bytes_read) != FR_OK)
        return -1;
    return ( bytes_read > 0 ) ? bytes_read : -1;
#else
    fseek(*(FILE**)handlePtr, offset, SEEK_SET);
    u32 bytes_read = fread(data, 1, size, *(FILE**)handlePtr);
    return ( bytes_read > 0 ) ? bytes_read : -1;
#endif
}

int fileHandleClose(void* handlePtr)
{
#ifdef ARM9
    return f_close((FIL*)handlePtr);
#else
    return fclose(*(FILE**)handlePtr);
#endif
}

int load_homemenu() {
#ifdef ARM9
    debug("load_homemenu not implemented");
    return -1;
#else
    Handle kill = 0;

    if (srvGetServiceHandle(&kill, "hb:kill") != 0) {
        return -1;
    }

    __appExit();
    srvExit();
    svcSignalEvent(kill);
    svcExitProcess();
#endif
}

void reboot() {
#ifdef ARM9
    i2cWriteRegister(I2C_DEV_MCU, 0x20, 1 << 2);
    while (true);
#else
    aptInit();
    //aptOpenSession();
    APT_HardwareResetAsync();
    //aptCloseSession();
    aptExit();
#endif
}

void poweroff() {
#ifdef ARM9
    i2cWriteRegister(I2C_DEV_MCU, 0x20, 1 << 0);
    while (true);
#else
    if (khaxInit() != 0) {
        debug("Err: khaxInit");
        return;
    }

    Handle handle;
    Result ret = srvGetServiceHandle(&handle, "ptm:s");
    if (ret != 0) {
        debug("Err: srvGetServiceHandle(ptm:s)");
        return;
    }

    u32 *cmdbuf = getThreadCommandBuffer();
    cmdbuf[0] = 0x040700C0; //ShutdownAsync
    cmdbuf[1] = 0; //?
    cmdbuf[2] = 0; //?
    cmdbuf[3] = 0; //?
    ret = svcSendSyncRequest(handle);
    if (ret != 0) {
        debug("Err: srvGetServiceHandle(ptm:s)");
    }
    svcCloseHandle(handle);
#endif
}

#ifdef ARM9

bool aptMainLoop() {
    return true;
}

void* memAlloc(unsigned int iSize) {
    static u8* nextFreeMem = PTR_DYNAMIC_MEM_START;
    void* retPtr = (void*)nextFreeMem;
    nextFreeMem += iSize;
    return retPtr;
}

#endif
