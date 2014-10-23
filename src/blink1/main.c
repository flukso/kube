#include "LPC8xx.h"

volatile uint32_t msTicks;

void SysTick_Handler (void) {
  ++msTicks;
}

static void delayMillis (uint32_t ms) {
  uint32_t now = msTicks;
  while ((msTicks-now) < ms)
    ;
}

int main (void) {
  SysTick_Config(__SYSTEM_CLOCK/1000-1);   // 1000 Hz
  LPC_GPIO_PORT->DIR0 |= (1 << 7);

  while (1) {
    LPC_GPIO_PORT->NOT0 = 1 << 7;
    delayMillis(250);
  }
  
  return 0;
}
