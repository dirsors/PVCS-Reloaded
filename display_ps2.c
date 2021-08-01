/*****************************************************************************

   This file is part of Virtual VCS, the Atari 2600 Emulator
   ===================================================

   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.
   Modified 1996 by Daniel Boris

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.

   See the file COPYING for Details.

   $Id: display.c,v 1.23 1996/03/21 15:52:38 alex Exp $
******************************************************************************/

/*
   Display handling code.
*/

#include <stdio.h>
#include <tamtypes.h>
#include "types.h"
#include "display.h"
#include "vmachine.h"
#include "sjpcm.h"


#define WIDTH		320
#define HEIGHT		256

/* VRAM layout:

0x000000 - FB 1
0x040000 - FB 2 (FB 1 + 256*256*4)
0x080000 - ZBuf (FB 2 * 2)
0x0A0000 - End of ZBuf. Star of TEX and CLUT area.

0x0B0000 - SMS Display Texture (0xC000 bytes long)
0x0BC000 - SMS Display Clut (0x200 bytes long)

*/

#define SND_RATE	48000

extern RGB myPalette[];
extern int whichdrawbuf;
extern int snd_sample;
extern void update_input();
extern void setSmallFont();


int disp_w, disp_h;

uInt32 tmp[WIDTH * HEIGHT] __attribute__((aligned(16))) __attribute__ ((section (".bss")));


void setmode_ps2()
{
//  if(gs_is_ntsc()) {
//    g2_init(NTSC_C64);
//    snd_sample = SND_RATE / 60;
//  } else {
//    g2_init(PAL_C64);
//    snd_sample = SND_RATE / 50;
//  }
//
//  g2_set_visible_frame(1);
//  g2_set_active_frame(0);
//
//  setSmallFont();
//
//  /* Clear Screen */
//  clear_screen_ps2();
}

int is_ntsc()
{
//  if(gs_is_ntsc()) {
//    return 1;
//  }
//  return 0;
}

void clear_screen_ps2()
{
//  /* Clear Screen */
//  g2_set_fill_color(0, 0, 0);
//  disp_w = g2_get_max_x();
//  disp_h = g2_get_max_y();
//  g2_fill_rect(0, 0, g2_get_max_x(), g2_get_max_y());
//  g2_flip_buffers();
//  g2_fill_rect(0, 0, g2_get_max_x(), g2_get_max_y());
}

void put_image_ps2()
{
//  int i;
//  uInt8 *ptr;
//  
//  update_input();
//
//  ptr = currentFrameBuffer();
//
//  for(i=0;i<WIDTH*HEIGHT;i++)
//    {
//      tmp[i] =	((uInt8)(myPalette[*ptr].red)  << 0)   |
//	((uInt8)(myPalette[*ptr].green)<< 8)   | 
//	((uInt8)(myPalette[*ptr].blue) << 16)  |
//	((uInt8)(255)				<< 24);
//      if (i % 2) ptr++;
//    }
//  
//  g2_fill_rect(0, 0, disp_w, disp_h);
//  g2_put_image(30, 15, WIDTH, HEIGHT, WIDTH, HEIGHT, (uInt32*)tmp);
//  g2_flip_buffers();

}
