TARGET   = BootMenu
TARGET_OBJS  = main.o blit/blit.o bm-loader/resume.o bm-loader/ldr_main.o blit/font.o
BOOTSTRAP_OBJS = bm-loader/payload_bootstrap.o

LIBS =	-ltaihenForKernel_stub -lSceSysclibForDriver_stub -lSceSysmemForDriver_stub \
	-lSceSysmemForKernel_stub -lSceThreadmgrForDriver_stub -lSceCpuForKernel_stub \
	-lSceCpuForDriver_stub -lSceUartForKernel_stub -lScePervasiveForDriver_stub \
	-lSceSysconForDriver_stub -lScePowerForDriver_stub -lSceIofilemgrForDriver_stub \
	-lSceSysrootForKernel_stub -lSceDebugForDriver_stub -lSceCtrlForDriver_stub \
	-lSceSblAIMgrForDriver_stub -lSceDisplayForDriver_stub -lSceRtcForDriver_stub \
	-lSceKernelSuspendForDriver_stub 

PREFIX  = arm-dolce-eabi
CC      = $(PREFIX)-gcc
AS      = $(PREFIX)-as
OBJCOPY = $(PREFIX)-objcopy
CFLAGS  = -Wl,-q -Wall -O0 -nostartfiles -mcpu=cortex-a9 -mthumb-interwork
ASFLAGS =

all: $(TARGET).skprx

%.skprx: %.velf
	dolce-make-fself -c $< $@

%.velf: %.elf
	dolce-elf-create -e $(TARGET).yml $< $@

payload_bootstrap.elf: $(BOOTSTRAP_OBJS)
	$(CC) -T bm-loader/payload_bootstrap.ld -nostartfiles -nostdlib $^ -o $@ -lgcc

payload_bootstrap.bin: payload_bootstrap.elf
	$(OBJCOPY) -S -O binary $^ $@

payload_bootstrap_bin.o: payload_bootstrap.bin
	$(OBJCOPY) -I binary -O elf32-littlearm --binary-architecture arm $^ $@

$(TARGET).elf: $(TARGET_OBJS) payload_bootstrap_bin.o
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

.PHONY: all clean send

clean:
	@rm -rf $(TARGET).skprx $(TARGET).velf $(TARGET).elf $(TARGET_OBJS) $(BOOTSTRAP_OBJS) \
	        payload_bootstrap.elf payload_bootstrap.bin payload_bootstrap_bin.o

send: $(TARGET).skprx
	curl --ftp-method nocwd -T $(TARGET).skprx ftp://$(PSVITAIP):1337/ux0:/data/tai/kplugin.skprx
	@echo "Sent."