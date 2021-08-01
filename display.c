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

#ifdef WINDOWS
#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#include <SDL_audio.h>
#endif
#include <string.h>
#include <malloc.h>
#ifdef PS2_EE
#include <tamtypes.h>
#include <gsKit.h>
#include <dmaKit.h>
#include <stdio.h>
#include <audsrv.h>
#endif
#include "types.h"
#include "config.h"
#include "vmachine.h"
#include "address.h"
#include "files.h"
#include "keyboard.h"
#include "resource.h"
#include "tiasound.h"

#define VGA256		0x13
#define TEXT_MODE	0x03
#define PALETTE_MASK      0x3c6
#define PALETTE_REGISTER  0x3c8
#define PALETTE_DATA	  0x3c9

/* unsigned char far *screen=(char far *)0xA0000000L; */

#ifdef PS2_EE
#define SND_RATE	44100

void put_image_ps2();
void setupVCSGS(void);
int snd_sample;
extern GSGLOBAL *gsGlobal;
#endif

int screen_num;

#ifdef WINDOWS

int is_ntsc() {return 1;}

SDL_Surface *screen;

#endif

RGB myPalette[256];

/* The width and height of the buffer */
int vwidth, vheight;

/* The refresh skipping counter */
int tv_counter=0;
int redraw_flag=1;

#define NUM_SOUNDS 2
#define SND_MULT        3

#ifdef WINDOWS
struct sample {
	Uint8 *data;
	Uint32 dpos;
	Uint32 dlen;
}sounds[NUM_SOUNDS];
#endif

/* Sound emulation structure */
typedef struct {
	int enabled;
	int bufsize;
	signed short *buffer[2];
	int log;
	void (*callback)(int data);
} t_snd;

t_snd snd;

void audio_init() {
	if(gsGlobal->Mode == GS_MODE_PAL)
		snd_sample = SND_RATE / 50;
	else
		snd_sample = SND_RATE / 60;
	
	/* Clear sound context */
	memset(&snd, 0, sizeof(t_snd));

	/* Reset logging data */
	snd.log = 0;
	snd.callback = NULL;

	/* Calculate buffer size in samples */
	snd.bufsize = snd_sample * SND_MULT; 

	/* Sound output */
	snd.buffer[0] = (signed short int *)memalign(16, snd.bufsize);
	if (!snd.buffer[0])
		return;

	/* Inform other functions that we can use sound */
	snd.enabled = 1;

	Tia_sound_init(31400, SND_RATE / SND_MULT); 

#ifdef WINDOWS_SOUND
	extern void mixaudio(void *unused, Uint8 *stream, int len);
	SDL_AudioSpec fmt;

	/* Set 16-bit stereo audio at 22Khz */
	fmt.freq = 22050;
	fmt.format = AUDIO_U8;
	fmt.channels = 1;
	fmt.samples = snd_sample; /* A good value for games */
	fmt.callback = mixaudio;
	fmt.userdata = NULL;

	/* Open the audio device and start playing sound! */
	if ( SDL_OpenAudio(&fmt, NULL) < 0 ) {
		fprintf(stderr, "Unable to open audio: %s\n", SDL_GetError());
		exit(1);
	}
	SDL_PauseAudio(0);
#endif
#ifdef PS2_EE
	//audio_init_ps2();
//	if (SjPCM_Init() < 0)
//			printf("SjPCM Bind failed!!\n");
#endif
}

#ifdef WINDOWS
void mixaudio(void *unused, Uint8 *stream, int len)
{
	int i;
	Uint32 amount;

	for ( i=0; i<NUM_SOUNDS; ++i ) {
		amount = (sounds[i].dlen-sounds[i].dpos);
		if ( amount > len ) {
			amount = len;
		}

		SDL_MixAudio(stream, &sounds[i].data[sounds[i].dpos], amount, SDL_MIX_MAXVOLUME);
		sounds[i].dpos += amount;
	}
}
#endif

void set_palette(int i, BYTE red, BYTE green, BYTE blue) {
#ifdef DOS
	outportb(0x3c6,0xff);
	outportb(0x3c8,index);
	outportb(0x3c9,red);
	outportb(0x3c9,green);
	outportb(0x3c9,blue);
#endif
	red=red/2;
	green=green/2;
	blue=blue/2;

	myPalette[i].red = red;
	myPalette[i].green = green;
	myPalette[i].blue = blue;
}

void setmode(int vmode) {
#ifdef DOS
	union REGS regs;

	regs.h.ah = 0;
	regs.h.al = vmode;
	int86(0x10, &regs, &regs);
#endif
#ifdef WINDOWS
	//screen=SDL_SetVideoMode(320,200,32,
	//		SDL_HWSURFACE|SDL_DOUBLEBUF);
	screen=SDL_SetVideoMode(320,200,8,
			SDL_HWPALETTE|SDL_HWSURFACE|SDL_DOUBLEBUF);
	if ( screen == NULL )
	{
		printf("Unable to set 640x480 video: %s\n", SDL_GetError());
		exit(1);
	}

#endif
#ifdef PS2_EE
	//setmode_ps2();
	setupVCSGS();
#endif
}

/** PUTIMAGE: macro copying image buffer into a window ********/
void put_image(void) {
#ifdef WINDOWS
	uInt8 *ptr;
	uInt8 *ptr2;
	int index,i;

	for ( index=0; index<NUM_SOUNDS; ++index ) {
		if ( sounds[index].dpos == sounds[index].dlen ) {
			break;
		}
	}
	if ( index != NUM_SOUNDS ) {
		/* Put the sound data in the slot (it starts playing immediately) */
		SDL_LockAudio();
		//Tia_process(snd.buffer[index], snd_sample);
		sounds[index].data = snd.buffer[index];
		sounds[index].dlen = snd_sample;
		sounds[index].dpos = 0;
		SDL_UnlockAudio();
	}

	SDL_Delay(10);
	SDL_LockSurface(screen);
	ptr=currentFrameBuffer();
	ptr2=screen->pixels;
	for(i=0;i<320*200;i++)
	{
		//*ptr2 =	((uInt8)(myPalette[*ptr].red)  << 0)   |
		//((uInt8)(myPalette[*ptr].green)<< 8)   | 
		//((uInt8)(myPalette[*ptr].blue) << 16)  |
		//((uInt8)(255)				<< 24);
		*ptr2 = *ptr;
		if (i % 2) ptr++;
		ptr2++;
	}
	SDL_UnlockSurface(screen);
	SDL_Flip(screen);
#endif
#ifdef PS2_EE
	int sound = 1;

	if (sound) {
		Tia_process((char *)snd.buffer[0], snd_sample / SND_MULT); 
		snd.enabled = 1;
		//SjPCM_Enqueue(snd.buffer[0], snd.buffer[0], snd_sample, 1);
		audsrv_play_audio((s8 *)snd.buffer[0], snd_sample); 
	}

	put_image_ps2();
#endif
}

extern uInt32 ourNTSCPaletteZ26[];
extern uInt32 ourPALPaletteZ26[];

/* Set up the colormap */
void create_cmap(void) {
	int i;
	BYTE red, green, blue;
	const uInt32* gamePalette;
#ifdef WINDOWS
	SDL_Color colors[256];
#endif

	if (gsGlobal->Mode == GS_MODE_NTSC)
		gamePalette = ourNTSCPaletteZ26;
	else
		gamePalette = ourPALPaletteZ26;

	/* Initialise parts of the colors array */
	for (i=0; i< 256; i++) {
		red = (BYTE)((gamePalette[i] & 0xff0000) >> 16);
		green = (BYTE)((gamePalette[i] & 0x00ff00) >> 8);
		blue = (BYTE)(gamePalette[i] & 0x0000ff);

		set_palette(i, red, green, blue);
#ifdef WINDOWS
		colors[i].r=red;
		colors[i].g=green;
		colors[i].b=blue;
#endif
	}
#ifdef WINDOWS
	SDL_SetPalette(screen, SDL_PHYSPAL, colors, 0, 255);
#endif
}

void tv_off(void) {
#ifdef DOS
	if (!app_data.debug) {
		Set_Old_Int9();
	}
#endif
	setmode(TEXT_MODE);
}

/* The main initialiser for the X stuff */
int tv_on() {

	if (!app_data.debug) {
		setmode(VGA256);
	}
	create_cmap();

	return (1);
}

/* The graceful shut down for the X stuff */

/* Displays the 2600 screen display */
void tv_display(void) {
	if (!app_data.debug) {
		put_image();
	}
	tv_counter++;
}

