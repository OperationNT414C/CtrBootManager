#pragma once

#include "common.h"

#define REG_SHA_CNT         ((vu32 *)0x1000A000)
#define REG_SHA_BLKCNT      ((vu32 *)0x1000A004)
#define REG_SHA_HASH        ((vu32 *)0x1000A040)
#define REG_SHA_INFIFO      ((vu32 *)0x1000A080)

#define SHA_CNT_STATE           0x00000003
#define SHA_CNT_UNK2            0x00000004
#define SHA_CNT_OUTPUT_ENDIAN   0x00000008
#define SHA_CNT_MODE            0x00000030
#define SHA_CNT_ENABLE          0x00010000
#define SHA_CNT_ACTIVE          0x00020000

#define SHA_HASH_READY      0x00000000
#define SHA_NORMAL_ROUND    0x00000001
#define SHA_FINAL_ROUND     0x00000002

#define SHA_OUTPUT_BE       SHA_CNT_OUTPUT_ENDIAN
#define SHA_OUTPUT_LE       0

#define SHA_256_MODE        0
#define SHA_224_MODE        0x00000010
#define SHA_1_MODE          0x00000020

#define SHA_256_HASH_SIZE   (256 / 8)
#define SHA_224_HASH_SIZE   (224 / 8)
#define SHA_1_HASH_SIZE     (160 / 8)

void sha(void *res, const void *src, u32 size, u32 mode);
