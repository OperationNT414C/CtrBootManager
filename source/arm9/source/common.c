#include "common.h"

u32 hexAtoi(const char *in, u32 digits)
{
    u32 res = 0;
    char *tmp = (char *)in;

    for(u32 i = 0; i < digits && *tmp != 0; tmp++, i++)
        res = (*tmp > '9' ? *tmp - 'A' + 10 : *tmp - '0') + (res << 4);

    return res;
}

u32 decAtoi(const char *in, u32 digits)
{
    u32 res = 0;
    char *tmp = (char *)in;

    for(u32 i = 0; i < digits && *tmp != 0; tmp++, i++)
        res = *tmp - '0' + res * 10;

    return res;
}
