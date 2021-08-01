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
// $Id: TIA.hxx,v 1.14 2004/06/13 04:53:04 bwmott Exp $
//============================================================================

#ifndef TIA_HXX
#define TIA_HXX

#include <string.h>
#include <malloc.h>
#include "bspf.h"

typedef unsigned char bool;

#define false 0
#define true 1

uInt32 stella_cycles;
int tv_xstart, tv_ystart, tv_width, tv_height;

char *tv_format = "NTSC";

/**
  This class is a device that emulates the Television Interface Adapator 
  found in the Atari 2600 and 7800 consoles.  The Television Interface 
  Adapator is an integrated circuit designed to interface between an 
  eight bit microprocessor and a television video modulator. It converts 
  eight bit parallel data into serial outputs for the color, luminosity, 
  and composite sync required by a video modulator.  

  This class outputs the serial data into a frame buffer which can then
  be displayed on screen.

  @author  Bradford W. Mott
  @version $Id: TIA.hxx,v 1.14 2004/06/13 04:53:04 bwmott Exp $
*/
    /**
      Reset device to its power-on state
    */
    void reset();

    /**
      This method should be called at an interval corresponding to
      the desired frame rate to update the media source.
    */
    void update();

    /**
      Get the palette which maps frame data to RGB values.

      @return Array of integers which represent the palette (RGB)
    */
    const uInt32* palette();

    /**
      Answers the height of the frame buffer

      @return The frame's height
    */
    uInt32 height();

    /**
      Answers the width of the frame buffer

      @return The frame's width
    */
    uInt32 width();

    /**
      Answers the total number of scanlines the media source generated
      in producing the current frame buffer.

      @return The total number of scanlines generated
    */
    uInt32 scanlines();

    // Compute the ball mask table
    void computeBallMaskTable();

    // Compute the collision decode table
    void computeCollisionTable();

    // Compute the missle mask table
    void computeMissleMaskTable();

    // Compute the player mask table
    void computePlayerMaskTable();

    // Compute the player position reset when table
    void computePlayerPositionResetWhenTable();

    // Compute the player reflect table
    void computePlayerReflectTable();

    // Compute playfield mask table
    void computePlayfieldMaskTable();

    // Update the current frame buffer up to one scanline
    void updateFrameScanline(uInt32 clocksToUpdate, uInt32 hpos);

    // Update the current frame buffer to the specified color clock
    void updateFrame(Int32 clock);

    // Waste cycles until the current scanline is finished
    void waitHorizontalSync();

    // Draw message to framebuffer
    void drawMessageText();

    // Indicates whether the emulation is paused or not
    bool myPauseState;

    // Message timer
    Int32 myMessageTime;

    // Message text
    char *myMessageText;

    // Indicates if color loss should be enabled or disabled.  Color loss
    // occurs on PAL (and maybe SECAM) systems when the previous frame
    // contains an odd number of scanlines.
    bool myColorLossEnabled;

    // Pointer to the current frame buffer
    uInt8 *myCurrentFrameBuffer;

    // Pointer to the previous frame buffer
    uInt8 *myPreviousFrameBuffer;

    // Pointer to the next pixel that will be drawn in the current frame buffer
    uInt8 *myFramePointer;

    // Indicates where the scanline should start being displayed
    uInt32 myFrameXStart;

    // Indicates the width of the scanline 
    uInt32 myFrameWidth;

    // Indicated what scanline the frame should start being drawn at
    uInt32 myFrameYStart;

    // Indicates the height of the frame in scanlines
    uInt32 myFrameHeight;

    // Indicates offset in color clocks when display should begin
    uInt32 myStartDisplayOffset;

    // Indicates offset in color clocks when display should stop
    uInt32 myStopDisplayOffset;

    // Indicates color clocks when the current frame began
    Int32 myClockWhenFrameStarted;

    // Indicates color clocks when frame should begin to be drawn
    Int32 myClockStartDisplay;

    // Indicates color clocks when frame should stop being drawn
    Int32 myClockStopDisplay;

    // Indicates color clocks when the frame was last updated
    Int32 myClockAtLastUpdate;

    // Indicates how many color clocks remain until the end of 
    // current scanline.  This value is valid during the 
    // displayed portion of the frame.
    Int32 myClocksToEndOfScanLine;

    // Indicates the total number of scanlines generated by the last frame
    Int32 myScanlineCountForLastFrame;

    // Indicates the maximum number of scanlines to be generated for a frame
    Int32 myMaximumNumberOfScanlines;

    // Color clock when VSYNC ending causes a new frame to be started
    Int32 myVSYNCFinishClock; 

    enum tia_bits
    {
      myP0Bit = 0x01,         // Bit for Player 0
      myM0Bit = 0x02,         // Bit for Missle 0
      myP1Bit = 0x04,         // Bit for Player 1
      myM1Bit = 0x08,         // Bit for Missle 1
      myBLBit = 0x10,         // Bit for Ball
      myPFBit = 0x20,         // Bit for Playfield
      ScoreBit = 0x40,        // Bit for Playfield score mode
      PriorityBit = 0x080     // Bit for Playfield priority
    };

    // Bitmap of the objects that should be considered while drawing
    uInt8 myEnabledObjects;

    uInt8 myVSYNC;        // Holds the VSYNC register value
    uInt8 myVBLANK;       // Holds the VBLANK register value

    uInt8 myNUSIZ0;       // Number and size of player 0 and missle 0
    uInt8 myNUSIZ1;       // Number and size of player 1 and missle 1

    uInt8 myPlayfieldPriorityAndScore;
    uInt32 myColor[4];
    uInt8 myPriorityEncoder[2][256];

    uInt32  myCOLUBK;       // Background color register (replicated 4 times)
    uInt32  myCOLUPF;       // Playfield color register (replicated 4 times)
    uInt32  myCOLUP0;       // Player 0 color register (replicated 4 times)
    uInt32  myCOLUP1;       // Player 1 color register (replicated 4 times)

    uInt8 myCTRLPF;       // Playfield control register

    bool myREFP0;         // Indicates if player 0 is being reflected
    bool myREFP1;         // Indicates if player 1 is being reflected

    uInt32 myPF;           // Playfield graphics (19-12:PF2 11-4:PF1 3-0:PF0)

    uInt8 myGRP0;         // Player 0 graphics register
    uInt8 myGRP1;         // Player 1 graphics register
    
    uInt8 myDGRP0;        // Player 0 delayed graphics register
    uInt8 myDGRP1;        // Player 1 delayed graphics register

    bool myENAM0;         // Indicates if missle 0 is enabled
    bool myENAM1;         // Indicates if missle 0 is enabled

    bool myENABL;         // Indicates if the ball is enabled
    bool myDENABL;        // Indicates if the virtically delayed ball is enabled

    Int8 myHMP0;          // Player 0 horizontal motion register
    Int8 myHMP1;          // Player 1 horizontal motion register
    Int8 myHMM0;          // Missle 0 horizontal motion register
    Int8 myHMM1;          // Missle 1 horizontal motion register
    Int8 myHMBL;          // Ball horizontal motion register

    bool myVDELP0;        // Indicates if player 0 is being virtically delayed
    bool myVDELP1;        // Indicates if player 1 is being virtically delayed
    bool myVDELBL;        // Indicates if the ball is being virtically delayed

    bool myRESMP0;        // Indicates if missle 0 is reset to player 0
    bool myRESMP1;        // Indicates if missle 1 is reset to player 1

    uInt16 myCollision;    // Collision register

    // Note that these position registers contain the color clock 
    // on which the object's serial output should begin (0 to 159)
    Int16 myPOSP0;         // Player 0 position register
    Int16 myPOSP1;         // Player 1 position register
    Int16 myPOSM0;         // Missle 0 position register
    Int16 myPOSM1;         // Missle 1 position register
    Int16 myPOSBL;         // Ball position register

    // Graphics for Player 0 that should be displayed.  This will be
    // reflected if the player is being reflected.
    uInt8 myCurrentGRP0;

    // Graphics for Player 1 that should be displayed.  This will be
    // reflected if the player is being reflected.
    uInt8 myCurrentGRP1;

    // It's VERY important that the BL, M0, M1, P0 and P1 current
    // mask pointers are always on a uInt32 boundary.  Otherwise,
    // the TIA code will fail on a good number of CPUs.

    // Pointer to the currently active mask array for the ball
    uInt8 *myCurrentBLMask;

    // Pointer to the currently active mask array for missle 0
    uInt8 *myCurrentM0Mask;

    // Pointer to the currently active mask array for missle 1
    uInt8 *myCurrentM1Mask;

    // Pointer to the currently active mask array for player 0
    uInt8 *myCurrentP0Mask;

    // Pointer to the currently active mask array for player 1
    uInt8 *myCurrentP1Mask;

    // Pointer to the currently active mask array for the playfield
    uInt32 *myCurrentPFMask;

    // Indicates when the dump for paddles was last set
    Int32 myDumpDisabledCycle;

    // Indicates if the dump is current enabled for the paddles
    bool myDumpEnabled;

    // Color clock when last HMOVE occured
    Int32 myLastHMOVEClock;

    // Indicates if HMOVE blanks are currently enabled
    bool myHMOVEBlankEnabled;

    // Indicates if we're allowing HMOVE blanks to be enabled
    bool myAllowHMOVEBlanks;

    // TIA M0 "bug" used for stars in Cosmic Ark flag
    bool myM0CosmicArkMotionEnabled;

    // Counter used for TIA M0 "bug" 
    uInt32 myM0CosmicArkCounter;

    /**
      Answers the current frame buffer

      @return Pointer to the current frame buffer
    */
    //uInt8 *currentFrameBuffer() { return myCurrentFrameBuffer; }

    /**
      Answers the previous frame buffer

      @return Pointer to the previous frame buffer
    */
    //uInt8 *previousFrameBuffer() { return myPreviousFrameBuffer; }

#endif
