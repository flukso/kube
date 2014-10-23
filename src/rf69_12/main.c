#if LPC_MAX + LPC_NXP + LPC_JEEa + LPC_JEE != 1
#error must define one of LPC_MAX, LPC_NXP, LPC_JEEa, or LPC_JEE
#endif

#if LPC_MAX
#define REMOTE_TYPE 0x200
#else
#define REMOTE_TYPE 0x300
#endif

#define PAIRING_GROUP 212

#define __VTOR_PRESENT 1

#include "LPC8xx.h"
#include <stdio.h>
#include "uart.h"
#include "rf69_12.h"
#include "iap_driver.h"

// #define printf(...)
#define dump(...)

static volatile uint32_t msTicks;

void SysTick_Handler (void) {
  ++msTicks;
}

static uint32_t millis () {
  return msTicks;
}

static void sleep (uint32_t ms) {
  // TODO: enter low-power sleep mode
  uint32_t now = millis();
  while ((millis() - now) < ms)
    ;
  // TODO: exit low-power sleep mode
}

extern uint16_t _crc16_update (uint16_t crc, uint8_t data);

uint32_t hwId [4];
  
#include "boot.h"

static void configurePins (void) {
  /* Enable clocks to IOCON & SWM */
  LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 18) | (1 << 7);
  /* Pin Assign 8 bit Configuration */
#ifndef printf
  /* U0_TXD */
  /* U0_RXD */
#if LPC_JEEa || LPC_JEE
  LPC_SWM->PINASSIGN0 = 0xffff0004UL; 
#else
  LPC_SWM->PINASSIGN0 = 0xffff0106UL; 
#endif
#endif
#if LPC_MAX
  // irq 8 ?
  /* SPI0_SCK 12 */
  LPC_SWM->PINASSIGN3 = 0x0cffffffUL; 
  /* SPI0_MOSI 14 */
  /* SPI0_MISO 15 */
  /* SPI0_SSEL 13 */
  LPC_SWM->PINASSIGN4 = 0xff0d0f0eUL;
#endif
#if LPC_NXP
  /* SPI0_SCK 14 */
  LPC_SWM->PINASSIGN3 = 0x0effffffUL; 
  /* SPI0_MOSI 13 */
  /* SPI0_MISO 12 */
  /* SPI0_SSEL 10 */
  LPC_SWM->PINASSIGN4 = 0xff0a0c0dUL;
#endif
#if LPC_JEEa
  /* SPI0_SCK 6 */
  LPC_SWM->PINASSIGN3 = 0x06ffffffUL; 
  /* SPI0_MOSI 9 */
  /* SPI0_MISO 8 */
  /* SPI0_SSEL 7 */
  LPC_SWM->PINASSIGN4 = 0xff070809UL;
#endif
#if LPC_JEE
  /* SPI0_SCK 6 */
  LPC_SWM->PINASSIGN3 = 0x06ffffffUL; 
  /* SPI0_MOSI 9 */
  /* SPI0_MISO 11 */
  /* SPI0_SSEL 8 */
  LPC_SWM->PINASSIGN4 = 0xff080b09UL;
#endif
}

static void launchApp() {
  printf("launchApp\n");
  SCB->VTOR = (uint32_t) BASE_ADDR;
  // __asm("LDR SP, [R0]    ;Load new stack pointer address")
  void (*fun)() = (void (*)()) ((uint32_t*) BASE_ADDR)[1];
  printf("go!\n");
  fun();
  printf("launchApp failed\n");
}

int main (void) {
  configurePins();
#ifndef printf
  uart0Init(57600);
#endif
  
  SysTick_Config(__SYSTEM_CLOCK/1000-1);   // 1000 Hz

  printf("clock %lu\n", __SYSTEM_CLOCK);
  // int e = iap_init();
  // printf("iap init %d\n", e);
  uint32_t partId;
  iap_read_part_id(&partId);
  printf("part id 0x%04X\n", (int) partId);
  iap_read_unique_id(hwId);
  
  bootLoader();
  launchApp();
  
  return 0;
}
