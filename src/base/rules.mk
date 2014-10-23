# gcc Makefile for LPC812
# based on original file by Kamal Mostafa <kamal@whence.com>

CROSS = arm-none-eabi-
CPU = -mthumb -mcpu=cortex-m0plus
WARN = -Wall
STD = -std=gnu99
LINK ?= LPC812-boot.ld

CC = $(CROSS)gcc
CXX = $(CROSS)g++
LD = $(CROSS)ld
OBJCOPY = $(CROSS)objcopy
SIZE = $(CROSS)size

CFLAGS += $(CPU) $(WARN) $(STD) -MMD -I../base -DIRQ_DISABLE \
          -Os -ffunction-sections -fno-builtin -ggdb
CXXFLAGS += $(CPU) $(WARN) -MMD -I../base -DIRQ_DISABLE \
          -Os -ffunction-sections -fno-builtin -ggdb
CXXFLAGS += -fno-rtti -fno-exceptions

LDFLAGS += --gc-sections -Map=firmware.map --cref --library-path=../base
LIBGCC = $(shell $(CC) $(CFLAGS) --print-libgcc-file-name)

OS := $(shell uname)

ifeq ($(OS), Linux)
TTY = /dev/ttyUSB*
endif

ifeq ($(OS), Darwin)
TTY = /dev/tty.usbserial-*
LPCX = /Applications/lpcxpresso_6.1.0_164/lpcxpresso/bin
endif

all: firmware.bin

firmware.elf: ../base/$(LINK) $(OBJS)
	@$(LD) -o $@ $(LDFLAGS) -T ../base/$(LINK) $(OBJS) $(LIBGCC)
	$(SIZE) $@

clean:
	rm -f *.o *.d # firmware.{elf,hex,bin,map}
ifeq (${MAKELEVEL}, 0) # careful with recursion in ./base itself
	make -C ../base clean
endif

# this works for EA's LPC812 MAX board
flash: firmware.bin
	cp firmware.bin /Volumes/MBED/

# these two work with NXP's LPC812 board, using JLink
dfu:
	$(LPCX)/dfu-util -d 0x471:0xdf55 -c 0 -t 2048 -R -D $(LPCX)/LPCXpressoWIN.enc
lpcx: firmware.elf
	$(LPCX)/crt_emu_cm3_gen -vendor=NXP -pLPC812 -wire=winUSB \
	     -flash-load-exec firmware.elf
			 
# this works with NXP's LPC812 board, using serial ISP
isp: firmware.bin
	lpc21isp $(ISPOPTS) -control -bin firmware.bin $(TTY) 115200 12000

.PHONY: all clean flash dfu lpcx isp
  
%.bin:%.elf
	@$(OBJCOPY) --strip-unneeded -O ihex firmware.elf firmware.hex
	@$(OBJCOPY) --strip-unneeded -O binary firmware.elf firmware.bin

-include $(OBJS:.o=.d)
