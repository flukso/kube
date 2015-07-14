/*

  acmp.c - analog comparator driver

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

#include "acmp.h"

void acmp_init(void)
{
#define ACMPS 19
    LPC_SYSCON->SYSAHBCLKCTRL |= (1 << ACMPS);
#define ACMP_RST_N 12
    LPC_SYSCON->PRESETCTRL &= ~(1 << ACMP_RST_N);
    LPC_SYSCON->PRESETCTRL |= (1 << ACMP_RST_N);
#define COMP_VP_SEL 8
    LPC_CMP->CTRL = (0x6 << COMP_VP_SEL);
}

static uint8_t acmp_compare(uint8_t ladder)
{
#define LADMASK 0x1F
    ladder &= LADMASK;
#define LADEN 0
#define LADSEL 1
    LPC_CMP->LAD = (1 << LADEN) | (ladder << LADSEL);
    spin(2);
#define COMPSTAT 21
    return (LPC_CMP->CTRL >> COMPSTAT) & 0x1;
}

uint8_t acmp_sample(void)
{
#define ACMP 15
    LPC_SYSCON->PDRUNCFG &= ~(1 << ACMP);
    uint8_t ladder = 0;
    for (int i = 0; i < 5; i++) {
        ladder += (acmp_compare(ladder + (1 << (4 - i))) << (4 - i));
    }
    LPC_SYSCON->PDRUNCFG |= (1 << ACMP);
    printf("[acmp] ladder: %u\n", ladder);
    return ladder;
}
