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
 * Holds the memory access routines to both memory and memory mapped
 * i/o, hence memory.c
 *
*/

#include <stdio.h>
#include <string.h>
#ifdef PS2_EE
#include <tamtypes.h>
#endif
#include "types.h"
#include "address.h"
#include "vmachine.h"
#include "misc.h"
#include "display.h"
#include "resource.h"
#include "mouse.h"
#include "keyboard.h"
#include "tiasound.h"

extern CLOCK clkcount;
extern CLOCK clk;
extern int beamadj;
extern int lastHMOVEclk;
extern int CosmicArkMotion;
extern int CosmicArkCounter;

void arBankSwitch (int a, char *str);
void decWrite( ADDRESS a, BYTE b);
BYTE decRead (ADDRESS a);
#ifdef STELLA_TIA
extern uInt8 stella_tia_peek(CLOCK c, uInt16 addr);
extern void stella_tia_poke(CLOCK c, uInt16 addr, uInt8 value);
#endif
int accesses = 0;

typedef struct {
  int b1, b2;
} ar_struct;

ar_struct ar[8] = { 
   { 2 * 2048, 3 * 2048 },
   { 0 * 2048, 3 * 2048 },
   { 2 * 2048, 0 * 2048 },
   { 0 * 2048, 2 * 2048 },
   { 2 * 2048, 3 * 2048 },
   { 1 * 2048, 3 * 2048 },
   { 2 * 2048, 1 * 2048 },
   { 1 * 2048, 2 * 2048 }};

int arImageOffset[2];

int arNumberOfLoadImages = 0;
int arDataHoldRegister = 0;
int arDistinctAccesses = 0;
int arWritePending = 0;
int arWriteEnabled = 0;
int arBank;
uInt8 myDataBusState;

BYTE arCart[65536];

// The 256 byte header for the current 8448 byte load
uInt8 arHeader[256];

int arChecksum(uInt8 *s, uInt16 length)
{
  uInt8 sum = 0;
  uInt32 i;

  for(i = 0; i < length; ++i)
  {
    sum += s[i];
  }

  return sum;
}

void arInitializeROM() {
  uInt32 i, j;

  static uInt8 dummyROMCode[] = {
    0xa5, 0xfa, 0x85, 0x80, 0x4c, 0x18, 0xf8, 0xff, 
    0xff, 0xff, 0x78, 0xd8, 0xa0, 0x0, 0xa2, 0x0, 
    0x94, 0x0, 0xe8, 0xd0, 0xfb, 0x4c, 0x50, 0xf8, 
    0xa2, 0x0, 0xbd, 0x6, 0xf0, 0xad, 0xf8, 0xff, 
    0xa2, 0x0, 0xad, 0x0, 0xf0, 0xea, 0xbd, 0x0, 
    0xf7, 0xca, 0xd0, 0xf6, 0x4c, 0x50, 0xf8, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xa2, 0x3, 0xbc, 0x1d, 0xf9, 0x94, 0xfa, 0xca, 
    0x10, 0xf8, 0xa0, 0x0, 0xa2, 0x28, 0x94, 0x4, 
    0xca, 0x10, 0xfb, 0xa2, 0x1c, 0x94, 0x81, 0xca, 
    0x10, 0xfb, 0xa9, 0x0, 0x85, 0x1b, 0x85, 0x1c, 
    0x85, 0x1d, 0x85, 0x1e, 0x85, 0x1f, 0x85, 0x19, 
    0x85, 0x1a, 0x85, 0x8, 0x85, 0x1, 0xa9, 0x10, 
    0x85, 0x21, 0x85, 0x2, 0xa2, 0x7, 0xca, 0xca, 
    0xd0, 0xfd, 0xa9, 0x0, 0x85, 0x20, 0x85, 0x10, 
    0x85, 0x11, 0x85, 0x2, 0x85, 0x2a, 0xa9, 0x5, 
    0x85, 0xa, 0xa9, 0xff, 0x85, 0xd, 0x85, 0xe, 
    0x85, 0xf, 0x85, 0x84, 0x85, 0x85, 0xa9, 0xf0, 
    0x85, 0x83, 0xa9, 0x74, 0x85, 0x9, 0xa9, 0xc, 
    0x85, 0x15, 0xa9, 0x1f, 0x85, 0x17, 0x85, 0x82, 
    0xa9, 0x7, 0x85, 0x19, 0xa2, 0x8, 0xa0, 0x0, 
    0x85, 0x2, 0x88, 0xd0, 0xfb, 0x85, 0x2, 0x85, 
    0x2, 0xa9, 0x2, 0x85, 0x2, 0x85, 0x0, 0x85, 
    0x2, 0x85, 0x2, 0x85, 0x2, 0xa9, 0x0, 0x85, 
    0x0, 0xca, 0x10, 0xe4, 0x6, 0x83, 0x66, 0x84, 
    0x26, 0x85, 0xa5, 0x83, 0x85, 0xd, 0xa5, 0x84, 
    0x85, 0xe, 0xa5, 0x85, 0x85, 0xf, 0xa6, 0x82, 
    0xca, 0x86, 0x82, 0x86, 0x17, 0xe0, 0xa, 0xd0, 
    0xc3, 0xa9, 0x2, 0x85, 0x1, 0xa2, 0x1c, 0xa0, 
    0x0, 0x84, 0x19, 0x84, 0x9, 0x94, 0x81, 0xca, 
    0x10, 0xfb, 0xa6, 0x80, 0xdd, 0x0, 0xf0, 0xa5, 
    0x80, 0x45, 0xfe, 0x45, 0xff, 0xa2, 0xff, 0xa0, 
    0x0, 0x9a, 0x4c, 0xfa, 0x0, 0xcd, 0xf8, 0xff, 
    0x4c
  };

  /* Copy all loads to the arCart ROM to be loaded later by the SC BIOS */
  memcpy(arCart, cart, 65536);

  uInt32 size = sizeof(dummyROMCode);

  // Initialize ROM with illegal 6502 opcode that causes a real 6502 to jam
  for(i = 0; i < 2048; ++i)
  {
    cart[3 * 2048 + i] = 0x02; 
  }

  // Copy the "dummy" Supercharger BIOS code into the ROM area
  for(j = 0; j < size; ++j)
  {
    cart[3 * 2048 + j] = dummyROMCode[j];
  }

  // Finally set 6502 vectors to point to initial load code at 0xF80A of BIOS
  cart[3 * 2048 + 2044] = 0x0A;
  cart[3 * 2048 + 2045] = 0xF8;
  cart[3 * 2048 + 2046] = 0x0A;
  cart[3 * 2048 + 2047] = 0xF8;
}

void arLoadIntoRAM (int load) {
  uInt8  sum, *src;
  uInt16 image;
  uInt32 bank, page, j;
  int invalidPageChecksumSeen = 0;

  // Scan through all of the loads to see if we find the one we're looking for
  for(image = 0; image < arNumberOfLoadImages; ++image)
  {
    // Is this the correct load?
    if(arCart[(image * 8448) + 8192 + 5] == load)
    {
      // Copy the load's header
      memcpy(arHeader, arCart + (image * 8448) + 8192, 256);

      // Verify the load's header 
      if(arChecksum(arHeader, 8) != 0x55)
      {
        printf("WARNING: The Supercharger header checksum is invalid...\n");
      }

      // Load all of the pages from the load
      invalidPageChecksumSeen = 0;
      for(j = 0; j < arHeader[3]; ++j)
      {
        bank = arHeader[16 + j] & 0x03;
        page = (arHeader[16 + j] >> 2) & 0x07;
        src = arCart + (image * 8448) + (j * 256);
        sum = arChecksum(src, 256) + arHeader[16 + j] + arHeader[64 + j];

        if(!invalidPageChecksumSeen && (sum != 0x55)) {
          printf("WARNING: Some Supercharger page checksums are invalid...\n");
          invalidPageChecksumSeen = 1;
        }

        // Copy page to Supercharger RAM (don't allow a copy into ROM area)
        if(bank < 3) {
          memcpy(cart + (bank * 2048) + (page * 256), src, 256);
        }
      }
      // Copy the bank switching byte and starting address into the 2600's
      // RAM for the "dummy" SC BIOS to access it
      decWrite(0xfe, arHeader[0]);
      decWrite(0xff, arHeader[1]);
      decWrite(0x80, arHeader[2]);
      
      return;
    }
  }
  //printf ("Ar load %d\n", load);
}

void arRAMWrite (ADDRESS addr) {

  if((addr & 0x0800) == 0) {
    theRom[(addr & 0x07FF)] = arDataHoldRegister;
    cart[(addr & 0x07FF) + arImageOffset[0]] = arDataHoldRegister;
  } else if(arImageOffset[1] != 3 * 2048) {    // Can't poke to ROM :-)
    theRom[(addr & 0x07FF) + 2048] = arDataHoldRegister;
    cart[(addr & 0x07FF) + arImageOffset[1]] = arDataHoldRegister;
  }
  //printf ("Ar RAM %x\n", addr);

  arWritePending = 0;
}
  

void arBankSwitch (int a, char *str) {
   arWriteEnabled = arDataHoldRegister & 0x02;
   arBank = (arDataHoldRegister >> 2) & 0x07;

   arImageOffset[0] = ar[arBank].b1;
   memcpy(&theRom[0],&cart[arImageOffset[0]],2048);
   arImageOffset[1] = ar[arBank].b2;
   memcpy(&theRom[2048],&cart[arImageOffset[1]],2048);
   arWritePending = 0;
   //printf ("Ar Bank %x\n", arBank);
}

uInt8 getDataBusState()
{
  return (myDataBusState);
}

/* UnDecoded Read */
BYTE undecRead (ADDRESS a) {
  BYTE res;

  accesses++;

  if(a & 0x1000) {
    // Special cases for Starpath ROMS
    if (app_data.bank == 5) {
      if (a == 0xf850) {
	return decRead(a);
      }    
      // Is the bank configuration hotspot being accessed?
      if (a == 0xfff8) {
	arBankSwitch(a, "undecRead");
      }
    }

    res=theRom[a & 0xfff];
  }
  else
    res=theRam[a & 0x7f];
  myDataBusState=res;
  return (res);
}

/* Decoded write */
void decWrite ( ADDRESS a, BYTE b) {

   ADDRESS a1, fa;
   
   myDataBusState = b;
   fa = a;
   accesses++;
   if (a & 0x1000) {
      a=a & 0xfff;
      if (app_data.bank == 3) {
	 if (a < 0x100) cartram[a]=b;
      }
      
      if (app_data.sc) {
	 if (a < 0x7f) cartram[a] = b;
      }
      switch (app_data.bank) {
      case 1:
	 if (a == 0xff6) memcpy(&theRom[0],&cart[0],4096);
	 if (a == 0xff7) memcpy(&theRom[0],&cart[4096],4096);
	 if (a == 0xff8) memcpy(&theRom[0],&cart[8192],4096);
	 if (a == 0xff9) memcpy(&theRom[0],&cart[12288],4096);
	 break;
      case 2:
	 if (a == 0xff8) memcpy(&theRom[0],&cart[0],4096);
	 if (a == 0xff9) memcpy(&theRom[0],&cart[4096],4096);
	 break;
      case 3:
	 if (a == 0xff8) memcpy(&theRom[0],&cart[0],4096);
	 if (a == 0xff9) memcpy(&theRom[0],&cart[4096],4096);
	 if (a == 0xffa) memcpy(&theRom[0],&cart[8192],4096);
	 break;
	 
      case 4:
	 if (a > 0xfdf && a < 0xfe8) {
	    a1=(a&0x07)<<10;
	    memcpy(&theRom[0],&cart[a1],0x400);
	 }
	 if (a > 0xfe7 && a < 0xff0) {
	    a1=(a&0x07)<<10;
	    memcpy(&theRom[0x400],&cart[a1],0x400);
	 }
	 if (a > 0xfef && a < 0xff8) {
	    a1=(a&0x07)<<10;
	    memcpy(&theRom[0x800],&cart[a1],0x400);
	 }
	 break;
      case 5:
	 if ((fa & 0xf000) == 0xf000) {
	    // Cancel any pending write if more than 5 distinct accesses 
	    // have occurred
	    // TODO: Modify to handle when the distinct counter wraps around...
	    if (arWritePending && (accesses > arDistinctAccesses + 5)) {
	       arWritePending = 0;
	    }
	   
	    // Is the data hold register being set?
	    if(!(a & 0xf00) && 
	       (!arWriteEnabled || !arWritePending)) {
	       arDataHoldRegister = a;
	       arDistinctAccesses = accesses;
	       arWritePending = 1;
	    // Is the bank configuration hotspot being accessed?
	    } else if (a == 0xff8) {
	       arBankSwitch(a, "write");
	    }
	    // Handle poke if writing enabled
	 } else if (arWriteEnabled && arWritePending &&
		    (accesses == (arDistinctAccesses + 5)) ) {
	    arRAMWrite(fa);
	 }
      }
      return;
   }
   
   /* RAM, mapped to page 0 and 1*/
   if ((a & 0x280) == 0x80) {
      theRam[a & 0x7f]=b;
      return;
   }
   
   if (!(a & 0x80)) {
     if (app_data.bank == 6 && !(a & 0x40)) {
	 memcpy(&theRom[0],&cart[b*2048],2048);
     }
#ifdef STELLA_TIA
     stella_tia_poke(clk+clkcount,a,b);
#endif
     switch(a & 0x3f){
	 /* TIA */
     case VBLANK:
        if (b & 0x80) {
	  /* Ground paddle ports */
	  tiaRead[INPT0]=0x00;
	  tiaRead[INPT1]=0x00;
	} else {
	  /* Processor now measures time for a logic 1 to appear
	     at each paddle port */
	  tiaRead[INPT0] = 0x80;
	  tiaRead[INPT1] = 0x80;
	  paddle[0].val = clk;
	  paddle[1].val = clk;
	}
	break;
      case AUDC0:
      case AUDC1:
      case AUDV0:
      case AUDV1:
      case AUDF0:
      case AUDF1:
	 Update_tia_sound(a & 0x3f, b);
	 break;
      }
   }
   else
      {
	if (a & 0x2ff) {
	  switch(a & 0x2ff) {
	    /* RIOT I/O ports */
	  case SWCHA:
	    riotWrite[SWCHA]=b;
	    break;
	  case SWACNT:
	    riotWrite[SWACNT]=b;
	    break;
	  case SWBCNT:
	    break;
	  }
	  
	  switch (a & 0x17) {
	    /* Timer ports */
	    /* case TIM1T: */
	  case 0x14:
	    set_timer( 0, b, clkcount);
	    break;
	    /* case TIM8T: */
	  case 0x15:
	    set_timer( 3, b, clkcount);
	    break;
	    /* case TIM64T: */
	  case 0x16:
	    set_timer( 6, b, clkcount);
	    break;
	    /*case T1024T: */
	  case 0x17:
	    set_timer( 10, b, clkcount);
	    break;
	  }
	}
      }
}


/* Decoded Read */
BYTE decReadMem (ADDRESS a)
{
  BYTE res;
  ADDRESS a1, fa;
  int x, load;

  fa = a;
  accesses++;
  if (a & 0x1000) {
    a=a & 0xfff;
    res=theRom[a & 0xfff];
    if (app_data.sc) {
      if (a > 0x7f && a < 0x100) {
	res=cartram[a & 0x7f];
	return res;
      }
    }
    if (app_data.bank == 3) {
      if (a > 0xFF && a < 0x200) {
	res=cartram[a & 0xFF];
	return res;
      }
    }
    if (app_data.bank == 0) return res;
    a=a & 0xfff;
    switch (app_data.bank) {
    case 1:
      if (a == 0xff6) memcpy(&theRom[0],&cart[0],4096);
      if (a == 0xff7) memcpy(&theRom[0],&cart[4096],4096);
      if (a == 0xff8) memcpy(&theRom[0],&cart[8192],4096);
      if (a == 0xff9) memcpy(&theRom[0],&cart[12288],4096);
      break;
    case 2:
      if (a == 0xff8) memcpy(&theRom[0],&cart[0],4096);
      if (a == 0xff9) memcpy(&theRom[0],&cart[4096],4096);
      break;
    case 3:
      if (a == 0xff8) memcpy(&theRom[0],&cart[0],4096);
      if (a == 0xff9) memcpy(&theRom[0],&cart[4096],4096);
      if (a == 0xffa) memcpy(&theRom[0],&cart[8192],4096);
      break;
      
    case 4:
      if (a > 0xfdf && a < 0xfe8) {
	a1=(a&0x07)<<10;
	memcpy(&theRom[0],&cart[a1],0x400);
      }
      if (a > 0xfe7 && a < 0xff0) {
	a1=(a&0x07)<<10;
	memcpy(&theRom[0x400],&cart[a1],0x400);
      }
      if (a > 0xfef && a < 0xff8) {
	a1=(a&0x07)<<10;
	memcpy(&theRom[0x800],&cart[a1],0x400);
      }
      break;
    case 5:
      if ((fa & 0xf000) == 0xf000) {
	// Is the "dummy" SC BIOS hotspot for 
	// reading a load being accessed?
	if((a == 0x850) && 
	   (arImageOffset[1] == (3 * 2048))) {
	  // Get load that's being accessed 
	  // (BIOS places load number at 0x80)
	  load = decRead(0x0080);
	  
	  // Read the specified load into RAM
	  arLoadIntoRAM(load);
	  res = cart[(a & 0x07FF) + arImageOffset[1]];
	  return res;
	}

	// Cancel any pending write if more than 5 distinct accesses 
	// have occurred
	// TODO: Modify to handle when the distinct counter wraps around...
	if (arWritePending && (accesses > arDistinctAccesses + 5)) {
	  arWritePending = 0;
	}

	// Is the data hold register being set?
	if(!(a & 0xf00) && 
	   (!arWriteEnabled || !arWritePending)) {
	  arDataHoldRegister = a;
	  arDistinctAccesses = accesses;
	  arWritePending = 1;
	// Is the bank configuration hotspot being accessed?
	} else if (a == 0xff8) {
	  arBankSwitch(a, "read");
	} else if (arWriteEnabled && arWritePending &&
		   (accesses == (arDistinctAccesses + 5)) ) {
	  arRAMWrite(fa);
	}
	return theRom[(a & 0x0fff)];
      }
    }
    return res;
  }

  if ((a & 0x280) == 0x80) {
     res = theRam[a & 0x7f];
     return res;
  }
  
  if (!(a & 0x80)) {
#ifdef STELLA_TIA
    res=stella_tia_peek(clk+clkcount,a);
#endif
     switch(a & 0x0f){
     case INPT0:
	if (app_data.left == PADDLE) {
 	   x=640-mouse_position(0);
	   x=x*20;
	   tiaRead[INPT0]=0x00;
	   if (paddle[0].val > x) {
	     tiaRead[INPT0]=0x80;
	   }
	}
	if (app_data.left == KEYPAD) {
	   keyboard_keypad();
	   tiaRead[INPT0]=0x80;
	   if (!(riotWrite[SWCHA] & 0x10)) tiaRead[INPT0]=(x26_keypad[4] & 0x01) << 7;
	   if (!(riotWrite[SWCHA] & 0x20)) tiaRead[INPT0]=(x26_keypad[5] & 0x01) << 7;
	   if (!(riotWrite[SWCHA] & 0x40)) tiaRead[INPT0]=(x26_keypad[6] & 0x01) << 7;
	   if (!(riotWrite[SWCHA] & 0x80)) tiaRead[INPT0]=(x26_keypad[7] & 0x01) << 7;
	}
	res=tiaRead[INPT0];
	break;
     case INPT1:
	if (app_data.left == KEYPAD) {
	   keyboard_keypad();
	   tiaRead[INPT1]=0x80;
	   if (!(riotWrite[SWCHA] & 0x10)) tiaRead[INPT1]=(x26_keypad[4] & 0x02) << 6;
	   if (!(riotWrite[SWCHA] & 0x20)) tiaRead[INPT1]=(x26_keypad[5] & 0x02) << 6;
	   if (!(riotWrite[SWCHA] & 0x40)) tiaRead[INPT1]=(x26_keypad[6] & 0x02) << 6;
	   if (!(riotWrite[SWCHA] & 0x80)) tiaRead[INPT1]=(x26_keypad[7] & 0x02) << 6;
	}
	res=tiaRead[INPT1];
	break;
     case INPT2:
	if (app_data.right == PADDLE) {
 	   x=640-mouse_position(2);
	   x=x*20;
	   tiaRead[INPT2]=0x00;
	   if (paddle[2].val > x) {
	     tiaRead[INPT2]=0x80;
	   }
	}
	if (app_data.right == KEYPAD) {
	   keyboard_keypad();
	   tiaRead[INPT2]=0x80;
	   if (!(riotWrite[SWCHA] & 0x01)) tiaRead[INPT2]=((x26_keypad[0] & 0x01) << 7);
	   if (!(riotWrite[SWCHA] & 0x02)) tiaRead[INPT2]=((x26_keypad[1] & 0x01) << 7);
	   if (!(riotWrite[SWCHA] & 0x04)) tiaRead[INPT2]=((x26_keypad[2] & 0x01) << 7);
	   if (!(riotWrite[SWCHA] & 0x08)) tiaRead[INPT2]=((x26_keypad[3] & 0x01) << 7);
	}
	
	res=tiaRead[INPT2];
	break;
     case INPT3:
	if (app_data.right == KEYPAD) {
	   keyboard_keypad();
	   tiaRead[INPT3]=0x80;
	   if (!(riotWrite[SWCHA] & 0x01)) tiaRead[INPT3]=((x26_keypad[0] & 0x02) << 6);
	   if (!(riotWrite[SWCHA] & 0x02)) tiaRead[INPT3]=((x26_keypad[1] & 0x02) << 6);
	   if (!(riotWrite[SWCHA] & 0x04)) tiaRead[INPT3]=((x26_keypad[2] & 0x02) << 6);
	   if (!(riotWrite[SWCHA] & 0x08)) tiaRead[INPT3]=((x26_keypad[3] & 0x02) << 6);
	   
	}
	
	res=tiaRead[INPT3];
	break;
     case INPT4:
	if (app_data.left == KEYPAD) {
	   keyboard_keypad();
	   tiaRead[INPT4]=0x80;
	   if (!(riotWrite[SWCHA] & 0x10)) tiaRead[INPT4]=(x26_keypad[4] & 0x04) << 5;
	   if (!(riotWrite[SWCHA] & 0x20)) tiaRead[INPT4]=(x26_keypad[5] & 0x04) << 5;
	   if (!(riotWrite[SWCHA] & 0x40)) tiaRead[INPT4]=(x26_keypad[6] & 0x04) << 5;
	   if (!(riotWrite[SWCHA] & 0x80)) tiaRead[INPT4]=(x26_keypad[7] & 0x04) << 5;
	}
	else
	   {
	      keytrig();
	   }
	res=tiaRead[INPT4];
	break;
     case INPT5:
	if (app_data.right == KEYPAD) {
	   keyboard_keypad();
	   tiaRead[INPT5]=0x80;
	   if (!(riotWrite[SWCHA] & 0x01)) tiaRead[INPT5]=((x26_keypad[0] & 0x04) << 5);
	   if (!(riotWrite[SWCHA] & 0x02)) tiaRead[INPT5]=((x26_keypad[1] & 0x04) << 5);
	   if (!(riotWrite[SWCHA] & 0x04)) tiaRead[INPT5]=((x26_keypad[2] & 0x04) << 5);
	   if (!(riotWrite[SWCHA] & 0x08)) tiaRead[INPT5]=((x26_keypad[3] & 0x04) << 5);
	   
	}
	else
	   {
	      keytrig();
	   }
	
	res=tiaRead[INPT5];
	break;
     case 0x0f:
	res=0x0f;
	break;
     }
  }
  else
     {
	switch(a & 0x2ff) {
	   /* Timer output */
	case INTIM:
	case 0x285:
	case 0x29D:
	   res=do_timer(a, clkcount);
	   break;
	case SWCHA:
	   if (app_data.left == PADDLE) {
	      x=mouse_button(0);
	      if (x)
		 riotRead[SWCHA] &= 0x7F;
	      else
		 riotRead[SWCHA] |= 0x80;
	   } else
	     keyjoy();
	   res=riotRead[SWCHA];
	   break;
	   
	   /* Switch B is hardwired to input */
	case SWCHB:
	   if (app_data.right == PADDLE) {
	      x=mouse_button(2);
	      if (x)
		 riotRead[SWCHB] &= 0x7F;
	      else
		 riotRead[SWCHB] |= 0x80;
	   }
	   keycons();
	   res=riotRead[SWCHB];
	   break;
	case TIM1T:
	case TIM8T:
	case TIM64T:
	case T1024T:
	   res=do_timer(a, clkcount);
	   break;
	default:
	   res=65;
	   break;
	}
     }
  return res;
}

BYTE decRead (ADDRESS a)
{
   BYTE res;
   res=decReadMem(a);
   myDataBusState=res;
   return(res);
}

/* Debug Read */
BYTE dbgRead (ADDRESS a){
	BYTE res;

	/* RAM, mapped to page 0 and 1*/

	if ((a>0x7f && a<0x100) || (a>0x17f && a<0x200)) {
		res=theRam[a & 0x7f];
		return res;
	}
	/* ROM Cartridge */
	if (a>0xFFF) {
		res=theRom[a & 0x0fff];
		return res;
	}

	switch(a){
		/* TIA */
		/* Timer output */
		case INTIM:
			res=riotRead[INTIM];
			break;
		case SWCHA:
			res=riotRead[SWCHA];
			break;
		/* Switch B is hardwired to input */
		case SWCHB:
			res=riotRead[SWCHB];
			break;

	default:
	res=0;
	break;
	}
	return res;
}





