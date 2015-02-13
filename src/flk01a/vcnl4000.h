#ifndef __VCNL4000_H__
#define __VCNL4000_H__

#include "i2c.h"

#define VCNL4000_ADDRESS 0x13
#define VCNL4000_CMD_READ_PID 0x81

ErrorCode_t vcnl4000_read_pid(void);

#endif

