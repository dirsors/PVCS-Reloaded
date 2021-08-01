/*****************************************************************************

   This file is part of Virtual VCS, the Atari 2600 Emulator
   ===================================================

   Copyright 1996 Daniel Boris. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.

   See the file COPYING for Details.

******************************************************************************/

/*
   Used to load cartridge images into memory.
   This module has been completely re-written since X2600
*/

#include <stdio.h>
#include <string.h>
#ifdef PS2_EE
#include <fileio.h>
#endif

#include "types.h"
#include "main.h"
#include "vmachine.h"
#include "config.h"
#include "resource.h"
#include "md5.h"

#define BOOT_CD 0
#define BOOT_MC 1
#define BOOT_HO 2
#define BOOT_HD 3

typedef struct {
	char md5[33];
	char type[5];
	char name[100];
	char controller_l[30];
	char controller_r[30];
} cartliststruct;

extern int boot_mode;
extern int arNumberOfLoadImages;
extern int arDataHoldRegister;
extern void arBankSwitch();
extern void arInitializeROM();
extern void arLoadIntoRAM();

#include "cartlist.h"

void show_config()
{
	printf("Left Controller: ");
	switch(app_data.left) {
		case STICK:
			printf("Stick\n");
			break;
		case PADDLE:
			printf("Paddle\n");
			break;
		case KEYPAD:
			printf("Keypad\n");
			break;
	}
	printf("Right Controller: ");
	switch(app_data.right) {
		case STICK:
			printf("Stick\n");
			break;
		case PADDLE:
			printf("Paddle\n");
			break;
		case KEYPAD:
			printf("Keypad\n");
			break;
	}
	printf("Frame Rate: %d\n",app_data.rr);
	printf("Bank Switch: ");
	switch (app_data.bank) {
		case 0: printf("None\n"); break;
		case 1: printf("F6\n"); break;
		case 2: printf("F8\n"); break;
		case 3: printf("FA\n"); break;
		case 4: printf("E0\n"); break;
		case 7: printf("F6SC\n"); break;
	}
}

#ifdef PS2_EE
long filesize(int fd)
{
  long length;

  length = fioLseek(fd, 0, SEEK_END);
  if (!length) {
     printf("Failed in fioLseek : %d\n", fd);
     fioClose(fd);
     return (0);
  }
  fioLseek(fd, 0, SEEK_SET);
  return length;
}
#else
long filesize(FILE *stream)
{
  long curpos, length;
  
  curpos = ftell(stream);
  fseek(stream, 0L, SEEK_END);
  length = ftell(stream);
  fseek(stream, curpos, SEEK_SET);
  return length;
}

void autoconfig(unsigned long sum)
{
	FILE *fn;
	int done;
	char line[100];
	char attr[50],val[50];
	char *p;
	char ch[10];
	int i;

	sprintf(ch,"%lx",sum);
	strupr(ch);
	fn=fopen("vcscfg.dat","r");
	done=0;
	while (!done) {
		fgets(line,100,fn);
		if (feof(fn)) {
			printf("  Configuration not found\n");
			return;
		}
		p=strtok(line,"=");
		strcpy(attr,p);
		p=strtok(NULL,"=");
		strcpy(val,p);
		val[strlen(val)-1]=0;

		if (!strcmpi(attr,"CHECKSUM")) {
			if (!strcmpi(val,ch)) {
				done=1;
			}
		}
	}
	printf("  Configuration Found...\n\n");
	done=0;
	while(!done) {
		fgets(line,100,fn);
		if (feof(fn)) return;
		p=strtok(line,"=");
		strcpy(attr,p);
		p=strtok(NULL,"=");
		strcpy(val,p);
		val[strlen(val)-1]=0;
		if (!strcmpi(attr,"CHECKSUM")) return;
		if (!strcmpi(attr,"NAME")) printf("Name: %s\n",val);
		if (!strcmpi(attr,"MAKER")) printf("Made by: %s\n",val);
		if (!strcmpi(attr,"EMULATION")) printf("Emulation: %s\n",val);
		if (!strcmpi(attr,"SENSITIVITY")) {
			sscanf(val,"%d",&i);
			app_data.pad_sens=i;
		}
		if (!strcmpi(attr,"FRAMERATE")) {
			sscanf(val,"%d",&i);
			if (i == 0) i=1;
			app_data.rr=i;
		}
		if (!strcmpi(attr,"TYPE")) {
			if (!strcmpi(val,"2K")) app_data.bank=0;
			if (!strcmpi(val,"4K")) app_data.bank=0;
			if (!strcmpi(val,"F6")) app_data.bank=1;
			if (!strcmpi(val,"F8")) app_data.bank=2;
			if (!strcmpi(val,"E0")) app_data.bank=4;
			if (!strcmpi(val,"FA")) app_data.bank=3;
			if (!strcmpi(val,"AR")) app_data.bank=5;
			if (!strcmpi(val,"F6SC")) {
				app_data.bank=1;
				app_data.sc=1;
			}
		}
		if (!strcmpi(attr,"RIGHT")) {
			if (!strcmpi(val,"STICK")) {
				app_data.right=STICK;
			}
			if (!strcmpi(val,"KEYPAD")) {
				app_data.right=KEYPAD;
			}
			if (!strcmpi(val,"PADDLE")) {
				app_data.right=PADDLE;
			}
		}

		if (!strcmpi(attr,"LEFT")) {
			if (!strcmpi(val,"STICK")) {
				app_data.left=STICK;
			}
			if (!strcmpi(val,"KEYPAD")) {
				app_data.left=KEYPAD;
			}
			if (!strcmpi(val,"PADDLE")) {
				app_data.left=PADDLE;
			}
		}

	}

}
#endif

int findmd5(uInt32 len)
{
  char md5val[33];
  int i;

  /* Calc MD5 for the cart */
  MD5(cart, len, md5val);
  md5val[32] = 0;
  
  printf("MD5 Val: %s\n",md5val);
  /* Look up for the MD5 in the cart database */
  i=0;
  while ((i<CARTCOUNT) && strcmp(md5val, cartlist[i].md5)) i++;
  return (i);
}

/* Loads a cart image. Returns -1 on error, 0 otherwise */
int load_rom(char *name) {
	long len;
	int i;
	unsigned long checksum;

#ifdef WINDOWS
	FILE *fp;

	printf("Load Cartridge: %s\n",name);

	fp=fopen(name, "rb");
	if (!fp) {
		printf("Error loading cartridge!\n");
		return(1);
	}
	len=filesize(fp);

	fread(&cart[0],1,len,fp);
	fclose(fp);
#endif

#ifdef PS2_EE
	char line[256];
	FILE *fd;

	fd = fopen(name, "rb");
	if(fd == NULL) {
		printf("Error opening file.\n");
		return 1;
	}
	
	fseek(fd,0,SEEK_END);
	len = ftell(fd);
	fseek(fd,0,SEEK_SET);

	memset(cart, 0, sizeof(cart));
	
	if(fread(cart, len, 1, fd) != 1) {
		printf("Error reading ROM.\n");
		return 1;
	}

	fclose(fd);
#endif

	// Assume it is joystick controllers
	app_data.left=STICK;
	app_data.right=STICK;
	app_data.bank=0;
	app_data.sc=0;

	printf("len = %ld, cart = %x%x%x\n", len, cart[0],cart[1],cart[2]);
	i=findmd5(len);

	if (i == CARTCOUNT)	{
		/* Not found */
	} else {
#ifdef PS2_EE
	   if (!strcmp(cartlist[i].type,"E7") ||
	       !strcmp(cartlist[i].type,"FE") ||
	       !strcmp(cartlist[i].type,"MB") ||
	       !strcmp(cartlist[i].type,"MC") ||
	       !strcmp(cartlist[i].type,"DPC")) {
	     snprintf(line, sizeof(line), "Unsupported banking scheme [%s]!\n",
		      cartlist[i].type);
	     printf(line);
	     return(1);
	   }
#endif

		/* Found */
		if (!strcmp(cartlist[i].type,"2K")) app_data.bank=0;
		if (!strcmp(cartlist[i].type,"4K")) app_data.bank=0;
		if (!strcmp(cartlist[i].type,"F6")) app_data.bank=1;
		if (!strcmp(cartlist[i].type,"F8")) app_data.bank=2;
		if (!strcmp(cartlist[i].type,"E0")) app_data.bank=4;
		if (!strcmp(cartlist[i].type,"FA")) app_data.bank=3;
		if (!strcmp(cartlist[i].type,"AR")) app_data.bank=5;
		if (!strcmp(cartlist[i].type,"3F")) app_data.bank=6;
		if (!strcmp(cartlist[i].type,"F6SC")) {
			app_data.bank=1;
			app_data.sc=1;
		}
		if (!strcmp(cartlist[i].type,"F8SC")) {
			app_data.bank=2;
			app_data.sc=1;
		}
		if (!strcmp(cartlist[i].type,"FASC")) {
			app_data.bank=3;
			app_data.sc=1;
		}
		printf("MD5 Name: %s\n",cartlist[i].name);
		printf("MD5 Type: %s\n", cartlist[i].type);
	}

        if (strcmp(cartlist[i].controller_l, "Paddles") == 0)
		app_data.left=PADDLE;
        if (strcmp(cartlist[i].controller_r, "Paddles") == 0)
		app_data.right=PADDLE;

	checksum=0;
	for(i=0; i<len; i++) checksum+=cart[i];
	printf("Checksum: %lx\n",checksum);
#ifdef WINDOWS
	if (app_data.autoconfig) {
		printf("\nAttempting to autoconfigure... \n");
		autoconfig(checksum);
	}
#endif
	/* Set to use 'standard' banking if not set. */
	if (app_data.bank == 0) {
	  if (len == 8192) {
	    app_data.bank=2;
	  }
	  if (len == 16384) {
	    app_data.bank=1;
	  }
	}

	show_config();
	switch (app_data.bank) {
	case 0:
	case 1:
	case 2:
		switch (len) {
			case 2048:
				memcpy(&theRom[0],&cart[0],2048);
				memcpy(&theRom[2048],&cart[0],2048);
				break;
			case 4096:
				memcpy(&theRom[0],&cart[0],4096);
				break;
			case 8192:
				memcpy(&theRom[0],&cart[4096],4096);
				break;
			case 16384:
				memcpy(&theRom[0],&cart[12288],4096);
				break;
		default:
		  printf("ROM length = %ld\n", len);
		}
		break;
	case 3:
		memcpy(&theRom[0],&cart[8192],4096);
		break;
	case 4:
		memcpy(&theRom[0],&cart[0],3072);
		memcpy(&theRom[0xC00],&cart[0x1C00],1024);
		break;
        case 5:
	        arNumberOfLoadImages = len / 8448;
	        arInitializeROM();
		arDataHoldRegister = 0;
		arLoadIntoRAM(0);
		arBankSwitch(0, "init");
	        break;
	case 6:
     	        memcpy(&theRom[0],&cart[0],2048);
     	        memcpy(&theRom[2048],&cart[len-2048],2048);
	        break;
	}

	return 0;
}

