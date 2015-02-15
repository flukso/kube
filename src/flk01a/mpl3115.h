#ifndef __MPL3115_H__
#define __MPL3115_H__

#include "i2c.h"

#define MPL3115_ADDRESS 0x60
#define MPL3115_REG_P_OUT 0x01
#define MPL3115_REG_WHOAMI 0x0C
#define MPL3115_REG_CTRL_REG1 0x26

#define MPL3115_CMD_ONE_SHOT 0x22

#define MPL3115_SAMPLE_ERR 0xFFFFFFFF

ErrorCode_t mpl3115_whoami(void);
uint8_t mpl3115_sample_pressure(uint32_t *sample);

#endif

