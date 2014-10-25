#include "LPC8xx.h"
#include "romapi_8xx.h"
#include "rom_i2c_8xx.h"
#include <stdio.h>
#include "uart.h"

const int ledPin = 0;

static I2C_HANDLE_T *i2cHandle;
static uint32_t i2cMem[24];

volatile uint32_t msTicks;

void SysTick_Handler(void)
{
    ++msTicks;
}

static void sleep(uint32_t ms)
{
    uint32_t now = msTicks;
    while ((msTicks-now) < ms);
}

static void clkInit(void)
{
#define I2C 5
#define IOCON 18
    LPC_SYSCON->SYSAHBCLKCTRL |= (1 << IOCON) | (1 << I2C);
}  

static void switchInit(void)
{
    /* UART0_TXD 9 */
    /* UART0_RXD 8 */
    LPC_SWM->PINASSIGN0 = 0xFFFF0809UL;
    /* disable SWCLK on PIO0_3 */
#define SWCLK_EN 2;
    LPC_SWM->PINENABLE0 |= 1 << SWCLK_EN;
    /* I2C0_SDA 10 TODO: 11? */
    LPC_SWM->PINASSIGN7 = 0x0AFFFFFFUL;
    /* I2C0_SCL 11 TODO: 10? */
    /* CLKOUT 3 */
    LPC_SWM->PINASSIGN8 = 0xFF03FF0BUL;
}

static void clkoutInit(void)
{
    /* select main clock as CLKOUT */
    LPC_SYSCON->CLKOUTSEL = 3;
    LPC_SYSCON->CLKOUTUEN = 0;
    LPC_SYSCON->CLKOUTUEN = 1;
    LPC_SYSCON->CLKOUTDIV = 1;
}

static void i2cInit(void)
{
    ErrorCode_t errCode;
    printf("[i2c] firmware: v%u\n", (unsigned int)LPC_I2CD_API->i2c_get_firmware_version());
    printf("[i2c] memsize: %uB\n", (unsigned int)LPC_I2CD_API->i2c_get_mem_size());
    i2cHandle = LPC_I2CD_API->i2c_setup(LPC_I2C_BASE, i2cMem);
#define I2C_BITRATE 400000UL
    errCode = LPC_I2CD_API->i2c_set_bitrate(i2cHandle, __SYSTEM_CLOCK, I2C_BITRATE);
    printf("[i2c] set_bitrate ErrorCode: %x\n", errCode);
#define I2C_TIMEOUT 100UL
    errCode = LPC_I2CD_API->i2c_set_timeout(i2cHandle, I2C_TIMEOUT);
    printf("[i2c] set_timeout StatusCode: %x\n", errCode);
}

static void htu21d_RdUser()
{
    uint8_t txBuffer[2];
    uint8_t rxBuffer[2];
    //uint32_t i = 0;
#define HTU21D_I2C_ADDRESS 0x40
#define HTU21D_I2C_CMD_READ_USER_REGISTER 0xE7
    txBuffer[0] = HTU21D_I2C_ADDRESS << 1;
    txBuffer[1] = HTU21D_I2C_CMD_READ_USER_REGISTER;
    rxBuffer[0] = HTU21D_I2C_ADDRESS << 1 | 0x01;

    I2C_PARAM_T param = {
        .num_bytes_send = 2,
        .num_bytes_rec = 2,
        .buffer_ptr_send = txBuffer,
        .buffer_ptr_rec = rxBuffer,
        .stop_flag = 1
    };
    I2C_RESULT_T result;
    ErrorCode_t errCode;

    errCode = LPC_I2CD_API->i2c_master_tx_rx_poll(i2cHandle, &param, &result);
    printf("[htu21d] poll ErrorCode: 0x%x\n", errCode);
    printf("[htu21d] user register: 0x%x\n", rxBuffer[1]);
}

int main(void)
{
    switchInit();
    clkInit();
    clkoutInit();
    uart0Init(115200);
    printf("\n--- kube start ---\n");
    printf("[clk] sys: %uHz\n", (unsigned int)__SYSTEM_CLOCK);
    i2cInit();

    htu21d_RdUser();

    SysTick_Config(__SYSTEM_CLOCK/1000-1);   // 1000 Hz
    LPC_GPIO_PORT->DIR0 |= (1 << ledPin);

    while (1) {
        LPC_GPIO_PORT->NOT0 = 1 << ledPin;
        sleep(100);
        LPC_GPIO_PORT->NOT0 = 1 << ledPin;
        sleep(900);
    }

    return 0;
}
