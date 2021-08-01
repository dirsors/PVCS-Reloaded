#ifndef DISPLAY_H
#define DISPLAY_H

#define VGA256		0x13
#define TEXT_MODE	0x03

#include "bspf.h"

extern void create_cmap(void);
extern void put_image(void);
void setmode(int);
void fselLoad(void);
void rate_update(void);
void tv_off(void);
int tv_on();
void tv_event(void);
void tv_display(void);
void tv_putpixel(int x, int y, BYTE value);
extern BYTE tv_color(BYTE b);
extern int redraw_flag;


#ifdef PS2_EE
extern u8 vscreen[320 * 256];
extern u16 clut[256];
void setCourier(void);
void audio_init_ps2(void);
void clear_screen_ps2(void);
void put_image_ps2(void);
#else
extern uInt8 *vscreen;
#endif
extern char coltable[256];

extern int magstep;
extern int vwidth,vheight;
extern int tv_counter;
extern unsigned int linetab[256];
#endif

