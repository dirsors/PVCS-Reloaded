/*
  The data is held here.
*/

extern struct resource {
	int rr;
	int debug;
	char bank;
	char pad_sens;
	char sc;
	char autoconfig;
	char left;
	char right;
	char swap;
} app_data;

#define STICK 	0x01
#define PADDLE 	0x02
#define KEYPAD  0x03

/* These can be used for 'input.pad[]' */
#define INPUT_UP          (0x00000001)
#define INPUT_DOWN        (0x00000002)
#define INPUT_LEFT        (0x00000004)
#define INPUT_RIGHT       (0x00000008)
#define INPUT_BUTTON2     (0x00000010)
#define INPUT_BUTTON1     (0x00000020)
#define INPUT_R1          (0x00000040)
#define INPUT_R2          (0x00000080)
#define INPUT_L1          (0x00000100)
#define INPUT_L2          (0x00000200)

/* These can be used for 'input.system' */
#define INPUT_START       (0x00000001)
#define INPUT_PAUSE       (0x00000002)
#define INPUT_SOFT_RESET  (0x00000004)
#define INPUT_HARD_RESET  (0x00000008)
#define INPUT_SELECT      (0x00000010)

/* User input structure */
typedef struct
{
    int pad[2];
    int system;
}t_input;
