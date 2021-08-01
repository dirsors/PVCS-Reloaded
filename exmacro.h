#ifndef EXMACRO_H
#define EXMACRO_H

/* returns TRUE if an address increment will cross a page boundary */
#define pagetest(a,b) (LOWER(a) + b > 0xff)
#define load_abs_addr (LOADEXEC(PC+1)|(LOADEXEC(PC+2)<<8))
#define load_addr (LOAD((PC+1)|((PC+2)<<8)))
#define brtest(a) (UPPER(PC)==(a))
#define toBCD(a) ((((a % 100) / 10) << 4) | (a % 10))
#define fromBCD(a) (((a >> 4) & 0xf) * 10 + (a & 0xf))


#endif
