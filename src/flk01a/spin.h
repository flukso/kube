#ifndef __SPIN_H__
#define __SPIN_H__

#include "LPC8xx.h"
#include "main.h"

extern volatile uint32_t mtime;

void SysTick_Handler(void);
void systick_init(void);
void spin(uint32_t ms);

#endif
