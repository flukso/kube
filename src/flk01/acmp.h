#ifndef __ACMP_H__
#define __ACMP_H__

#include "LPC8xx.h"
#include "spin.h"
#include "debug.h"

void acmp_init(void);
uint8_t acmp_sample(void);

#endif
