
#ifndef _OSD_H_
#define _OSD_H_

#include <gccore.h>
#include <ogcsys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <sys/dir.h>
#include <unistd.h>
#include <asndlib.h>
#include <oggplayer.h>
#include <zlib.h>

#ifdef HW_RVL
#include <di/di.h>
#include "vi_encoder.h"
#endif

#include "gx_input.h"
#include "gx_audio.h"
#include "gx_video.h"
#include "config.h"
#include "fileio.h"

#define DEFAULT_PATH  "/genplus"
#define GG_ROM        "/genplus/ggenie.bin"
#define AR_ROM        "/genplus/areplay.bin"
#define SK_ROM        "/genplus/sk.bin"
#define SK_UPMEM      "/genplus/sk2chip.bin"
#define MD_BIOS       "/genplus/bios.bin"
#define MS_BIOS       "/genplus/bios.sms"
#define GG_BIOS       "/genplus/bios.gg"

#ifdef HW_RVL
#define VERSION "Genesis Plus GX 1.6.1 (WII)"
#else
#define VERSION "Genesis Plus GX 1.6.1 (GCN)"
#endif

#define osd_input_Update() gx_input_UpdateEmu()

/* globals */
extern void legal(void);
extern void reloadrom(void);
extern void shutdown(void);
extern u32 frameticker;
extern u32 Shutdown;
extern u32 ConfigRequested;

#endif /* _OSD_H_ */
