#ifndef _loader_h_
#define _loader_h_

typedef struct {
    char memToSearch[256];
    char memOverwrite[256];
    int memToSearchSize;
    int memOverwriteSize;
    int occurence;
} binary_patch;

int load(char *path, long offset, binary_patch* patches, int patchesCount, char* splashTop, char* splashBot);

void screensBehavior(char* splashTop, char* splashBot);

#ifndef ARM9
int load_3dsx(char *path, char* splashTop, char* splashBot);
#endif

#endif // _loader_h_
