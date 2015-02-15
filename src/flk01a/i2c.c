/*

  i2c.c - i2c convenience functions

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

#include "i2c.h"

static unsigned int i = 0;
static volatile struct i2c_s i2c;
static const struct i2c_slave_s i2c_slaves[] = {
    { HTU21D_ADDRESS, "htu21d" },
    { VCNL4K_ADDRESS, "vcnl4k" },
    { MPL3115_ADDRESS, "mpl3115" },
    { 0x00, NULL }
};

void I2C_IRQHandler(void)
{
    LPC_I2CD_API->i2c_isr_handler(i2c.handle);
}

void i2c_init(void)
{
#define I2C 5
    LPC_SYSCON->SYSAHBCLKCTRL |= (1 << I2C);
#define I2C_RST_N 6
    LPC_SYSCON->PRESETCTRL &= ~(1 << I2C_RST_N);
    LPC_SYSCON->PRESETCTRL |= (1 << I2C_RST_N);
#define I2C_MODE 8
    LPC_IOCON->PIO0_10 = (0x2 << I2C_MODE);
    LPC_IOCON->PIO0_11 = (0x2 << I2C_MODE);
    NVIC_EnableIRQ(I2C_IRQn);
    NVIC_SetPriority(I2C_IRQn, PRIO_HIGH);
    ErrorCode_t err_code;
    printf("[i2c] firmware: v%u\n", (unsigned int)LPC_I2CD_API->i2c_get_firmware_version());
    printf("[i2c] memsize: %uB\n", (unsigned int)LPC_I2CD_API->i2c_get_mem_size());
    i2c.handle = LPC_I2CD_API->i2c_setup(LPC_I2C_BASE, (uint32_t *)i2c.mem);
    printf("[i2c] clk: %uHz\n", (unsigned int)I2C_CLOCKRATE);
    err_code = LPC_I2CD_API->i2c_set_bitrate(i2c.handle, __SYSTEM_CLOCK, I2C_CLOCKRATE);
    printf("[i2c] set_bitrate err: %x\n", err_code);
}

static void i2c_callback(uint32_t err_code, uint32_t n)
{
    i2c.err_code = err_code; 
    i2c.ready = 1;
}

void i2c_bus_clear(void)
{
    /* TODO increase bus speed to 10kHz */
    /* free PIO0_10 from SCL */
    LPC_SWM->PINASSIGN8 |= 0xFF;
#define SCL_PIN 10
    LPC_GPIO_PORT->DIR0 |= (1 << SCL_PIN);
    for (int i = 0; i < 10; i++) {
        LPC_GPIO_PORT->CLR0 = (1 << SCL_PIN);
        spin(2);
        LPC_GPIO_PORT->SET0 = (1 << SCL_PIN);
        spin(2);
    }
    LPC_GPIO_PORT->DIR0 &= ~(1 << SCL_PIN);
    /* re-assign SCL to PIO0_10 */
    LPC_SWM->PINASSIGN8 &= 0xFFFFFF0AUL;
}

static const char *i2c_name(uint8_t addr)
{
    uint8_t i = 0;
    while (i2c_slaves[i].addr > 0) {
        if (i2c_slaves[i].addr == addr)
            return i2c_slaves[i].name;
        i++;
    }
    return NULL;
}

ErrorCode_t i2c_write(uint8_t addr, uint8_t reg, uint8_t cmd)
{
    uint8_t tx_count;
    uint8_t tx_buffer[3];
    tx_buffer[0] = addr << 1;
    if (reg == I2C_REG_NULL) {
        tx_buffer[1] = cmd;
        tx_count = 2;
    } else {
        tx_buffer[1] = reg;
        tx_buffer[2] = cmd;
        tx_count = 3;
    };

    I2C_PARAM_T param = {
        .num_bytes_send = tx_count,
        .num_bytes_rec = 0,
        .buffer_ptr_send = tx_buffer,
        .buffer_ptr_rec = NULL,
        .func_pt = i2c_callback,
        .stop_flag = 1
    };
    I2C_RESULT_T result;

    i2c.ready = 0;
    LPC_I2CD_API->i2c_master_transmit_intr(i2c.handle, &param, &result);
    LPC_I2CD_API->i2c_set_timeout(i2c.handle, I2C_TIMEOUT);
    while (!i2c.ready);
    if (reg == I2C_REG_NULL) {
        printf("[%s][w] i: %u err: 0x%02X cmd: 0x%02X\n",
               i2c_name(addr), i++, i2c.err_code, cmd);
    } else {
        printf("[%s][w] i: %u err: 0x%02X reg: 0x%02X cmd: 0x%02X\n",
               i2c_name(addr), i++, i2c.err_code, reg, cmd);
    };
    return i2c.err_code;
}

ErrorCode_t i2c_read(uint8_t addr, uint8_t rx_buffer[], size_t rx_count)
{
    rx_buffer[0] = addr << 1 | 0x01;

    I2C_PARAM_T param = {
        .num_bytes_send = 0,
        .num_bytes_rec = rx_count,
        .buffer_ptr_send = NULL,
        .buffer_ptr_rec = rx_buffer,
        .func_pt = i2c_callback,
        .stop_flag = 1
    };
    I2C_RESULT_T result;

    i2c.ready = 0;
    LPC_I2CD_API->i2c_master_receive_intr(i2c.handle, &param, &result);
    LPC_I2CD_API->i2c_set_timeout(i2c.handle, I2C_TIMEOUT);
    while (!i2c.ready);
    printf("[%s][r] i: %u err: 0x%02X rx: 0x", i2c_name(addr), i++, i2c.err_code);
    for (uint32_t j = 1; j < rx_count; j++) {
        printf("%02X", rx_buffer[j]);
    }
    printf("\n");
    return i2c.err_code;
}

ErrorCode_t i2c_write_read(uint8_t addr, uint8_t reg, uint8_t rx_buffer[],
                           size_t rx_count)
{
    uint8_t tx_buffer[2];
    tx_buffer[0] = addr << 1;
    tx_buffer[1] = reg;
    rx_buffer[0] = addr << 1 | 0x01;

    I2C_PARAM_T param = {
        .num_bytes_send = 2,
        .num_bytes_rec = rx_count,
        .buffer_ptr_send = tx_buffer,
        .buffer_ptr_rec = rx_buffer,
        .func_pt = i2c_callback,
        .stop_flag = 0
    };
    I2C_RESULT_T result;

    i2c.ready = 0;
    LPC_I2CD_API->i2c_master_tx_rx_intr(i2c.handle, &param, &result);
    LPC_I2CD_API->i2c_set_timeout(i2c.handle, I2C_TIMEOUT);
    while (!i2c.ready);
    printf("[%s][wr] i: %u err: 0x%02X reg: 0x%02X rx: 0x",
           i2c_name(addr), i++, i2c.err_code, reg);
    for (uint32_t j = 1; j < rx_count; j++) {
        printf("%02X", rx_buffer[j]);
    }
    printf("\n");
    return i2c.err_code;
}

