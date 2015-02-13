#ifndef __HTU21D_H__
#define __HTU21D_H__

#include "i2c.h"

#define HTU21D_ADDRESS 0x40
#define HTU21D_CMD_TEMP_HOLD 0xE3
#define HTU21D_CMD_HUMID_HOLD 0xE5
#define HTU21D_CMD_TEMP_NO_HOLD 0xF3
#define HTU21D_CMD_HUMID_NO_HOLD 0xF5
#define HTU21D_CMD_READ_USER 0xE7
#define HTU21D_CMD_SOFT_RESET 0xFE
#define HTU21D_SAMPLE_ERR 0xFFFF

ErrorCode_t htu21d_soft_reset(void);
ErrorCode_t htu21d_read_user(void);
uint8_t htu21d_sample_temp(uint16_t *sample);
uint8_t htu21d_sample_humid(uint16_t *sample);

#endif

