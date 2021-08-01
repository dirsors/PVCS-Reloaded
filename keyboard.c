/*****************************************************************************

   This file is part of Virtual VCS, the Atari 2600 Emulator
   ===================================================

   Copyright 1996 Daniel Boris. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.

   See the file COPYING for Details.

 ******************************************************************************/
/*
  This module has been completely re-written since X2600
 */

#ifdef WINDOWS
#include <stdio.h>
#include <SDL_events.h>
#endif
#ifdef PS2_EE
#include <libpad.h>
#include <string.h>
#include <tamtypes.h>
#include <audsrv.h>
#endif
#include "types.h"
#include "address.h"
#include "vmachine.h"
#include "extern.h"
#include "memory.h"
#include "display.h"
#include "resource.h"

#define MESSAGE_INTERVAL     2
#define TOGGLE_DELAY         10

extern void showMessage(char *message, Int32 duration);
static int toggle_delay = 0;

#ifdef PS2_EE

void Ingame_Menu(void);

t_input input;

extern u32 old_pad[2];

static struct padButtonStatus pad1; // just in case
static struct padButtonStatus pad2;
static int mouse_x[4];
static int mouse_dx;

extern int sound;

int init_mouse()
{
	int i;
	for (i = 0; i < 4; i++)
		mouse_x[i]=320;

	mouse_dx = 0;
	return (0);
}

int mouse_position(int pad)
{
	static int pad_delay[4] = { 0, 0, 0, 0};

	mouse_dx = 0;
	// TODO : write better paddle control code.
	if ((pad_delay[pad]++ % 7) == 0) {
		if (pad == 0) {
			if (pad1.ljoy_h < 192 && pad1.ljoy_h > 64) {
			} else {
				if(pad1.ljoy_h < 64) mouse_dx = -1;
				else if(pad1.ljoy_h > 192) mouse_dx = 1;
			}
		}
		if (pad == 2) {
			if (pad2.ljoy_h < 192 && pad2.ljoy_h > 64) {
			} else {
				if(pad2.ljoy_h < 64) mouse_dx = -1;
				else if(pad2.ljoy_h > 192) mouse_dx = 1;
			}
		}
	}

	mouse_x[pad] += mouse_dx;

	if (mouse_x[pad] < 0)
		mouse_x[pad] = 0;
	if (mouse_x[pad] > 640)
		mouse_x[pad] = 640;

	return (mouse_x[pad]);
}

int mouse_button(int pad)
{
	if (pad == 0) {
		if (input.pad[0] & INPUT_BUTTON1)
			return (1);
	}
	if (pad == 2) {
		if (input.pad[1] & INPUT_BUTTON1)
			return (1);
	}
	return(0);
}

void update_input()
{
	static int pad1_connected = 0, pad2_connected = 0;
	int pad1_data = 0;
	int pad2_data = 0;
	static u32 new_pad[2];

	if (toggle_delay)
		toggle_delay--;

	memset(&input, 0, sizeof(t_input));

	if(pad1_connected) {
		padRead(0, 0, &pad1); // port, slot, buttons
		pad1_data = 0xffff ^ pad1.btns;
		new_pad[0] = pad1_data & ~old_pad[0];
		old_pad[0] = pad1_data;

//		if(pad1_data & PAD_R1)
//			input.system |= INPUT_START;
//		if((pad1_data & PAD_L1) && !toggle_delay)
//			input.pad[0] |= INPUT_L1;
//		if((pad1_data & PAD_L2) && !toggle_delay)
//			input.pad[0] |= INPUT_L2;
		if(pad1_data & PAD_LEFT)
			input.pad[0] |= INPUT_LEFT;
		if(pad1_data & PAD_RIGHT)
			input.pad[0] |= INPUT_RIGHT;
		if(pad1_data & PAD_UP)
			input.pad[0] |= INPUT_UP;
		if(pad1_data & PAD_DOWN)
			input.pad[0] |= INPUT_DOWN;
		if(pad1_data & PAD_CIRCLE)
			input.pad[0] |= INPUT_BUTTON2;
		if(pad1_data & PAD_CROSS)
			input.pad[0] |= INPUT_BUTTON1;
		if(pad1_data & PAD_START)
			input.system |= INPUT_START; //INPUT_PAUSE;
		if(pad1_data & PAD_SELECT)
			input.system |= INPUT_SELECT;

		if((pad1.mode >> 4) == 0x07) {
			if(pad1.ljoy_h < 64) input.pad[0] |= INPUT_LEFT;
			else if(pad1.ljoy_h > 192) input.pad[0] |= INPUT_RIGHT;

			if(pad1.ljoy_v < 64) input.pad[0] |= INPUT_UP;
			else if(pad1.ljoy_v > 192) input.pad[0] |= INPUT_DOWN;
		}
	}

	if(pad2_connected) {
		padRead(1, 0, &pad2); // port, slot, buttons
		pad2_data = 0xffff ^ pad2.btns;
		new_pad[1] = pad1_data & ~old_pad[1];
		old_pad[1] = pad2_data;

//		if(pad2_data & PAD_R1)
//			input.system |= INPUT_START;
		if(pad2_data & PAD_LEFT)
			input.pad[1] |= INPUT_LEFT;
		if(pad2_data & PAD_RIGHT)
			input.pad[1] |= INPUT_RIGHT;
		if(pad2_data & PAD_UP)
			input.pad[1] |= INPUT_UP;
		if(pad2_data & PAD_DOWN)
			input.pad[1] |= INPUT_DOWN;
		if(pad2_data & PAD_CIRCLE)
			input.pad[1] |= INPUT_BUTTON2;
		if(pad2_data & PAD_CROSS)
			input.pad[1] |= INPUT_BUTTON1;
//		if(pad2_data & PAD_START)
//			input.system |= INPUT_PAUSE;

		if((pad2.mode >> 4) == 0x07) {
			if(pad2.ljoy_h < 64) input.pad[1] |= INPUT_LEFT;
			else if(pad2.ljoy_h > 192) input.pad[1] |= INPUT_RIGHT;

			if(pad2.ljoy_v < 64) input.pad[1] |= INPUT_UP;
			else if(pad2.ljoy_v > 192) input.pad[1] |= INPUT_DOWN;
		}
	}

	if((new_pad[0] & PAD_TRIANGLE)) {
		if(sound) audsrv_stop_audio();
		Ingame_Menu();
		//if(sound) SjPCM_Play();
	}

	//check controller status
	if((padGetState(0, 0)) == PAD_STATE_STABLE) {
		if(pad1_connected == 0) {
#ifdef DEVEL
			printf("Pad 1 inserted!\n");
#endif
			//WaitForNextVRstart(1);
		}
		pad1_connected = 1;
	} else pad1_connected = 0;

	if((padGetState(1, 0)) == PAD_STATE_STABLE) {
		if(pad2_connected == 0) {
#ifdef DEVEL
			printf("Pad 2 inserted!\n");
#endif
			//WaitForNextVRstart(1);
		}
		pad2_connected = 1;
	} else pad2_connected = 0;

}
#endif

void keyjoy(void) {
#ifdef PS2_EE
	u8 v1,v2;

	v1=v2=0x0f;
	if (input.pad[0] & INPUT_UP) v1&=0x0E;
	if (input.pad[0] & INPUT_DOWN) v1&=0x0D;
	if (input.pad[0] & INPUT_LEFT) v1&=0x0B;
	if (input.pad[0] & INPUT_RIGHT) v1&=0x07;
	if (input.pad[1] & INPUT_UP) v2&=0x0E;
	if (input.pad[1] & INPUT_DOWN) v2&=0x0D;
	if (input.pad[1] & INPUT_LEFT) v2&=0x0B;
	if (input.pad[1] & INPUT_RIGHT) v2&=0x07;
	if (app_data.swap)
		riotRead[0x280]=(v2 << 4) |v1;
	else
		riotRead[0x280]=(v1 << 4) | v2;
#endif
#ifdef NDEF
	BYTE val;
	BYTE v1,v2;

	v1=v2=0x0f;
	if (keys[kUARROW]) v1&=0x0E;
	if (keys[kDARROW]) v1&=0x0D;
	if (keys[kLARROW]) v1&=0x0B;
	if (keys[kRARROW]) v1&=0x07;
	if (keys[kW]) v2&=0x0E;
	if (keys[kZ]) v2&=0x0D;
	if (keys[kA]) v2&=0x0B;
	if (keys[kS]) v2&=0x07;
	if (app_data.swap)
		riotRead[0x280]=(v2 << 4) |v1;
	else
		riotRead[0x280]=(v1 << 4) | v2;
#endif
}

void toggle_p0_diff(void) {
	toggle_delay = TOGGLE_DELAY;
	if (riotRead[0x282] & 0x40) {
		riotRead[0x282] &= 0xBF; 	/* P0 amateur */
		showMessage("P0 Difficulty A", MESSAGE_INTERVAL * 60);
	} else {
		riotRead[0x282] |= 0x40;    /* P0 pro */
		showMessage("P0 Difficulty B", MESSAGE_INTERVAL * 60);
	}
}

void toggle_p1_diff(void) {
	toggle_delay = TOGGLE_DELAY;
	if (riotRead[0x282] & 0x80) {
		riotRead[0x282] &= 0x7f;    /* P1 amateur */
		showMessage("P1 Difficulty A", MESSAGE_INTERVAL * 60);
	} else {
		riotRead[0x282] |= 0x80;	/* P1 pro */
		showMessage("P1 Difficulty B", MESSAGE_INTERVAL * 60);
	}
}

void keycons(void) {
#ifdef PS2_EE
	riotRead[0x282] |= 0x03;
	if(input.system & INPUT_SELECT)
		riotRead[0x282] &= 0xFD;     /* Select */

	if(input.system & INPUT_START)
		riotRead[0x282] &= 0xFE;     /* Reset */

	if (input.pad[0] & INPUT_L1)
		toggle_p0_diff();

	if (input.pad[0] & INPUT_L2)
		toggle_p1_diff();

#endif
#ifdef WINDOWS
	SDL_Event event;

	SDL_PollEvent(&event);

	riotRead[0x282] |= 0x03;
	switch (event.type) {
	case SDL_KEYDOWN:
		if (strcmp(SDL_GetKeyName(event.key.keysym.sym), "f1") == 0) {
			riotRead[0x282] &= 0xFE;     /* Reset */
			/* app_data.debug=1; */
		}
		if (strcmp(SDL_GetKeyName(event.key.keysym.sym), "f2") == 0)
			riotRead[0x282] &= 0xFD;     /* Select */
		if (strcmp(SDL_GetKeyName(event.key.keysym.sym), "f3") == 0) {
			showMessage("P0 Difficulty A", MESSAGE_INTERVAL * 60);
			toggle_p0_diff();
		}
		break;
	case SDL_QUIT:
		exit(0);
	}
#endif

#ifdef NDEF
	BYTE val;

	riotRead[0x282] |= 0x03;
	if (keys[kF7]) riotRead[0x282] &= 0xF7;		/* BW */
	if (keys[kF8]) riotRead[0x282] |= 0x08;		/* Color */
	if (keys[kF1]) riotRead[0x282] &= 0xFE;		/* Reset */
	if (keys[kF2]) riotRead[0x282] &= 0xFD;     /* Select */
	if (keys[kF9]) riotRead[0x282] &= 0xBF; 	/* P0 amateur */
	if (keys[kF10]) riotRead[0x282] |= 0x40;    /* P0 pro */
	if (keys[kF11]) riotRead[0x282] &= 0x7f;    /* P1 amateur */
	if (keys[kF12]) riotRead[0x282] |= 0x80;	/* P1 pro */
#endif
}

void keytrig(void) {
#ifdef PS2_EE
	if (!(tiaWrite[VBLANK] & 0x40)) {
		if (input.pad[0] & INPUT_BUTTON1)
			tiaRead[INPT4]=0x00;
		else
			tiaRead[INPT4]=0x80;
		if (input.pad[1] & INPUT_BUTTON1)
			tiaRead[INPT5]=0x00;
		else
			tiaRead[INPT5]=0x80;
	} else {
		if (input.pad[0] & INPUT_BUTTON1) tiaRead[INPT4]=0x00;
		if (input.pad[1] & INPUT_BUTTON1) tiaRead[INPT5]=0x00;
	}

#endif
#ifdef NDEF
	int kr;
	int kl;

	if (app_data.swap) {
		kr=keys[kLEFTALT];
		kl=keys[kRIGHTALT];
	}
	else
	{
		kl=keys[kLEFTALT];
		kr=keys[kRIGHTALT];
	}

	if (!(tiaWrite[VBLANK] & 0x40)) {
		if (kr)
			tiaRead[INPT4]=0x00;
		else
			tiaRead[INPT4]=0x80;
		if (kl)
			tiaRead[INPT5]=0x00;
		else
			tiaRead[INPT5]=0x80;

	}
	else
	{
		if (kr)  tiaRead[INPT4]=0x00;
		if (kl) tiaRead[INPT5]=0x00;

	}
#endif
}

void keyboard_keypad(void) {
#ifdef NDEF
	int i;

	for(i=0; i<8; i++) keypad[i]=0xFF;
	if (keys[k1]) keypad[4]=0xFE;
	if (keys[k2]) keypad[4]=0xFD;
	if (keys[k3]) keypad[4]=0xFB;
	if (keys[kQ]) keypad[5]=0xFE;
	if (keys[kW]) keypad[5]=0xFD;
	if (keys[kE]) keypad[5]=0xFB;
	if (keys[kA]) keypad[6]=0xFE;
	if (keys[kS]) keypad[6]=0xFD;
	if (keys[kD]) keypad[6]=0xFB;
	if (keys[kZ]) keypad[7]=0xFE;
	if (keys[kX]) keypad[7]=0xFD;
	if (keys[kC]) keypad[7]=0xFB;
	if (app_data.right == KEYPAD) {
		if (keys[k4]) keypad[0]=0xFE;
		if (keys[k5]) keypad[0]=0xFD;
		if (keys[k6]) keypad[0]=0xFB;
		if (keys[kR]) keypad[1]=0xFE;
		if (keys[kT]) keypad[1]=0xFD;
		if (keys[kY]) keypad[1]=0xFB;
		if (keys[kF]) keypad[2]=0xFE;
		if (keys[kG]) keypad[2]=0xFD;
		if (keys[kH]) keypad[2]=0xFB;
		if (keys[kV]) keypad[3]=0xFE;
		if (keys[kB]) keypad[3]=0xFD;
		if (keys[kN]) keypad[3]=0xFB;
	}
#endif
}

void keyboard(void) {
#ifdef NDEF

	if (keys[kF5]) {
		Set_Old_Int9();
		setmode(TEXT_MODE);
		app_data.debug=1;
	}

	if (keys[kESC]) {
		Set_Old_Int9();
		tv_off();
		exit(0);
	}
#endif
}

