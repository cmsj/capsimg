#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <dlfcn.h>

#include "Core/CommonTypes.h"
#include "LibIPF/CapsAPI.h"

typedef struct {
    int len;
    void *data;
} capsTrack;

typedef int8_t uae_s8;
typedef uint8_t uae_u8;

typedef int16_t uae_s16;
typedef uint16_t uae_u16;

typedef int32_t uae_s32;
typedef uint32_t uae_u32;

#ifndef uae_s64
typedef long long int uae_s64;
#endif
#ifndef uae_u64
typedef unsigned long long int uae_u64;
#endif

#define CAPSCALL
typedef SDWORD (CAPSCALL * CAPSINIT)(void);
static CAPSINIT pCAPSInit;
typedef SDWORD (CAPSCALL * CAPSADDIMAGE)(void);
static CAPSADDIMAGE pCAPSAddImage;
typedef SDWORD (CAPSCALL * CAPSLOCKIMAGEMEMORY)(SDWORD,PUBYTE,UDWORD,UDWORD);
static CAPSLOCKIMAGEMEMORY pCAPSLockImageMemory;
typedef SDWORD (CAPSCALL * CAPSUNLOCKIMAGE)(SDWORD);
static CAPSUNLOCKIMAGE pCAPSUnlockImage;
typedef SDWORD (CAPSCALL * CAPSLOADIMAGE)(SDWORD,UDWORD);
static CAPSLOADIMAGE pCAPSLoadImage;
typedef SDWORD (CAPSCALL * CAPSGETIMAGEINFO)(PCAPSIMAGEINFO,SDWORD);
static CAPSGETIMAGEINFO pCAPSGetImageInfo;
typedef SDWORD (CAPSCALL * CAPSLOCKTRACK)(PCAPSTRACKINFO,SDWORD,UDWORD,UDWORD,UDWORD);
static CAPSLOCKTRACK pCAPSLockTrack;
typedef SDWORD (CAPSCALL * CAPSUNLOCKTRACK)(SDWORD,UDWORD);
static CAPSUNLOCKTRACK pCAPSUnlockTrack;
typedef SDWORD (CAPSCALL * CAPSUNLOCKALLTRACKS)(SDWORD);
static CAPSUNLOCKALLTRACKS pCAPSUnlockAllTracks;
typedef SDWORD (CAPSCALL * CAPSGETVERSIONINFO)(PCAPSVERSIONINFO,UDWORD);
static CAPSGETVERSIONINFO pCAPSGetVersionInfo;
typedef SDWORD (CAPSCALL * CAPSGETINFO)(PVOID pinfo, SDWORD id, UDWORD cylinder, UDWORD head, UDWORD inftype, UDWORD infid);
static CAPSGETINFO pCAPSGetInfo;
typedef SDWORD (CAPSCALL * CAPSSETREVOLUTION)(SDWORD id, UDWORD value);
static CAPSSETREVOLUTION pCAPSSetRevolution;
typedef SDWORD (CAPSCALL * CAPSGETIMAGETYPEMEMORY)(PUBYTE buffer, UDWORD length);
static CAPSGETIMAGETYPEMEMORY pCAPSGetImageTypeMemory;

void caps_init();
void caps_loadfile();
