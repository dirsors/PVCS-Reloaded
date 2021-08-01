/*****************************************************************************

 This file is part of Virtual VCS, the Atari 2600 Emulator
 ===================================================

 Copyright 1996 Alex Hornby. For contributions see the file CREDITS.
 Modified 1996 Daniel Boris

 This software is distributed under the terms of the GNU General Public
 License. This is free software with ABSOLUTELY NO WARRANTY.

 See the file COPYING for Details.

 $Id: main.c,v 1.13 1996/03/21 16:36:01 alex Exp $
 ******************************************************************************/

/*
 The main program body.
 */

#ifdef WINDOWS
#include <stdlib.h>
#include <stdio.h>
#endif

#include <string.h>
#ifdef PS2_EE
#include <kernel.h>
#include <audsrv.h>
#include <sifrpc.h>
#include <fileio.h>
#include <stdio.h>
#include <loadfile.h>
#include <libpad.h>
#include <tamtypes.h>
#include <gsKit.h>
#include <dmaKit.h>
#include "browser.h"
#include "cnfsettings.h"
#include "init.h"
#include "bdraw.h"
#include "ps2font.h"
#include "pad.h"
#define strcmpi strcmp
#endif

#include "types.h"
#include "main.h"
#include "menu.h"
#include "display.h"
#include "keyboard.h"
#include "files.h"
#include "config.h"
#include "vmachine.h"
#include "address.h"

#define STICK 	0x01
#define PADDLE 	0x02
#define KEYPAD  0x03

struct resource {
	int rr;
	int debug;
	char bank;
	char pad_sens;
	char sc;
	char autoconfig;
	char left;
	char right;
	char swap;
} app_data;

void clear_tia();
int load_rom(char *name);

/* The mainloop from cpu.c */
extern void mainloop();
extern void Set_Old_Int9();
void LoadModules();

void Set_Old_Int9() {
}

#ifdef PS2_EE

GSGLOBAL *gsGlobal;
extern skin FCEUSkin;
extern vars Settings;
extern RGB myPalette[];
GSTEXTURE VCSTEX;
extern char path[4096];

int init_machine();
void update_input();

char rom_filename[1024];

//int snd_sample;

int sound = 1;

u8 bitmap_data[WIDTH * HEIGHT] __attribute__((aligned(128))) __attribute__ ((section (".bss")));

void setupVCSTexture(void);
void setupVCSGS(void);
void updatePS2Clut(void);

void x_loop() {
}
#endif

void audio_init();
void stella_init_tia();
void stella_reset_tia();

/* The main entry point */
int main(int argc, char **argv) {
#ifdef PS2_EE

	char *temp;
	init_machine();

	audio_init();
#endif
	int i;

#ifdef WINDOWS
	char file[255];
#endif

	//	char attr[50], val[50];
	//	char *p;

#ifdef WINDOWS
	/* clrscr(); */
	printf("                         Virtural VCS  ver 0.60\n");
	printf("                      copyright 1996 by Daniel Boris\n");
	printf("                ported from X2600 copyright 1996 by Alex Hornby\n");
	printf("      This software is released under the terms of the GNU Public License\n");
	printf("\nVirtual VCS starting...\n");
	if (argc < 2) {
		printf("\nUse: vcs [file] [options]");
		printf("  -bank=#: Set bank select mode\n");
		printf("  -debug:  Start in debug mode\n");
		printf("  -r=#: Set frame rate\n");
		printf("  -left=(STICK,PADDLE,KEYPAD): Set left controller\n");
		printf("  -right=(STICK,PADDLE,KEYPAD): Set right controller\n");
		printf("  -sens=#: Set mouse sensitivity for paddle emulation\n");
		printf("  -auto: Autoconfigure from config file\n");
		printf("  -swap: Swap left and right joystick keys\n");
		/* exit(0); */
	}
#endif

	app_data.rr=1;
	app_data.debug=0;
	app_data.bank=0;
	app_data.pad_sens=5;
	app_data.sc=0;
	app_data.left=STICK;
	app_data.right=STICK;
	for (i=0; i<4; i++) {
		paddle[i].pos=0;
		paddle[i].val=0;
	}
	app_data.autoconfig=0;
	app_data.swap=0;

	//	if (argc > 1) {
	//		for (i=1; i<argc; i++) {
	//			if (argv[i][0] != 45) {
	//				strcpy(file, argv[i]);
	//			} else {
	//				p=strtok(argv[i], "=");
	//				strcpy(attr, p);
	//				p=strtok(NULL,"=");
	//				strcpy(val, p);
	//				if (!strcmpi(attr,"-swap")) {
	//					app_data.swap=1;
	//				}
	//				if (!strcmpi(attr,"-auto")) {
	//					app_data.autoconfig=1;
	//				}
	//				/*
	//				 if (!strcmpi(attr,"-sens")) {
	//				 sscanf(val,"%d",&app_data.pad_sens);
	//				 } */
	//				if (!strcmpi(attr,"-left")) {
	//					if (!strcmpi(val,"stick"))
	//						app_data.left=STICK;
	//					if (!strcmpi(val,"paddle"))
	//						app_data.left=PADDLE;
	//					if (!strcmpi(val,"keypad"))
	//						app_data.left=KEYPAD;
	//				}
	//				if (!strcmpi(attr,"-right")) {
	//					if (!strcmpi(val,"stick"))
	//						app_data.right=STICK;
	//					if (!strcmpi(val,"paddle"))
	//						app_data.right=PADDLE;
	//					if (!strcmpi(val,"keypad"))
	//						app_data.right=KEYPAD;
	//				}
	//				if (!strcmpi(attr,"-bank")) {
	//					if (!strcmpi(val,"F6"))
	//						app_data.bank=1;
	//					if (!strcmpi(val,"F8"))
	//						app_data.bank=2;
	//					if (!strcmpi(val,"E0"))
	//						app_data.bank=4;
	//					if (!strcmpi(val,"FA"))
	//						app_data.bank=3;
	//					if (!strcmpi(val,"F6SC")) {
	//						app_data.bank=1;
	//						app_data.sc=1;
	//					}
	//					if (!strcmpi(val,"F8SC")) {
	//						app_data.bank=2;
	//						app_data.sc=1;
	//					}
	//				}
	//				if (!strcmpi(attr,"-debug")) {
	//					app_data.debug=1;
	//				}
	//				/*
	//				 if (!strcmpi(attr,"-r")) {
	//				 sscanf(val,"%d",&app_data.rr);
	//				 } */
	//			}
	//		}
	//	}
#ifdef PS2_EE

	if(sound) {
		struct audsrv_fmt_t format;
		format.bits = 8;
		format.freq = 44100; //44100;
		format.channels = 1;
		audsrv_set_format(&format);
		audsrv_set_volume(MAX_VOLUME);
	}

	setupVCSTexture();
	
	while (1) {
		if (sound)
			audsrv_stop_audio();

		//file = GetRom();
		strcpy(rom_filename, Browser(1, 0, 1));

		init_hardware();

		stella_init_tia();
		stella_reset_tia();

		/* load image */
		if (load_rom(rom_filename) == 0) {
			//setupVCSGS();

//			if (sound) {
//				SjPCM_Clearbuff();
//				SjPCM_Play();
//			}

			/* start cpu */
			mainloop();

			clear_tia();

//			if (sound) {
//				SjPCM_Clearbuff();
//				SjPCM_Pause();
//			}

			temp = strrchr(path, '/');
			temp++;
			*temp = 0;
		}
	}
#endif

#ifdef WINDOWS
	audio_init();

	init_hardware();

	stella_init_tia();
	stella_reset_tia();

	/* load image */
	load_rom(file);
	/* start cpu */
	mainloop();

	clear_tia();
#endif

	/* Thats it folk's */
	exit(0);
}

#ifdef PS2_EE
int init_machine() {
	int i, sometime;

	InitPS2();
	setupPS2Pad();

	Default_Global_CNF();

	Load_Global_CNF("mc0:/PVCS/PVCS.CNF");

//	if (Settings.display) // PAL
//		snd_sample = SND_RATE / 50;
//	else
//		// NTSC
//		snd_sample = SND_RATE / 60;

	for (i = 0; i < 3; i++) {
		sometime = 0x10000;
		while (sometime--)
			asm("nop\nnop\nnop\nnop");
	}

	SetupGSKit();

	gsKit_init_screen(gsGlobal); //initialize everything
	init_custom_screen(); //init user screen settings

	loadFont(0);

	Default_Skin_CNF();

//	if (SjPCM_Init() < 0)
//		printf("SjPCM Bind failed!!\n");

	return 0;
}

void setupVCSTexture(void) {

	VCSTEX.PSM = GS_PSM_T8;
	VCSTEX.ClutPSM = GS_PSM_CT32;
	VCSTEX.Clut = memalign(128, gsKit_texture_size_ee(16, 16, VCSTEX.ClutPSM));
	VCSTEX.VramClut = gsKit_vram_alloc(gsGlobal, gsKit_texture_size(16, 16,
			VCSTEX.ClutPSM), GSKIT_ALLOC_USERBUFFER);
	VCSTEX.Width = WIDTH;
	VCSTEX.Height = HEIGHT;
	VCSTEX.TBW = 4; //256;
	VCSTEX.Vram = gsKit_vram_alloc(gsGlobal, gsKit_texture_size(VCSTEX.Width,
			VCSTEX.Height, VCSTEX.PSM), GSKIT_ALLOC_USERBUFFER);

}

void updatePS2Clut(void) {
	int i, j, r[4], g[4], b[4];
	
	for (i = 0; i < 32; i += 4) {
		for (j = 0; j < 8; j++) {
			r[0] = myPalette[(i * 8) + j].red;
			r[1] = myPalette[((i + 2) * 8) + j].red;
			r[2] = myPalette[((i + 1) * 8) + j].red;
			r[3] = myPalette[((i + 3) * 8) + j].red;

			g[0] = myPalette[(i * 8) + j].green;
			g[1] = myPalette[((i + 2) * 8) + j].green;
			g[2] = myPalette[((i + 1) * 8) + j].green;
			g[3] = myPalette[((i + 3) * 8) + j].green;

			b[0] = myPalette[(i * 8) + j].blue;
			b[1] = myPalette[((i + 2) * 8) + j].blue;
			b[2] = myPalette[((i + 1) * 8) + j].blue;
			b[3] = myPalette[((i + 3) * 8) + j].blue;

			VCSTEX.Clut[(i * 8) + j] = ((b[0]<<16)|(g[0]<<8)|(r[0]<<0));
			VCSTEX.Clut[((i + 1) * 8) + j] = ((b[1]<<16)|(g[1]<<8)|(r[1]<<0));
			VCSTEX.Clut[((i + 2) * 8) + j] = ((b[2]<<16)|(g[2]<<8)|(r[2]<<0));
			VCSTEX.Clut[((i + 3) * 8) + j] = ((b[3]<<16)|(g[3]<<8)|(r[3]<<0));
		}
	}
}

void setupVCSGS(void) {
	gsGlobal->DrawOrder = GS_OS_PER;
	gsKit_mode_switch(gsGlobal, GS_PERSISTENT);
	gsKit_queue_reset(gsGlobal->Per_Queue);

	gsKit_clear(gsGlobal, GS_SETREG_RGBA(0x00,0x00,0x00,0x80));

	if (Settings.filter)
		VCSTEX.Filter = GS_FILTER_LINEAR;
	else
		VCSTEX.Filter = GS_FILTER_NEAREST;

	gsKit_prim_sprite_texture( gsGlobal, &VCSTEX,
			0.0f, /* X1 */
			0.0f, /* Y1 */
			0.0f, /* U1 */
			0.0f, /* V1 */
			gsGlobal->Width, /* X2 *///stretch to screen width
			gsGlobal->Height, /* Y2 *///stretch to screen height
			VCSTEX.Width, /* U2 */
			VCSTEX.Height, /* V2*/
			2, /* Z */
			GS_SETREG_RGBA(0x80,0x80,0x80,0x80) /* RGBA */
	);
	
	updatePS2Clut();
}

void put_image_ps2() {
	int i;
	uInt8 *ptr;

	ptr = currentFrameBuffer();

	// I didn't like this...
	for (i=0; i<WIDTH*HEIGHT; i++) {
		bitmap_data[i] = *ptr;
		if (i % 2)
			ptr++;
	}

	VCSTEX.Mem = (u32 *)bitmap_data;

	gsKit_texture_upload(gsGlobal, &VCSTEX);

	/* vsync and flip buffer */
	gsKit_sync_flip(gsGlobal);

	/* execute render queue */
	gsKit_queue_exec(gsGlobal);
	
	update_input();

}

#endif
