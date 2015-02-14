#ifndef __VCNL4000_H__
#define __VCNL4000_H__

#include "i2c.h"

#define VCNL4000_ADDRESS 0x13
#define VCNL4000_REG_COMMAND 0x80
#define VCNL4000_REG_PID 0x81
#define VCNL4000_REG_IR_LED_CURRENT 0x83
#define VCNL4000_REG_LIGHT_PAR 0x84
#define VCNL4000_REG_LIGHT_RESULT 0x85
#define VCNL4000_REG_PROX_RESULT 0x87
#define VCNL4000_REG_PROX_FREQ 0x89
#define VCNL4000_REG_PROX_TIMING 0x8A

#define VCNL4000_CMD_START_ALS 0x10

#define VCNL4000_SAMPLE_ERR 0xFFFF

ErrorCode_t vcnl4000_read_pid(void);
uint8_t vcnl4000_sample_light(uint16_t *sample);

#endif

