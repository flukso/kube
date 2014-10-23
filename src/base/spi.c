/**************************************************************************/
/*!
    @file     spi.c
    @author   K. Townsend

    @section LICENSE

    Software License Agreement (BSD License)

    Copyright (c) 2013, K. Townsend (microBuilder.eu)
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    3. Neither the name of the copyright holders nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/**************************************************************************/
#include <string.h>

#include "spi.h"

#define CFG_ENABLE          (1 << 0)
#define CFG_MASTER          (1 << 2)

#define TXDATCTL_EOT        (1 << 20)
#define TXDATCTL_FSIZE(s)   ((s) << 24)

#define STAT_RXRDY          (1 << 0)
#define STAT_TXRDY          (1 << 1)

void spiInit () {
  /* Enable SPI clock */
  LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 11);

  /* Peripheral reset control to SPI0, a "1" bring it out of reset. */
  LPC_SYSCON->PRESETCTRL &= ~(1 << 0);
  LPC_SYSCON->PRESETCTRL |= (1 << 0);
  
  // 7.5 MHz, i.e. 30 MHz / 4
  LPC_SPI0->DIV = 3;
  LPC_SPI0->DLY = 0;

  LPC_SPI0->CFG = CFG_MASTER;
  LPC_SPI0->CFG |= CFG_ENABLE;
}

uint16_t spiTransfer (uint16_t cmd) {
  // while ((LPC_SPI0->STAT & STAT_TXRDY) == 0)
  //   ;
  LPC_SPI0->TXDATCTL = TXDATCTL_FSIZE(16-1) | TXDATCTL_EOT | cmd;
  while ((LPC_SPI0->STAT & STAT_RXRDY) == 0)
    ;
  return LPC_SPI0->RXDAT;
}
