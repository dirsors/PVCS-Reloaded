/*****************************************************************************

   This file is part of Virtual VCS, the Atari 2600 Emulator
   ===================================================

   Copyright 1996 Daniel Boris

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.

   See the file COPYING for Details.

   $Id: xdebug.c,v 2.5 1996/03/21 16:36:01 alex Exp $
******************************************************************************/

/*
 * This code has been almost complete re-written since X2600
 *
 */



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <curses.h>

#include "cpu.h"
#include "macro.h"
#include "vmachine.h"	/* I_SPECX */
#include "extern.h"
#include "misc.h"
#include "memory.h"
#include "display.h"
#include "config.h"
#include "address.h"

/*
 * Variables
 */

#define VGA256		0x13
#define TEXT_MODE	0x03


int debugf_halt=0, debugf_trace=0, debugf_raster=0;

static ADDRESS  current_point;
static char     buf[256];
static ADDRESS  brk=0;
//static ADDRESS  brk=0x10ff;
//static ADDRESS  brk=0xf2e2;
//static ADDRESS  brk=0xf80a;
static ADDRESS  dPC;
static WINDOW   *curs_win=0;


extern    uInt8 myGRP0;         // Player 0 graphics register
extern    uInt8 myGRP1;         // Player 1 graphics register

extern    uInt8 myDGRP0;        // Player 0 delayed graphics register
extern    uInt8 myDGRP1;        // Player 1 delayed graphics register

extern    Int16 myPOSP0;         // Player 0 position register
extern    Int16 myPOSP1;         // Player 1 position register
extern    Int16 myPOSM0;         // Missle 0 position register
extern    Int16 myPOSM1;         // Missle 1 position register
extern    Int16 myPOSBL;         // Ball position register

extern    int arBank;

enum Register {
	R_AC, R_XR, R_YR, R_SP, R_PC, R_SR, R_EFF, R_DL, R_CLK, NO_REGISTERS
};

static char *registerLabels[] = {
	"AC:", "XR:", "YR:", "SP:", "PC:", "SR:", "EA>", "DL>", "Clk"
};


/*
 * Functions
 */

extern void  debug_main ( void );


static void UpdateRegisters ( void );
static void UpdateHardware ( void ); /* New */
#ifdef NDEF
static void UpdateButtons ( void );
static void UpdateStatus ( enum DState );
#endif

static void  set_asm ( ADDRESS );
static void  draw_asm ( ADDRESS );

/* ------------------------------------------------------------------------- */

#define printf printw

void gotoxy(int x,int y)
{
   move(y,x);
}

int kbhit()
{
   return 0;
}

void set_single(void)
{
	debugf_halt=1;
}

void  x_loop(void)
{
  char c,cc;
  int done=0;
  unsigned int d;

  if (curs_win == 0) {
    curs_win=initscr();
    nonl();
    noecho();
    raw();
    erase();
    nodelay(curs_win, true);
    timeout(10);
  }

  if (PC == brk) {
    brk=0;
  }  else {
    gotoxy(30,6);
    printf ("PC: $%04X", PC);
    done=1;
  }

  dPC=PC;
  if (!brk) {
    /* clrscr(); */
    UpdateRegisters();
    UpdateHardware();
    set_asm(PC);
    done=0;
  }
  do {
    if (!done && (c=getch())) {
      set_asm(dPC);
      switch (c) {
      case 0:
	cc=getch();
	switch (cc) {
	case 72:
	  dPC--;
	  break;
	case 80:
	  dPC+=clength[lookup[DLOAD(dPC)&0xff].addr_mode];
	  break;
	}
	break;
      case 'B':
      case 'b':
	cc=1;
	break;
      case 'D':
      case 'd':
	setmode(VGA256);
	create_cmap();
	put_image();
	//while(!kbhit()){};
	setmode(TEXT_MODE);
	UpdateRegisters();
	UpdateHardware();
	set_asm(PC);
	getch();
	break;
      case 'Q':
      case 'q':
	tv_off();
	noraw();
	endwin();
	exit(0);
      case 's':
      case 'S':
	done=1;
	break;
      case 'm':
      case 'M':
	gotoxy(3,23);
	printf("Address: ");
	scanf("%x",&d);
	gotoxy(18,23);
	printf("= %x   ",dbgRead(d));
	break;
      case 'g':
      case 'G':
	gotoxy(1,23);
	printf("Address to run to: ");
	scanf("%x",&brk);
	gotoxy(1,23);
	printf("                  ");
	done=1;
	break;
      }
    }
  } while (!done);

}

static void UpdateHardware  (void)
{
#ifdef NDEF
	int temp;

	gotoxy(50,1);
	printf ("ebeamx: %04d", ebeamx);
	gotoxy(50,2);
	printf ("ebeamy: %04d", ebeamy);
	gotoxy(50,3);
	printf ("CTRLPF: %04d", tiaWrite[CTRLPF]);
#endif
	gotoxy(50,3);
	printf ("ar bank: %04d", arBank);
	gotoxy(50,4);
	printf ("timer: %04d", timer_count);
	gotoxy(50,5);
	printf ("INTIM: %06d", riotRead[INTIM]);
	gotoxy(50,6);
	printf ("timer_res: %06d", timer_res);
	gotoxy(50,7);
	printf ("timer_clks: %06d", timer_clks);
#ifdef NDEF
	gotoxy(50,8);
	temp= pl[0].vdel;
	printf ("Pl0 vdel: %02x", temp);
	gotoxy(50,9);
	temp= pl[1].vdel;
	printf ("Pl1 vdel: %02x", temp);
#endif
	gotoxy(50,10);
	printf ("Pl0 grp: %02x", myDGRP0);
	gotoxy(50,11);
	printf ("Pl1 grp: %02x", myDGRP1);
	gotoxy(50,12);
	printf ("Pl0 x: %03d", myPOSP0);
	gotoxy(50,13);
	printf ("Pl1 x: %03d", myPOSP1);
	gotoxy(50,14);
	printf ("Ml0 x: %03d", myPOSM0);
	gotoxy(50,15);
	printf ("Ml1 x: %03d", myPOSM1);
	gotoxy(50,16);
	printf ("BL  x: %03d", myPOSBL);
}

static void UpdateRegisters (void)
{
        char stat[7] = "nzcidv\0";
	int eff;

	gotoxy(30,2);
	printf ("AC: $%02X", AC);
	gotoxy(30,3);
	printf ("XR: $%02X", XR);
	gotoxy(30,4);
	printf ("YR: $%02X", YR);
	gotoxy(30,5);
	printf ("SP: $%02X", SP);
	gotoxy(30,6);
	printf ("PC: $%04X", PC);
	gotoxy(30,7);
	printf ("SR: $%02X", GET_SR());

	gotoxy(30,1);
	if (IF_SIGN()) {
	  stat[0]='N';
	}
	if (IF_ZERO()) {
	  stat[1]='Z';
	}
	if (IF_CARRY()) {
	  stat[2]='C';
	}
	if (IF_INTERRUPT()) {
	  stat[3]='I';
	}
	if (IF_DECIMAL()) {
	  stat[4]='D';
	}
	if (IF_OVERFLOW()) {
	  stat[5]='V';
	}
	printf ("cpu: %5.5s", stat);

	/* Memory */
	gotoxy(30,8);
	eff = eff_address(PC, 1);
	if (eff >= 0)
	printf ("EA: $%04X", eff);
	else
	printf ("EA: ----");

	gotoxy(30,9);
	if (eff >= 0)
	printf ("DL: $%02X", DLOAD(eff));
	else
	printf ("DL: --");

	/* Clock */
	gotoxy(30,10);
	printf ("CL: %06ld", clk);
}


/* Debugger  Status */

#ifdef NDEF
static void UpdateStatus (enum DState dst)
{
	static int s = -1;
	if (s == (int)dst)
	return;

	s = dst;
}
#endif


/* ------------------------------------------------------------------------- */

/*
 * Assembly Window
 */


void set_asm (ADDRESS p)
{
	draw_asm (p);
        refresh();
}


void draw_asm (ADDRESS p)
{
	char    buf2[8192];
	short   lines;  /* NOT ints */
	int i,l;
	BYTE j;

	buf2[0] = '\0';
	current_point = p;
	lines=20;
	for (i = 0; i <= lines; i++) {
		p &= 0xffff;
		sprintf (buf, "%04X %s  %-s", p, sprint_ophex (p), sprint_opcode(p, 1));
		l=strlen(buf);
		if (l < 28) {
			for(j=0; j < (28-l); j++) strcat(buf," ");
		}
		gotoxy(1,i+1);
		printf("%s",buf);
		p+=clength[lookup[DLOAD(p)&0xff].addr_mode];

	}

}


