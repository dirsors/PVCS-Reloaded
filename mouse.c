#ifdef DOS
#include <dos.h>
#include <bios.h>
#include <stdio.h>
#include <math.h>
#endif


#ifdef WINDOWS
int init_mouse() {
#ifdef DOS
	union REGS inregs,outregs;

	inregs.x.ax = 0x00; // subfunction 0: reset
	int86(0x33, &inregs, &outregs);
	return(outregs.x.ax);    // return overall success/failure
#endif
}
#endif

#ifdef WINDOWS
int	mouse_position() {
#ifdef DOS
	union REGS inregs,outregs;


	inregs.x.ax = 0x03;
	int86(0x33, &inregs, &outregs);
	return(outregs.x.cx);
#endif
}
#endif

#ifdef WINDOWS
int mouse_button() {
#ifdef DOS
	union REGS inregs,outregs;


	inregs.x.ax = 0x03;
	int86(0x33, &inregs, &outregs);
	return(outregs.x.bx);
#endif
}
#endif

void mouse_sensitivity(int sens) {
#ifdef DOS
	union REGS inregs,outregs;

	inregs.x.bx = sens;
	inregs.x.cx = sens;
	inregs.x.dx = 1;
	inregs.x.ax = 0x1A;
	int86(0x33, &inregs, &outregs);
#endif
}

