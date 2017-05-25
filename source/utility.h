#ifndef _utility_h_
#define _utility_h_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ARM9

#define MANAGED_DRIVES_COUNT 2
#define SD_CARD_ID 0
#define SYSNAND_ID 1

void initFileSystems(bool isLoadedFromSD);

bool isDefaultDriveReadOnly();

bool switchCurrentDrive(unsigned int driveID);

void setCurrentDriveAsDefault();

int getDefaultDrive();

char* computeFullPath(const char* relative, char* absolute);

bool aptMainLoop();

void* memAlloc(unsigned int iSize);

#define START_DRIVE_RO (isDefaultDriveReadOnly())

#else

#include "descriptor.h"

typedef struct menuEntry_s {
    char executablePath[128 + 1];
    char arg[64 + 1];
    descriptor_s descriptor;
} menuEntry_s;

void openSDArchive();

void closeSDArchive();

#define START_DRIVE_RO false

#endif

bool fileExists(const char *path);

size_t fileSize(const char *path);

int fileReadOffset(const char *path, void *data, size_t size, u32 offset);

int fileRead(const char *path, void *data, size_t size);

int fileWrite(const char *path, void *data, size_t size);

int getFileHandleSize();

int fileHandleOpen(void* handlePtr, const char *filePath);

size_t fileHandleSize(void* handlePtr);

int fileHandleRead(void* handlePtr, void *data, size_t size, u32 offset);

int fileHandleClose(void* handlePtr);

void svcSleep(u32 millis);

const char *get_filename_ext(const char *filename);

bool end_with(const char *str, const char c);

char *get_button(int button);

void debug(const char *fmt, ...);

bool confirm(int confirmButton, const char *fmt, ...);

int load_homemenu();

void reboot();

void poweroff();

#ifdef __cplusplus
}
#endif
#endif // _utility_h_
