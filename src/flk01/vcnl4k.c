/*

  vcnl4k.c - VCNL4K i2c driver

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

#include "vcnl4k.h"

void vcnl4k_init(void)
{
    /* disable pull-up */
#define MODE1 4
    LPC_IOCON->PIO0_16 &= ~(1 << MODE1);
}

ErrorCode_t vcnl4k_read_pid(void)
{
    uint8_t rx_buffer[2];
    ErrorCode_t err_code;
    err_code = i2c_write(VCNL4K_ADDRESS, I2C_REG_NULL, VCNL4K_REG_PID);
    if (err_code != LPC_OK) {
        return err_code;
    }
    return i2c_read(VCNL4K_ADDRESS, rx_buffer, sizeof(rx_buffer));
}

static ErrorCode_t vcnl4k_sample(uint16_t *sample)
{
    uint8_t rx_buffer[3];
    ErrorCode_t err_code;
    err_code =
        i2c_write(VCNL4K_ADDRESS, VCNL4K_REG_COMMAND, VCNL4K_CMD_START_ALS);
    if (err_code != LPC_OK) {
        return err_code;
    }
    spin(105);
    err_code = i2c_write(VCNL4K_ADDRESS, I2C_REG_NULL, VCNL4K_REG_LIGHT_RESULT);
    if (err_code != LPC_OK) {
        return err_code;
    }
    err_code = i2c_read(VCNL4K_ADDRESS, rx_buffer, sizeof(rx_buffer));
    *sample = (rx_buffer[1] << 8) | rx_buffer[2];
    return err_code;
}

uint8_t vcnl4k_sample_light(uint16_t *sample)
{
    unsigned int light;
    ErrorCode_t err_code;
    err_code = vcnl4k_sample(sample);
    if (err_code == LPC_OK) {
        light = *sample * 250;
        printf("[light] %umlx\n", light);
        return 0;
    }
    *sample = VCNL4K_SAMPLE_ERR;
    return 1;
}
