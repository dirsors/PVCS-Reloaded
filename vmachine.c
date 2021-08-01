/*****************************************************************************

   This file is part of Virtual VCS, the Atari 2600 Emulator
   ===================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.
   Modified 1996 by Daniel Boris

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.

   See the file COPYING for Details.

******************************************************************************/

/*
   The virtual machine. Contains the RIOT timer code, and hardware
   initialisation.
*/
#ifdef WINDOWS
#include <stdio.h>
#endif
#ifdef PS2_EE
#include <tamtypes.h>
#endif
#include "types.h"
#include "address.h"
#include "resource.h"
#include "display.h"
#include "cpu.h"
#include "misc.h"
#include "mouse.h"

/* The Rom define might need altering for paged carts */
BYTE cartram[256];
BYTE cart[65536] __attribute__((aligned(16)));
BYTE theRom[4096];
BYTE theRam[128];
BYTE tiaRead[0x0e];
BYTE tiaWrite[0x2d];

/* These don't strictly need so much space */
BYTE riotRead[0x298];
BYTE riotWrite[0x298];

/* 
Hardware addresses not programmer accessible 
*/

/* Set if processor is reset */
int reset_flag=0;

/* The timer resolution, can be 1,8,64,1024 */
uInt32 timer_res=6;
uInt32 timer_count=0;
Int32 timer_clks=0;
int myCyclesWhenInterruptReset = 0;
int myTimerReadAfterInterrupt = 0;

extern CLOCK clk;

/* The tv size, varies with PAL/NTSC */
int tv_width, tv_height, tv_vsync, tv_vblank,
    tv_overscan, tv_frame, tv_hertz, tv_hsync;
int tv_xstart, tv_ystart;

BYTE x26_keypad[8];

struct Paddle {
	long pos;
	long val;
} paddle[4];

/* Device independent screen initialisations */
void init_screen(void)
{
   tv_vsync=3;
   tv_hsync=68;
   
   tv_width=160; tv_height=210;
   tv_vblank=40; tv_overscan=30;
   tv_frame=262; tv_hertz=60;
   tv_xstart=0; tv_ystart=34;

#ifdef NDEF
   tv_width=160; tv_height=192;
   tv_vblank=40; tv_overscan=30;
   tv_frame=262; tv_hertz=60;
   tv_xstart=0; tv_ystart=43;
   
   /* TODO */
   /* A lot of games use non standard settings */
   tv_width=144; tv_height=192;
   tv_vblank=40; tv_overscan=30;
   tv_frame=262; tv_hertz=60;
   tv_xstart=8; tv_ystart=43;
#endif
   
}

/* Initialise the RIOT */
void init_riot(void)
{
	int i;
	/* Wipe the RAM */
	//for(i=0;i<0x80;i++) theRam[i]=0;
	for(i=0;i<0x7e;i++) theRam[i]=0;

	/* Set the timer to zero */
	riotRead[INTIM]=0;

	/* Set the joysticks and switches to input */
	riotWrite[SWACNT]=0;
	riotWrite[SWBCNT]=0;

	/* Centre the joysticks */
	riotRead[SWCHA]=0xff;
	/* riotRead[SWCHB]=0x0b; */
	riotRead[SWCHB]=0x3f;

	/* Set the counter resolution */
	timer_res=6;
	timer_count=25;
	timer_clks=0;
}

/* Initialise the television interface adaptor (TIA) */
void init_tia(void)
{
   tiaRead[INPT4]=0x80;
   tiaRead[INPT5]=0x80;
}


/* Main hardware startup */
void init_hardware(void)
{
  int i;
  
  init_screen();
  init_riot();
  init_tia();
  for(i=0; i<8; i++) x26_keypad[i]=0;
  if (app_data.left == PADDLE) {
    init_mouse();
    mouse_sensitivity(app_data.pad_sens*10);
  }
  
  init_cpu(0xfffc);
}

/*
   Called when the timer is set .
   Note that res is the bit shift, not absolute value.
   Assumes that any timer interval set will last longer than the instruction
   setting it.
*/
void set_timer(int res, int count, int clkadj)
{
   timer_count=count;
   timer_clks=clk+clkadj;
   timer_res=res;
   myTimerReadAfterInterrupt = 0;
}

void resetTimerCycles(int cycles)
{
  // System cycles are being reset to zero so we need to adjust
  // the cycle count we remembered when the timer was last set
  //myCyclesWhenTimerSet -= cycles;
  timer_clks -= cycles;
  myCyclesWhenInterruptReset -= cycles;
}

BYTE do_timer(ADDRESS a, int clkadj)
{
   BYTE result=0x00;
   uInt32 delta;
   Int32 timer, offset;
  
   switch(a & 0x07) {
   case 0x04:    // Timer Output
   case 0x06:
     {
       delta=(clk+clkadj-1)-timer_clks;
       timer=timer_count - (delta >> timer_res) - 1;
       if(timer >= 0) {
	 /* Timer is still going down in res intervals */
	 result=timer;
       }
       else {
	 timer = (Int32)(timer_count << timer_res) - (Int32)delta - 1;
	 
	 if((timer <= -2) && !myTimerReadAfterInterrupt)
	   {
	     // Indicate that timer has been read after interrupt occured
	     myTimerReadAfterInterrupt = 1;
	     myCyclesWhenInterruptReset = clk+clkadj;
	   }
	 
	 if(myTimerReadAfterInterrupt)
	   {
	     offset = myCyclesWhenInterruptReset -
	       (timer_clks + (timer_count << timer_res));
	     
	     timer = (Int32)timer_count - (Int32)(delta >> timer_res) - offset;
	   }
	 result = timer;
       }
       break;
     }
   case 0x05:    // Interrupt Flag
   case 0x07:
     {
       delta=(clk+clkadj)-timer_clks - 1;
       timer = (Int32)timer_count - (Int32)(delta >> timer_res) - 1;
       
       if((timer >= 0) || myTimerReadAfterInterrupt)
	 return 0x00;
       else
	 return 0x80;
       break;
     }
   }
   return result;
}
