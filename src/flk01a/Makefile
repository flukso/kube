OBJS = main.o spin.o i2c.o htu21d.o vcnl4k.o ekmb.o acmp.o led.o \
		../base/rf69_12.o ../base/spi.o ../base/uart.o \
		../base/printf.o ../base/printf-retarget.o \
		../base/system_LPC8xx.o ../base/gcc_startup_lpc8xx.o

ifeq ($(DEBUG), 1)
LINK = LPC812-boot.ld
CFLAGS += -DDEBUG
else
LINK = LPC812-himem.ld
endif

include ../base/rules.mk
-include ../base/local.mk
-include local.mk
