#ifndef _MENU_H_
#define _MENU_H_

// Defines related to the starfield
#define SF_SIZE		500
#define Z_MAX  		100
#define SPEED  		1

#define Z_MAX5		(Z_MAX - 50)
#define Z_MAX4		(Z_MAX - 32)
#define Z_MAX3		(Z_MAX - 24)
#define Z_MAX2		(Z_MAX - 16)
#define Z_MAX1		(Z_MAX - 8)

typedef struct {
	long x, y, z;
}t_starfield;

struct ROMdata {
	char name[80]; // These values should be more than enough
	char filename[256];
};

extern int whichdrawbuf;

char* menu_main();
int menu_update_input();
void menu_draw_stars();
void menu_create_star(int i);
void menu_init_stars();

char* ChooseDir();
int ProcessROMlist();
int ProcessROMscan(char *dir);

void display_error();

#endif /* _PSMS_H_ */
