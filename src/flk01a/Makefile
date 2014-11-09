OBJS = main.o ../base/rf69_12.o ../base/spi.o ../base/uart.o \
       ../base/printf.o ../base/printf-retarget.o \
			 ../base/system_LPC8xx.o ../base/gcc_startup_lpc8xx.o
LINK = LPC812-himem.ld

ifeq ($(DEBUG), 1)
CFLAGS += -DDEBUG
endif

include ../base/rules.mk
-include ../base/local.mk
-include local.mk
