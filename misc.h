/*
 * $Id: misc.h,v 1.1 1995/12/12 16:21:39 alex Exp alex $
 *
 * This file is part of Commodore 64 emulator.
 * See README for copyright notice
 *
 * This file contains misc funtions to help debugging.
 * Included are:
 *	o Show numeric conversions
 *	o Show CPU registers
 *	o Show Stack contents
 *	o Print binary number
 *	o Print instruction from memory
 *	o Decode instruction
 *	o Find effective address for operand
 *	o Create a copy of string
 *	o Move memory
 *
 * sprint_opcode returns mnemonic code of machine instruction.
 * sprint_binary returns binary form of given code (8bit)
 *
 *
 * Written by
 *   Vesa-Matti Puro (vmp@lut.fi)
 *   Jouko Valta (jopi@stekt.oulu.fi)
 *
 *
 * $Log: misc.h,v $
 * Revision 1.1  1995/12/12 16:21:39  alex
 * Initial revision
 *
 * Revision 1.1  1995/04/01  07:50:52  jopi
 * Initial revision
 *
 *
 */

#ifndef X64_MISC_H
#define X64_MISC_H

#include "types.h"


extern void    show_bases ( char *line, int mode );
extern void    print_stack ( BYTE sp );
extern char   *sprint_binary ( BYTE code );
extern char   *sprint_ophex ( ADDRESS p);
extern char   *sprint_opcode ( ADDRESS counter, int base );
extern char   *sprint_disassembled ( ADDRESS counter, BYTE x, BYTE p1, BYTE p2, int base );
extern int   eff_address(ADDRESS counter, int step);

#endif  /* X64_MISC_H */
