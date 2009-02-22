/*****************************************************************************
 * font.c
 *
 *   IPL FONT Engine, based on Qoob MP3 Player Font
 *
 *   code by Softdev (2006), Eke-Eke(2007-2008)
 * 
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 ********************************************************************************/

#include "shared.h"
#include "font.h"
#include "Overlay_bar_s2.h"

#include <png.h>


/* Backdrop Frame Width (to avoid writing outside of the background frame) */
u16 back_framewidth = 640;
int font_size[256], fheight;

#ifndef HW_RVL
/* disable Qoob Modchip before IPL access (emukiddid) */
void ipl_set_config(unsigned char c)
{
        volatile unsigned long* exi = (volatile unsigned long*)0xCC006800;
        unsigned long val,addr;
        addr=0xc0000000;
        val = c << 24;
        exi[0] = ((((exi[0]) & 0x405) | 256) | 48);     //select IPL
        //write addr of IPL
        exi[0 * 5 + 4] = addr;
        exi[0 * 5 + 3] = ((4 - 1) << 4) | (1 << 2) | 1;
        while (exi[0 * 5 + 3] & 1);
        //write the ipl we want to send
        exi[0 * 5 + 4] = val;
        exi[0 * 5 + 3] = ((4 - 1) << 4) | (1 << 2) | 1;
        while (exi[0 * 5 + 3] & 1);
        exi[0] &= 0x405;        //deselect IPL
}
#endif

//static u8 *sys_fontarea;
static sys_fontheader *fontHeader;
static u8 *texture;

extern u32 __SYS_LoadFont(void *src,void *dest);

void init_font(void)
{
#ifndef HW_RVL
  /* disable Qoob before accessing IPL */
  ipl_set_config(6);
#endif

  /* initialize IPL font */
  fontHeader = memalign(32,sizeof(sys_fontheader));
  SYS_InitFont(&fontHeader);

  int i,c;
  for (i=0; i<256; ++i)
  {
    if ((i < fontHeader->first_char) || (i > fontHeader->last_char)) c = fontHeader->inval_char;
    else c = i - fontHeader->first_char;

    font_size[i] = ((unsigned char*)fontHeader)[fontHeader->width_table + c];
  }

  fheight = fontHeader->cell_height;
  texture = memalign(32, fontHeader->cell_width * fontHeader->cell_height / 2);
}

u8 draw_done = 0;
void font_DrawChar(unsigned char c, u32 xpos, u32 ypos, u32 color)
{
  s32 width;
  memset(texture,0,fontHeader->cell_width * fontHeader->cell_height / 2); 
  GXTexObj texobj;
  GX_InitTexObj(&texobj, texture, fontHeader->cell_width, fontHeader->cell_height, GX_TF_I4, GX_CLAMP, GX_CLAMP, GX_FALSE);
  GX_LoadTexObj(&texobj, GX_TEXMAP0);
  SYS_GetFontTexel(c,texture,0,fontHeader->cell_width/2,&width);

  DCFlushRange(texture, fontHeader->cell_width * fontHeader->cell_height / 2);
  GX_InvalidateTexAll();

  GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
  GX_Position2s16(xpos, ypos - fontHeader->cell_height);
  GX_TexCoord2f32(0.0, 0.0);
  GX_Position2s16(xpos + fontHeader->cell_width, ypos - fontHeader->cell_height);
  GX_TexCoord2f32(1.0, 0.0);
  GX_Position2s16(xpos + fontHeader->cell_width, ypos);
  GX_TexCoord2f32(1.0, 1.0);
  GX_Position2s16(xpos, ypos);
  GX_TexCoord2f32(0.0, 1.0);
  GX_End ();

  GX_DrawDone();
}

void setfontcolour (int fcolour)
{
}

void write_font(int x, int y, char *string)
{
  int ox = x;
  while (*string && (x < (ox + back_framewidth)))
  {
    font_DrawChar(*string, x -(vmode->fbWidth/2), y-(vmode->efbHeight/2),1);
    x += font_size[(u8)*string];
    string++;
  }
}

int hl = 0;
void WriteCentre( int y, char *string)
{
  int x, t;
  for (x=t=0; t<strlen(string); t++) x += font_size[(u8)string[t]];
  if (x>back_framewidth) x=back_framewidth;
  x = (640 - x) >> 1;
  write_font(x, y, string);
  if (hl)
  {
    png_texture texture;
    texture.data   = 0;
    texture.width  = 0;
    texture.height = 0;
    texture.format = 0;
    OpenPNGFromMemory(&texture, Overlay_bar_s2);
    DrawTexture(&texture, 0, y-fheight,  640, fheight);
  }
}

void WriteCentre_HL( int y, char *string)
{
  hl = 1;
  WriteCentre(y, string);
  hl = 0;
}


/****************************************************************************
 *  Draw functions (FrameBuffer)
 *
 ****************************************************************************/
void fntDrawHLine (int x1, int x2, int y, int color)
{
  int i;
  y = 320 * y;
  x1 >>= 1;
  x2 >>= 1;
  for (i = x1; i <= x2; i++) xfb[whichfb][y + i] = color;
}

void fntDrawVLine (int x, int y1, int y2, int color)
{
  int i;
  x >>= 1;
  for (i = y1; i <= y2; i++) xfb[whichfb][x + (640 * i) / 2] = color;
}

void fntDrawBox (int x1, int y1, int x2, int y2, int color)
{
  fntDrawHLine (x1, x2, y1, color);
  fntDrawHLine (x1, x2, y2, color);
  fntDrawVLine (x1, y1, y2, color);
  fntDrawVLine (x2, y1, y2, color);
}

void fntDrawBoxFilled (int x1, int y1, int x2, int y2, int color)
{
  int h;
  for (h = y1; h <= y2; h++) fntDrawHLine (x1, x2, h, color);
}

/****************************************************************************
 *  Draw functions (GX)
 *
 ****************************************************************************/

/* Callback for the read function */
static void png_read_from_mem (png_structp png_ptr, png_bytep data, png_size_t length)
{
  png_file *file = (png_file *)png_get_io_ptr (png_ptr);

  /* copy data from image buffer */
  memcpy (data, file->buffer + file->offset, length);

  /* advance in the file */
  file->offset += length;
}

void OpenPNGFromMemory(png_texture *texture, const u8 *buffer)
{
  int i;
  png_file file;

  /* init PNG file structure */
  file.buffer = (u8 *) buffer;
  file.offset = 0;

  /* check for valid magic number */
  if (!png_check_sig (file.buffer, 8)) return;

  /* create a png read struct */
  png_structp png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr) return;

  /* create a png info struct */
  png_infop info_ptr = png_create_info_struct (png_ptr);
  if (!info_ptr)
  {
    png_destroy_read_struct (&png_ptr, NULL, NULL);
    return;
  }

  /* set callback for the read function */
  png_set_read_fn (png_ptr, (png_voidp *)(&file), png_read_from_mem);

  /* read png info */
  png_read_info (png_ptr, info_ptr);

  /* retrieve image information */
  u32 width  = png_get_image_width(png_ptr, info_ptr);
  u32 height = png_get_image_height(png_ptr, info_ptr);
  u32 bit_depth = png_get_bit_depth(png_ptr, info_ptr);
  u32 color_type = png_get_color_type(png_ptr, info_ptr);

  /* support for RGBA8 textures ONLY !*/
  if ((color_type != PNG_COLOR_TYPE_RGB_ALPHA) || (bit_depth != 8))
  {
    png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
    return;
  }

  /* 4x4 tiles are required */
  if ((width%4) || (height%4))
  {
    png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
    return;
  }

  /* allocate memory to store raw image data */
  u32 stride = width << 2;
  u8 *img_data = memalign (32, stride * height);
  if (!img_data)
  {
    png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
    return;
  }

  /* allocate row pointer data */
  png_bytep *row_pointers = (png_bytep *)memalign (32, sizeof (png_bytep) * height);
  if (!row_pointers)
  {
    free (img_data);
    png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
    return;
  }

  /* store raw image data */
  for (i = 0; i < height; i++)
  {
    row_pointers[i] = img_data + (i * stride);
  }

  /* decode image */
  png_read_image (png_ptr, row_pointers);

  /* finish decompression and release memory */
  png_read_end (png_ptr, NULL);
  png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
  free(row_pointers);

  /* initialize texture */
  texture->data   = memalign(32, stride * height);
  memset(texture->data, 0, stride * height);
  texture->width  = width;
  texture->height = height;
  texture->format = GX_TF_RGBA8;


  /* encode to GX_TF_RGBA8 format (4x4 pixels paired titles) */

  u16 *dst_ar = (u16 *)(texture->data);
  u16 *dst_gb = (u16 *)(texture->data + 32);
  u32 *src1 = (u32 *)(img_data);
  u32 *src2 = (u32 *)(img_data + stride);
  u32 *src3 = (u32 *)(img_data + 2*stride);
  u32 *src4 = (u32 *)(img_data + 3*stride);
  u32 pixel,h,w;

  for (h=0; h<height; h+=4)
  {
    for (w=0; w<width; w+=4)
    {
      /* line N (4 pixels) */
      for (i=0; i<4; i++)
      {
        pixel = *src1++;
        *dst_ar++= ((pixel << 8) & 0xff00) | ((pixel >> 24) & 0x00ff);
        *dst_gb++= (pixel >> 8) & 0xffff;
      }

      /* line N + 1 (4 pixels) */
      for (i=0; i<4; i++)
      {
        pixel = *src2++;
        *dst_ar++= ((pixel << 8) & 0xff00) | ((pixel >> 24) & 0x00ff);
        *dst_gb++= (pixel >> 8) & 0xffff;
      }

      /* line N + 2 (4 pixels) */
      for (i=0; i<4; i++)
      {
        pixel = *src3++;
        *dst_ar++= ((pixel << 8) & 0xff00) | ((pixel >> 24) & 0x00ff);
        *dst_gb++= (pixel >> 8) & 0xffff;
      }

      /* line N + 3 (4 pixels) */
      for (i=0; i<4; i++)
      {
        pixel = *src4++;
        *dst_ar++= ((pixel << 8) & 0xff00) | ((pixel >> 24) & 0x00ff);
        *dst_gb++= (pixel >> 8) & 0xffff;
      }

      /* next paired tiles */
      dst_ar += 16;
      dst_gb += 16;
    }

    /* next 4 lines */
    src1 = src4;
    src2 = src1 + width;
    src3 = src2 + width;
    src4 = src3 + width;
  }

  /* release memory */
  free(img_data);

  /* flush texture data from cache */
  DCFlushRange(texture->data, height * stride);
}

void DrawTexture(png_texture *texture, u32 xOrigin, u32 yOrigin, u32 w, u32 h)
{
  if (texture->data)
  {
    /* load texture object */
    GXTexObj texObj;
    GX_InitTexObj(&texObj, texture->data, texture->width, texture->height, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GX_LoadTexObj(&texObj, GX_TEXMAP0);
    GX_InvalidateTexAll();
    DCFlushRange(texture->data, texture->width * texture->height * 4);

    /* current coordinate system */
    xOrigin -= (vmode->fbWidth/2);
    yOrigin -= (vmode->efbHeight/2);


    /* Draw item */
    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
    GX_Position2s16(xOrigin,yOrigin+h);
    GX_TexCoord2f32(0.0, 1.0);
    GX_Position2s16(xOrigin+w,yOrigin+h);
    GX_TexCoord2f32(1.0, 1.0);
    GX_Position2s16(xOrigin+w,yOrigin);
    GX_TexCoord2f32(1.0, 0.0);
    GX_Position2s16(xOrigin,yOrigin);
    GX_TexCoord2f32(0.0, 0.0);
    GX_End ();

    GX_DrawDone();
    free(texture->data);
  }
}

/****************************************************************************
 *  Display functions
 *
 ****************************************************************************/
u8 SILENT = 0;

void SetScreen ()
{
  GX_CopyDisp(xfb[whichfb], GX_FALSE);
  GX_Flush();
  VIDEO_SetNextFramebuffer (xfb[whichfb]);
  VIDEO_Flush ();
  VIDEO_WaitVSync ();
}

void ClearScreen (GXColor color)
{
  whichfb ^= 1;
  GX_SetCopyClear(color,0x00ffffff);
  GX_CopyDisp(xfb[whichfb], GX_TRUE);
  GX_Flush();
}

void WaitButtonA ()
{
  s16 p = ogc_input__getMenuButtons();
  while (p & PAD_BUTTON_A)    p = ogc_input__getMenuButtons();
  while (!(p & PAD_BUTTON_A)) p = ogc_input__getMenuButtons();
}

void WaitPrompt (char *msg)
{
  if (SILENT) return;
  ClearScreen((GXColor)BLACK);
  WriteCentre(254, msg);
  WriteCentre(254 + fheight, "Press A to Continue");
  SetScreen();
  WaitButtonA ();
}

void ShowAction (char *msg)
{
  if (SILENT) return;

  ClearScreen((GXColor)BLACK);
  WriteCentre(254, msg);
  SetScreen();
}

/****************************************************************************
 * Unpack Backdrop
 *
 * Called at startup to unpack our backdrop to a temporary 
 * framebuffer.
 ****************************************************************************/
void unpackBackdrop ()
{
}
