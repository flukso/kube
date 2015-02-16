#ifndef __MMA8452_H__
#define __MMA8452_H__

#include "i2c.h"

#define MMA8452_ADDRESS 0x1C
#define MMA8452_REG_WHOAMI 0x0D

ErrorCode_t mma8452_whoami(void);

#endif

