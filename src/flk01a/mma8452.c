/*

  mma8452.c - MMA8452Q i2c driver

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

#include "mma8452.h"

void PININT1_IRQHandler(void)
{
    /* clear rising edge */
#define PINT1 1
    LPC_PIN_INT->IST |= (1 << PINT1);
    LPC_PMU->GPREG1 += 1;
}

void mma8452_init(void)
{
    /* disable pull-up */
#define MODE1 4
    LPC_IOCON->PIO0_16 &= ~(1 << MODE1);
#define MMA8452_PIN 16
    LPC_SYSCON->PINTSEL[1] = MMA8452_PIN;
    NVIC_EnableIRQ(PININT1_IRQn);
    NVIC_SetPriority(PININT1_IRQn, PRIO_HIGH);
    /* wake-up from power-down  */
    LPC_SYSCON->STARTERP0 |= (1 << PINT1);
    /* clear rising edge */
    LPC_PIN_INT->IST |= (1 << PINT1);
    /* enable rising edge int */
    LPC_PIN_INT->SIENR |= (1 << PINT1);
}

ErrorCode_t mma8452_whoami(void)
{
    uint8_t rx_buffer[2];
    return i2c_write_read(MMA8452_ADDRESS, MMA8452_REG_WHOAMI, rx_buffer,
                              sizeof(rx_buffer));
}

ErrorCode_t mma8452_trans_init(void)
{
    ErrorCode_t err_code;
    err_code = i2c_write(MMA8452_ADDRESS, MMA8452_REG_TRANS_CFG,
                         MMA8452_CMD_TRANS_XYZ_E);
    if (err_code != LPC_OK) {
        return err_code;
    }
    err_code = i2c_write(MMA8452_ADDRESS, MMA8452_REG_TRANS_THS,
                         MMA8452_CMD_TRANS_THS);
    if (err_code != LPC_OK) {
        return err_code;
    }
    err_code = i2c_write(MMA8452_ADDRESS, MMA8452_REG_TRANS_COUNT,
                         MMA8452_CMD_TRANS_COUNT);
    if (err_code != LPC_OK) {
        return err_code;
    }
    err_code = i2c_write(MMA8452_ADDRESS, MMA8452_REG_CTRL_REG4,
                         MMA8452_CMD_INT_EN_TRANS);
    if (err_code != LPC_OK) {
        return err_code;
    }
    err_code = i2c_write(MMA8452_ADDRESS, MMA8452_REG_CTRL_REG5,
                         MMA8452_CMD_INT1_TRANS);
    if (err_code != LPC_OK) {
        return err_code;
    }
    return i2c_write(MMA8452_ADDRESS, MMA8452_REG_CTRL_REG1,
                         MMA8452_CMD_ACT_12HZ);
}

