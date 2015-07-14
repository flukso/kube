/*

  ekmb.c - EKMB config

  Copyright (c) 2015 Bart Van Der Meerssche <bart@flukso.net>

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject to
  the following conditions:

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include "ekmb.h"

void PININT0_IRQHandler(void)
{
    /* clear rising edge */
#define PINT0 0
    LPC_PIN_INT->IST |= (1 << PINT0);
    LPC_PMU->GPREG0 += 1;
}

void ekmb_init(void)
{
    /* disable pull-up */
#define MODE1 4
    LPC_IOCON->PIO0_15 &= ~(1 << MODE1);
#define EKMB_PIN 15
    LPC_SYSCON->PINTSEL[0] = EKMB_PIN;
    NVIC_EnableIRQ(PININT0_IRQn);
    NVIC_SetPriority(PININT0_IRQn, PRIO_HIGH);
    /* wake-up from power-down  */
    LPC_SYSCON->STARTERP0 |= (1 << PINT0);
     /* clear rising edge */
    LPC_PIN_INT->IST |= (1 << PINT0);
    /* enable rising edge int */
    LPC_PIN_INT->SIENR |= (1 << PINT0);
}

