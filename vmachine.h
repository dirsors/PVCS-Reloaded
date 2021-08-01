#ifndef VMACHINE_H
#define VMACHINE_H

/*
   $Id: vmachine.h,v 2.5 1996/03/12 16:15:24 alex Exp alex $
   External definitions of the vmachine.c stuff
*/

#include <btypes.h>
#include "bspf.h"

extern BYTE cartram[256];
extern BYTE cart[65536] __attribute__((aligned(16)));
extern BYTE theRom[4096];
extern BYTE theRam[128];
extern BYTE riotRead[0x298];
extern BYTE riotWrite[0x298];
extern BYTE tiaWrite[0x2d];
extern BYTE tiaRead[0x0e];

extern int reset_flag;

extern int ebeamx, ebeamy, sbeamx;

extern BYTE x26_keypad[8];


#define VSYNCSTATE 1
#define VBLANKSTATE 2
#define HSYNCSTATE 4
#define DRAWSTATE 8
/* #define OVERSTATE 16 removed as is only a special case of HSYNC/DRAW*/

extern int vbeam_state, hbeam_state;

extern int tv_width, tv_height, tv_vsync, tv_vblank,
    tv_overscan, tv_frame, tv_hertz, tv_hsync, tv_xstart, tv_ystart;

extern int timer_res, timer_count, timer_clks; 

extern struct Paddle {
	long pos;
	long val;
} paddle[4];


extern void init_hardware(void);
extern void set_timer(int res, int count, int clkadj);
extern BYTE do_timer(ADDRESS a, int clkadj);
extern void do_screen(int clks);
extern void resetTimerCycles(int count);
extern uInt8 *currentFrameBuffer();

#endif
