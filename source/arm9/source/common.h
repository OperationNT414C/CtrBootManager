#pragma once

#include <inttypes.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t
#define s16 int16_t

#define vu8 volatile u8
#define vu16 volatile u16
#define vu32 volatile u32
#define vu64 volatile u64

#define max(a,b) \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
       _a > _b ? _a : _b; })
#define min(a,b) \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
       _a < _b ? _a : _b; })
#define getbe16(d) \
    (((d)[0]<<8) | (d)[1])
#define getbe32(d) \
    ((((u32) getbe16(d))<<16) | ((u32) getbe16(d+2)))
#define getbe64(d) \
    ((((u64) getbe32(d))<<32) | ((u64) getbe32(d+4)))
#define getle16(d) (*((u16*) (d)))
#define getle32(d) (*((u32*) (d)))
#define getle64(d) (*((u64*) (d)))
#define align(v,a) \
    (((v) % (a)) ? ((v) + (a) - ((v) % (a))) : (v))

#define CFG_SYSPROT9        (*(vu8  *)0x10000000)
#define CFG_BOOTENV         (*(vu32 *)0x10010000)
#define CFG_UNITINFO        (*(vu8  *)0x10010010)
#define CFG_TWLUNITINFO     (*(vu8  *)0x10010014)
#define OTP_DEVCONSOLEID    (*(vu64 *)0x10012000)
#define OTP_TWLCONSOLEID    (*(vu64 *)0x10012100)
#define PDN_MPCORE_CFG      (*(vu32 *)0x10140FFC)
#define PDN_SPI_CNT         (*(vu32 *)0x101401C0)

#define ISN3DS       (PDN_MPCORE_CFG == 7)
#define ISDEVUNIT    (CFG_UNITINFO != 0)

u32 hexAtoi(const char *in, u32 digits);
u32 decAtoi(const char *in, u32 digits);
