/*

  htu21d.c - HTU21D i2c driver

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

#include "htu21d.h"

ErrorCode_t htu21d_soft_reset(void)
{
    return i2c_write(HTU21D_ADDRESS, I2C_REG_NULL, HTU21D_CMD_SOFT_RESET);
}

ErrorCode_t htu21d_read_user(void)
{
    uint8_t rx_buffer[2];
    ErrorCode_t err_code;
    err_code = i2c_write(HTU21D_ADDRESS, I2C_REG_NULL, HTU21D_CMD_READ_USER);
    if (err_code != LPC_OK) {
        return err_code;
    }
    return i2c_read(HTU21D_ADDRESS, rx_buffer, sizeof(rx_buffer));
}

static ErrorCode_t htu21d_sample(uint8_t cmd, uint16_t *sample)
{
    uint8_t rx_buffer[4];
    ErrorCode_t err_code;
    err_code = i2c_write(HTU21D_ADDRESS, I2C_REG_NULL, cmd);
    if (err_code != LPC_OK) {
        return err_code;
    }
    spin(50);
    err_code = i2c_read(HTU21D_ADDRESS, rx_buffer, sizeof(rx_buffer));
    /* TODO add CRC8 checking */
    *sample = (rx_buffer[1] << 8) | (rx_buffer[2] & 0xFC);
    return err_code;
}

uint8_t htu21d_sample_temp(uint16_t *sample)
{
    double temp;
    ErrorCode_t err_code;
    err_code = htu21d_sample(HTU21D_CMD_TEMP_NO_HOLD, sample);
    if (err_code == LPC_OK) {
        temp = -46.85 + 175.72 * ((double) *sample / 65536);
        printf("[temp] %dmC\n", (int)(1000 * temp));
        return 0;
    }
    *sample = HTU21D_SAMPLE_ERR;
    return 1;
}

uint8_t htu21d_sample_humid(uint16_t *sample)
{
    double humid;
    ErrorCode_t err_code;
    err_code = htu21d_sample(HTU21D_CMD_HUMID_NO_HOLD, sample);
    if (err_code == LPC_OK) {
        humid = -6 + 125 * ((double) *sample / 65536);
        printf("[humid] %dpm\n", (int)(10 * humid));
        return 0;
    }
    *sample = HTU21D_SAMPLE_ERR;
    return 1;
}

