// Driver for the RF69 radio chip, microcontroller-specific code.
// 2013-11-06 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

#include "LPC8xx.h"

#define CFG_ENABLE          (1 << 0)
#define CFG_MASTER          (1 << 2)

#define TXDATCTL_EOT        (1 << 20)
#define TXDATCTL_FSIZE(s)   ((s) << 24)

#define STAT_RXRDY          (1 << 0)
#define STAT_TXRDY          (1 << 1)

static void spiInit (void) {
  /* Enable SPI clock */
  LPC_SYSCON->SYSAHBCLKCTRL |= (1<<11);

  /* Peripheral reset control to SPI, a "1" bring it out of reset. */
  LPC_SYSCON->PRESETCTRL &= ~(0x1<<0);
  LPC_SYSCON->PRESETCTRL |= (0x1<<0);
  
  // 10 MHz, i.e. 30 MHz / 3
  LPC_SPI0->DIV = 2;
  LPC_SPI0->DLY = 0;

  LPC_SPI0->CFG = CFG_MASTER;
  LPC_SPI0->CFG |= CFG_ENABLE;
}

static uint16_t spiTransfer (uint16_t cmd) {
  // while ((LPC_SPI0->STAT & STAT_TXRDY) == 0)
  //   ;
  LPC_SPI0->TXDATCTL = TXDATCTL_FSIZE(16-1) | TXDATCTL_EOT | cmd;
  while ((LPC_SPI0->STAT & STAT_RXRDY) == 0)
    ;
  return LPC_SPI0->RXDAT;
}
