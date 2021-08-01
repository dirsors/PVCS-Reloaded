//============================================================================
//
//   SSSS    tt          lll  lll       
//  SS  SS   tt           ll   ll        
//  SS     tttttt  eeee   ll   ll   aaaa 
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2004 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: TIA.cxx,v 1.34 2004/06/13 16:51:15 stephena Exp $
//============================================================================

#include "types.h"
#include "tia.h"
#include "tia_tables.h"
#include "resource.h"
#include "vmachine.h"

extern void tv_display(void);
extern uInt8 getDataBusState();
extern CLOCK clk;
extern int accesses;
extern int freeze_cpu;
int frame_cnt=0;

#define HBLANK 68

/**
   Answers the current frame buffer
   
   @return Pointer to the current frame buffer
*/
uInt8 *currentFrameBuffer() { return myCurrentFrameBuffer; }

/**
   Answers the previous frame buffer
   
   @return Pointer to the previous frame buffer
*/
uInt8 *previousFrameBuffer() { return myPreviousFrameBuffer; }


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void stella_init_tia()
{
  uInt16 x, enabled;
  uInt32 i;

  myColorLossEnabled = false;
  myMaximumNumberOfScanlines = 262;
  myCOLUBK = myColor[0];
  myCOLUPF = myColor[1];
  myCOLUP0 = myColor[2];
  myCOLUP1 = myColor[3];

  // Allocate buffers for two frame buffers
  myCurrentFrameBuffer = (uInt8 *) malloc(sizeof(uInt8) * 160 * 300);
  myPreviousFrameBuffer = (uInt8 *) malloc(sizeof(uInt8) * 160 * 300);

  for(x = 0; x < 2; ++x)
  {
    for(enabled = 0; enabled < 256; ++enabled)
    {
      if(enabled & PriorityBit)
      {
        uInt8 color = 0;

        if((enabled & (myP1Bit | myM1Bit)) != 0)
          color = 3;
        if((enabled & (myP0Bit | myM0Bit)) != 0)
          color = 2;
        if((enabled & myBLBit) != 0)
          color = 1;
        if((enabled & myPFBit) != 0)
          color = 1;  // NOTE: Playfield has priority so ScoreBit isn't used

        myPriorityEncoder[x][enabled] = color;
      }
      else
      {
        uInt8 color = 0;

        if((enabled & myBLBit) != 0)
          color = 1;
        if((enabled & myPFBit) != 0)
          color = (enabled & ScoreBit) ? ((x == 0) ? 2 : 3) : 1;
        if((enabled & (myP1Bit | myM1Bit)) != 0)
          color = 3;
        if((enabled & (myP0Bit | myM0Bit)) != 0)
          color = 2;

        myPriorityEncoder[x][enabled] = color;
      }
    }
  }

  for(i = 0; i < 640; ++i)
  {
    ourDisabledMaskTable[i] = 0;
  }

  // Compute all of the mask tables
  computeBallMaskTable();
  computeCollisionTable();
  computeMissleMaskTable();
  computePlayerMaskTable();
  computePlayerPositionResetWhenTable();
  computePlayerReflectTable();
  computePlayfieldMaskTable();
}

void clear_tia()
{
  free(myCurrentFrameBuffer);
  free(myPreviousFrameBuffer);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void stella_reset_tia()
{
  uInt32 i;

  // Clear frame buffers
  for(i = 0; i < 160 * 300; ++i)
  {
    myCurrentFrameBuffer[i] = myPreviousFrameBuffer[i] = 0;
  }

  // Reset pixel pointer and drawing flag
  myFramePointer = myCurrentFrameBuffer;

  // Calculate color clock offsets for starting and stoping frame drawing
  myStartDisplayOffset = 228 * tv_ystart;
  myStopDisplayOffset = myStartDisplayOffset + 228 * tv_height;

  // Reasonable values to start and stop the current frame drawing
  myClockWhenFrameStarted = stella_cycles * 3;
  myClockStartDisplay = myClockWhenFrameStarted + myStartDisplayOffset;
  myClockStopDisplay = myClockWhenFrameStarted + myStopDisplayOffset;
  myClockAtLastUpdate = myClockWhenFrameStarted;
  myClocksToEndOfScanLine = 228;
  myVSYNCFinishClock = 0x7FFFFFFF;
  myScanlineCountForLastFrame = 0;

  // Currently no objects are enabled
  myEnabledObjects = 0;

  // Some default values for the registers
  myVSYNC = 0;
  myVBLANK = 0;
  myNUSIZ0 = 0;
  myNUSIZ1 = 0;
  myCOLUP0 = 0;
  myCOLUP1 = 0;
  myCOLUPF = 0;
  myPlayfieldPriorityAndScore = 0;
  myCOLUBK = 0;
  myCTRLPF = 0;
  myREFP0 = false;
  myREFP1 = false;
  myPF = 0;
  myGRP0 = 0;
  myGRP1 = 0;
  myDGRP0 = 0;
  myDGRP1 = 0;
  myENAM0 = false;
  myENAM1 = false;
  myENABL = false;
  myDENABL = false;
  myHMP0 = 0;
  myHMP1 = 0;
  myHMM0 = 0;
  myHMM1 = 0;
  myHMBL = 0;
  myVDELP0 = false;
  myVDELP1 = false;
  myVDELBL = false;
  myRESMP0 = false;
  myRESMP1 = false;
  myCollision = 0;
  myPOSP0 = 0;
  myPOSP1 = 0;
  myPOSM0 = 0;
  myPOSM1 = 0;
  myPOSBL = 0;


  // Some default values for the "current" variables
  myCurrentGRP0 = 0;
  myCurrentGRP1 = 0;
  myCurrentBLMask = ourBallMaskTable[0][0];
  myCurrentM0Mask = ourMissleMaskTable[0][0][0];
  myCurrentM1Mask = ourMissleMaskTable[0][0][0];
  myCurrentP0Mask = ourPlayerMaskTable[0][0][0];
  myCurrentP1Mask = ourPlayerMaskTable[0][0][0];
  myCurrentPFMask = ourPlayfieldTable[0];

  myLastHMOVEClock = 0;
  myHMOVEBlankEnabled = false;
  myM0CosmicArkMotionEnabled = false;
  myM0CosmicArkCounter = 0;

  myDumpEnabled = false;
  myDumpDisabledCycle = 0;

  myAllowHMOVEBlanks = true;

  myFrameXStart = tv_xstart;
  myFrameWidth = tv_width;
  myFrameYStart = tv_ystart;
  myFrameHeight = tv_height;

  // Make sure the starting x and width values are reasonable
  if((myFrameXStart + myFrameWidth) > 160)
  {
    // Values are illegal so reset to default values
    myFrameXStart = 0;
    myFrameWidth = 160;
  }

  if(strcmp(tv_format, "PAL") == 0)
  {
    myColorLossEnabled = true;
    myMaximumNumberOfScanlines = 342;
  }
  else
  {
    myColorLossEnabled = false;
    myMaximumNumberOfScanlines = 290;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void systemCyclesReset()
{
  uInt32 clocks;

  // Get the current system cycle
  uInt32 cycles = stella_cycles;

  // Adjust the sound cycle indicator
  //mySound.adjustCycleCounter(-cycles);

  // Adjust the dump cycle
  myDumpDisabledCycle -= cycles;

  // Get the current color clock the system is using
  clocks = cycles * 3;

  resetTimerCycles(cycles);

  // Adjust the clocks by this amount since we're reseting the clock to zero
  myClockWhenFrameStarted -= clocks;
  myClockStartDisplay -= clocks;
  myClockStopDisplay -= clocks;
  myClockAtLastUpdate -= clocks;
  myVSYNCFinishClock -= clocks;
  myLastHMOVEClock -= clocks;
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void stella_update()
{
  uInt8 *tmp = myCurrentFrameBuffer;
  uInt32 clocks, totalClocks;
  
  // TODO: have code here that handles errors....
  // Draw any pending messages to the framebuffer
  if(myMessageTime > 0)
  {
    drawMessageText();
    --myMessageTime;
  }

  /* Display screen */
  tv_display();
  
  frame_cnt++;
  myCurrentFrameBuffer = myPreviousFrameBuffer;
  myPreviousFrameBuffer = tmp;

  // Compute the number of scanlines in the frame
  totalClocks = (stella_cycles * 3) - myClockWhenFrameStarted;
  myScanlineCountForLastFrame = totalClocks / 228;

  if (myScanlineCountForLastFrame != 262) {
    //printf ("scan count = %d\n", myScanlineCountForLastFrame);
  }

  // Remember the number of clocks which have passed on the current scanline
  // so that we can adjust the frame's starting clock by this amount.  This
  // is necessary since some games position objects during VSYNC and the
  // TIA's internal counters are not reset by VSYNC.
  clocks = ((stella_cycles * 3) - myClockWhenFrameStarted) % 228;

  // Ask the system to reset the cycle count so it doesn't overflow
  systemCyclesReset();
  stella_cycles = 0;
  clk = 0;
  accesses = 0;

  // Setup clocks that'll be used for drawing this frame
  myClockWhenFrameStarted = -clocks;
  myClockStartDisplay = myClockWhenFrameStarted + myStartDisplayOffset;
  myClockStopDisplay = myClockWhenFrameStarted + myStopDisplayOffset;
  myClockAtLastUpdate = myClockStartDisplay;
  myClocksToEndOfScanLine = 228;

  // Reset frame buffer pointer
  myFramePointer = myCurrentFrameBuffer;

  // If color loss is enabled then update the color registers based on
  // the number of scanlines in the last frame that was generated
  if(myColorLossEnabled)
  {
    if(myScanlineCountForLastFrame & 0x01)
    {
      myCOLUP0 |= 0x01010101;
      myCOLUP1 |= 0x01010101;
      myCOLUPF |= 0x01010101;
      myCOLUBK |= 0x01010101;
    }
    else
    {
      myCOLUP0 &= 0xfefefefe;
      myCOLUP1 &= 0xfefefefe;
      myCOLUPF &= 0xfefefefe;
      myCOLUBK &= 0xfefefefe;
    }
  }   

  // Execute instructions until frame is finished
  //mySystem->m6502().execute(25000);
  freeze_cpu = 0;

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void drawMessageText()
{
  // Set up the correct coordinates to draw the surrounding box
  uInt32 xBoxOffSet = 2 + myFrameXStart;
  uInt32 yBoxOffSet = myFrameHeight - 28;
  uInt32 boxToTextXOffSet = 2;
  uInt32 boxToTextYOffSet = 4;

  // Set up the correct coordinates to print the message
  uInt32 xTextOffSet = xBoxOffSet + boxToTextXOffSet;
  uInt32 yTextOffSet = yBoxOffSet + boxToTextYOffSet;

  // Used to indicate the current x/y position of a pixel
  uInt32 xPos, yPos;

  // The actual font data for a letter
  uInt32 data;

  // The index into the palette to color the current text and background
  uInt8 fontColor, backColor;

  // Palette index depends on whether we are in NTSC or PAL mode
  if(strcmp(tv_format, "PAL") == 0)
  {
    fontColor = 10;
    backColor = 0;
  }
  else
  {
    fontColor = 10;
    backColor = 0;
  }

  // Clip the length if its wider than the screen
  uInt8 length = strlen(myMessageText);
  if(((length * 5) + xTextOffSet) >= myFrameWidth)
    length = (myFrameWidth - xTextOffSet) / 5;

  // Reset the offsets to center the message
  uInt32 boxWidth  = (5 * length) + boxToTextXOffSet;
  uInt32 boxHeight = 8 + (2 * (yTextOffSet - yBoxOffSet));
  xBoxOffSet = (myFrameWidth >> 1) - (boxWidth >> 1);
  xTextOffSet = xBoxOffSet + boxToTextXOffSet;

  // First, draw the surrounding box
  uInt32 xx,y;
  for(xx = 0; xx < boxWidth; ++xx)
  {
    for(y = 0; y < boxHeight; ++y)
    {
      uInt32 position = ((yBoxOffSet + y) * myFrameWidth) + xx + xBoxOffSet;

      if((xx == 0) || (xx == boxWidth - 1) || (y == 0) || (y == boxHeight - 1))
        myCurrentFrameBuffer[position] = fontColor;
      else
        myCurrentFrameBuffer[position] = backColor;
    }
  }

  // Then, draw the text
  uInt8 x;
  for(x = 0; x < length; ++x)
  {
    char letter = myMessageText[x];

    if((letter >= 'A') && (letter <= 'Z'))
      data = ourFontData[(int)letter - 65];
    else if((letter >= '0') && (letter <= '9'))
      data = ourFontData[(int)letter - 48 + 26];
    else   // unknown character or space
    {
      xTextOffSet += 3;
      continue;
    }

    // start scanning the font data from the bottom up
    yPos = 7;

    uInt8 y;
    for(y = 0; y < 32; ++y)
    {
      // determine the correct scanline
      xPos = y % 4;
      if(xPos == 0)
        --yPos;

      if((data >> y) & 1)
      {
        uInt32 position = (yPos + yTextOffSet) * myFrameWidth + 
	                  (4 - xPos) + xTextOffSet;
        myCurrentFrameBuffer[position] = fontColor;
      }
    }

    // move left to the next character
    xTextOffSet += 5;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void showMessage(char *message, Int32 duration)
{
  uInt32 i;

  myMessageText = message;
  myMessageTime = duration;

  // Make message uppercase, since there are no lowercase fonts defined
  for(i = 0; i < strlen(myMessageText); ++i)
    myMessageText[i] = toupper(myMessageText[i]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt32 *palette()
{
  // See which palette we should be using
  char *type   = "standard";

  if(strcmp(type,"standard") == 0)
    return (strcmp(tv_format,"PAL") == 0) ? ourPALPalette : ourNTSCPalette;
  else if(strcmp(type,"original") == 0)
    return (strcmp(tv_format,"PAL") == 0) ? ourPALPalette11 : ourNTSCPalette11;
  else if(strcmp(type,"z26") == 0)
    return (strcmp(tv_format,"PAL") == 0) ? ourPALPaletteZ26 : ourNTSCPaletteZ26;
  else  // return normal palette by default
    return (strcmp(tv_format,"PAL") == 0) ? ourPALPalette : ourNTSCPalette;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 width()
{
  return myFrameWidth; 
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 height()
{
  return myFrameHeight; 
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 scanlines()
{
  return (uInt32)myScanlineCountForLastFrame;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void computeBallMaskTable()
{
  Int32 size, x, align;

  // First, calculate masks for alignment 0
  for(size = 0; size < 4; ++size)
  {

    // Set all of the masks to false to start with
    for(x = 0; x < 160; ++x)
    {
      ourBallMaskTable[0][size][x] = false;
    }

    // Set the necessary fields true
    for(x = 0; x < 160 + 8; ++x)
    {
      if((x >= 0) && (x < (1 << size)))
      {
        ourBallMaskTable[0][size][x % 160] = true;
      }
    }

    // Copy fields into the wrap-around area of the mask
    for(x = 0; x < 160; ++x)
    {
      ourBallMaskTable[0][size][x + 160] = ourBallMaskTable[0][size][x];
    }
  }

  // Now, copy data for alignments of 1, 2 and 3
  for(align = 1; align < 4; ++align)
  {
    for(size = 0; size < 4; ++size)
    {
      for(x = 0; x < 320; ++x)
      {
        ourBallMaskTable[align][size][x] = 
            ourBallMaskTable[0][size][(x + 320 - align) % 320];
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void computeCollisionTable()
{
  uInt8 i;

  for(i = 0; i < 64; ++i)
  { 
    ourCollisionTable[i] = 0;

    if((i & myM0Bit) && (i & myP1Bit))    // M0-P1
      ourCollisionTable[i] |= 0x0001;

    if((i & myM0Bit) && (i & myP0Bit))    // M0-P0
      ourCollisionTable[i] |= 0x0002;

    if((i & myM1Bit) && (i & myP0Bit))    // M1-P0
      ourCollisionTable[i] |= 0x0004;

    if((i & myM1Bit) && (i & myP1Bit))    // M1-P1
      ourCollisionTable[i] |= 0x0008;

    if((i & myP0Bit) && (i & myPFBit))    // P0-PF
      ourCollisionTable[i] |= 0x0010;

    if((i & myP0Bit) && (i & myBLBit))    // P0-BL
      ourCollisionTable[i] |= 0x0020;

    if((i & myP1Bit) && (i & myPFBit))    // P1-PF
      ourCollisionTable[i] |= 0x0040;

    if((i & myP1Bit) && (i & myBLBit))    // P1-BL
      ourCollisionTable[i] |= 0x0080;

    if((i & myM0Bit) && (i & myPFBit))    // M0-PF
      ourCollisionTable[i] |= 0x0100;

    if((i & myM0Bit) && (i & myBLBit))    // M0-BL
      ourCollisionTable[i] |= 0x0200;

    if((i & myM1Bit) && (i & myPFBit))    // M1-PF
      ourCollisionTable[i] |= 0x0400;

    if((i & myM1Bit) && (i & myBLBit))    // M1-BL
      ourCollisionTable[i] |= 0x0800;

    if((i & myBLBit) && (i & myPFBit))    // BL-PF
      ourCollisionTable[i] |= 0x1000;

    if((i & myP0Bit) && (i & myP1Bit))    // P0-P1
      ourCollisionTable[i] |= 0x2000;

    if((i & myM0Bit) && (i & myM1Bit))    // M0-M1
      ourCollisionTable[i] |= 0x4000;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void computeMissleMaskTable()
{
  // First, calculate masks for alignment 0
  Int32 x, size, number, align;

  // Clear the missle table to start with
  for(number = 0; number < 8; ++number)
    for(size = 0; size < 4; ++size)
      for(x = 0; x < 160; ++x)
        ourMissleMaskTable[0][number][size][x] = false;

  for(number = 0; number < 8; ++number)
  {
    for(size = 0; size < 4; ++size)
    {
      for(x = 0; x < 160 + 72; ++x)
      {
        // Only one copy of the missle
        if((number == 0x00) || (number == 0x05) || (number == 0x07))
        {
          if((x >= 0) && (x < (1 << size)))
            ourMissleMaskTable[0][number][size][x % 160] = true;
        }
        // Two copies - close
        else if(number == 0x01)
        {
          if((x >= 0) && (x < (1 << size)))
            ourMissleMaskTable[0][number][size][x % 160] = true;
          else if(((x - 16) >= 0) && ((x - 16) < (1 << size)))
            ourMissleMaskTable[0][number][size][x % 160] = true;
        }
        // Two copies - medium
        else if(number == 0x02)
        {
          if((x >= 0) && (x < (1 << size)))
            ourMissleMaskTable[0][number][size][x % 160] = true;
          else if(((x - 32) >= 0) && ((x - 32) < (1 << size)))
            ourMissleMaskTable[0][number][size][x % 160] = true;
        }
        // Three copies - close
        else if(number == 0x03)
        {
          if((x >= 0) && (x < (1 << size)))
            ourMissleMaskTable[0][number][size][x % 160] = true;
          else if(((x - 16) >= 0) && ((x - 16) < (1 << size)))
            ourMissleMaskTable[0][number][size][x % 160] = true;
          else if(((x - 32) >= 0) && ((x - 32) < (1 << size)))
            ourMissleMaskTable[0][number][size][x % 160] = true;
        }
        // Two copies - wide
        else if(number == 0x04)
        {
          if((x >= 0) && (x < (1 << size)))
            ourMissleMaskTable[0][number][size][x % 160] = true;
          else if(((x - 64) >= 0) && ((x - 64) < (1 << size)))
            ourMissleMaskTable[0][number][size][x % 160] = true;
        }
        // Three copies - medium
        else if(number == 0x06)
        {
          if((x >= 0) && (x < (1 << size)))
            ourMissleMaskTable[0][number][size][x % 160] = true;
          else if(((x - 32) >= 0) && ((x - 32) < (1 << size)))
            ourMissleMaskTable[0][number][size][x % 160] = true;
          else if(((x - 64) >= 0) && ((x - 64) < (1 << size)))
            ourMissleMaskTable[0][number][size][x % 160] = true;
        }
      }

      // Copy data into wrap-around area
      for(x = 0; x < 160; ++x)
        ourMissleMaskTable[0][number][size][x + 160] = 
          ourMissleMaskTable[0][number][size][x];
    }
  }

  // Now, copy data for alignments of 1, 2 and 3
  for(align = 1; align < 4; ++align)
  {
    for(number = 0; number < 8; ++number)
    {
      for(size = 0; size < 4; ++size)
      {
        for(x = 0; x < 320; ++x)
        {
          ourMissleMaskTable[align][number][size][x] = 
            ourMissleMaskTable[0][number][size][(x + 320 - align) % 320];
        }
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void computePlayerMaskTable()
{
  // First, calculate masks for alignment 0
  Int32 x, enable, mode, align;

  // Set the player mask table to all zeros
  for(enable = 0; enable < 2; ++enable)
    for(mode = 0; mode < 8; ++mode)
      for(x = 0; x < 160; ++x)
        ourPlayerMaskTable[0][enable][mode][x] = 0x00;

  // Now, compute the player mask table
  for(enable = 0; enable < 2; ++enable)
  {
    for(mode = 0; mode < 8; ++mode)
    {
      for(x = 0; x < 160 + 72; ++x)
      {
        if(mode == 0x00)
        {
          if((enable == 0) && (x >= 0) && (x < 8))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> x;
        }
        else if(mode == 0x01)
        {
          if((enable == 0) && (x >= 0) && (x < 8))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> x;
          else if(((x - 16) >= 0) && ((x - 16) < 8))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> (x - 16);
        }
        else if(mode == 0x02)
        {
          if((enable == 0) && (x >= 0) && (x < 8))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> x;
          else if(((x - 32) >= 0) && ((x - 32) < 8))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> (x - 32);
        }
        else if(mode == 0x03)
        {
          if((enable == 0) && (x >= 0) && (x < 8))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> x;
          else if(((x - 16) >= 0) && ((x - 16) < 8))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> (x - 16);
          else if(((x - 32) >= 0) && ((x - 32) < 8))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> (x - 32);
        }
        else if(mode == 0x04)
        {
          if((enable == 0) && (x >= 0) && (x < 8))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> x;
          else if(((x - 64) >= 0) && ((x - 64) < 8))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> (x - 64);
        }
        else if(mode == 0x05)
        {
          // For some reason in double size mode the player's output
          // is delayed by one pixel thus we use > instead of >=
          if((enable == 0) && (x > 0) && (x <= 16))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> ((x - 1)/2);
        }
        else if(mode == 0x06)
        {
          if((enable == 0) && (x >= 0) && (x < 8))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> x;
          else if(((x - 32) >= 0) && ((x - 32) < 8))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> (x - 32);
          else if(((x - 64) >= 0) && ((x - 64) < 8))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> (x - 64);
        }
        else if(mode == 0x07)
        {
          // For some reason in quad size mode the player's output
          // is delayed by one pixel thus we use > instead of >=
          if((enable == 0) && (x > 0) && (x <= 32))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> ((x - 1)/4);
        }
      }
  
      // Copy data into wrap-around area
      for(x = 0; x < 160; ++x)
      {
        ourPlayerMaskTable[0][enable][mode][x + 160] = 
            ourPlayerMaskTable[0][enable][mode][x];
      }
    }
  }

  // Now, copy data for alignments of 1, 2 and 3
  for(align = 1; align < 4; ++align)
  {
    for(enable = 0; enable < 2; ++enable)
    {
      for(mode = 0; mode < 8; ++mode)
      {
        for(x = 0; x < 320; ++x)
        {
          ourPlayerMaskTable[align][enable][mode][x] =
              ourPlayerMaskTable[0][enable][mode][(x + 320 - align) % 320];
        }
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void computePlayerPositionResetWhenTable()
{
  uInt32 mode, oldx, newx, s1, s2;

  // Loop through all player modes, all old player positions, and all new
  // player positions and determine where the new position is located:
  // 1 means the new position is within the display of an old copy of the
  // player, -1 means the new position is within the delay portion of an
  // old copy of the player, and 0 means it's neither of these two
  for(mode = 0; mode < 8; ++mode)
  {
    for(oldx = 0; oldx < 160; ++oldx)
    {
      // Set everything to 0 for non-delay/non-display section
      for(newx = 0; newx < 160; ++newx)
      {
        ourPlayerPositionResetWhenTable[mode][oldx][newx] = 0;
      }

      // Now, we'll set the entries for non-delay/non-display section
      for(newx = 0; newx < 160 + 72 + 5; ++newx)
      {
        if(mode == 0x00)
        {
          if((newx >= oldx) && (newx < (oldx + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;

          if((newx >= oldx + 4) && (newx < (oldx + 4 + 8)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
        }
        else if(mode == 0x01)
        {
          if((newx >= oldx) && (newx < (oldx + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;
          else if((newx >= (oldx + 16)) && (newx < (oldx + 16 + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;

          if((newx >= oldx + 4) && (newx < (oldx + 4 + 8)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
          else if((newx >= oldx + 16 + 4) && (newx < (oldx + 16 + 4 + 8)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
        }
        else if(mode == 0x02)
        {
          if((newx >= oldx) && (newx < (oldx + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;
          else if((newx >= (oldx + 32)) && (newx < (oldx + 32 + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;

          if((newx >= oldx + 4) && (newx < (oldx + 4 + 8)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
          else if((newx >= oldx + 32 + 4) && (newx < (oldx + 32 + 4 + 8)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
        }
        else if(mode == 0x03)
        {
          if((newx >= oldx) && (newx < (oldx + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;
          else if((newx >= (oldx + 16)) && (newx < (oldx + 16 + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;
          else if((newx >= (oldx + 32)) && (newx < (oldx + 32 + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;

          if((newx >= oldx + 4) && (newx < (oldx + 4 + 8)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
          else if((newx >= oldx + 16 + 4) && (newx < (oldx + 16 + 4 + 8)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
          else if((newx >= oldx + 32 + 4) && (newx < (oldx + 32 + 4 + 8)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
        }
        else if(mode == 0x04)
        {
          if((newx >= oldx) && (newx < (oldx + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;
          else if((newx >= (oldx + 64)) && (newx < (oldx + 64 + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;

          if((newx >= oldx + 4) && (newx < (oldx + 4 + 8)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
          else if((newx >= oldx + 64 + 4) && (newx < (oldx + 64 + 4 + 8)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
        }
        else if(mode == 0x05)
        {
          if((newx >= oldx) && (newx < (oldx + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;

          if((newx >= oldx + 4) && (newx < (oldx + 4 + 16)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
        }
        else if(mode == 0x06)
        {
          if((newx >= oldx) && (newx < (oldx + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;
          else if((newx >= (oldx + 32)) && (newx < (oldx + 32 + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;
          else if((newx >= (oldx + 64)) && (newx < (oldx + 64 + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;

          if((newx >= oldx + 4) && (newx < (oldx + 4 + 8)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
          else if((newx >= oldx + 32 + 4) && (newx < (oldx + 32 + 4 + 8)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
          else if((newx >= oldx + 64 + 4) && (newx < (oldx + 64 + 4 + 8)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
        }
        else if(mode == 0x07)
        {
          if((newx >= oldx) && (newx < (oldx + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;

          if((newx >= oldx + 4) && (newx < (oldx + 4 + 32)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
        }
      }

      // Let's do a sanity check on our table entries
      s1 = 0;
      s2 = 0;
      for(newx = 0; newx < 160; ++newx)
      {
        if(ourPlayerPositionResetWhenTable[mode][oldx][newx] == -1)
          ++s1;
        if(ourPlayerPositionResetWhenTable[mode][oldx][newx] == 1)
          ++s2;
      }
      //assert((s1 % 4 == 0) && (s2 % 8 == 0));
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void computePlayerReflectTable()
{
  uInt16 i, t;
  uInt8 r;

  for(i = 0; i < 256; ++i)
  {
    r = 0;

    for(t = 1; t <= 128; t *= 2)
    {
      r = (r << 1) | ((i & t) ? 0x01 : 0x00);
    }

    ourPlayerReflectTable[i] = r;
  } 
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void computePlayfieldMaskTable()
{
  Int32 x;

  // Compute playfield mask table for non-reflected mode
  for(x = 0; x < 160; ++x)
  {
    if(x < 16)
      ourPlayfieldTable[0][x] = 0x00001 << (x / 4);
    else if(x < 48)
      ourPlayfieldTable[0][x] = 0x00800 >> ((x - 16) / 4);
    else if(x < 80) 
      ourPlayfieldTable[0][x] = 0x01000 << ((x - 48) / 4);
    else if(x < 96) 
      ourPlayfieldTable[0][x] = 0x00001 << ((x - 80) / 4);
    else if(x < 128)
      ourPlayfieldTable[0][x] = 0x00800 >> ((x - 96) / 4);
    else if(x < 160) 
      ourPlayfieldTable[0][x] = 0x01000 << ((x - 128) / 4);
  }

  // Compute playfield mask table for reflected mode
  for(x = 0; x < 160; ++x)
  {
    if(x < 16)
      ourPlayfieldTable[1][x] = 0x00001 << (x / 4);
    else if(x < 48)
      ourPlayfieldTable[1][x] = 0x00800 >> ((x - 16) / 4);
    else if(x < 80) 
      ourPlayfieldTable[1][x] = 0x01000 << ((x - 48) / 4);
    else if(x < 112) 
      ourPlayfieldTable[1][x] = 0x80000 >> ((x - 80) / 4);
    else if(x < 144) 
      ourPlayfieldTable[1][x] = 0x00010 << ((x - 112) / 4);
    else if(x < 160) 
      ourPlayfieldTable[1][x] = 0x00008 >> ((x - 144) / 4);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void updateFrameScanline(uInt32 clocksToUpdate, uInt32 hpos)
{
  uInt8 enabled;
  // Calculate the ending frame pointer value
  uInt8 *ending = myFramePointer + clocksToUpdate;

  // See if we're in the vertical blank region
  if(myVBLANK & 0x02)
  {
    memset(myFramePointer, 0, clocksToUpdate);
  }
  // Handle all other possible combinations
  else
  {
    switch(myEnabledObjects | myPlayfieldPriorityAndScore)
    {
      // Background 
      case 0x00:
      case 0x00 | ScoreBit:
      case 0x00 | PriorityBit:
      case 0x00 | PriorityBit | ScoreBit:
      {
        memset(myFramePointer, myCOLUBK, clocksToUpdate);
        break;
      }

      // Playfield is enabled and the score bit is not set
      case myPFBit: 
      case myPFBit | PriorityBit:
      {
        uInt32 *mask = &myCurrentPFMask[hpos];

        // Update a uInt8 at a time until reaching a uInt32 boundary
        for(; ((int)myFramePointer & 0x03) && (myFramePointer < ending);
            ++myFramePointer, ++mask)
        {
          *myFramePointer = (myPF & *mask) ? myCOLUPF : myCOLUBK;
        }

        // Now, update a uInt32 at a time
        for(; myFramePointer < ending; myFramePointer += 4, mask += 4)
        {
          *((uInt32*)myFramePointer) = (myPF & *mask) ? myCOLUPF : myCOLUBK;
        }
        break;
      }

      // Playfield is enabled and the score bit is set
      case myPFBit | ScoreBit:
      case myPFBit | ScoreBit | PriorityBit:
      {
        uInt32 *mask = &myCurrentPFMask[hpos];

        // Update a uInt8 at a time until reaching a uInt32 boundary
        for(; ((int)myFramePointer & 0x03) && (myFramePointer < ending); 
            ++myFramePointer, ++mask, ++hpos)
        {
          *myFramePointer = (myPF & *mask) ? 
              (hpos < 80 ? myCOLUP0 : myCOLUP1) : myCOLUBK;
        }

        // Now, update a uInt32 at a time
        for(; myFramePointer < ending; 
            myFramePointer += 4, mask += 4, hpos += 4)
        {
          *((uInt32*)myFramePointer) = (myPF & *mask) ?
              (hpos < 80 ? myCOLUP0 : myCOLUP1) : myCOLUBK;
        }
        break;
      }

      // Player 0 is enabled
      case myP0Bit:
      case myP0Bit | ScoreBit:
      case myP0Bit | PriorityBit:
      case myP0Bit | ScoreBit | PriorityBit:
      {
        uInt8 *mP0 = &myCurrentP0Mask[hpos];

        while(myFramePointer < ending)
        {
          if(!((int)myFramePointer & 0x03) && !*(uInt32*)mP0)
          {
            *(uInt32*)myFramePointer = myCOLUBK;
            mP0 += 4; myFramePointer += 4;
          }
          else
          {
            *myFramePointer = (myCurrentGRP0 & *mP0) ? myCOLUP0 : myCOLUBK;
            ++mP0; ++myFramePointer;
          }
        }
        break;
      }

      // Player 1 is enabled
      case myP1Bit:
      case myP1Bit | ScoreBit:
      case myP1Bit | PriorityBit:
      case myP1Bit | ScoreBit | PriorityBit:
      {
        uInt8 *mP1 = &myCurrentP1Mask[hpos];

        while(myFramePointer < ending)
        {
          if(!((int)myFramePointer & 0x03) && !*(uInt32*)mP1)
          {
            *(uInt32*)myFramePointer = myCOLUBK;
            mP1 += 4; myFramePointer += 4;
          }
          else
          {
            *myFramePointer = (myCurrentGRP1 & *mP1) ? myCOLUP1 : myCOLUBK;
            ++mP1; ++myFramePointer;
          }
        }
        break;
      }

      // Player 0 and 1 are enabled
      case myP0Bit | myP1Bit:
      case myP0Bit | myP1Bit | ScoreBit:
      case myP0Bit | myP1Bit | PriorityBit:
      case myP0Bit | myP1Bit | ScoreBit | PriorityBit:
      {
        uInt8 *mP0 = &myCurrentP0Mask[hpos];
        uInt8 *mP1 = &myCurrentP1Mask[hpos];

        while(myFramePointer < ending)
        {
          if(!((int)myFramePointer & 0x03) && !*(uInt32*)mP0 &&
              !*(uInt32*)mP1)
          {
            *(uInt32*)myFramePointer = myCOLUBK;
            mP0 += 4; mP1 += 4; myFramePointer += 4;
          }
          else
          {
            *myFramePointer = (myCurrentGRP0 & *mP0) ? 
                myCOLUP0 : ((myCurrentGRP1 & *mP1) ? myCOLUP1 : myCOLUBK);

            if((myCurrentGRP0 & *mP0) && (myCurrentGRP1 & *mP1))
              myCollision |= ourCollisionTable[myP0Bit | myP1Bit];

            ++mP0; ++mP1; ++myFramePointer;
          }
        }
        break;
      }

      // Missle 0 is enabled
      case myM0Bit:
      case myM0Bit | ScoreBit:
      case myM0Bit | PriorityBit:
      case myM0Bit | ScoreBit | PriorityBit:
      {
        uInt8 *mM0 = &myCurrentM0Mask[hpos];

        while(myFramePointer < ending)
        {
          if(!((int)myFramePointer & 0x03) && !*(uInt32*)mM0)
          {
            *(uInt32*)myFramePointer = myCOLUBK;
            mM0 += 4; myFramePointer += 4;
          }
          else
          {
            *myFramePointer = *mM0 ? myCOLUP0 : myCOLUBK;
            ++mM0; ++myFramePointer;
          }
        }
        break;
      }

      // Missle 1 is enabled
      case myM1Bit:
      case myM1Bit | ScoreBit:
      case myM1Bit | PriorityBit:
      case myM1Bit | ScoreBit | PriorityBit:
      {
        uInt8 *mM1 = &myCurrentM1Mask[hpos];

        while(myFramePointer < ending)
        {
          if(!((int)myFramePointer & 0x03) && !*(uInt32*)mM1)
          {
            *(uInt32*)myFramePointer = myCOLUBK;
            mM1 += 4; myFramePointer += 4;
          }
          else
          {
            *myFramePointer = *mM1 ? myCOLUP1 : myCOLUBK;
            ++mM1; ++myFramePointer;
          }
        }
        break;
      }

      // Ball is enabled
      case myBLBit:
      case myBLBit | ScoreBit:
      case myBLBit | PriorityBit:
      case myBLBit | ScoreBit | PriorityBit:
      {
        uInt8 *mBL = &myCurrentBLMask[hpos];

        while(myFramePointer < ending)
        {
          if(!((int)myFramePointer & 0x03) && !*(uInt32*)mBL)
          {
            *(uInt32*)myFramePointer = myCOLUBK;
            mBL += 4; myFramePointer += 4;
          }
          else
          {
            *myFramePointer = *mBL ? myCOLUPF : myCOLUBK;
            ++mBL; ++myFramePointer;
          }
        }
        break;
      }

      // Missle 0 and 1 are enabled
      case myM0Bit | myM1Bit:
      case myM0Bit | myM1Bit | ScoreBit:
      case myM0Bit | myM1Bit | PriorityBit:
      case myM0Bit | myM1Bit | ScoreBit | PriorityBit:
      {
        uInt8 *mM0 = &myCurrentM0Mask[hpos];
        uInt8 *mM1 = &myCurrentM1Mask[hpos];

        while(myFramePointer < ending)
        {
          if(!((int)myFramePointer & 0x03) && !*(uInt32*)mM0 && !*(uInt32*)mM1)
          {
            *(uInt32*)myFramePointer = myCOLUBK;
            mM0 += 4; mM1 += 4; myFramePointer += 4;
          }
          else
          {
            *myFramePointer = *mM0 ? myCOLUP0 : (*mM1 ? myCOLUP1 : myCOLUBK);

            if(*mM0 && *mM1)
              myCollision |= ourCollisionTable[myM0Bit | myM1Bit];

            ++mM0; ++mM1; ++myFramePointer;
          }
        }
        break;
      }

      // Ball and Missle 0 are enabled and playfield priority is not set
      case myBLBit | myM0Bit:
      case myBLBit | myM0Bit | ScoreBit:
      {
        uInt8 *mBL = &myCurrentBLMask[hpos];
        uInt8 *mM0 = &myCurrentM0Mask[hpos];

        while(myFramePointer < ending)
        {
          if(!((int)myFramePointer & 0x03) && !*(uInt32*)mBL && !*(uInt32*)mM0)
          {
            *(uInt32*)myFramePointer = myCOLUBK;
            mBL += 4; mM0 += 4; myFramePointer += 4;
          }
          else
          {
            *myFramePointer = (*mM0 ? myCOLUP0 : (*mBL ? myCOLUPF : myCOLUBK));

            if(*mBL && *mM0)
              myCollision |= ourCollisionTable[myBLBit | myM0Bit];

            ++mBL; ++mM0; ++myFramePointer;
          }
        }
        break;
      }

      // Ball and Missle 0 are enabled and playfield priority is set
      case myBLBit | myM0Bit | PriorityBit:
      case myBLBit | myM0Bit | ScoreBit | PriorityBit:
      {
        uInt8 *mBL = &myCurrentBLMask[hpos];
        uInt8 *mM0 = &myCurrentM0Mask[hpos];

        while(myFramePointer < ending)
        {
          if(!((int)myFramePointer & 0x03) && !*(uInt32*)mBL && !*(uInt32*)mM0)
          {
            *(uInt32*)myFramePointer = myCOLUBK;
            mBL += 4; mM0 += 4; myFramePointer += 4;
          }
          else
          {
            *myFramePointer = (*mBL ? myCOLUPF : (*mM0 ? myCOLUP0 : myCOLUBK));

            if(*mBL && *mM0)
              myCollision |= ourCollisionTable[myBLBit | myM0Bit];

            ++mBL; ++mM0; ++myFramePointer;
          }
        }
        break;
      }

      // Ball and Missle 1 are enabled and playfield priority is not set
      case myBLBit | myM1Bit:
      case myBLBit | myM1Bit | ScoreBit:
      {
        uInt8 *mBL = &myCurrentBLMask[hpos];
        uInt8 *mM1 = &myCurrentM1Mask[hpos];

        while(myFramePointer < ending)
        {
          if(!((int)myFramePointer & 0x03) && !*(uInt32*)mBL && 
              !*(uInt32*)mM1)
          {
            *(uInt32*)myFramePointer = myCOLUBK;
            mBL += 4; mM1 += 4; myFramePointer += 4;
          }
          else
          {
            *myFramePointer = (*mM1 ? myCOLUP1 : (*mBL ? myCOLUPF : myCOLUBK));

            if(*mBL && *mM1)
              myCollision |= ourCollisionTable[myBLBit | myM1Bit];

            ++mBL; ++mM1; ++myFramePointer;
          }
        }
        break;
      }

      // Ball and Missle 1 are enabled and playfield priority is set
      case myBLBit | myM1Bit | PriorityBit:
      case myBLBit | myM1Bit | ScoreBit | PriorityBit:
      {
        uInt8 *mBL = &myCurrentBLMask[hpos];
        uInt8 *mM1 = &myCurrentM1Mask[hpos];

        while(myFramePointer < ending)
        {
          if(!((int)myFramePointer & 0x03) && !*(uInt32*)mBL && 
              !*(uInt32*)mM1)
          {
            *(uInt32*)myFramePointer = myCOLUBK;
            mBL += 4; mM1 += 4; myFramePointer += 4;
          }
          else
          {
            *myFramePointer = (*mBL ? myCOLUPF : (*mM1 ? myCOLUP1 : myCOLUBK));

            if(*mBL && *mM1)
              myCollision |= ourCollisionTable[myBLBit | myM1Bit];

            ++mBL; ++mM1; ++myFramePointer;
          }
        }
        break;
      }

      // Ball and Player 1 are enabled and playfield priority is not set
      case myBLBit | myP1Bit:
      case myBLBit | myP1Bit | ScoreBit:
      {
        uInt8 *mBL = &myCurrentBLMask[hpos];
        uInt8 *mP1 = &myCurrentP1Mask[hpos];

        while(myFramePointer < ending)
        {
          if(!((int)myFramePointer & 0x03) && !*(uInt32*)mP1 && !*(uInt32*)mBL)
          {
            *(uInt32*)myFramePointer = myCOLUBK;
            mBL += 4; mP1 += 4; myFramePointer += 4;
          }
          else
          {
            *myFramePointer = (myCurrentGRP1 & *mP1) ? myCOLUP1 : 
                (*mBL ? myCOLUPF : myCOLUBK);

            if(*mBL && (myCurrentGRP1 & *mP1))
              myCollision |= ourCollisionTable[myBLBit | myP1Bit];

            ++mBL; ++mP1; ++myFramePointer;
          }
        }
        break;
      }

      // Ball and Player 1 are enabled and playfield priority is set
      case myBLBit | myP1Bit | PriorityBit:
      case myBLBit | myP1Bit | PriorityBit | ScoreBit:
      {
        uInt8 *mBL = &myCurrentBLMask[hpos];
        uInt8 *mP1 = &myCurrentP1Mask[hpos];

        while(myFramePointer < ending)
        {
          if(!((int)myFramePointer & 0x03) && !*(uInt32*)mP1 && !*(uInt32*)mBL)
          {
            *(uInt32*)myFramePointer = myCOLUBK;
            mBL += 4; mP1 += 4; myFramePointer += 4;
          }
          else
          {
            *myFramePointer = *mBL ? myCOLUPF : 
                ((myCurrentGRP1 & *mP1) ? myCOLUP1 : myCOLUBK);

            if(*mBL && (myCurrentGRP1 & *mP1))
              myCollision |= ourCollisionTable[myBLBit | myP1Bit];

            ++mBL; ++mP1; ++myFramePointer;
          }
        }
        break;
      }

      // Playfield and Player 0 are enabled and playfield priority is not set
      case myPFBit | myP0Bit:
      {
        uInt32 *mPF = &myCurrentPFMask[hpos];
        uInt8 *mP0 = &myCurrentP0Mask[hpos];

        while(myFramePointer < ending)
        {
          if(!((int)myFramePointer & 0x03) && !*(uInt32*)mP0)
          {
            *(uInt32*)myFramePointer = (myPF & *mPF) ? myCOLUPF : myCOLUBK;
            mPF += 4; mP0 += 4; myFramePointer += 4;
          }
          else
          {
            *myFramePointer = (myCurrentGRP0 & *mP0) ? 
                  myCOLUP0 : ((myPF & *mPF) ? myCOLUPF : myCOLUBK);

            if((myPF & *mPF) && (myCurrentGRP0 & *mP0))
              myCollision |= ourCollisionTable[myPFBit | myP0Bit];

            ++mPF; ++mP0; ++myFramePointer;
          }
        }

        break;
      }

      // Playfield and Player 0 are enabled and playfield priority is set
      case myPFBit | myP0Bit | PriorityBit:
      {
        uInt32 *mPF = &myCurrentPFMask[hpos];
        uInt8 *mP0 = &myCurrentP0Mask[hpos];

        while(myFramePointer < ending)
        {
          if(!((int)myFramePointer & 0x03) && !*(uInt32*)mP0)
          {
            *(uInt32*)myFramePointer = (myPF & *mPF) ? myCOLUPF : myCOLUBK;
            mPF += 4; mP0 += 4; myFramePointer += 4;
          }
          else
          {
            *myFramePointer = (myPF & *mPF) ? myCOLUPF : 
                ((myCurrentGRP0 & *mP0) ? myCOLUP0 : myCOLUBK);

            if((myPF & *mPF) && (myCurrentGRP0 & *mP0))
              myCollision |= ourCollisionTable[myPFBit | myP0Bit];

            ++mPF; ++mP0; ++myFramePointer;
          }
        }

        break;
      }

      // Playfield and Player 1 are enabled and playfield priority is not set
      case myPFBit | myP1Bit:
      {
        uInt32 *mPF = &myCurrentPFMask[hpos];
        uInt8 *mP1 = &myCurrentP1Mask[hpos];

        while(myFramePointer < ending)
        {
          if(!((int)myFramePointer & 0x03) && !*(uInt32*)mP1)
          {
            *(uInt32*)myFramePointer = (myPF & *mPF) ? myCOLUPF : myCOLUBK;
            mPF += 4; mP1 += 4; myFramePointer += 4;
          }
          else
          {
            *myFramePointer = (myCurrentGRP1 & *mP1) ? 
                  myCOLUP1 : ((myPF & *mPF) ? myCOLUPF : myCOLUBK);

            if((myPF & *mPF) && (myCurrentGRP1 & *mP1))
              myCollision |= ourCollisionTable[myPFBit | myP1Bit];

            ++mPF; ++mP1; ++myFramePointer;
          }
        }

        break;
      }

      // Playfield and Player 1 are enabled and playfield priority is set
      case myPFBit | myP1Bit | PriorityBit:
      {
        uInt32 *mPF = &myCurrentPFMask[hpos];
        uInt8 *mP1 = &myCurrentP1Mask[hpos];

        while(myFramePointer < ending)
        {
          if(!((int)myFramePointer & 0x03) && !*(uInt32*)mP1)
          {
            *(uInt32*)myFramePointer = (myPF & *mPF) ? myCOLUPF : myCOLUBK;
            mPF += 4; mP1 += 4; myFramePointer += 4;
          }
          else
          {
            *myFramePointer = (myPF & *mPF) ? myCOLUPF : 
                ((myCurrentGRP1 & *mP1) ? myCOLUP1 : myCOLUBK);

            if((myPF & *mPF) && (myCurrentGRP1 & *mP1))
              myCollision |= ourCollisionTable[myPFBit | myP1Bit];

            ++mPF; ++mP1; ++myFramePointer;
          }
        }

        break;
      }

      // Playfield and Ball are enabled
      case myPFBit | myBLBit:
      case myPFBit | myBLBit | PriorityBit:
      {
        uInt32 *mPF = &myCurrentPFMask[hpos];
        uInt8 *mBL = &myCurrentBLMask[hpos];

        while(myFramePointer < ending)
        {
          if(!((int)myFramePointer & 0x03) && !*(uInt32*)mBL)
          {
            *(uInt32*)myFramePointer = (myPF & *mPF) ? myCOLUPF : myCOLUBK;
            mPF += 4; mBL += 4; myFramePointer += 4;
          }
          else
          {
            *myFramePointer = ((myPF & *mPF) || *mBL) ? myCOLUPF : myCOLUBK;

            if((myPF & *mPF) && *mBL)
              myCollision |= ourCollisionTable[myPFBit | myBLBit];

            ++mPF; ++mBL; ++myFramePointer;
          }
        }
        break;
      }

      // Handle all of the other cases
      default:
      {
        for(; myFramePointer < ending; ++myFramePointer, ++hpos)
        {
          enabled = (myPF & myCurrentPFMask[hpos]) ? myPFBit : 0;

          if((myEnabledObjects & myBLBit) && myCurrentBLMask[hpos])
            enabled |= myBLBit;

          if(myCurrentGRP1 & myCurrentP1Mask[hpos])
            enabled |= myP1Bit;

          if((myEnabledObjects & myM1Bit) && myCurrentM1Mask[hpos])
            enabled |= myM1Bit;

          if(myCurrentGRP0 & myCurrentP0Mask[hpos])
            enabled |= myP0Bit;

          if((myEnabledObjects & myM0Bit) && myCurrentM0Mask[hpos])
            enabled |= myM0Bit;

          myCollision |= ourCollisionTable[enabled];

	  myColor[0] = myCOLUBK;
	  myColor[1] = myCOLUPF;
	  myColor[2] = myCOLUP0;
	  myColor[3] = myCOLUP1;

          *myFramePointer = myColor[myPriorityEncoder[hpos < 80 ? 0 : 1]
                [enabled | myPlayfieldPriorityAndScore]];
        }
        break;  
      }
    }
  }
  myFramePointer = ending;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void updateFrame(Int32 clock)
{
  Int32 startOfScanLine;
  uInt8 *oldFramePointer;

  // See if we're in the nondisplayable portion of the screen or if
  // we've already updated this portion of the screen
  if((clock < myClockStartDisplay) || 
      (myClockAtLastUpdate >= myClockStopDisplay) ||  
      (myClockAtLastUpdate >= clock))
  {
    return;
  }

  // Truncate the number of cycles to update to the stop display point
  if(clock > myClockStopDisplay)
  {
    clock = myClockStopDisplay;
  }

  // Update frame one scanline at a time
  do
  {
    // Compute the number of clocks we're going to update
    Int32 clocksToUpdate = 0;

    // Remember how many clocks we are from the left side of the screen
    Int32 clocksFromStartOfScanLine = 228 - myClocksToEndOfScanLine;

    // See if we're updating more than the current scanline
    if(clock > (myClockAtLastUpdate + myClocksToEndOfScanLine))
    {
      // Yes, we have more than one scanline to update so finish current one
      clocksToUpdate = myClocksToEndOfScanLine;
      myClocksToEndOfScanLine = 228;
      myClockAtLastUpdate += clocksToUpdate;
    }
    else
    {
      // No, so do as much of the current scanline as possible
      clocksToUpdate = clock - myClockAtLastUpdate;
      myClocksToEndOfScanLine -= clocksToUpdate;
      myClockAtLastUpdate = clock;
    }

    startOfScanLine = HBLANK + myFrameXStart;

    // Skip over as many horizontal blank clocks as we can
    if(clocksFromStartOfScanLine < startOfScanLine)
    {
      uInt32 tmp;

      if((startOfScanLine - clocksFromStartOfScanLine) < clocksToUpdate)
        tmp = startOfScanLine - clocksFromStartOfScanLine;
      else
        tmp = clocksToUpdate;

      clocksFromStartOfScanLine += tmp;
      clocksToUpdate -= tmp;
    }

    // Remember frame pointer in case HMOVE blanks need to be handled
    oldFramePointer = myFramePointer;

    // Update as much of the scanline as we can
    if(clocksToUpdate != 0)
    {
      updateFrameScanline(clocksToUpdate, clocksFromStartOfScanLine - HBLANK);
    }

    // Handle HMOVE blanks if they are enabled
    if(myHMOVEBlankEnabled && (startOfScanLine < HBLANK + 8) &&
        (clocksFromStartOfScanLine < (HBLANK + 8)))
    {
      Int32 blanks = (HBLANK + 8) - clocksFromStartOfScanLine;
      memset(oldFramePointer, 0, blanks);

      if((clocksToUpdate + clocksFromStartOfScanLine) >= (HBLANK + 8))
      {
        myHMOVEBlankEnabled = false;
      }
    }

    // See if we're at the end of a scanline
    if(myClocksToEndOfScanLine == 228)
    {
      myFramePointer -= (160 - myFrameWidth - myFrameXStart);

      // Yes, so set PF mask based on current CTRLPF reflection state 
      myCurrentPFMask = ourPlayfieldTable[myCTRLPF & 0x01];

      // TODO: These should be reset right after the first copy of the player
      // has passed.  However, for now we'll just reset at the end of the 
      // scanline since the other way would be to slow (01/21/99).
      myCurrentP0Mask = &ourPlayerMaskTable[myPOSP0 & 0x03]
          [0][myNUSIZ0 & 0x07][160 - (myPOSP0 & 0xFC)];
      myCurrentP1Mask = &ourPlayerMaskTable[myPOSP1 & 0x03]
          [0][myNUSIZ1 & 0x07][160 - (myPOSP1 & 0xFC)];

      // Handle the "Cosmic Ark" TIA bug if it's enabled
      if(myM0CosmicArkMotionEnabled)
      {
        // Movement table associated with the bug
        static uInt32 m[4] = {18, 33, 0, 17};

        myM0CosmicArkCounter = (myM0CosmicArkCounter + 1) & 3;
        myPOSM0 -= m[myM0CosmicArkCounter];

        if(myPOSM0 >= 160)
          myPOSM0 -= 160;
        else if(myPOSM0 < 0)
          myPOSM0 += 160;

        if(myM0CosmicArkCounter == 1)
        {
          // Stretch this missle so it's at least 2 pixels wide
          myCurrentM0Mask = &ourMissleMaskTable[myPOSM0 & 0x03]
              [myNUSIZ0 & 0x07][((myNUSIZ0 & 0x30) >> 4) | 0x01]
              [160 - (myPOSM0 & 0xFC)];
        }
        else if(myM0CosmicArkCounter == 2)
        {
          // Missle is disabled on this line 
          myCurrentM0Mask = &ourDisabledMaskTable[0];
        }
        else
        {
          myCurrentM0Mask = &ourMissleMaskTable[myPOSM0 & 0x03]
              [myNUSIZ0 & 0x07][(myNUSIZ0 & 0x30) >> 4][160 - (myPOSM0 & 0xFC)];
        }
      } 
    }
  } 
  while(myClockAtLastUpdate < clock);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void stella_stop()
{
  freeze_cpu=1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void waitHorizontalSync()
{
  uInt32 cyclesToEndOfLine = 76 - ((stella_cycles - 
      (myClockWhenFrameStarted / 3)) % 76);

  if(cyclesToEndOfLine < 76)
  {
    stella_cycles +=cyclesToEndOfLine;
    clk +=cyclesToEndOfLine;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 stella_tia_peek(CLOCK c, uInt16 addr)
{
  uInt8 noise = getDataBusState() & 0x3F;
  stella_cycles = c;

  // Update frame to current color clock before we look at anything!
  updateFrame(stella_cycles * 3);

  switch(addr & 0x000f)
  {
    case 0x00:    // CXM0P
      return ((myCollision & 0x0001) ? 0x80 : 0x00) | 
          ((myCollision & 0x0002) ? 0x40 : 0x00) | noise;

    case 0x01:    // CXM1P
      return ((myCollision & 0x0004) ? 0x80 : 0x00) | 
          ((myCollision & 0x0008) ? 0x40 : 0x00) | noise;

    case 0x02:    // CXP0FB
      return ((myCollision & 0x0010) ? 0x80 : 0x00) | 
          ((myCollision & 0x0020) ? 0x40 : 0x00) | noise;

    case 0x03:    // CXP1FB
      return ((myCollision & 0x0040) ? 0x80 : 0x00) | 
          ((myCollision & 0x0080) ? 0x40 : 0x00) | noise;

    case 0x04:    // CXM0FB
      return ((myCollision & 0x0100) ? 0x80 : 0x00) | 
          ((myCollision & 0x0200) ? 0x40 : 0x00) | noise;

    case 0x05:    // CXM1FB
      return ((myCollision & 0x0400) ? 0x80 : 0x00) | 
          ((myCollision & 0x0800) ? 0x40 : 0x00) | noise;

    case 0x06:    // CXBLPF
      return ((myCollision & 0x1000) ? 0x80 : 0x00) | noise;

    case 0x07:    // CXPPMM
      return ((myCollision & 0x2000) ? 0x80 : 0x00) | 
          ((myCollision & 0x4000) ? 0x40 : 0x00) | noise;

    case 0x0e:
      return noise;

    default:
      return noise;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void stella_tia_poke(CLOCK c, uInt16 addr, uInt8 value)
{
  Int32 s_clock;
  Int16 delay;

  stella_cycles = c;
  addr = addr & 0x003f;

  s_clock = stella_cycles * 3;
  delay = ourPokeDelayTable[addr];

  // See if this is a poke to a PF register
  if(delay == -1)
  {
    static uInt32 d[4] = {4, 5, 2, 3};
    Int32 x = ((s_clock - myClockWhenFrameStarted) % 228);
    delay = d[(x / 3) & 3];
  }

  // Update frame to current CPU cycle before we make any changes!
  updateFrame(s_clock + delay);

  // If a VSYNC hasn't been generated in time go ahead and end the frame
  if(((s_clock - myClockWhenFrameStarted) / 228) > myMaximumNumberOfScanlines)
  {
    stella_stop();
  }

  switch(addr)
  {
    case 0x00:    // Vertical sync set-clear
    {
      myVSYNC = value;

      if(myVSYNC & 0x02)
      {
        // Indicate when VSYNC should be finished.  This should really 
        // be 3 * 228 according to Atari's documentation, however, some 
        // games don't supply the full 3 scanlines of VSYNC.
        myVSYNCFinishClock = s_clock + 228;
      }
      else if(!(myVSYNC & 0x02) && (s_clock >= myVSYNCFinishClock))
      {
        // We're no longer interested in myVSYNCFinishClock
        myVSYNCFinishClock = 0x7FFFFFFF;

        // Since we're finished with the frame tell the processor to halt
        stella_stop();
      }
      break;
    }

    case 0x01:    // Vertical blank set-clear
    {
      // Is the dump to ground path being set for I0, I1, I2, and I3?
      if(!(myVBLANK & 0x80) && (value & 0x80))
      {
        myDumpEnabled = true;
      }

      // Is the dump to ground path being removed from I0, I1, I2, and I3?
      if((myVBLANK & 0x80) && !(value & 0x80))
      {
        myDumpEnabled = false;
        myDumpDisabledCycle = stella_cycles;
      }

      myVBLANK = value;
      break;
    }

    case 0x02:    // Wait for leading edge of HBLANK
    {
      // Tell the cpu to waste the necessary amount of time
      waitHorizontalSync();
      break;
    }

    case 0x03:    // Reset horizontal sync counter
    {
      // cerr << "TIA Poke: " << hex << addr << endl;
      break;
    }

    case 0x04:    // Number-size of player-missle 0
    {
      myNUSIZ0 = value;

      // TODO: Technically the "enable" part, [0], should depend on the current
      // enabled or disabled state.  This mean we probably need a data member
      // to maintain that state (01/21/99).
      myCurrentP0Mask = &ourPlayerMaskTable[myPOSP0 & 0x03]
          [0][myNUSIZ0 & 0x07][160 - (myPOSP0 & 0xFC)];

      myCurrentM0Mask = &ourMissleMaskTable[myPOSM0 & 0x03]
          [myNUSIZ0 & 0x07][(myNUSIZ0 & 0x30) >> 4][160 - (myPOSM0 & 0xFC)];

      break;
    }

    case 0x05:    // Number-size of player-missle 1
    {
      myNUSIZ1 = value;

      // TODO: Technically the "enable" part, [0], should depend on the current
      // enabled or disabled state.  This mean we probably need a data member
      // to maintain that state (01/21/99).
      myCurrentP1Mask = &ourPlayerMaskTable[myPOSP1 & 0x03]
          [0][myNUSIZ1 & 0x07][160 - (myPOSP1 & 0xFC)];

      myCurrentM1Mask = &ourMissleMaskTable[myPOSM1 & 0x03]
          [myNUSIZ1 & 0x07][(myNUSIZ1 & 0x30) >> 4][160 - (myPOSM1 & 0xFC)];

      break;
    }

    case 0x06:    // Color-Luminance Player 0
    {
      uInt32 color = (uInt32)(value & 0xfe);
      if(myColorLossEnabled && (myScanlineCountForLastFrame & 0x01))
      {
        color |= 0x01;
      }
      myCOLUP0 = (((((color << 8) | color) << 8) | color) << 8) | color;
      break;
    }

    case 0x07:    // Color-Luminance Player 1
    {
      uInt32 color = (uInt32)(value & 0xfe);
      if(myColorLossEnabled && (myScanlineCountForLastFrame & 0x01))
      {
        color |= 0x01;
      }
      myCOLUP1 = (((((color << 8) | color) << 8) | color) << 8) | color;
      break;
    }

    case 0x08:    // Color-Luminance Playfield
    {
      uInt32 color = (uInt32)(value & 0xfe);
      if(myColorLossEnabled && (myScanlineCountForLastFrame & 0x01))
      {
        color |= 0x01;
      }
      myCOLUPF = (((((color << 8) | color) << 8) | color) << 8) | color;
      break;
    }

    case 0x09:    // Color-Luminance Background
    {
      uInt32 color = (uInt32)(value & 0xfe);
      if(myColorLossEnabled && (myScanlineCountForLastFrame & 0x01))
      {
        color |= 0x01;
      }
      myCOLUBK = (((((color << 8) | color) << 8) | color) << 8) | color;
      break;
    }

    case 0x0A:    // Control Playfield, Ball size, Collisions
    {
      myCTRLPF = value;

      // The playfield priority and score bits from the control register
      // are accessed when the frame is being drawn.  We precompute the 
      // necessary value here so we can save time while drawing.
      myPlayfieldPriorityAndScore = ((myCTRLPF & 0x06) << 5);

      // Update the playfield mask based on reflection state if 
      // we're still on the left hand side of the playfield
      if(((s_clock - myClockWhenFrameStarted) % 228) < (68 + 79))
      {
        myCurrentPFMask = ourPlayfieldTable[myCTRLPF & 0x01];
      }

      myCurrentBLMask = &ourBallMaskTable[myPOSBL & 0x03]
          [(myCTRLPF & 0x30) >> 4][160 - (myPOSBL & 0xFC)];

      break;
    }

    case 0x0B:    // Reflect Player 0
    {
      // See if the reflection state of the player is being changed
      if(((value & 0x08) && !myREFP0) || (!(value & 0x08) && myREFP0))
      {
        myREFP0 = (value & 0x08);
        myCurrentGRP0 = ourPlayerReflectTable[myCurrentGRP0];
      }
      break;
    }

    case 0x0C:    // Reflect Player 1
    {
      // See if the reflection state of the player is being changed
      if(((value & 0x08) && !myREFP1) || (!(value & 0x08) && myREFP1))
      {
        myREFP1 = (value & 0x08);
        myCurrentGRP1 = ourPlayerReflectTable[myCurrentGRP1];
      }
      break;
    }

    case 0x0D:    // Playfield register byte 0
    {
      myPF = (myPF & 0x000FFFF0) | ((value >> 4) & 0x0F);

      if(myPF != 0)
        myEnabledObjects |= myPFBit;
      else
        myEnabledObjects &= ~myPFBit;

      break;
    }

    case 0x0E:    // Playfield register byte 1
    {
      myPF = (myPF & 0x000FF00F) | ((uInt32)value << 4);

      if(myPF != 0)
        myEnabledObjects |= myPFBit;
      else
        myEnabledObjects &= ~myPFBit;

      break;
    }

    case 0x0F:    // Playfield register byte 2
    {
      myPF = (myPF & 0x00000FFF) | ((uInt32)value << 12);

      if(myPF != 0)
        myEnabledObjects |= myPFBit;
      else
        myEnabledObjects &= ~myPFBit;

      break;
    }

    case 0x10:    // Reset Player 0
    {
      Int32 hpos = (s_clock - myClockWhenFrameStarted) % 228;
      Int32 newx = hpos < HBLANK ? 3 : (((hpos - HBLANK) + 5) % 160);

      // Find out under what condition the player is being reset
      Int8 when = ourPlayerPositionResetWhenTable[myNUSIZ0 & 7][myPOSP0][newx];

      // Player is being reset during the display of one of its copies
      if(when == 1)
      {
        // So we go ahead and update the display before moving the player
        // TODO: The 11 should depend on how much of the player has already
        // been displayed.  Probably change table to return the amount to
        // delay by instead of just 1 (01/21/99).
        updateFrame(s_clock + 11);

        myPOSP0 = newx;

        // Setup the mask to skip the first copy of the player
        myCurrentP0Mask = &ourPlayerMaskTable[myPOSP0 & 0x03]
            [1][myNUSIZ0 & 0x07][160 - (myPOSP0 & 0xFC)];
      }
      // Player is being reset in neither the delay nor display section
      else if(when == 0)
      {
        myPOSP0 = newx;

        // So we setup the mask to skip the first copy of the player
        myCurrentP0Mask = &ourPlayerMaskTable[myPOSP0 & 0x03]
            [1][myNUSIZ0 & 0x07][160 - (myPOSP0 & 0xFC)];
      }
      // Player is being reset during the delay section of one of its copies
      else if(when == -1)
      {
        myPOSP0 = newx;

        // So we setup the mask to display all copies of the player
        myCurrentP0Mask = &ourPlayerMaskTable[myPOSP0 & 0x03]
            [0][myNUSIZ0 & 0x07][160 - (myPOSP0 & 0xFC)];
      }
      break;
    }

    case 0x11:    // Reset Player 1
    {
      Int32 hpos = (s_clock - myClockWhenFrameStarted) % 228;
      Int32 newx = hpos < HBLANK ? 3 : (((hpos - HBLANK) + 5) % 160);

      // Find out under what condition the player is being reset
      Int8 when = ourPlayerPositionResetWhenTable[myNUSIZ1 & 7][myPOSP1][newx];

      // Player is being reset during the display of one of its copies
      if(when == 1)
      {
        // So we go ahead and update the display before moving the player
        // TODO: The 11 should depend on how much of the player has already
        // been displayed.  Probably change table to return the amount to
        // delay by instead of just 1 (01/21/99).
        updateFrame(s_clock + 11);

        myPOSP1 = newx;

        // Setup the mask to skip the first copy of the player
        myCurrentP1Mask = &ourPlayerMaskTable[myPOSP1 & 0x03]
            [1][myNUSIZ1 & 0x07][160 - (myPOSP1 & 0xFC)];
      }
      // Player is being reset in neither the delay nor display section
      else if(when == 0)
      {
        myPOSP1 = newx;

        // So we setup the mask to skip the first copy of the player
        myCurrentP1Mask = &ourPlayerMaskTable[myPOSP1 & 0x03]
            [1][myNUSIZ1 & 0x07][160 - (myPOSP1 & 0xFC)];
      }
      // Player is being reset during the delay section of one of its copies
      else if(when == -1)
      {
        myPOSP1 = newx;

        // So we setup the mask to display all copies of the player
        myCurrentP1Mask = &ourPlayerMaskTable[myPOSP1 & 0x03]
            [0][myNUSIZ1 & 0x07][160 - (myPOSP1 & 0xFC)];
      }
      break;
    }

    case 0x12:    // Reset Missle 0
    {
      int hpos = (s_clock - myClockWhenFrameStarted) % 228;
      myPOSM0 = hpos < HBLANK ? 2 : (((hpos - HBLANK) + 4) % 160);

      // TODO: Remove the following special hack for Dolphin by
      // figuring out what really happens when Reset Missle 
      // occurs 20 cycles after an HMOVE (04/13/02).
      if(((s_clock - myLastHMOVEClock) == (20 * 3)) && (hpos == 69))
      {
        myPOSM0 = 8;
      }
 
      myCurrentM0Mask = &ourMissleMaskTable[myPOSM0 & 0x03]
          [myNUSIZ0 & 0x07][(myNUSIZ0 & 0x30) >> 4][160 - (myPOSM0 & 0xFC)];
      break;
    }

    case 0x13:    // Reset Missle 1
    {
      int hpos = (s_clock - myClockWhenFrameStarted) % 228;
      myPOSM1 = hpos < HBLANK ? 2 : (((hpos - HBLANK) + 4) % 160);

      // TODO: Remove the following special hack for Pitfall II by
      // figuring out what really happens when Reset Missle 
      // occurs 3 cycles after an HMOVE (04/13/02).
      if(((s_clock - myLastHMOVEClock) == (3 * 3)) && (hpos == 18))
      {
        myPOSM1 = 3;
      }
 
      myCurrentM1Mask = &ourMissleMaskTable[myPOSM1 & 0x03]
          [myNUSIZ1 & 0x07][(myNUSIZ1 & 0x30) >> 4][160 - (myPOSM1 & 0xFC)];
      break;
    }

    case 0x14:    // Reset Ball
    {
      int hpos = (s_clock - myClockWhenFrameStarted) % 228 ;
      myPOSBL = hpos < HBLANK ? 2 : (((hpos - HBLANK) + 4) % 160);

      // TODO: Remove the following special hack for Escape from the 
      // Mindmaster by figuring out what really happens when Reset Ball 
      // occurs 18 cycles after an HMOVE (01/09/99).
      if(((s_clock - myLastHMOVEClock) == (18 * 3)) && 
          ((hpos == 60) || (hpos == 69)))
      {
        myPOSBL = 10;
      }
      // TODO: Remove the following special hack for Decathlon by
      // figuring out what really happens when Reset Ball 
      // occurs 3 cycles after an HMOVE (04/13/02).
      else if(((s_clock - myLastHMOVEClock) == (3 * 3)) && (hpos == 18))
      {
        myPOSBL = 3;
      } 
      // TODO: Remove the following special hack for Robot Tank by
      // figuring out what really happens when Reset Ball 
      // occurs 7 cycles after an HMOVE (04/13/02).
      else if(((s_clock - myLastHMOVEClock) == (7 * 3)) && (hpos == 30))
      {
        myPOSBL = 6;
      } 
      // TODO: Remove the following special hack for Hole Hunter by
      // figuring out what really happens when Reset Ball 
      // occurs 6 cycles after an HMOVE (04/13/02).
      else if(((s_clock - myLastHMOVEClock) == (6 * 3)) && (hpos == 27))
      {
        myPOSBL = 5;
      }
 
      myCurrentBLMask = &ourBallMaskTable[myPOSBL & 0x03]
          [(myCTRLPF & 0x30) >> 4][160 - (myPOSBL & 0xFC)];
      break;
    }

    case 0x15:    // Audio control 0
    {
      //mySound.set(addr, value, mySystem->cycles());
      break;
    }
  
    case 0x16:    // Audio control 1
    {
      //mySound.set(addr, value, mySystem->cycles());
      break;
    }
  
    case 0x17:    // Audio frequency 0
    {
      //mySound.set(addr, value, mySystem->cycles());
      break;
    }
  
    case 0x18:    // Audio frequency 1
    {
      //mySound.set(addr, value, mySystem->cycles());
      break;
    }
  
    case 0x19:    // Audio volume 0
    {
      //mySound.set(addr, value, mySystem->cycles());
      break;
    }
  
    case 0x1A:    // Audio volume 1
    {
      //mySound.set(addr, value, mySystem->cycles());
      break;
    }

    case 0x1B:    // Graphics Player 0
    {
      // Set player 0 graphics
      myGRP0 = value;

      // Copy player 1 graphics into its delayed register
      myDGRP1 = myGRP1;

      // Get the "current" data for GRP0 base on delay register and reflect
      uInt8 grp0 = myVDELP0 ? myDGRP0 : myGRP0;
      myCurrentGRP0 = myREFP0 ? ourPlayerReflectTable[grp0] : grp0; 

      // Get the "current" data for GRP1 base on delay register and reflect
      uInt8 grp1 = myVDELP1 ? myDGRP1 : myGRP1;
      myCurrentGRP1 = myREFP1 ? ourPlayerReflectTable[grp1] : grp1; 

      // Set enabled object bits
      if(myCurrentGRP0 != 0)
        myEnabledObjects |= myP0Bit;
      else
        myEnabledObjects &= ~myP0Bit;

      if(myCurrentGRP1 != 0)
        myEnabledObjects |= myP1Bit;
      else
        myEnabledObjects &= ~myP1Bit;

      break;
    }

    case 0x1C:    // Graphics Player 1
    {
      // Set player 1 graphics
      myGRP1 = value;

      // Copy player 0 graphics into its delayed register
      myDGRP0 = myGRP0;

      // Copy ball graphics into its delayed register
      myDENABL = myENABL;

      // Get the "current" data for GRP0 base on delay register
      uInt8 grp0 = myVDELP0 ? myDGRP0 : myGRP0;
      myCurrentGRP0 = myREFP0 ? ourPlayerReflectTable[grp0] : grp0; 

      // Get the "current" data for GRP1 base on delay register
      uInt8 grp1 = myVDELP1 ? myDGRP1 : myGRP1;
      myCurrentGRP1 = myREFP1 ? ourPlayerReflectTable[grp1] : grp1; 

      // Set enabled object bits
      if(myCurrentGRP0 != 0)
        myEnabledObjects |= myP0Bit;
      else
        myEnabledObjects &= ~myP0Bit;

      if(myCurrentGRP1 != 0)
        myEnabledObjects |= myP1Bit;
      else
        myEnabledObjects &= ~myP1Bit;

      if(myVDELBL ? myDENABL : myENABL)
        myEnabledObjects |= myBLBit;
      else
        myEnabledObjects &= ~myBLBit;

      break;
    }

    case 0x1D:    // Enable Missle 0 graphics
    {
      myENAM0 = value & 0x02;

      if(myENAM0 && !myRESMP0)
        myEnabledObjects |= myM0Bit;
      else
        myEnabledObjects &= ~myM0Bit;
      break;
    }

    case 0x1E:    // Enable Missle 1 graphics
    {
      myENAM1 = value & 0x02;

      if(myENAM1 && !myRESMP1)
        myEnabledObjects |= myM1Bit;
      else
        myEnabledObjects &= ~myM1Bit;
      break;
    }

    case 0x1F:    // Enable Ball graphics
    {
      myENABL = value & 0x02;

      if(myVDELBL ? myDENABL : myENABL)
        myEnabledObjects |= myBLBit;
      else
        myEnabledObjects &= ~myBLBit;

      break;
    }

    case 0x20:    // Horizontal Motion Player 0
    {
      myHMP0 = value >> 4;
      break;
    }

    case 0x21:    // Horizontal Motion Player 1
    {
      myHMP1 = value >> 4;
      break;
    }

    case 0x22:    // Horizontal Motion Missle 0
    {
      Int8 tmp = value >> 4;

      // Should we enabled TIA M0 "bug" used for stars in Cosmic Ark?
      if((s_clock == (myLastHMOVEClock + 21 * 3)) && (myHMM0 == 7) && (tmp == 6))
      {
        myM0CosmicArkMotionEnabled = true;
        myM0CosmicArkCounter = 0;
      }

      myHMM0 = tmp;
      break;
    }

    case 0x23:    // Horizontal Motion Missle 1
    {
      myHMM1 = value >> 4;
      break;
    }

    case 0x24:    // Horizontal Motion Ball
    {
      myHMBL = value >> 4;
      break;
    }

    case 0x25:    // Vertial Delay Player 0
    {
      myVDELP0 = value & 0x01;

      uInt8 grp0 = myVDELP0 ? myDGRP0 : myGRP0;
      myCurrentGRP0 = myREFP0 ? ourPlayerReflectTable[grp0] : grp0; 

      if(myCurrentGRP0 != 0)
        myEnabledObjects |= myP0Bit;
      else
        myEnabledObjects &= ~myP0Bit;
      break;
    }

    case 0x26:    // Vertial Delay Player 1
    {
      myVDELP1 = value & 0x01;

      uInt8 grp1 = myVDELP1 ? myDGRP1 : myGRP1;
      myCurrentGRP1 = myREFP1 ? ourPlayerReflectTable[grp1] : grp1; 

      if(myCurrentGRP1 != 0)
        myEnabledObjects |= myP1Bit;
      else
        myEnabledObjects &= ~myP1Bit;
      break;
    }

    case 0x27:    // Vertial Delay Ball
    {
      myVDELBL = value & 0x01;

      if(myVDELBL ? myDENABL : myENABL)
        myEnabledObjects |= myBLBit;
      else
        myEnabledObjects &= ~myBLBit;
      break;
    }

    case 0x28:    // Reset missle 0 to player 0
    {
      if(myRESMP0 && !(value & 0x02))
      {
        uInt16 middle;

        if((myNUSIZ0 & 0x07) == 0x05)
          middle = 8;
        else if((myNUSIZ0 & 0x07) == 0x07)
          middle = 16;
        else
          middle = 4;

        myPOSM0 = (myPOSP0 + middle) % 160;
        myCurrentM0Mask = &ourMissleMaskTable[myPOSM0 & 0x03]
            [myNUSIZ0 & 0x07][(myNUSIZ0 & 0x30) >> 4][160 - (myPOSM0 & 0xFC)];
      }

      myRESMP0 = value & 0x02;

      if(myENAM0 && !myRESMP0)
        myEnabledObjects |= myM0Bit;
      else
        myEnabledObjects &= ~myM0Bit;

      break;
    }

    case 0x29:    // Reset missle 1 to player 1
    {
      if(myRESMP1 && !(value & 0x02))
      {
        uInt16 middle;

        if((myNUSIZ1 & 0x07) == 0x05)
          middle = 8;
        else if((myNUSIZ1 & 0x07) == 0x07)
          middle = 16;
        else
          middle = 4;

        myPOSM1 = (myPOSP1 + middle) % 160;
        myCurrentM1Mask = &ourMissleMaskTable[myPOSM1 & 0x03]
            [myNUSIZ1 & 0x07][(myNUSIZ1 & 0x30) >> 4][160 - (myPOSM1 & 0xFC)];
      }

      myRESMP1 = value & 0x02;

      if(myENAM1 && !myRESMP1)
        myEnabledObjects |= myM1Bit;
      else
        myEnabledObjects &= ~myM1Bit;
      break;
    }

    case 0x2A:    // Apply horizontal motion
    {
      // Figure out what cycle we're at
      Int32 x = ((s_clock - myClockWhenFrameStarted) % 228) / 3;

      // See if we need to enable the HMOVE blank bug
      if(myAllowHMOVEBlanks && ourHMOVEBlankEnableCycles[x])
      {
        // TODO: Allow this to be turned off using properties...
        myHMOVEBlankEnabled = true;
      }

      myPOSP0 += ourCompleteMotionTable[x][(Int32) myHMP0];
      myPOSP1 += ourCompleteMotionTable[x][(Int32) myHMP1];
      myPOSM0 += ourCompleteMotionTable[x][(Int32) myHMM0];
      myPOSM1 += ourCompleteMotionTable[x][(Int32) myHMM1];
      myPOSBL += ourCompleteMotionTable[x][(Int32) myHMBL];

      if(myPOSP0 >= 160)
        myPOSP0 -= 160;
      else if(myPOSP0 < 0)
        myPOSP0 += 160;

      if(myPOSP1 >= 160)
        myPOSP1 -= 160;
      else if(myPOSP1 < 0)
        myPOSP1 += 160;

      if(myPOSM0 >= 160)
        myPOSM0 -= 160;
      else if(myPOSM0 < 0)
        myPOSM0 += 160;

      if(myPOSM1 >= 160)
        myPOSM1 -= 160;
      else if(myPOSM1 < 0)
        myPOSM1 += 160;

      if(myPOSBL >= 160)
        myPOSBL -= 160;
      else if(myPOSBL < 0)
        myPOSBL += 160;

      myCurrentBLMask = &ourBallMaskTable[myPOSBL & 0x03]
          [(myCTRLPF & 0x30) >> 4][160 - (myPOSBL & 0xFC)];

      myCurrentP0Mask = &ourPlayerMaskTable[myPOSP0 & 0x03]
          [0][myNUSIZ0 & 0x07][160 - (myPOSP0 & 0xFC)];
      myCurrentP1Mask = &ourPlayerMaskTable[myPOSP1 & 0x03]
          [0][myNUSIZ1 & 0x07][160 - (myPOSP1 & 0xFC)];

      myCurrentM0Mask = &ourMissleMaskTable[myPOSM0 & 0x03]
          [myNUSIZ0 & 0x07][(myNUSIZ0 & 0x30) >> 4][160 - (myPOSM0 & 0xFC)];
      myCurrentM1Mask = &ourMissleMaskTable[myPOSM1 & 0x03]
          [myNUSIZ1 & 0x07][(myNUSIZ1 & 0x30) >> 4][160 - (myPOSM1 & 0xFC)];

      // Remember what clock HMOVE occured at
      myLastHMOVEClock = s_clock;

      // Disable TIA M0 "bug" used for stars in Cosmic ark
      myM0CosmicArkMotionEnabled = false;
      break;
    }

    case 0x2b:    // Clear horizontal motion registers
    {
      myHMP0 = 0;
      myHMP1 = 0;
      myHMM0 = 0;
      myHMM1 = 0;
      myHMBL = 0;
      break;
    }

    case 0x2c:    // Clear collision latches
    {
      myCollision = 0;
      break;
    }

    default:
    {
      //cerr << "BAD TIA Poke: " << hex << addr << endl;
      break;
    }
  }
}
