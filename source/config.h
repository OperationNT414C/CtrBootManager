#ifndef _config_h_
#define _config_h_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ARM9

#include "arm9/source/common.h"
#define CONFIG_PATH "/a9lh.cfg\0Free space for binary patch to chainload another instance of CtrBootManager9... Space ends up now!"

#else
    
#include <3ds.h>
#include "utility.h"
#define CONFIG_PATH "/boot.cfg"

#endif

#include "draw.h"
#include "loader.h" // Binary patches define

#define BIT(n) (1U<<(n))

#define PASSWORD_MAX_SIZE 16
#define MAX_KEY 11

#define DEFAULT_ENTRIES_COUNT 11
#define ENTRIES_COUNT_INC 12
#define PATCHES_MAX_PER_ENTRY 4

#define FILE_STREAM 0
#define MEMORY_STREAM 1
#define MEMORY_COMPRESSED_STREAM 2

typedef struct {
    char path[128];
    int posX;
    int posY;
    int sizeX;
    int sizeY;
    bool isRGBA;
    u8 *imgBuff;
    int loaded;
} thumbnail_s;

typedef struct {
    char title[64];
    char path[128];
    int key;
    long offset;
    binary_patch patches[PATCHES_MAX_PER_ENTRY];
    int patchesCount;

    char splashTop[128];
    char splashBot[128];

    thumbnail_s thumbTop;
    thumbnail_s thumbTop3D;
    thumbnail_s thumbBot;
} boot_entry_s;

typedef struct {
    char path[128];
    int compressed;
    int loopCount;
    int loopReverse;
    int loopStreamType;
    int loopStartFrame;
    int loopTimeOnStartFrame;
    int loopEndFrame;
    int loopTimeOnEndFrame;
} movie_config_s;

typedef struct {
    int timeout;
    int autobootfix;
    int brightness;
    int index;
    int recovery;
    
    int password[PASSWORD_MAX_SIZE];
    int passSize;
    int passMaxAttempt;
    char failPath[128];
    long failOffset;
    char failSplashTop[128];
    char failSplashBot[128];

    int count;
    int maxCount;
    boot_entry_s* entries;

    char splashTopDef[128];
    char splashBotDef[128];
    
    u8 bgTop1[3];
    u8 bgTop2[3];
    u8 bgBot[3];
    u8 highlight[4];
    u8 borders[4];
    u8 fntDef[4];
    u8 fntSel[4];
    u8 fntBot[4];
    bool fntBotActive;

    u8 bgTop1AnimColor[3];
    u8 bgTop2AnimColor[3];
    u8 bgBotAnimColor[3];
    u8 highlightAnimColor[4];
    u8 bordersAnimColor[4];
    u8 fntDefAnimColor[4];
    u8 fntSelAnimColor[4];
    u8 fntBotAnimColor[4];

    int bgTop1AnimTime;
    int bgTop2AnimTime;
    int bgBotAnimTime;
    int highlightAnimTime;
    int bordersAnimTime;
    int fntDefAnimTime;
    int fntSelAnimTime;
    int fntBotAnimTime;

    int bgTop1AnimTimeStart;
    int bgTop2AnimTimeStart;
    int bgBotAnimTimeStart;
    int highlightAnimTimeStart;
    int bordersAnimTimeStart;
    int fntDefAnimTimeStart;
    int fntSelAnimTimeStart;
    int fntBotAnimTimeStart;

    int menuFadeInTime;
    int menuFadeInTimeStart;
    int bgImgTopFadeInTime;
    int bgImgTopFadeInTimeStart;
    int bgImgBotFadeInTime;
    int bgImgBotFadeInTimeStart;

    char bgImgTop[128];
    char bgImgTop3D[128];
    char bgImgBot[128];
    bool imgTopIsRGBA;
    bool imgTop3DIsRGBA;
    bool imgBotIsRGBA;
    bool imgError;
    bool imgError3D;
    bool imgErrorBot;
    u8 *bgImgTopBuff;
    u8 *bgImgTop3DBuff;
    u8 *bgImgBotBuff;
    off_t bgImgTopSize;
    off_t bgImgTop3DSize;
    off_t bgImgBotSize;

    movie_config_s movieTop;
    movie_config_s movieTop3D;
    movie_config_s movieBot;
} boot_config_s;

boot_config_s *config;

int configInit();

int configAddEntry(char *title, char *path, long offset);

int configRemoveEntry(int index);

void configSave();

void configExit();

void loadBg(gfxScreen_t screen, gfx3dSide_t side);

#ifdef __cplusplus
}
#endif
#endif // _config_h_
