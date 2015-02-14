#ifndef __I2C_H__
#define __I2C_H__

#include <stdint.h>
#include "romapi_8xx.h"
#include "rom_i2c_8xx.h"

#include "main.h"
#include "spin.h"
#include "debug.h"
#include "htu21d.h"
#include "vcnl4k.h"

#define I2C_CLOCKRATE 100000UL
#define I2C_TIMEOUT 1000UL
#define I2C_REG_NULL 0xFF

struct i2c_s {
    I2C_HANDLE_T *handle;
    uint32_t mem[24];
    ErrorCode_t err_code;
    uint8_t ready;
};

struct i2c_slave_s {
    uint8_t addr;
    const char *name;
};

void I2C_IRQHandler(void);

void i2c_init(void);
void i2c_bus_clear(void);
ErrorCode_t i2c_write(uint8_t addr, uint8_t reg, uint8_t cmd);
ErrorCode_t i2c_read(uint8_t addr, uint8_t rx_buffer[], size_t rx_count);

#endif

