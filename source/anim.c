#include "anim.h"

#define TOP_SCREEN_SIZE (400*240*3)
#define BOT_SCREEN_SIZE (320*240*3)

#ifdef ARM9
#include "memory.h"

// Top compression required memory: 576000+28809 = 604809
#define TOP_COMP_PTR (char*)PTR_MOVIE_COMP
#define TOP_COMP_PREV_FRAME_PTR (TOP_COMP_PTR+sizeof(comp_manager_s))
#define TOP_COMP_COMP_FRAME_PTR (TOP_COMP_PREV_FRAME_PTR+TOP_SCREEN_SIZE)

// Bottom compression required memory: 460800+23049 = 483849
#define BOT_COMP_PTR (TOP_COMP_COMP_FRAME_PTR+TOP_SCREEN_SIZE+(TOP_SCREEN_SIZE/10)+9)
#define BOT_COMP_PREV_FRAME_PTR (BOT_COMP_PTR+sizeof(comp_manager_s))
#define BOT_COMP_COMP_FRAME_PTR (BOT_COMP_PREV_FRAME_PTR+BOT_SCREEN_SIZE)

// Total compression required memory: 1088658 = 0x109C92
#else
#include <stdlib.h>
#endif

#include "utility.h"

void movieInit(movie_state_s* movie)
{
    movie->file = NULL;
    movie->fileOffset = 0;
    movie->comp = NULL;
    movie->loopStream = NULL;
    movie->curLoop = 0;
    movie->loopsToDo = 0;
    movie->loopStartFrameStream = NULL;
    movie->loopEndFrameStream = NULL;
}

void animInit()
{
#ifdef ARM9
    anim = (anim_state_s*)PTR_ANIM;
#else
    anim = malloc(sizeof(anim_state_s));
#endif
    
    anim->timer = 0;
    anim->active = 0;
    
    anim->topMovie.frameSize = TOP_SCREEN_SIZE;
    movieInit(&anim->topMovie);
#ifndef ARM9
    anim->top3DMovie.frameSize = TOP_SCREEN_SIZE;
    movieInit(&anim->top3DMovie);
#endif
    anim->botMovie.frameSize = BOT_SCREEN_SIZE;
    movieInit(&anim->botMovie);
}

void movieLoopSetup(movie_state_s* movie)
{
    movie->loopFramesCount = movie->loopEndFrame - movie->loopStartFrame
                           + movie->loopTimeOnStartFrame + movie->loopTimeOnEndFrame;

    movie->loopStartFrameStream = movie->loopStream;
    movie->loopEndFrameStream = movie->loopStream + movie->frameSize
                                *( ( MEMORY_STREAM == movie->loopStreamType ) ? (movie->loopEndFrame-movie->loopStartFrame) : 1 );
}


#ifdef ARM9
void movieSetup(movie_config_s* mvConf, movie_state_s* movie, void* filePtr, comp_manager_s* compPtr, void* streamPtr)
#else
void movieSetup(movie_config_s* mvConf, movie_state_s* movie)
#endif
{
    if ( mvConf->path[0] == '\0' )
        return;

#ifdef ARM9
    movie->file = (void*)(filePtr);
#else
    int fileHandleSize = getFileHandleSize();
    movie->file = malloc(fileHandleSize);
#endif
    if ( fileHandleOpen(movie->file, mvConf->path) < 0 )
    {
    #ifndef ARM9
        free(movie->file);
    #endif
        movie->file = NULL;
        return;
    }

    if ( mvConf->compressed )
    {
    #ifdef ARM9
        movie->comp = compPtr;
    #else
        movie->comp = (comp_manager_s*)malloc(sizeof(comp_manager_s));
        movie->comp->frame_prev_read = (const char*)malloc(movie->frameSize);
        movie->comp->frame_comp = (char*)malloc(movie->frameSize + (movie->frameSize/10) + 9);
    #endif
        movie->comp->frame_prev_write = (char*)movie->comp->frame_prev_read;

        memset(&movie->comp->state, 0, sizeof(qlz_state_decompress));
        memset(movie->comp->frame_prev_write, 0, movie->frameSize);
        memset(movie->comp->frame_comp, 0, movie->frameSize + (movie->frameSize/10) + 9);
        movie->comp->reverse = 0;
    }
    
    movie->framesCount = getMovieFramesCount(movie->file, movie->frameSize, (NULL != movie->comp));
    if ( movie->framesCount <= 0 )
    {
        fileHandleClose(movie->file);
    #ifndef ARM9
        free(movie->file);
    #endif
        movie->file = NULL;
        return;
    }

    movie->curFrame = 0;
    movie->curLoop = 0;
    movie->loopsToDo = mvConf->loopCount;
    if ( 0 == movie->loopsToDo )
        return;

    movie->loopStreamType = mvConf->loopStreamType;
    if ( movie->loopStreamType == MEMORY_COMPRESSED_STREAM )
    {
        if ( !mvConf->compressed )
            movie->loopStreamType = MEMORY_STREAM;
    }
    else if ( movie->loopStreamType != FILE_STREAM )
        movie->loopStreamType = MEMORY_STREAM;

    movie->loopReverse = mvConf->loopReverse;
    
    movie->loopTimeOnStartFrame = ( mvConf->loopTimeOnStartFrame > 0 ) ? mvConf->loopTimeOnStartFrame : 0;
    movie->loopTimeOnEndFrame = ( mvConf->loopTimeOnEndFrame > 0 ) ? mvConf->loopTimeOnEndFrame : 0;

    movie->loopStartFrame = (mvConf->loopStartFrame < movie->framesCount) ? mvConf->loopStartFrame : (movie->framesCount-1);
    if ( movie->loopStartFrame < 0 )
        movie->loopStartFrame = 0;
    
    movie->loopEndFrame = (mvConf->loopEndFrame < 0 || mvConf->loopEndFrame > movie->loopStartFrame)
                        ? mvConf->loopEndFrame : movie->loopStartFrame;
    if ( movie->loopEndFrame < 0 || movie->loopEndFrame >= movie->framesCount )
        movie->loopEndFrame = movie->framesCount-1;

    switch ( movie->loopStreamType )
    {
    case FILE_STREAM:
        // We will keep file opened to stream next loops content (slower but avoid memory issues).
        // But we still keep at least uncompressed loop first and last frame in memory.
        movie->loopStreamSize = movie->frameSize*2 + ((movie->loopEndFrame-movie->loopStartFrame+2)*sizeof(int));
		break;
        
    case MEMORY_STREAM:
        // Full uncompressed frames are kept in memory, file is closed.
        movie->loopStreamSize = movie->frameSize * (movie->loopEndFrame-movie->loopStartFrame+1);
        break;
        
    case MEMORY_COMPRESSED_STREAM:
        // For compressed movie, we will keep compressed data in memory and close the file.
        getMovieDataOffsetForFrames(movie->file, movie->frameSize, (NULL != movie->comp),
                                    movie->loopStartFrame, movie->loopEndFrame,
                                    NULL, &movie->loopStreamSize);
        movie->loopStreamSize += movie->frameSize*2 + ((movie->loopEndFrame-movie->loopStartFrame+2)*sizeof(int));
		break;
    }
    
#ifdef ARM9
    movie->loopStream = streamPtr;
#else
    movie->loopStream = (char*)malloc(movie->loopStreamSize);
#endif

    if ( MEMORY_STREAM != movie->loopStreamType )
        *(int*)(movie->loopStream+movie->frameSize*2) = (movie->loopEndFrame-movie->loopStartFrame+1);
    movieLoopSetup(movie);
}

void animSetup()
{
    memcpy(anim->bgTop1, config->bgTop1, sizeof(u8[3]));
    memcpy(anim->bgTop2, config->bgTop2, sizeof(u8[3]));
    memcpy(anim->bgBot, config->bgBot, sizeof(u8[3]));
    memcpy(anim->highlight, config->highlight, sizeof(u8[4]));
    memcpy(anim->borders, config->borders, sizeof(u8[4]));
    memcpy(anim->fntDef, config->fntDef, sizeof(u8[4]));
    memcpy(anim->fntSel, config->fntSel, sizeof(u8[4]));
    
#ifdef ARM9
    comp_manager_s* topComp = (comp_manager_s*)PTR_MOVIE_COMP;
    topComp->frame_prev_read = TOP_COMP_PREV_FRAME_PTR;
    topComp->frame_comp = TOP_COMP_COMP_FRAME_PTR;

    comp_manager_s* botComp = (comp_manager_s*)BOT_COMP_PTR;
    botComp->frame_prev_read = BOT_COMP_PREV_FRAME_PTR;
    botComp->frame_comp = BOT_COMP_COMP_FRAME_PTR;

    int fileHandleSize = getFileHandleSize();
    movieSetup(&config->movieTop, &anim->topMovie, PTR_ANIM+sizeof(anim_state_s), topComp, PTR_MOVIE_LOOP);
    movieSetup(&config->movieBot, &anim->botMovie, PTR_ANIM+sizeof(anim_state_s)+fileHandleSize,
               botComp, PTR_MOVIE_LOOP+anim->topMovie.loopStreamSize);
#else
    movieSetup(&config->movieTop, &anim->topMovie);
    movieSetup(&config->movieTop3D, &anim->top3DMovie);
    if ( NULL != anim->top3DMovie.file && !gfxIs3D() )
        gfxSet3D(true);
    movieSetup(&config->movieBot, &anim->botMovie);
#endif

    if ( config->bgTop1AnimTime > 0 || config->bgTop2AnimTime > 0 || config->bgBotAnimTime > 0
      || config->highlightAnimTime > 0 || config->bordersAnimTime > 0
      || config->fntDefAnimTime > 0 || config->fntSelAnimTime > 0
      || config->menuFadeInTime > 0 || config->menuFadeInTimeStart != 0 )
    {
        anim->active = 1;
        incrementAnimTime();
    }
}

float computeMerge(int elapsedTime, int animTime, int animStart)
{
    int curTime = (elapsedTime+animStart) % (2*animTime);
    int animStep = (curTime/animTime == 0) ? (curTime%animTime) : (animTime-curTime%animTime-1);
    return (float)animStep / (float)animTime;
}

void setMergedColor(float mergeVal, u8 color1[3], u8 color2[3], u8 finalColor[3])
{
    float oneMinusMerge = 1.0f - mergeVal;
    finalColor[0] = (u8)(oneMinusMerge*color1[0]+mergeVal*color2[0]);
    finalColor[1] = (u8)(oneMinusMerge*color1[1]+mergeVal*color2[1]);
    finalColor[2] = (u8)(oneMinusMerge*color1[2]+mergeVal*color2[2]);
}

void setMergedColorAlpha(float mergeVal, u8 color1[4], u8 color2[4], u8 finalColor[4])
{
    float oneMinusMerge = 1.0f - mergeVal;
    finalColor[0] = (u8)(oneMinusMerge*color1[0]+mergeVal*color2[0]);
    finalColor[1] = (u8)(oneMinusMerge*color1[1]+mergeVal*color2[1]);
    finalColor[2] = (u8)(oneMinusMerge*color1[2]+mergeVal*color2[2]);
    finalColor[3] = (u8)(oneMinusMerge*color1[3]+mergeVal*color2[3]);
}

void incrementAnimTime()
{
    if (!anim->active)
        return;
    
    if ( config->bgTop1AnimTime > 0 )
    {
        float merge = computeMerge(anim->timer, config->bgTop1AnimTime, config->bgTop1AnimTimeStart);
        setMergedColor(merge, config->bgTop1, config->bgTop1AnimColor, anim->bgTop1);
    }
    
    if ( config->bgTop2AnimTime > 0 )
    {
        float merge = computeMerge(anim->timer, config->bgTop2AnimTime, config->bgTop2AnimTimeStart);
        setMergedColor(merge, config->bgTop2, config->bgTop2AnimColor, anim->bgTop2);
    }

    if ( config->bgBotAnimTime > 0 )
    {
        float merge = computeMerge(anim->timer, config->bgBotAnimTime, config->bgBotAnimTimeStart);
        setMergedColor(merge, config->bgBot, config->bgBotAnimColor, anim->bgBot);
    }

    if ( config->highlightAnimTime > 0 )
    {
        float merge = computeMerge(anim->timer, config->highlightAnimTime, config->highlightAnimTimeStart);
        setMergedColorAlpha(merge, config->highlight, config->highlightAnimColor, anim->highlight);
    }
    else if ( anim->timer > config->menuFadeInTimeStart )
        memcpy(anim->highlight, config->highlight, sizeof(u8[4]));

    if ( config->bordersAnimTime > 0 )
    {
        float merge = computeMerge(anim->timer, config->bordersAnimTime, config->bordersAnimTimeStart);
        setMergedColorAlpha(merge, config->borders, config->bordersAnimColor, anim->borders);
    }
    else if ( anim->timer > config->menuFadeInTimeStart )
        memcpy(anim->borders, config->borders, sizeof(u8[4]));
    
    if ( config->fntDefAnimTime > 0 )
    {
        float merge = computeMerge(anim->timer, config->fntDefAnimTime, config->fntDefAnimTimeStart);
        setMergedColorAlpha(merge, config->fntDef, config->fntDefAnimColor, anim->fntDef);
    }
    else if ( anim->timer > config->menuFadeInTimeStart )
        memcpy(anim->fntDef, config->fntDef, sizeof(u8[4]));

    if ( config->fntSelAnimTime > 0 )
    {
        float merge = computeMerge(anim->timer, config->fntSelAnimTime, config->fntSelAnimTimeStart);
        setMergedColorAlpha(merge, config->fntSel, config->fntSelAnimColor, anim->fntSel);
    }
    else if ( anim->timer > config->menuFadeInTimeStart )
        memcpy(anim->fntSel, config->fntSel, sizeof(u8[4]));
    
    if ( anim->timer >= config->menuFadeInTimeStart )
    {
        int fadeInTime = anim->timer - config->menuFadeInTimeStart;
        if ( fadeInTime <= config->menuFadeInTime )
        {
            float fadeIn = (config->menuFadeInTime > 0) ? (float)fadeInTime / (float)config->menuFadeInTime : 1.f;
            
            anim->highlight[3] = (u8)(fadeIn*(float)anim->highlight[3]);
            anim->borders[3] = (u8)(fadeIn*(float)anim->borders[3]);
            anim->fntDef[3] = (u8)(fadeIn*(float)anim->fntDef[3]);
            anim->fntSel[3] = (u8)(fadeIn*(float)anim->fntSel[3]);
            
            if ( fadeInTime == config->menuFadeInTime && config->bgTop1AnimTime == 0
              && config->bgTop2AnimTime == 0 && config->bgBotAnimTime == 0
              && config->highlightAnimTime == 0 && config->bordersAnimTime == 0
              && config->fntDefAnimTime == 0 && config->fntSelAnimTime == 0 )
                anim->active = 0;
        }
    }
    else
    {
        anim->highlight[3] = 0x00;
        anim->borders[3] = 0x00;
        anim->fntDef[3] = 0x00;
        anim->fntSel[3] = 0x00;
    }

    anim->timer++;
}

int readMovie(u8* fb, movie_state_s* movie)
{
    if ( movie->curLoop == 0 )
    {
        if ( NULL == movie->file )
            return -1;

        int prevOffset = movie->fileOffset;

        comp_manager_s* compPtr = movie->comp;
        comp_manager_s compTmp;
        if ( NULL != compPtr && MEMORY_COMPRESSED_STREAM == movie->loopStreamType && NULL != movie->loopStream
          && movie->curFrame >= movie->loopStartFrame && movie->curFrame <= movie->loopEndFrame )
        {
            int* loopFramesPtr = (int*)(movie->loopStream+movie->frameSize*2);
            int* frameOffsets = loopFramesPtr+1;
            compTmp = *movie->comp;
            compTmp.frame_comp = (char*)loopFramesPtr + (*loopFramesPtr+1)*sizeof(int);
            if ( movie->curFrame > movie->loopStartFrame )
                compTmp.frame_comp += prevOffset-frameOffsets[0];
            compPtr = &compTmp;
        }
        
        int readMovieRes = readMovieFrame(movie->file, &movie->fileOffset, compPtr, (char*)fb, movie->frameSize);
        if ( readMovieRes < 0 || movie->curFrame >= movie->framesCount-1
          || ( movie->loopReverse && movie->curFrame >= movie->loopEndFrame ) )
        {
            if ( FILE_STREAM != movie->loopStreamType )
            {
                fileHandleClose(movie->file);
            #ifndef ARM9
                free(movie->file);
            #endif
                movie->file = NULL;
            }
            movie->curLoop = 1;
        }

        if ( NULL != movie->loopStream && movie->curFrame >= movie->loopStartFrame && movie->curFrame <= movie->loopEndFrame )
        {
            if ( MEMORY_STREAM != movie->loopStreamType )
                *(int*)(movie->loopStream+movie->frameSize*2+(movie->curFrame-movie->loopStartFrame+1)*sizeof(int)) = prevOffset;

            if ( MEMORY_COMPRESSED_STREAM == movie->loopStreamType && ( movie->curFrame == movie->loopEndFrame || readMovieRes < 0 ) )
            {
                int* loopFramesPtr = (int*)(movie->loopStream+movie->frameSize*2);
                int* frameOffsets = loopFramesPtr+1;
                int remapOffset = (int)((char*)frameOffsets + (*loopFramesPtr)*sizeof(int) - frameOffsets[0]);
                for ( int i = 0 ; i < *loopFramesPtr ; i++ )
                    frameOffsets[i] += remapOffset;
            }

            if ( movie->curFrame == movie->loopStartFrame )
                memcpy(movie->loopStartFrameStream, fb, movie->frameSize);
            else if ( movie->curFrame == movie->loopEndFrame )
                memcpy(movie->loopEndFrameStream, fb, movie->frameSize);
            else if ( MEMORY_STREAM == movie->loopStreamType )
                memcpy(movie->loopStream+(movie->curFrame-movie->loopStartFrame)*movie->frameSize, fb, movie->frameSize);
        }

        movie->curFrame++;
        if ( movie->curLoop > 0 )
            movie->curFrame = 0;
    }
    else if ( movie->loopsToDo < 0 || movie->curLoop <= movie->loopsToDo )
    {
        if ( movie->loopReverse && movie->curLoop%2 == 1 )
        {
            if ( movie->curFrame <= movie->loopTimeOnEndFrame )
                memcpy(fb, movie->loopEndFrameStream, movie->frameSize);
            else if ( movie->curFrame >= movie->loopTimeOnEndFrame+movie->loopEndFrame-movie->loopStartFrame )
                memcpy(fb, movie->loopStartFrameStream, movie->frameSize);            
            else
            {
                int frameToGet = (movie->loopEndFrame-movie->loopStartFrame)-(movie->curFrame-movie->loopTimeOnEndFrame);
                if ( MEMORY_STREAM == movie->loopStreamType )
                    memcpy(fb, movie->loopStream + movie->frameSize*frameToGet, movie->frameSize);
                else
                {
                    if ( NULL != movie->comp )
                    {
                        comp_manager_s compTmp = *movie->comp;
                        compTmp.reverse = 1;
                        if ( frameToGet == movie->loopEndFrame-movie->loopStartFrame-1 )
                             compTmp.frame_prev_read = movie->loopEndFrameStream;
                        int offset = *(int*)(movie->loopStream + 2*movie->frameSize + (2+frameToGet)*sizeof(int));
                        if ( MEMORY_COMPRESSED_STREAM == movie->loopStreamType )
                        {
                            compTmp.frame_comp = (char*)offset;
                            readCompressedMovieFrameFromBuffer(&compTmp, (char*)fb, movie->frameSize );
                        }
                        else
                            readMovieFrame(movie->file, &offset, &compTmp, (char*)fb, movie->frameSize);
                    }
                    else
                    {
                        int fileOffset = (frameToGet+movie->loopStartFrame)*movie->frameSize;
                        readMovieFrame(movie->file, &fileOffset, NULL, (char*)fb, movie->frameSize);
                    }
                }
            }
        }
        else
        {
            if ( movie->curFrame <= movie->loopTimeOnStartFrame )
                memcpy(fb, movie->loopStartFrameStream, movie->frameSize);
            else if ( movie->curFrame >= movie->loopTimeOnStartFrame+movie->loopEndFrame-movie->loopStartFrame )
                memcpy(fb, movie->loopEndFrameStream, movie->frameSize);            
            else
            {
                int frameToGet = movie->curFrame-movie->loopTimeOnStartFrame;
                if ( MEMORY_STREAM == movie->loopStreamType )
                    memcpy(fb, movie->loopStream+movie->frameSize*frameToGet, movie->frameSize);
                else
                {
                    if ( NULL != movie->comp )
                    {
                        comp_manager_s compTmp = *movie->comp;
                        if ( frameToGet == 1 )
                            compTmp.frame_prev_read = movie->loopStartFrameStream;
                        int offset = *(int*)(movie->loopStream + 2*movie->frameSize + (1+frameToGet)*sizeof(int));
                        if ( MEMORY_COMPRESSED_STREAM == movie->loopStreamType )
                        {
                            compTmp.frame_comp = (char*)offset;
                            readCompressedMovieFrameFromBuffer(&compTmp, (char*)fb, movie->frameSize );
                        }
                        else
                            readMovieFrame(movie->file, &offset, &compTmp, (char*)fb, movie->frameSize);
                    }
                    else
                    {
                        int fileOffset = (frameToGet+movie->loopStartFrame)*movie->frameSize;
                        readMovieFrame(movie->file, &fileOffset, NULL, (char*)fb, movie->frameSize);
                    }
                }
            }
        }

        movie->curFrame++;
        if ( movie->curFrame > movie->loopFramesCount )
        {
            movie->curFrame = 0;
            movie->curLoop++;
        }
    }
    else
        return -1;

    return 0;
}

void animExit()
{
    if (anim) {
        if (anim->topMovie.file)
            fileHandleClose(anim->topMovie.file);
        if (anim->botMovie.file)
            fileHandleClose(anim->botMovie.file);
#ifndef ARM9
        if (anim->top3DMovie.file)
            fileHandleClose(anim->top3DMovie.file);

        if (anim->topMovie.file)
            free(anim->topMovie.file);
        if (anim->topMovie.comp)
        {
            free(anim->topMovie.comp->frame_prev_write); // Same as frame_prev_read
            free(anim->topMovie.comp->frame_comp);
            free(anim->topMovie.comp);
        }
        if (anim->topMovie.loopStream)
            free(anim->topMovie.loopStream);

        if (anim->top3DMovie.file)
            free(anim->top3DMovie.file);
        if (anim->top3DMovie.comp)
        {
            free(anim->top3DMovie.comp->frame_prev_write); // Same as frame_prev_read
            free(anim->top3DMovie.comp->frame_comp);
            free(anim->top3DMovie.comp);
        }
        if (anim->top3DMovie.loopStream)
            free(anim->top3DMovie.loopStream);
        
        if (anim->botMovie.file)
            free(anim->botMovie.file);
        if (anim->botMovie.comp)
        {
            free(anim->botMovie.comp->frame_prev_write); // Same as frame_prev_read
            free(anim->botMovie.comp->frame_comp);
            free(anim->botMovie.comp);
        }
        if (anim->botMovie.loopStream)
            free(anim->botMovie.loopStream);
        free(anim);
#endif
        anim = NULL;
    }
}
