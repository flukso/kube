/*

  mpl3115.c - MPL3115A2 i2c driver

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

#include "mpl3115.h"

ErrorCode_t mpl3115_whoami(void)
{
    uint8_t rx_buffer[2];
    return i2c_write_read(MPL3115_ADDRESS, MPL3115_REG_WHOAMI, rx_buffer,
                              sizeof(rx_buffer));
}

static ErrorCode_t mpl3115_sample(uint32_t *sample)
{
    uint8_t rx_buffer[4];
    ErrorCode_t err_code;
    err_code = i2c_write(MPL3115_ADDRESS, MPL3115_REG_CTRL_REG1,
                         MPL3115_CMD_ONE_SHOT);
    if (err_code != LPC_OK) {
        return err_code;
    }
    spin(520);
    err_code = i2c_write_read(MPL3115_ADDRESS, MPL3115_REG_P_OUT, rx_buffer,
                              sizeof(rx_buffer));
    *sample = (rx_buffer[1] << 12) | (rx_buffer[2] << 4) | (rx_buffer[3] >> 4);
    return err_code;
}

uint8_t mpl3115_sample_pressure(uint32_t *sample)
{
    unsigned int pressure;
    ErrorCode_t err_code;
    err_code = mpl3115_sample(sample);
    if (err_code == LPC_OK) {
        pressure = *sample * 250;
        printf("[pressure] %umPa\n", pressure);
        return 0;
    }
    *sample = MPL3115_SAMPLE_ERR;
    return 1;
}

