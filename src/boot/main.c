#define REMOTE_TYPE 0x4B12
#define PAIRING_GROUP 212

#define __VTOR_PRESENT 1

#include "LPC8xx.h"
#include <stdio.h>
#include "uart.h"
#include "rf69_12.h"
#include "iap_driver.h"

#ifndef DEBUG
#define printf(...)
#define dump(...)
#endif

static volatile uint32_t msTicks;

void SysTick_Handler (void) {
  ++msTicks;
}

static uint32_t millis () {
  return msTicks;
}

static void sleep (uint32_t ms) {
  uint32_t now = millis();
  while ((millis() - now) < ms)
    ;
}

extern uint16_t _crc16_update (uint16_t crc, uint8_t data);

uint32_t hwId [4];
  
#include "boot.h"

static void configurePins (void) {
  /* Enable clocks to IOCON & SWM */
  LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 18) | (1 << 7);
#ifndef printf
  /* UART0_TXD 9 */
  /* UART0_RXD 8  */
  LPC_SWM->PINASSIGN0 = 0xffff0809UL;
#endif
#define MODE0 3
#define MODE1 4
  /* SPI0_SCK 7 */
  LPC_IOCON->PIO0_7 &= ~(1 << MODE1);
  LPC_SWM->PINASSIGN3 = 0x07ffffffUL;
  /* SPI0_MOSI 4 */
  LPC_IOCON->PIO0_4 &= ~(1 << MODE1);
  /* SPI0_MISO 13 (repeater mode) */
  LPC_IOCON->PIO0_13 |= (1 << MODE0);
  /* SPI0_SSEL 14 */
  LPC_IOCON->PIO0_14 &= ~(1 << MODE1);
  LPC_SWM->PINASSIGN4 = 0xff0e0d04UL;
  /* IRQ 17 */
  LPC_IOCON->PIO0_17 &= ~(1 << MODE1);
  /* EKMB INT */
  LPC_IOCON->PIO0_15 &= ~(1 << MODE1);
  /* LED 0 */
#define LED_PIN 0
    LPC_GPIO_PORT->DIR0 |= (1 << LED_PIN);
#define MODE1 4
    LPC_IOCON->PIO0_0 &= ~(1 << MODE1);
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
  uart0Init(115200);
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
