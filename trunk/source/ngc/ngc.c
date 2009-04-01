/****************************************************************************
 *  ngc.c
 *
 *  Genesis Plus GX main
 *
 *  code by Softdev (2006), Eke-Eke (2007,2008)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 ***************************************************************************/

#include "shared.h"
#include "font.h"
#include "history.h"
#include "gcaram.h"

#ifdef HW_DOL
#include "dvd.h"
#else
#include <di/di.h>
#endif

#include <fat.h>


#ifdef HW_RVL
/* Power Button callback */
u8 Shutdown = 0;
void Power_Off(void)
{
  Shutdown = 1;
  ConfigRequested = 1;
}
#endif


/***************************************************************************
 * Genesis Plus Virtual Machine
 *
 ***************************************************************************/
static void load_bios()
{
  char pathname[MAXPATHLEN];

  /* reset BIOS found flag */
  config.bios_enabled &= ~2;

  /* open BIOS file */
  sprintf (pathname, "%s/BIOS.bin",DEFAULT_PATH);
  FILE *fp = fopen(pathname, "rb");
  if (fp == NULL) return;

  /* read file */
  fread(bios_rom, 1, 0x800, fp);
  fclose(fp);

  /* update BIOS flags */
  config.bios_enabled |= 2;

  if (config.bios_enabled == 3)
  {
    /* initialize system */
    system_init ();
    audio_init(48000);
  }
}

static void init_machine (void)
{
  /* Allocate cart_rom here ( 10 MBytes ) */
  cart_rom = memalign(32, 10 * 1024 * 1024);

  /* BIOS support */
  load_bios();

  /* allocate global work bitmap */
  memset (&bitmap, 0, sizeof (bitmap));
  bitmap.width  = 720;
  bitmap.height = 576;
  bitmap.depth  = 16;
  bitmap.granularity = 2;
  bitmap.pitch = bitmap.width * bitmap.granularity;
  bitmap.viewport.w = 256;
  bitmap.viewport.h = 224;
  bitmap.viewport.x = 0;
  bitmap.viewport.y = 0;
  bitmap.data = texturemem;

  /* default system */
  input.system[0] = SYSTEM_GAMEPAD;
  input.system[1] = SYSTEM_GAMEPAD;
}

/**************************************************
  Load a new rom and performs some initialization
***************************************************/
void reloadrom ()
{
  load_rom("");       /* Load ROM */
  system_init ();     /* Initialize System */
  audio_init(48000);  /* Audio System initialization */
  ClearGGCodes ();    /* Clear Game Genie patches */
  system_reset ();    /* System Power ON */
}

/***************************************************************************
 *  M A I N
 *
 ***************************************************************************/
u8 fat_enabled = 0;
u32 frameticker = 0;

int main (int argc, char *argv[])
{
#ifdef HW_RVL
  /* initialize Wii DVD interface first */
  DI_Close();
  DI_Init();
#endif

  uint32 RenderedFrames   = 0;
  uint32 TotalFrames      = 0;
  uint32 FramesPerSecond  = 0;

  /* initialize OGC subsystems */
  ogc_video__init();
  ogc_input__init();
  ogc_audio__init();

#ifdef HW_DOL
  /* initialize GC DVD interface */
  DVD_Init ();
  dvd_drive_detect();
#endif

#ifdef HW_RVL
  /* Power button callback */
  SYS_SetPowerCallback(Power_Off);
#endif

  /* initialize FAT Interface */
  if (fatInitDefault())
  {
    fat_enabled = 1;
#ifdef HW_RVL
    fatEnableReadAhead ("sd", 6, 64);
    fatEnableReadAhead ("usb", 6, 64);
#else
    fatEnableReadAhead ("carda", 6, 64);
    fatEnableReadAhead ("cardb", 6, 64);
#endif
  }

  /* default config */
  legal();
  set_config_defaults();
  config_load();

  /* restore recent files list */
  set_history_defaults();
  history_load();

  /* initialize Virtual Machine */
  init_machine ();

  /* load any injected rom */
  if (genromsize)
  {
    ARAMFetch((char *)cart_rom, (void *)0x8000, genromsize);
    reloadrom ();
  }

  /* show menu first */
  ConfigRequested = 1;

  /* main emulation loop */
  while (1)
  {
    /* check for menu request */
    if (ConfigRequested)
    {
      /* stop audio & video */
      ogc_audio__stop();
      ogc_video__stop();

      /* go to menu */
      MainMenu (FramesPerSecond);
      ConfigRequested = 0;

      /* reset framecounts */
      RenderedFrames  = 0;
      TotalFrames     = 0;
      FramesPerSecond = vdp_rate;

      /* start audio & video */
      ogc_video__start();
      ogc_audio__start();

      /* reset framesync */
      frameticker = 1;
    }

    if (frameticker > 1)
    {
      /* skip frame */
      frameticker-=2;
      system_frame (1);

      /* update audio only */
      ogc_audio__update();
    }
    else
    {
      /* framesync */
      while (frameticker < 1) usleep(1);
      frameticker--;

      /* render frame */
      system_frame (0);

      /* update video & audio */
      ogc_video__update();
      ogc_audio__update();
      RenderedFrames++;
    }

    /* update framecounts */
    TotalFrames++;
    if (TotalFrames == vdp_rate)
    {
      FramesPerSecond = RenderedFrames;
      RenderedFrames  = 0;
      TotalFrames     = 0;
    }
  }

  return 0;
}