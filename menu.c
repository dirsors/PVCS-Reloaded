/*

	menu.c - Quick & Dirty rom selection screen. The code in this file is
	         far from optimised or efficient, but it does the job and thats
			 good enough for me.

	                  (c) Nick Van Veen (aka Sjeep), 2002

	-------------------------------------------------------------------------

    This file is part of the PSMS.

    PSMS is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    PSMS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#include <tamtypes.h>
#include <string.h>
#include <kernel.h>
#include <sifrpc.h>
#include <fileio.h>
#include <malloc.h>
#include <libmc.h>
#include <libpad.h>
#include <stdio.h>
#include "cdvd_rpc.h"

#include "g2.h"
#include "menu.h"

#define Z_LOGO		1
#define Z_STARS		2
#define Z_BOX1		3
#define Z_BOX2		4
#define Z_LIST		5
#define Z_SELECT	6
#define Z_SCROLLBG	7
#define Z_SCROLL	8
#define Z_SCROLL_M	9

#define WIDTH     380
#define HEIGHT    230

#define BOOT_CD 0
#define BOOT_MC 1
#define BOOT_HO 2
#define BOOT_HD 3

#define EXIT_MENU  "<<RETURN>>"
#define BROWSE_MC  "BROWSE MC:"
#define BROWSE_CD  "BROWSE CD:"
#define BROWSE_HD  "BROWSE HD:"

struct ROMdata romdata[2000];
int num_roms = 0;

int selection = 0;
int frame_position;
int frame_selection = 0;

// Note: Space = 4pix wide
char scroll_text[] = "                                                    PVCS v1.3 - VCS emulator coded by Daniel Boris, ported from X2600 by Alex Hornbey                 Thanks to: 7not6, evilo, <G>, and Red_N_Dead for the PVCS logo, - Sjeep for releasing src to PSMS, - Pukko for padlib and many fixes to the sif code in psx2lib - Gustavo Scotti for psx2lib - Oobles for his IOP tutorial and example code - The people behind naplink, which was used to develop PVCS.    Updates available at http://pvcs.ps2-scene.org ";

static t_starfield starfield[SF_SIZE];

#define ARRAY_ENTRIES              128
#define TOC_ENTRIES                2000
#define TEXT_SPACER                10
#define MAX_FRAME                  14
#define MAX_DISPLAY_ROM_NAME       45

static int DEVFLAG=0;

const char cdpath[120]="/";
const char mcpath[120]="/*";

int numMC=0;
static  mcTable mcDir[ARRAY_ENTRIES] __attribute__((aligned(64)));
static int mc_Type, mc_Free, mc_Format;

struct TocEntry myTocEntries[TOC_ENTRIES] __attribute__((aligned(64)));

static uint16 maxx, maxy;

extern int boot_mode;
extern void clear_screen_ps2();

// good fonts
extern uint32 small_font[];
extern uint32 courier_new[];
extern uint32 tahoma[];
extern uint16 fixed_tc[];
extern uint16 fixed_tc8[];

#include "font5200.c"
#include "pvcs_logo.c"

void setCourier()
{
  // Courier-New Fixed
  g2_set_font(courier_new, 256, 128, fixed_tc);
  g2_set_font_spacing(-6);
  g2_set_font_mag(1);
}

void setSmallFont()
{
  // Small Courier Fixed
  g2_set_font(small_font, 128, 64, fixed_tc8);
  g2_set_font_spacing(-2);
  g2_set_font_mag(1);
}

void drawChar(char c, int x, int y, int r, int g, int b)
{
  unsigned int i, j;
  unsigned char cc;
  unsigned char *pc;

  // set character pointer
  pc = &font5200[(c-32)*8];
  
  // set screen pointer
  for(i=0; i<8; i++) {
    cc = *pc++;
    for(j=0; j<8; j++) {
      if(cc & 0x80) g2_set_color(r,g,b);
      else g2_set_color(0,0,0);
      g2_put_pixel(x+j, y+i);
      cc = cc << 1;
    }
  }
}

////////////////////////////////////////////////////////////////////////
// draw a string of characters (8x8)
void printXY(char *s, int x, int y, int r, int g, int b)
{
  while(*s) {
    drawChar(*s++, x, y, r, g, b);
    x += 8;
    if(x>=(WIDTH+10))
	return;
  }
}

char* menu_main()
{
  int i;
  int y;
  static int scroll_x = 8, scroll_pos = 0,scroll_delay = 0;

  maxx = g2_get_max_x();
  maxy = g2_get_max_y();

  g2_set_visible_frame(1);
  g2_set_active_frame(0);
  g2_set_visible_frame(0);

  clear_screen_ps2();
  //menu_init_stars();

  setSmallFont();

  while(1) {
    g2_set_fill_color(0,0,0);
    g2_fill_rect(0, 0, WIDTH, HEIGHT);

    if(menu_update_input()) { // X was pressed
      g2_flip_buffers();
      g2_set_fill_color(0,0,0);
      g2_fill_rect(0, 0, WIDTH, HEIGHT);
      g2_set_color(255,255,255);
      g2_rect(40, 96, 216, 144);
      g2_rect(39, 95, 216, 144);
      g2_out_text(45, 112, "Loading ...");
      break;
    }

#ifdef NDEF    
    // Draw Scroll Box
    //gp_gouradrect(&thegp, 32<<4, 60<<4, GS_SET_RGBA(140, 0, 255, 128), (192+32)<<4, (142+60)<<4, GS_SET_RGBA(13, 88, 174, 128), Z_BOX1);
#endif

    // Draw PVCS Logo
    g2_put_image((WIDTH/2)-(pvcs_logo_w/2), 5, 
		 pvcs_logo_w, pvcs_logo_h, 
		 pvcs_logo_w, pvcs_logo_h, (const uint32 *) &pvcs_logo);
 
    // Draw starfield
    //menu_draw_stars();

    g2_set_fill_color(0, 0, 255);
    g2_fill_rect(31, 56, 320, 200);
    g2_set_color(255, 255, 0);
    g2_rect(31, 56, 320, 200);

    // Draw text in scroll box
    y = 60;
    for(i=frame_position;i<(frame_position+14);i++) {
      if (i<num_roms)
	g2_out_text(32,y,(char *)&romdata[i].name);
      //printXY((char *)&romdata[i].name, 32, y, 200, 200, 200);
      y += TEXT_SPACER;
    }
    /* color selected rom */
    g2_set_color(255, 255, 0);
    g2_rect(32, (59+(frame_selection*TEXT_SPACER)), 
            320, (67+(frame_selection*TEXT_SPACER)));
    //g2_out_text(32,60+(frame_selection*TEXT_SPACER),&romdata[frame_selection].name);

    // Draw scroller
    printXY(&scroll_text[scroll_pos],scroll_x, 210, 200, 200, 200);
    if(--scroll_delay < 0) {
      if(--scroll_x < 0) {
	scroll_x = (int)8;
	if(++scroll_pos > sizeof(scroll_text)) scroll_pos = 0;
      }
      scroll_delay = 0;
    }
    g2_set_fill_color(0,0,0);
    g2_fill_rect(0,0,9,200);
    
    g2_flip_buffers();    
  }

  return (char *)&romdata[selection].filename;
}

int menu_update_input()
{
  static struct padButtonStatus pad1; // just in case
  static int pad1_connected = 0;
  static int padcountdown = 0;
  static int pad_held_down = 0;
  
  unsigned short pad1_data = 0;

  if(pad1_connected) {
    padRead(0, 0, &pad1); // port, slot, buttons
    pad1_data = 0xffff ^ pad1.btns;
    
    if((pad1.mode >> 4) == 0x07) {
      if(pad1.ljoy_v < 64) pad1_data |= PAD_UP;
      else if(pad1.ljoy_v > 192) pad1_data |= PAD_DOWN;
      
      if(pad1.ljoy_h < 64) pad1_data |= PAD_LEFT;
      else if(pad1.ljoy_h > 192) pad1_data |= PAD_RIGHT;
    }
  }
  
  if(pad1_data & PAD_CROSS) return 1;
  
  if(padcountdown) padcountdown--;
  
  if((pad1_data & PAD_DOWN) && (padcountdown==0) && 
     (selection!=(num_roms-1))) {
    selection++;
    
    if(frame_selection<MAX_FRAME-1) frame_selection++;
    
    //if the pad has been held down for a certain amount of time, give padcountdown
    //a lower value, in effect making the scrolling of the text faster
    if(pad_held_down++<4) 
      padcountdown=10;
    else 
      padcountdown=2;
		
    //move the display frame if necessary
    if(selection>(frame_position+(MAX_FRAME-1))) 
      frame_position++;
    
    return 0;
		
  }
  else if((pad1_data & PAD_UP) && (padcountdown==0) && (selection>0)) {
    selection--;

    if(frame_selection>0)frame_selection--;
    
    if(pad_held_down++<4) 
      padcountdown=10;
    else 
      padcountdown = 2;
    
    if(selection<frame_position) 
      frame_position--;
    
    return 0;
  }
  else if ((pad1_data & PAD_LEFT) && (padcountdown==0) && 
	   (frame_position>MAX_FRAME)) {
    frame_position -= MAX_FRAME;
    selection -= MAX_FRAME;
    
    if(pad_held_down++<4) 
      padcountdown=10;
    else 
      padcountdown = 2;
    
    return 0;
  }
  else if((pad1_data & PAD_RIGHT) && (padcountdown==0) && 
	  (frame_position<(num_roms-2*MAX_FRAME))) {
    frame_position += MAX_FRAME;
    selection += MAX_FRAME;
    
    if(pad_held_down++<4) 
      padcountdown=10;
    else 
      padcountdown = 2;
    
    return 0;
  }
  
  //if up or down are NOT being pressed, reset the pad_held_down flag
  if(!(pad1_data & (PAD_UP | PAD_DOWN))) pad_held_down = 0;
  
  //check controller status
  if((padGetState(0, 0)) == PAD_STATE_STABLE) {
    pad1_connected = 1;
  } else pad1_connected = 0;
  return 0;
}

int initmc()
{
  
  int ret;
  
  if(mcInit(MC_TYPE_MC) < 0) {
    printf("Failed to initialise memcard server!\n");
    SleepThread();
  }
  
  // Since this is the first call, -1 should be returned.
  mcGetInfo(0, 0, &mc_Type, &mc_Free, &mc_Format); 
  mcSync(0, NULL, &ret);
  printf("mcGetInfo returned %d\n",ret);
  printf("Type: %d Free: %d Format: %d\n\n", mc_Type, mc_Free, mc_Format);
  
  // Assuming that the same memory card is connected, this should return 0
  mcGetInfo(numMC,0,&mc_Type,&mc_Free,&mc_Format);
  mcSync(0, NULL, &ret);
  printf("mcGetInfo returned %d\n",ret);
  printf("Type: %d Free: %d Format: %d\n\n", mc_Type, mc_Free, mc_Format);
  
  return (int)(mc_Free*1000);
}

char *GetRom()
{
   static int hostmode=1;
   static int rom_found=0;
   char *dir, *rom;

   /* Look for ROMS on host: first */
   if (hostmode)
      if (ProcessROMlist() > 0) {
	 /* If we don't find any, don't look again */
	 hostmode=0;
      }

 loop:
   while (!hostmode && !rom_found) {
      /* Now try the memcard or cdrom */
      while ((dir=ChooseDir(NULL)) == NULL);
      
      if (ProcessROMscan(dir) == 0)
	 rom_found=1;
   }
   rom=menu_main();
   if (strcmp(EXIT_MENU, rom) == 0) {
      rom_found=0;
      goto loop;
   }

   return(rom);
}

char *ChooseDir(char *path)
{   
  static char dir[120] __attribute__((aligned(64)));
  int i,ret2;
  
  selection = 0;
  frame_selection = 0;
  frame_position = 0;

  if(DEVFLAG==1){

    num_roms=0;
    
    strcpy(romdata[num_roms].name,BROWSE_MC);
    strcpy(romdata[num_roms].filename,BROWSE_MC);
   
    num_roms++;	
    
    while(CDVD_DiskReady(CdBlock)==CdNotReady);
	ret2 = CDVD_GetDir(cdpath, NULL, CDVD_GET_DIRS_ONLY, myTocEntries, 
			   ARRAY_ENTRIES, NULL);
    
	printf("Retrieved %d directory entries\n\n",ret2);

	for (i = 0;i<ret2;i++)
	{
    			
	  //printf("Dir name: %s\tLBA = %d\tSize = %d\n",
	  //		myTocEntries[i].filename,
	  //		myTocEntries[i].fileLBA,
	  //		myTocEntries[i].fileSize); 
                sprintf(romdata[num_roms].filename,"/%s",myTocEntries[i].filename);
                strcpy(romdata[num_roms].name,myTocEntries[i].filename);
                num_roms++;	 
	}
	
  }
  if (DEVFLAG==0) {
     mcGetDir(numMC, 0, mcpath, 0, ARRAY_ENTRIES - 10, mcDir);
     mcSync(0, NULL, &ret2);
     printf("\nmcGetDir returned %d\n", ret2);
     
     num_roms=0;
    
     strcpy(romdata[num_roms].name,BROWSE_CD);
     strcpy(romdata[num_roms].filename,BROWSE_CD);
    
     num_roms++;	
    
     for(i=0; i < ret2; i++) {
        if (mcDir[i].attrFile & MC_ATTR_SUBDIR){
	   sprintf(romdata[num_roms].filename,"/%s",mcDir[i].name);
	   strcpy(romdata[num_roms].name,mcDir[i].name);
	   num_roms++;
	}
     }
  }

  strcpy(dir,menu_main());

  if(strstr(dir,BROWSE_MC) != NULL){
    display_error(" BROWSE MC0: ", 0);
    DEVFLAG=0;
    return (NULL);
  }
  else if(strstr(dir,BROWSE_CD) != NULL){
    display_error(" BROWSE CDROM: ", 0);
    DEVFLAG=1;
    return (NULL);
  }
  return(dir);
}

void ScanMCDirForFiles(int numMC, char *tname, char *ext)
{
  char mcPath[128] __attribute__((aligned(64)));

  int i, ret;
  
  strcpy(mcPath,tname);
  strcat(mcPath,"/*");
  strcat(mcPath,ext);

  printf("Listing of %s directory on memory card:\n\n",mcPath);
  mcGetDir(numMC, 0, mcPath, 0, ARRAY_ENTRIES - 10, mcDir);
  mcSync(0, NULL, &ret);
  printf("\nmcGetDir returned %d\n", ret);
	
  for(i=0; i < ret; i++) {  
    if (! (mcDir[i].attrFile & MC_ATTR_SUBDIR)) {
      sprintf(romdata[num_roms].filename,"mc0:%s/%s",tname,mcDir[i].name);
      strncpy(romdata[num_roms].name,mcDir[i].name,MAX_DISPLAY_ROM_NAME);
      //printf("%s - %d bytes %s\n", mcDir[i].name, mcDir[i].fileSizeByte,
      //     romdata[num_roms].filename);
      num_roms++;	     	 		     
    }
  }
}

void ScanCDDirForFiles(char *tname, char *ext)
{
  int i, ret;

  while(CDVD_DiskReady(CdBlock)==CdNotReady);
  ret = CDVD_GetDir(tname,ext, CDVD_GET_FILES_ONLY, 
		    myTocEntries, 2000, tname);

  printf("Retrieved %d %s entries\n\n",ret,ext);

  for (i = 0;i<ret;i++) {
    //printf("Dir name: %s\tLBA = %d\tSize = %d\n",
    //   myTocEntries[i].filename,
    //   myTocEntries[i].fileLBA,
    //   myTocEntries[i].fileSize); 
    
    sprintf(romdata[num_roms].filename,"cdfs:%s/%s",tname,
	    myTocEntries[i].filename);
    strncpy(romdata[num_roms].name,
	    myTocEntries[i].filename,MAX_DISPLAY_ROM_NAME);
    num_roms++;
  }
  printf("%d roms found:\n\n",num_roms);
}

int ProcessROMscan(char *path)
{ 
  static char tname[120];
  int status=0;
  
  selection = 0;
  frame_selection = 0;
  frame_position = 0;
  num_roms=0;

  strcpy(romdata[num_roms].name,EXIT_MENU);
  strcpy(romdata[num_roms].filename,EXIT_MENU);
  num_roms++;

  if (DEVFLAG==0) {
    strcpy(tname,path);
    printf("scanning %s directory on memory card:\n\n",tname);
    ScanMCDirForFiles(numMC, tname, ".BIN");
    ScanMCDirForFiles(numMC, tname, ".bin");
    ScanMCDirForFiles(numMC, tname, ".A26");
  }
  if (DEVFLAG==1) {
    strcpy(tname,path);
    ScanCDDirForFiles(tname, ".BIN");
    ScanCDDirForFiles(tname, ".A26");
  }

  if (num_roms == 1) {
     display_error(" No ROMS found! ", 0);
     status=1;
  }

  return(status);
}

int ProcessROMlist()
{
	int fd, fd_size;
	char* buffer;
	int i,j,k,l;
	int num_lines = 0;
	char path[256];
	char line[256];

	strcpy (path,"host:");
	strcat (path,"FILES.TXT");

	fd = fioOpen(path, O_RDONLY);

	if (fd < 0) {
	  printf("[%s] not found!\n", path);
	  return(1);
	}

	fd_size = fioLseek(fd,0,SEEK_END);
	fioLseek(fd,0,SEEK_SET);

	buffer = memalign(64, fd_size);
	if(buffer == NULL) display_error("Failed to allocate memory!", 1);

	if(fioRead(fd, buffer, fd_size) != fd_size) {
	  display_error("Error reading FILES.TXT!", 1);
	}
#ifdef DEVEL
	printf("File loaded! %d bytes.\n",fd_size);
#endif

	fioClose(fd);

	for(i=0;i<fd_size;i++) {
		if(buffer[i] == '\n') num_lines++;
	}

	memset(line,0,256);

	num_roms = 0;
	i=j=k=l=0;

	for(j=0;j<num_lines;j++) {

		while(buffer[i] != '\n') {
			line[k] = buffer[i];
			i++;
			k++;
		}
		k=0;
		i++;

		if(strstr(line,",") == NULL) continue;

		while(line[k] != ',') {
			romdata[num_roms].name[l] = line[k];
			k++;
			l++;
		}
		strcpy(romdata[num_roms].filename,"host:");
		l=strlen(romdata[num_roms].filename);
		k++;
		while(line[k] != '\0') {
		  if (line[k] != '\r')
		    romdata[num_roms].filename[l] = line[k];
			k++;
			l++;
		}
		l=0;
		k=0;
		memset(line,0,256);
		num_roms++;
	}
	return(0);
}

void menu_draw_stars()
{
  int x,y,i;
  
  for(i=0;i<SF_SIZE;i++)
    {
      starfield[i].z -= SPEED;
      if(starfield[i].z < 2) menu_create_star(i);

      x = ((starfield[i].x<<4) / starfield[i].z);
      y = ((starfield[i].y<<4) / starfield[i].z);
      
      if ((x<0) || (y<0) || (x>(WIDTH-1)) || (y>(HEIGHT-1))) {
	menu_create_star(i);
	x = (WIDTH/2) - (starfield[i].x / starfield[i].z);
	y = (HEIGHT/2) - (starfield[i].y / starfield[i].z);
      }
      g2_set_color(255,255,255);
      g2_put_pixel(x, y);
    }
}

void menu_create_star(int i)
{
	// Set a star to new random values
	starfield[i].x=rand()%WIDTH;
	starfield[i].y=rand()%HEIGHT;
	starfield[i].z=Z_MAX;
}

void menu_init_stars()
{
	int i;
	static int inited = 0;

	if(inited) return;

	for(i=0;i<SF_SIZE;i++) {
		starfield[i].x=rand()%WIDTH;
		starfield[i].y=rand()%HEIGHT;
		starfield[i].z=Z_MAX;
	}

	inited = 1;
}

void display_error(char *errmsg, int fatal)
{
  struct padButtonStatus pad1;
  unsigned short pad1_data = 0;

  setCourier();

  g2_set_visible_frame(1);
  g2_set_active_frame(0);
  g2_set_visible_frame(0);

  clear_screen_ps2();

  while(1) {
    g2_set_color(13,88,174);
    g2_rect(14, 72, 360, 152);
    g2_set_color(255,255,255);
    g2_rect(13, 71, 360, 152);
    printXY(errmsg,20,80,255,255,255);
    if(!fatal) g2_out_text(40,180,"Press START to continue");
    g2_flip_buffers();
    
    if(!fatal) {
      if(padGetState(0, 0) == PAD_STATE_STABLE) {
	padRead(0, 0, &pad1); // port, slot, buttons
	pad1_data = 0xffff ^ pad1.btns;
      }
      
      if(pad1_data & PAD_START) break;
    }
  }
}
