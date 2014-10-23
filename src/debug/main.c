// Small test app to get some debug info out the serial port.

#include "LPC8xx.h"
#include <stdio.h>
#include "uart.h"

const int ledPin = 7;

volatile uint32_t msTicks;

void SysTick_Handler (void) {
  ++msTicks;
}

static void delayMillis (uint32_t ms) {
  uint32_t now = msTicks;
  while ((msTicks-now) < ms)
    ;
}

static void configurePins (void) {
  /* Enable clocks to IOCON & SWM */
  LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 18) | (1 << 7);
  /* U0_TXD */
  /* U0_RXD */
#if LPC_JEE || LPC_BES
  LPC_SWM->PINASSIGN0 = 0xffff0004UL;
#else
  LPC_SWM->PINASSIGN0 = 0xffff0106UL; 
#endif
}

int main (void) {
  configurePins();
  uart0Init(57600);
  
  SysTick_Config(__SYSTEM_CLOCK/1000-1);   // 1000 Hz
  LPC_GPIO_PORT->DIR0 |= (1 << ledPin);

  printf("clock %u\n", __SYSTEM_CLOCK);
    
  while (1) {
    LPC_GPIO_PORT->NOT0 = 1 << ledPin;
    delayMillis(500);
  }
  
  return 0;
}
