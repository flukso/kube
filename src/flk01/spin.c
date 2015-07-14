/*

  spin.c - cpu spinning

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

#include "spin.h"

volatile uint32_t mtime = 0;

void SysTick_Handler(void)
{
    ++mtime;
}

void systick_init(void)
{
    /* 1000Hz */
    SysTick_Config(__SYSTEM_CLOCK / 1000 - 1);
    NVIC_SetPriority(SysTick_IRQn, PRIO_SYSTICK);
}

void spin(uint32_t ms)
{
    uint32_t start = mtime;
    uint32_t stop = start + ms;
    if (stop < start) {         /* overflow */
        while (mtime > start);
    }
    while (mtime < stop);
}
