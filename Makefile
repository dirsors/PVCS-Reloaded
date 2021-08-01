EE_OBJS = cpu.o display.o files.o \
	keyboard.o memory.o md5.o main.o misc.o mouse.o \
	cdvd_irx.o audsrv_irx.o table.o tia.o tiasound.o \
	vmachine.o \
	browser/browser.o browser/cd.o browser/bdraw.o \
	browser/font_uLE.o browser/init.o browser/pad.o \
	browser/ps2font.o browser/cnfsettings.o browser/menu.o \
	browser/Reboot_ELF.o \
	iomanX_irx.o fileXio_irx.o ps2dev9_irx.o ps2atad_irx.o \
	ps2hdd_irx.o ps2fs_irx.o poweroff_irx.o usbd_irx.o \
	usbhdfsd_irx.o

EE_LDFLAGS += -L$(PS2DEV)/gskit/lib -L$(PS2DEV)/ps2sdk/ports/lib -s
#EE_LIBS        = -lmc -lcdvdfs -lpad -lc #-lg2
EE_LIBS = -lgskit -ldmakit -ljpg -lpng -lz -lm -lfileXio -lhdd -lmc -lpadx -lpoweroff -lpatches -ldebug -lcdvdfs -laudsrv
EE_CFLAGS   = -DVERBOSE -DSTELLA_TIA -DPS2_EE #-I./libg2




EE_INCS += -I. -I./browser 
EE_INCS += -I$(PS2SDK)/sbv/include -I$(PS2SDK)/ports/include -I$(PS2DEV)/gsKit/include


.SUFFIXES: .c .s .cc .dsm

EE_BIN = pvcs.elf

#all: build-libg2 $(EE_BIN)
all: $(EE_BIN)

#build-libg2:
#	@echo Building libg2
#	$(MAKE) -C libg2

cdvd_irx.s:
	bin2s $(PS2SDK)/iop/irx/cdvd.irx cdvd_irx.s cdvd_irx

iomanX_irx.s:
	bin2s $(PS2SDK)/iop/irx/iomanX.irx iomanX_irx.s iomanX_irx

fileXio_irx.s:
	bin2s $(PS2SDK)/iop/irx/fileXio.irx fileXio_irx.s fileXio_irx

ps2dev9_irx.s:
	bin2s $(PS2SDK)/iop/irx/ps2dev9.irx ps2dev9_irx.s ps2dev9_irx

ps2atad_irx.s:
	bin2s $(PS2SDK)/iop/irx/ps2atad.irx ps2atad_irx.s ps2atad_irx

ps2hdd_irx.s:
	bin2s $(PS2SDK)/iop/irx/ps2hdd.irx ps2hdd_irx.s ps2hdd_irx

ps2fs_irx.s:
	bin2s $(PS2SDK)/iop/irx/ps2fs.irx ps2fs_irx.s ps2fs_irx

poweroff_irx.s:
	bin2s $(PS2SDK)/iop/irx/poweroff.irx poweroff_irx.s poweroff_irx

usbd_irx.s:
	bin2s $(PS2SDK)/iop/irx/usbd.irx usbd_irx.s usbd_irx

usbhdfsd_irx.s:
	bin2s $(PS2SDK)/iop/irx/usbhdfsd.irx usbhdfsd_irx.s usbhdfsd_irx

#sjpcm_irx.s:
#	bin2s sjpcm.irx sjpcm_irx.s sjpcm_irx

audsrv_irx.s:
	bin2s $(PS2SDK)/iop/irx/audsrv.irx audsrv_irx.s audsrv_irx


clean:
	#$(MAKE) -C libg2 clean
	rm -f *.elf *.o *.a *.s browser/*.o

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
