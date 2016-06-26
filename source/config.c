//
// Created by cpasjuste on 29/02/16.
//
#include <string.h>
#include <stdlib.h>
#include <sys/unistd.h>
#include "ini.h"
#include "config.h"
#include "font.h"
#ifdef ARM9
#include "arm9/source/fatfs/ff.h"
#include "utility.h"
#include "memory.h"
#endif

#define SECTION_MATCH(s) strcmp(section, s) == 0
#define NAME_MATCH(n) strcmp(name, n) == 0

typedef struct {
    const char *ptr;
    int bytes_left;
} buffer_ctx;

void setColor(u8 *cfgColor, const char *color) {
    long l = strtoul(color, NULL, 16);
    cfgColor[0] = (u8) (l >> 16 & 0xFF);
    cfgColor[1] = (u8) (l >> 8 & 0xFF);
    cfgColor[2] = (u8) (l & 0xFF);
}

void setColorAlpha(u8 *cfgColor, const char *color) {
	if ( (color[6] >= '0' && color[6] <= '9') || (color[6] >= 'A' && color[6] <= 'F') || (color[6] >= 'a' && color[6] <= 'f') )
	{
		long l = strtoul(color, NULL, 16);
		cfgColor[0] = (u8) (l >> 24 & 0xFF);
		cfgColor[1] = (u8) (l >> 16 & 0xFF);
		cfgColor[2] = (u8) (l >> 8 & 0xFF);
		cfgColor[3] = (u8) (l & 0xFF);
	}
	else
	{
		setColor(cfgColor, color);
		cfgColor[3] = 0xFF;
	}
}

int setAnimTimes(int* animTime, int* animStart, char *item)
{
    int sep1Ind = 0;
    while (item[sep1Ind] != ':')
    {
        if (item[sep1Ind] == '\0')
            return -1;
        sep1Ind++;
    }
    
    item[sep1Ind] = '\0';
    *animTime = atoi(item);

    item = &item[sep1Ind+1];
    int sep2Ind = 0;
    while (item[sep2Ind] != ':')
        sep2Ind++;
    
    if ( item[sep2Ind] == '\0' )
        sep2Ind = -sep1Ind-3;
    else
        item[sep2Ind] = '\0';
    *animStart = atoi(item);

    return sep1Ind+sep2Ind+2;
}

void setAnimWithColor(int* animTime, int* animStart, u8 *cfgColor, char *item)
{
    int colorInd = setAnimTimes(animTime, animStart, item);
    if (colorInd < 2)
        return;
    setColor(cfgColor, &item[colorInd]);
}

void setAnimWithColorAlpha(int* animTime, int* animStart, u8 *cfgColor, char *item)
{
    int colorInd = setAnimTimes(animTime, animStart, item);
    if (colorInd < 2)
        return;
    setColorAlpha(cfgColor, &item[colorInd]);
}

void readStrAsHexData(const char *dataAsStr, char *data, int* dataSize) {
    *dataSize = strlen(dataAsStr)/2;
    for (int i = 0; i < *dataSize; i++)
    {
        if ( dataAsStr[0] >= '0' && dataAsStr[0] <= '9' )
            data[i] = (dataAsStr[0]-'0')<<4;
        else if ( dataAsStr[0] >= 'A' && dataAsStr[0] <= 'F' )
            data[i] = (10+dataAsStr[0]-'A')<<4;
        else if ( dataAsStr[0] >= 'a' && dataAsStr[0] <= 'f' )
            data[i] = (10+dataAsStr[0]-'a')<<4;
        else
            data[i] = 0;
        if ( dataAsStr[1] >= '0' && dataAsStr[1] <= '9' )
            data[i] += (dataAsStr[1]-'0');
        else if ( dataAsStr[1] >= 'A' && dataAsStr[1] <= 'F' )
            data[i] += (10+dataAsStr[1]-'A');
        else if ( dataAsStr[1] >= 'a' && dataAsStr[1] <= 'f' )
            data[i] += (10+dataAsStr[1]-'a');
        dataAsStr+=2;
    }
}

void writeHexDataAsStr(const char *data, int dataSize, char* dataAsStr) {
    
    char const hex_chars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
    for (int i = 0; i < dataSize; i++)
    {
        dataAsStr[0] = hex_chars[data[i]/16];
        dataAsStr[1] = hex_chars[data[i]%16];
        dataAsStr += 2;
    }
}

void readMovieProperty(movie_config_s* mvConf, const char *name, const char* value) {
    
    if (NAME_MATCH("path")) {
        strncpy(mvConf->path, value, 128);
    } else if (NAME_MATCH("compression")) {
        if ( value[0] >= '0' && value[0] <= '9' )
            mvConf->compressed = atoi(value);
        else if ( 0 == strcmp(value, "QuickLZ") )
            mvConf->compressed = 1;
    } else if (NAME_MATCH("loopCount")) {
        if ( value[0] == 'I' || value[0] == 'i' ) // Value for "INFINITE" or "infinite"
            mvConf->loopCount = -1;
        else
            mvConf->loopCount = atoi(value);
    } else if (NAME_MATCH("loopStreamType")) {
        if ( value[0] >= '0' && value[0] <= '9' )
            mvConf->loopStreamType = atoi(value);
        else if ( 0 == strcmp(value, "file") )
            mvConf->loopStreamType = FILE_STREAM;
        else if ( 0 == strcmp(value, "memory") )
            mvConf->loopStreamType = MEMORY_STREAM;
        else if ( 0 == strcmp(value, "compressed_memory") )
            mvConf->loopStreamType = MEMORY_COMPRESSED_STREAM;
    } else if (NAME_MATCH("loopReverse")) {
        if ( value[0] >= '0' && value[0] <= '9' )
            mvConf->loopReverse = atoi(value);
        else if ( value[0] == 'Y' || value[0] == 'y' || value[0] == 'T' || value[0] == 't' )
            mvConf->loopReverse = 1; // Value for "YES", "yes", "TRUE" and "true"
        else if ( value[0] == 'N' || value[0] == 'n' || value[0] == 'F' || value[0] == 'f' )
            mvConf->loopReverse = 0; // Value for "NO", "no", "FALSE" and "false"
    } else if (NAME_MATCH("loopStartFrame")) {
        mvConf->loopStartFrame = atoi(value);
    } else if (NAME_MATCH("delayOnLoopStart")) {
        mvConf->loopTimeOnStartFrame = atoi(value);
    } else if (NAME_MATCH("loopEndFrame")) {
        mvConf->loopEndFrame = atoi(value);
    } else if (NAME_MATCH("delayOnLoopEnd")) {
        mvConf->loopTimeOnEndFrame = atoi(value);
    }
}

void writeMovieProperty(movie_config_s* mvConf, char *cfg, int* sizePtr) {

    *sizePtr += snprintf(cfg+*sizePtr, 256, "path=%s;\n", mvConf->path);
    if ( 0 != mvConf->compressed )
        *sizePtr += snprintf(cfg+*sizePtr, 256, "compression=QuickLZ;\n");
    if ( 0 != mvConf->loopCount )
    {
        if ( mvConf->loopCount < 0 )
            *sizePtr += snprintf(cfg+*sizePtr, 256, "loopCount=infinite;\n");
        else
            *sizePtr += snprintf(cfg+*sizePtr, 256, "loopCount=%i;\n", mvConf->loopCount);
        if ( 0 != mvConf->loopStreamType )
        {
            if ( MEMORY_COMPRESSED_STREAM == mvConf->loopStreamType )
                *sizePtr += snprintf(cfg+*sizePtr, 256, "loopStreamType=compressed_memory;\n");
            else
                *sizePtr += snprintf(cfg+*sizePtr, 256, "loopStreamType=memory;\n");
        }
        if ( 0 != mvConf->loopReverse )
            *sizePtr += snprintf(cfg+*sizePtr, 256, "loopReverse=yes;\n");
        if ( mvConf->loopStartFrame > 0 )
            *sizePtr += snprintf(cfg+*sizePtr, 256, "loopStartFrame=%i;\n", mvConf->loopStartFrame);
        if ( mvConf->loopTimeOnStartFrame > 0 )
            *sizePtr += snprintf(cfg+*sizePtr, 256, "delayOnLoopStart=%i;\n", mvConf->loopTimeOnStartFrame);
        if ( mvConf->loopEndFrame >= 0 )
            *sizePtr += snprintf(cfg+*sizePtr, 256, "loopEndFrame=%i;\n", mvConf->loopEndFrame);
        if ( mvConf->loopTimeOnEndFrame > 0 )
            *sizePtr += snprintf(cfg+*sizePtr, 256, "delayOnLoopEnd=%i;\n", mvConf->loopTimeOnEndFrame);
    }
}

static char *ini_buffer_reader(char *str, int num, void *stream) {
    buffer_ctx *ctx = (buffer_ctx *) stream;
    int idx = 0;
    char newline = 0;

    if (ctx->bytes_left <= 0)
        return NULL;

    for (idx = 0; idx < num - 1; ++idx) {
        if (idx == ctx->bytes_left)
            break;

        if (ctx->ptr[idx] == '\n')
            newline = '\n';
        else if (ctx->ptr[idx] == '\r')
            newline = '\r';

        if (newline)
            break;
    }

    memcpy(str, ctx->ptr, idx);
    str[idx] = 0;

    ctx->ptr += idx + 1;
    ctx->bytes_left -= idx + 1;

    if (newline && ctx->bytes_left > 0 &&
        ((newline == '\r' && ctx->ptr[0] == '\n') ||
         (newline == '\n' && ctx->ptr[0] == '\r'))) {
        ctx->bytes_left--;
        ctx->ptr++;
    }
    return str;
}

static int handler(void *user, const char *section, const char *name,
                   const char *value) {

    char item[256];
    memset(item, 0, 256);
    if(value[strlen(value)-1] == ';') {
        strncpy(item, value, strlen(value)-1);
    } else {
        strncpy(item, value, strlen(value));
    }

    // general
    if (SECTION_MATCH("general"))
    {
        if (NAME_MATCH("timeout")) {
            config->timeout = atoi(item);
        } else if (NAME_MATCH("recovery")) {
            config->recovery = atoi(item);
        } else if (NAME_MATCH("default")) {
            config->index = atoi(item);
        }
    #ifndef ARM9
        else if (NAME_MATCH("autobootfix")) {
            config->autobootfix = atoi(item);
        }
    #endif
    }

    // theme
    else if (SECTION_MATCH("theme"))
    {
        if (NAME_MATCH("bgTop1")) {
            setColor(config->bgTop1, item);
        } else if (NAME_MATCH("bgTop2")) {
            setColor(config->bgTop2, item);
        } else if (NAME_MATCH("bgBottom")) {
            setColor(config->bgBot, item);
        } else if (NAME_MATCH("highlight")) {
            setColorAlpha(config->highlight, item);
        } else if (NAME_MATCH("borders")) {
            setColorAlpha(config->borders, item);
        } else if (NAME_MATCH("font1")) {
            setColorAlpha(config->fntDef, item);
        } else if (NAME_MATCH("font2")) {
            setColorAlpha(config->fntSel, item);
        } else if (NAME_MATCH("bgImgTop")) {
            strncpy(config->bgImgTop, item, 128);
        } else if (NAME_MATCH("bgImgBot")) {
            strncpy(config->bgImgBot, item, 128);
        }
    #ifndef ARM9
        else if (NAME_MATCH("bgImgTop3D")) {
            strncpy(config->bgImgTop3D, item, 128);
        }
    #endif
    }
    
    // animation
    else if (SECTION_MATCH("animation"))
    {
        if (NAME_MATCH("bgTop1")) {
            setAnimWithColor(&config->bgTop1AnimTime, &config->bgTop1AnimTimeStart, config->bgTop1AnimColor, item);
        } else if (NAME_MATCH("bgTop2")) {
            setAnimWithColor(&config->bgTop2AnimTime, &config->bgTop2AnimTimeStart, config->bgTop2AnimColor, item);
        } else if (NAME_MATCH("bgBottom")) {
            setAnimWithColor(&config->bgBotAnimTime, &config->bgBotAnimTimeStart, config->bgBotAnimColor, item);
        } else if (NAME_MATCH("highlight")) {
            setAnimWithColorAlpha(&config->highlightAnimTime, &config->highlightAnimTimeStart, config->highlightAnimColor, item);
        } else if (NAME_MATCH("borders")) {
            setAnimWithColorAlpha(&config->bordersAnimTime, &config->bordersAnimTimeStart, config->bordersAnimColor, item);
        } else if (NAME_MATCH("font1")) {
            setAnimWithColorAlpha(&config->fntDefAnimTime, &config->fntDefAnimTimeStart, config->fntDefAnimColor, item);
        } else if (NAME_MATCH("font2")) {
            setAnimWithColorAlpha(&config->fntSelAnimTime, &config->fntSelAnimTimeStart, config->fntSelAnimColor, item);
        } else if (NAME_MATCH("menuFadeIn")) {
            setAnimTimes(&config->menuFadeInTime, &config->menuFadeInTimeStart, item);
        }
    }
    
    // movies
    else if (SECTION_MATCH("topMovie")) {
        readMovieProperty(&config->movieTop, name, item);
    } else if (SECTION_MATCH("bottomMovie")) {
        readMovieProperty(&config->movieBot, name, item);
    }
#ifndef ARM9
    else if (SECTION_MATCH("top3DMovie")) {
        readMovieProperty(&config->movieTop3D, name, item);
    }
#endif
    
    // entries
    else if (SECTION_MATCH("entry"))
    {
        int entryInd = config->count;
        if ( entryInd < CONFIG_MAX_ENTRIES )
        {
            if (NAME_MATCH("title")) {
                strncpy(config->entries[entryInd].title, item, 64);
            } else if (NAME_MATCH("path")) {
                strncpy(config->entries[entryInd].path, item, 128);
            } else if (NAME_MATCH("offset")) {
                config->entries[entryInd].offset = strtoul(item, NULL, 16);
            }
            // End current entry
            else if (NAME_MATCH("key")) {
                if(strlen(config->entries[entryInd].title) > 0) {
                    config->entries[entryInd].key = atoi(item);
                    config->count++;
                }
            }
        #ifdef ARM9
            else
            {
                int patchInd = config->entries[entryInd].patchesCount;
                if ( patchInd < PATCHES_MAX_PER_ENTRY )
                {
                    // Binary patches for current entry
                    if (NAME_MATCH("patchMemSearch")) {
                        binary_patch* curPatch = &config->entries[entryInd].patches[patchInd];
                        readStrAsHexData(item, curPatch->memToSearch, &curPatch->memToSearchSize);
                    } else if (NAME_MATCH("patchMemOverwrite")) {
                        binary_patch* curPatch = &config->entries[entryInd].patches[patchInd];
                        readStrAsHexData(item, curPatch->memOverwrite, &curPatch->memOverwriteSize);
                    } else if (NAME_MATCH("patchMemOverwriteStr")) {
                        binary_patch* curPatch = &config->entries[entryInd].patches[patchInd];
                        curPatch->memOverwriteSize = strlen(item)+1;
                        memcpy(curPatch->memOverwrite, item, curPatch->memOverwriteSize);
                    } else if (NAME_MATCH("patchMemOverwriteWStr")) {
                        binary_patch* curPatch = &config->entries[entryInd].patches[patchInd];
                        int strSize = strlen(item)+1;
                        curPatch->memOverwriteSize = strSize*2;
                        for (int i = 0; i < strSize; i++)
                        {
                            curPatch->memOverwrite[2*i] = item[i];
                            curPatch->memOverwrite[2*i+1] = '\0';
                        }
                    } else if (NAME_MATCH("patchOccurence")) {
                        config->entries[entryInd].patches[patchInd].occurence = atoi(item);
                        config->entries[entryInd].patchesCount++;
                    }
                }
            }
        #endif
        }
    }
    return 0;
}

void configMovieInit(movie_config_s* mvConf)
{
    mvConf->path[0] = '\0';
    mvConf->compressed = 0;
    mvConf->loopCount = 0;
    mvConf->loopStreamType = 0;
    mvConf->loopReverse = 0;
    mvConf->loopStartFrame = 0;
    mvConf->loopTimeOnStartFrame = 0;
    mvConf->loopEndFrame = -1;
    mvConf->loopTimeOnEndFrame = 0;
}

void configThemeInit() {

    // theme
    config->imgError = true;
    config->imgError3D = true;
    config->imgErrorBot = true;
    config->bgImgTop[0] = '\0';
    config->bgImgTop3D[0] = '\0';
    config->bgImgBot[0] = '\0';
    memcpy(config->bgTop1, (u8[3]) {0x4a, 0x00, 0x31}, sizeof(u8[3]));
    memcpy(config->bgTop2, (u8[3]) {0x6f, 0x01, 0x49}, sizeof(u8[3]));
    memcpy(config->bgBot, (u8[3]) {0x6f, 0x01, 0x49}, sizeof(u8[3]));
    memcpy(config->highlight, (u8[4]) {0xdc, 0xdc, 0xdc, 0xff}, sizeof(u8[4]));
    memcpy(config->borders, (u8[4]) {0xff, 0xff, 0xff, 0xff}, sizeof(u8[4]));
    memcpy(config->fntDef, (u8[4]) {0xff, 0xff, 0xff, 0xff}, sizeof(u8[4]));
    memcpy(config->fntSel, (u8[4]) {0x00, 0x00, 0x00, 0xff}, sizeof(u8[4]));
    
    // animation
    config->bgTop1AnimTime = 0;
    config->bgTop2AnimTime = 0;
    config->bgBotAnimTime = 0;
    config->highlightAnimTime = 0;
    config->bordersAnimTime = 0;
    config->fntDefAnimTime = 0;
    config->fntSelAnimTime = 0;
    config->menuFadeInTime = 0;
    config->menuFadeInTimeStart = 0;
    
    // movie
    configMovieInit(&config->movieTop);
    configMovieInit(&config->movieTop3D);
    configMovieInit(&config->movieBot);
}

int configInit() {
    buffer_ctx ctx;

    // init config
#ifdef ARM9
    config = (boot_config_s *)PTR_CFG;
#else
    config = malloc(sizeof(boot_config_s));
#endif
    memset(config, 0, sizeof(boot_config_s));

    config->timeout = 3;
    config->autobootfix = 8;
    config->index = 0;
    config->recovery = 2;
    config->count = 0;
    configThemeInit();

    // read config file to buffer
    size_t size = fileSize(CONFIG_PATH);
    if ( size != -1 && size > 0 )
    {
        char buffer[size];
        memset(buffer, 0, size);
        if (fileRead(CONFIG_PATH, buffer, size) != 0)
        {
            debug("Could not read config file, creating one...");
            configSave();
        }
        else
        {
            ctx.ptr = buffer;
            ctx.bytes_left = strlen(ctx.ptr);

            if (ini_parse_stream((ini_reader) ini_buffer_reader, &ctx, handler, config) < 0) {
                debug("Could not parse config file, creating one...");
                configSave(); // write new config file
            }
        }
    }
    
    // Fix for fade in effect first frame
    fontDefault.color[3] = 0x00;

    loadBg(GFX_TOP, GFX_LEFT);
#ifndef ARM9
    loadBg(GFX_TOP, GFX_RIGHT);
    if (!config->imgError3D && !gfxIs3D())
        gfxSet3D(true);
#endif
    loadBg(GFX_BOTTOM, GFX_LEFT);

    return 0;
}

int configAddEntry(char *title, char *path, long offset) {
    
    if (config->count >= CONFIG_MAX_ENTRIES)
        return 1;

    strncpy(config->entries[config->count].title, title, 64);
    strncpy(config->entries[config->count].path, path, 128);
    config->entries[config->count].offset = offset;
    config->entries[config->count].patchesCount = 0;
    config->entries[config->count].key = -1;
    config->count++;
    configSave();

    return 0;
}

int configRemoveEntry(int index) {

    if ( index >= config->count )
        return 1;

    int i = 0;
    for(i=0; i<config->count; i++) {
        if(i > index) {
            config->entries[i-1] = config->entries[i];
        }
    }

    if (config->index >= index && config->index > 0) {
        config->index--;
    }
    config->count--;
    configSave();

    return 0;
}

void configSave() {

    int size = 0, i = 0;
    int buffSize = 256*(256*sizeof(char));
#ifdef ARM9
    char *cfg = (char*)PTR_CFG_TMP;
#else
    char *cfg = malloc(buffSize);
#endif
    memset(cfg, 0, buffSize); // 256 lines * 256 char

    // general section
    size += snprintf(cfg, 256, "[general]\n");
#ifndef ARM9
    size += snprintf(cfg+size, 256, "autobootfix=%i;\n", config->autobootfix);
#endif
    size += snprintf(cfg+size, 256, "timeout=%i;\n", config->timeout);
    size += snprintf(cfg+size, 256, "recovery=%i;\n", config->recovery);
    size += snprintf(cfg+size, 256, "default=%i;\n\n", config->index);

    // theme section
    size += snprintf(cfg+size, 256, "[theme]\n");
    size += snprintf(cfg+size, 256, "bgTop1=%02X%02X%02X;\n", config->bgTop1[0], config->bgTop1[1], config->bgTop1[2]);
    size += snprintf(cfg+size, 256, "bgTop2=%02X%02X%02X;\n", config->bgTop2[0], config->bgTop2[1], config->bgTop2[2]);
    size += snprintf(cfg+size, 256, "bgBottom=%02X%02X%02X;\n", config->bgBot[0], config->bgBot[1], config->bgBot[2]);
    if ( config->highlight[3] < 0xFF )
        size += snprintf(cfg+size, 256, "highlight=%02X%02X%02X%02X;\n", config->highlight[0], config->highlight[1], config->highlight[2], config->highlight[3]);
    else
        size += snprintf(cfg+size, 256, "highlight=%02X%02X%02X;\n", config->highlight[0], config->highlight[1], config->highlight[2]);
    if ( config->borders[3] < 0xFF )
        size += snprintf(cfg+size, 256, "borders=%02X%02X%02X%02X;\n", config->borders[0], config->borders[1], config->borders[2], config->borders[3]);
    else
        size += snprintf(cfg+size, 256, "borders=%02X%02X%02X;\n", config->borders[0], config->borders[1], config->borders[2]);
    if ( config->fntDef[3] < 0xFF )
        size += snprintf(cfg+size, 256, "font1=%02X%02X%02X%02X;\n", config->fntDef[0], config->fntDef[1], config->fntDef[2], config->fntDef[3]);
    else
        size += snprintf(cfg+size, 256, "font1=%02X%02X%02X;\n", config->fntDef[0], config->fntDef[1], config->fntDef[2]);
    
    if ( config->fntSel[3] < 0xFF )
        size += snprintf(cfg+size, 256, "font2=%02X%02X%02X%02X;\n", config->fntSel[0], config->fntSel[1], config->fntSel[2], config->fntSel[3]);
    else
        size += snprintf(cfg+size, 256, "font2=%02X%02X%02X;\n", config->fntSel[0], config->fntSel[1], config->fntSel[2]);
    size += snprintf(cfg+size, 256, "bgImgTop=%s;\n", config->bgImgTop);
#ifndef ARM9
    if (config->bgImgTop3D[0] != '\0')
        size += snprintf(cfg+size, 256, "bgImgTop3D=%s;\n", config->bgImgTop3D);
#endif
    size += snprintf(cfg+size, 256, "bgImgBot=%s;\n\n", config->bgImgBot);

    // animation section
    size += snprintf(cfg+size, 256, "[animation]\n");
    if ( config->bgTop1AnimTime > 0 )
    {
        size += snprintf(cfg+size, 256, "bgTop1=%i:%i:%02X%02X%02X;\n", config->bgTop1AnimTime, config->bgTop1AnimTimeStart,
                config->bgTop1AnimColor[0], config->bgTop1AnimColor[1], config->bgTop1AnimColor[2]);
    }
    if ( config->bgTop2AnimTime > 0 )
    {
        size += snprintf(cfg+size, 256, "bgTop2=%i:%i:%02X%02X%02X;\n", config->bgTop2AnimTime, config->bgTop2AnimTimeStart,
                config->bgTop2AnimColor[0], config->bgTop2AnimColor[1], config->bgTop2AnimColor[2]);
    }
    if ( config->bgBotAnimTime > 0 )
    {
        size += snprintf(cfg+size, 256, "bgBottom=%i:%i:%02X%02X%02X;\n", config->bgBotAnimTime, config->bgBotAnimTimeStart,
                config->bgBotAnimColor[0], config->bgBotAnimColor[1], config->bgBotAnimColor[2]);
    }
    if ( config->highlightAnimTime > 0 )
    {
        if ( config->highlightAnimColor[3] < 0xFF )
        {
            size += snprintf(cfg+size, 256, "highlight=%i:%i:%02X%02X%02X%02X;\n", config->highlightAnimTime, config->highlightAnimTimeStart,
                    config->highlightAnimColor[0], config->highlightAnimColor[1], config->highlightAnimColor[2], config->highlightAnimColor[3]);
        }
        else
        {
            size += snprintf(cfg+size, 256, "highlight=%i:%i:%02X%02X%02X;\n", config->highlightAnimTime, config->highlightAnimTimeStart,
                    config->highlightAnimColor[0], config->highlightAnimColor[1], config->highlightAnimColor[2]);
        }
    }
    if ( config->bordersAnimTime > 0 )
    {
        if ( config->highlightAnimColor[3] < 0xFF )
        {
            size += snprintf(cfg+size, 256, "borders=%i:%i:%02X%02X%02X%02X;\n", config->bordersAnimTime, config->bordersAnimTimeStart,
                    config->bordersAnimColor[0], config->bordersAnimColor[1], config->bordersAnimColor[2], config->bordersAnimColor[3]);
        }
        else
        {
            size += snprintf(cfg+size, 256, "borders=%i:%i:%02X%02X%02X;\n", config->bordersAnimTime, config->bordersAnimTimeStart,
                    config->bordersAnimColor[0], config->bordersAnimColor[1], config->bordersAnimColor[2]);
        }
    }
    if ( config->fntDefAnimTime > 0 )
    {
        if ( config->fntDefAnimColor[3] < 0xFF )
        {
            size += snprintf(cfg+size, 256, "font1=%i:%i:%02X%02X%02X%02X;\n", config->fntDefAnimTime, config->fntDefAnimTimeStart,
                    config->fntDefAnimColor[0], config->fntDefAnimColor[1], config->fntDefAnimColor[2], config->fntDefAnimColor[3]);
        }
        else
        {
            size += snprintf(cfg+size, 256, "font1=%i:%i:%02X%02X%02X;\n", config->fntDefAnimTime, config->fntDefAnimTimeStart,
                    config->fntDefAnimColor[0], config->fntDefAnimColor[1], config->fntDefAnimColor[2]);
        }
    }
    if ( config->fntSelAnimTime > 0 )
    {
        if ( config->fntDefAnimColor[3] < 0xFF )
        {
            size += snprintf(cfg+size, 256, "font2=%i:%i:%02X%02X%02X%02X;\n", config->fntSelAnimTime, config->fntSelAnimTimeStart,
                    config->fntSelAnimColor[0], config->fntSelAnimColor[1], config->fntSelAnimColor[2], config->fntSelAnimColor[3]);
        }
        else
        {
            size += snprintf(cfg+size, 256, "font2=%i:%i:%02X%02X%02X;\n", config->fntDefAnimTime, config->fntSelAnimTimeStart,
                    config->fntSelAnimColor[0], config->fntSelAnimColor[1], config->fntSelAnimColor[2]);
        }
    }
    if ( config->menuFadeInTime > 0 )
        size += snprintf(cfg+size, 256, "menuFadeIn=%i:%i;\n", config->menuFadeInTime, config->menuFadeInTimeStart);
    
    // movie section
    if ( config->movieTop.path[0] != '\0' )
    {
        size += snprintf(cfg+size, 256, "\n[topMovie]\n");
        writeMovieProperty(&config->movieTop, cfg, &size);
    }
#ifndef ARM9
    if ( config->movieTop3D.path[0] != '\0' )
    {
        size += snprintf(cfg+size, 256, "\n[top3DMovie]\n");
        writeMovieProperty(&config->movieTop3D, cfg, &size);
    }
#endif
    if ( config->movieBot.path[0] != '\0' )
    {
        size += snprintf(cfg+size, 256, "\n[bottomMovie]\n");
        writeMovieProperty(&config->movieBot, cfg, &size);
    }

    // entries section
    for(i=0; i<config->count; i++) {
        size += snprintf(cfg+size, 256, "\n[entry]\n");
        size += snprintf(cfg+size, 256, "title=%s;\n", config->entries[i].title);
        size += snprintf(cfg+size, 256, "path=%s;\n", config->entries[i].path);
        size += snprintf(cfg+size, 256, "offset=%x;\n", (int)config->entries[i].offset);
        int patchesCount = config->entries[i].patchesCount;
    #ifdef ARM9
        for (int j = 0; j < patchesCount; j++)
        {
            binary_patch* curPatch = &config->entries[i].patches[j];
            size += snprintf(cfg+size, 256, "patchMemSearch=");
            writeHexDataAsStr(curPatch->memToSearch, curPatch->memToSearchSize, cfg+size);
            size += curPatch->memToSearchSize*2;
            size += snprintf(cfg+size, 256, ";\npatchMemOverwrite=");
            writeHexDataAsStr(curPatch->memOverwrite, curPatch->memOverwriteSize, cfg+size);
            size += curPatch->memOverwriteSize*2;
            size += snprintf(cfg+size, 256, ";\npatchOccurence=%i;\n", curPatch->occurence);
        }
    #endif
        size += snprintf(cfg+size, 256, "key=%i;\n", config->entries[i].key);
    }
#ifdef ARM9
    FIL file;
    unsigned int br = 0;
    f_unlink(CONFIG_PATH);
    if(f_open(&file, CONFIG_PATH, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) {
        debug("Could not open cfg: %s", CONFIG_PATH);
        return;
    }
    f_write(&file, cfg, size, &br);
    f_close(&file);
#else
    unlink(CONFIG_PATH);
    FILE *file = fopen(CONFIG_PATH, "w");
    fwrite(cfg, 1 , size, file);
    fclose(file);
    free(cfg);
#endif
}

void configExit() {
#ifndef ARM9
    if (config) {
        if (config->bgImgTopBuff) {
            free(config->bgImgTopBuff);
        }
        if (config->bgImgTop3DBuff) {
            free(config->bgImgTop3DBuff);
        }
        if (config->bgImgBotBuff) {
            free(config->bgImgBotBuff);
        }
        free(config);
    }
#endif
}

void loadBg(gfxScreen_t screen, gfx3dSide_t side) {

#ifdef ARM9
    const char *path = (screen == GFX_TOP ? config->bgImgTop : config->bgImgBot);
#else
    const char *path = (screen == GFX_TOP ? (side == GFX_RIGHT ? config->bgImgTop3D : config->bgImgTop) : config->bgImgBot);
#endif
    size_t size = fileSize(path);
    if ( size == -1 || size == 0 ) {
        return;
    }

#ifdef ARM9
    u8 *bg = screen == GFX_TOP ? PTR_TOP_BG : PTR_BOT_BG;
#else
    u8 *bg = malloc(size);
#endif
    if (fileRead(path, bg, size) != 0) {
        return;
    }

    if (screen == GFX_TOP) {
    #ifndef ARM9
        if (side == GFX_RIGHT) {
            config->bgImgTop3DSize = size;
            config->bgImgTop3DBuff = bg;
            config->imgError3D = false;
        } else
        {
    #endif
            config->bgImgTopSize = size;
            config->bgImgTopBuff = bg;
            config->imgError = false;
    #ifndef ARM9
        }
    #endif
    } else {
        config->bgImgBotSize = size;
        config->bgImgBotBuff = bg;
        config->imgErrorBot = false;
    }
}
