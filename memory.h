#ifndef VCSMEMORY_H
#define VCSMEMORY_H

extern BYTE undecRead (ADDRESS a);

void decWrite ( ADDRESS a, BYTE b);

BYTE decRead (ADDRESS a);

BYTE dbgRead (ADDRESS a);

#endif
