OBJS = main.o ../base/uart.o \
       ../base/printf.o ../base/printf-retarget.o \
			 ../base/system_LPC8xx.o ../base/gcc_startup_lpc8xx.o
       
include ../base/rules.mk

CFLAGS += -DLPC_BES

-include ../base/local.mk
-include local.mk
