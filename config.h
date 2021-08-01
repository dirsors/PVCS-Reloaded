/*
 * Originally by
 *   Jarkko Sonninen (sonninen@lut.fi)
 *
 * Modified by
 *   Alex Hornby (wilhor@dcs.warwick.ac.uk)
 */

/* ------------------------------------------------------------------------- *
 *
 * x2600 Configuration Set Up
 * Recommendations for each define are shown after them, so
 * that you can restore the settings.
 *
 * Use 'xmkmf' to update the Makefile.
 *
 * ------------------------------------------------------------------------- */

/*
 * Interfaces, Windows
 */


/*
 * X11 front-end for the debugger
 *
 *   (not recommended if the new 'xdebug.c' compiles.)
 */


/*#define XDEBUGGER		no */

/* ------------------------------------------------------------------------- */

/*
 * CPU
 */


/*
 * define either PAL or NTSC 
 */

/*#define PAL*/			/* yes */
#define NTSC		/* no */


/*
 * "Hacked" and faster CPU emulation
 * For CPU debugging, NORMAL (preferred) or HACK2 should be used.
 */

#define HACK2			/* yes */


/*
 * If you do NOT want to emulate undocumented commands, uncomment
 * the line below, and also select "NORMAL" CPU emulation.
 */

/*#define NO_UNDOC_CMDS*/	/* no */


/*
 * Uncommenting following line causes some debug information to be printed
 * and with 'NORMAL', execution of different 6510 intructions can be counted.
 */

/*#define DEBUG*/		/* no */


/* ------------------------------------------------------------------------- */

/*
 * Joystick (needs the joystick module for Linux)
 */

/*#define JOYSTICK*/		/* Linux only */

/* ------------------------------------------------------------------------- */

/*
 * Miscellaneous
 */

/*
 * Trap idle
 */

/*#define IDLE_TRAP*/		/* no */


/* ------------------------------------------------------------------------- */
/* #define VERBOSE
#ifndef _VERBOSE
#define _VERBOSE
#endif */


