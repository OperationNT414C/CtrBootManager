#include "firm.h"

#include "sha.h"

static __attribute__((noinline)) bool overlaps(u32 as, u32 ae, u32 bs, u32 be)
{
    if (as <= bs && bs <= ae)
        return true;
    else if (bs <= as && as <= be)
        return true;
    return false;
}

bool checkFirmPayload(Firm* firm)
{
    if(memcmp(firm->magic, "FIRM", 4) != 0)
        return false;

    if(firm->arm9Entry == NULL)  //allow for the arm11 entrypoint to be zero in which case nothing is done on the arm11 side
        return false;

    u32 size = 0x200;
    for(u32 i = 0; i < 4; i++)
        size += firm->section[i].size;

    bool arm9EpFound = false, arm11EpFound = false;
    for(u32 i = 0; i < 4; i++)
    {
        __attribute__((aligned(4))) u8 hash[0x20];

        FirmSection *section = &firm->section[i];

        // allow empty sections
        if (section->size == 0)
            continue;

        if(section->offset < 0x200)
            return false;

        if(section->address + section->size < section->address) //overflow check
            return false;

        if(((u32)section->address & 3) || (section->offset & 0x1FF) || (section->size & 0x1FF)) //alignment check
            return false;

        if(overlaps((u32)section->address, (u32)section->address + section->size, 0x27FFE000, 0x28000000))
            return false;
        else if(overlaps((u32)section->address, (u32)section->address + section->size, 0x27FFE000 - 0x1000, 0x27FFE000))
            return false;
        else if(overlaps((u32)section->address, (u32)section->address + section->size, (u32)firm, (u32)firm + size))
            return false;

        sha(hash, (u8 *)firm + section->offset, section->size, SHA_256_MODE);
        if(memcmp(hash, section->hash, 0x20) != 0)
            return false;

        if(firm->arm9Entry >= section->address && firm->arm9Entry < (section->address + section->size))
            arm9EpFound = true;

        if(firm->arm11Entry >= section->address && firm->arm11Entry < (section->address + section->size))
            arm11EpFound = true;
    }

    return arm9EpFound && (firm->arm11Entry == NULL || arm11EpFound);
}
