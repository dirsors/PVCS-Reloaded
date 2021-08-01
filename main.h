#ifndef _PSMS_H_
#define _PSMS_H_


#define WIDTH		320
#define HEIGHT		200

#define BOOT_CD 0
#define BOOT_MC 1
#define BOOT_HO 2
#define BOOT_HD 3

#define WAIT_PAD_READY(p, s) {while(padGetState((p),(s)) != PAD_STATE_STABLE) WaitForNextVRstart(1); }

// PSMS Logo
extern unsigned char __attribute__((aligned(16))) psms_image[];
extern unsigned char __attribute__((aligned(16))) psms_clut[];

#define psms_width 256
#define psms_height 128
#define psms_mode 1

// Menu font
extern unsigned char __attribute__((aligned(16))) vixar_image[];
extern unsigned char __attribute__((aligned(16))) vixar_clut[];
extern unsigned char vixarmet[];

#define vixar_width 256
#define vixar_height 256
#define vixar_mode 1

void LoadModules();
int InitPad(int port, int slot, char* buffer);
void TextOut(int x, int y, char *string, int z);
void TextOutC(int x_start, int x_end, int y, char *string, int z);

void display_error(char* errmsg, int fatal);

#endif /* _PSMS_H_ */
