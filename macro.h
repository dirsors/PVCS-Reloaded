/*
 * $Id: macro.h,v 1.4 1996/02/26 17:41:48 alex Exp alex $
 *
 * This file is part x2600.
 *
 * Originally from x64 by
 *   Vesa-Matti Puro (vmp@lut.fi)
 *   Jarkko Sonninen (sonninen@lut.fi)
 * 
 * Modified by
 *   Alex Hornby
 * NOTE: Can add zero page optimizations
 */

#ifndef X2600_MACRO_H
#define X2600_MACRO_H


#include "types.h"
#include "extern.h"
#include "cpu.h"

#define LOAD(a)      		decRead(a)
#define LOADEXEC(a)      	undecRead(a)
#define DLOAD(a)                dbgRead(a)
#define LOAD_ZERO(a) 		decRead((ADDRESS)a)
#define LOAD_ADDR(a)		((LOADEXEC(a+1)<<8)+LOADEXEC(a))
#define LOAD_ZERO_ADDR(a)	LOAD_ADDR(a)

#define STORE(a,b)     	 	decWrite((a),(b))
/*#define STORE_ZERO(a,b)		ram[(a)&0xff]=(b)*/
#define STORE_ZERO(a,b)         decWrite((a),(b))


#define PUSH(b) 		decWrite(SP+0x100,(b));SP--
#define PULL()			decRead((++SP)+0x100)

#define UPPER(ad)		(((ad)>>8)&0xff)
#define LOWER(ad)		((ad)&0xff)
#define LOHI(lo,hi)             ((lo)|((hi)<<8))

extern int pagetest( ADDRESS a, BYTE b);
extern int brtest( BYTE a);

#define REL_ADDR(pc,src) 	(pc+((SIGNED_CHAR)src))
/*#define REL_ADDR(pc,src) 	(pc+((src&0x80)?-1-(src^0xff):src))*/


#define SET_SIGN(a)		(SF=(a)&S_SIGN)
#define SET_ZERO(a)		(ZF=!(a))
#define SET_CARRY(a)  		(CF=(a))

#define SET_INTERRUPT(a)	(IF=(a))
#define SET_DECIMAL(a)		(DF=(a))
#define SET_OVERFLOW(a)		(OF=(a))
#define SET_BREAK(a)		(BF=(a))

#define SET_SR(a)		(SF=(a) & S_SIGN,ZF=(a) & S_ZERO,CF=(a) & S_CARRY,IF=(a) & S_INTERRUPT,DF=(a) & S_DECIMAL,OF=(a) & S_OVERFLOW,BF=(a) & S_BREAK)

#define GET_SR()		((SF ? S_SIGN : 0) | (ZF ? S_ZERO : 0) | (CF ? S_CARRY : 0) | (IF ? S_INTERRUPT : 0) | (DF ? S_DECIMAL : 0) | (OF ? S_OVERFLOW : 0) | (BF ? S_BREAK : 0) | S_NOTUSED)

#define IF_SIGN()		SF
#define IF_ZERO()		ZF
#define IF_CARRY()		CF
#define IF_INTERRUPT()		IF
#define IF_DECIMAL()		DF
#define IF_OVERFLOW()		OF
#define IF_BREAK()		BF


#define sprint_status()	 sprint_binary(GET_SR())

#define LOAD_ABS_X(addr)                                     \
   ((((addr) & 0xff) + XR) > 0xff                            \
    ? (LOAD(((addr) & 0xff00) | (((addr) + XR) & 0xff)),     \
       LOAD((addr) + XR))                                    \
    : LOAD((addr) + XR))

#define LOAD_ABS_Y(addr)                                     \
   ((((addr) & 0xff) + YR) > 0xff                            \
    ? (LOAD(((addr) & 0xff00) | (((addr) + YR) & 0xff)),     \
       LOAD((addr) + YR))                                    \
    : LOAD((addr) + YR))

#endif  /* X2600_MACRO_H */
