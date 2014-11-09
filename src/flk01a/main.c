#include "LPC8xx.h"
#include "romapi_8xx.h"
#include "rom_i2c_8xx.h"
#include <stdio.h>
#include "uart.h"

#ifndef DEBUG
#define printf(...)
#endif

#include "config.h"

enum NVIC_priorities {
    PRIO_SYSTICK,
    PRIO_HIGH,
    PRIO_MEDIUM,
    PRIO_LOW
};

static struct config_s cfg;

static struct {
    I2C_HANDLE_T *handle;
    uint32_t mem[24];
} i2c;

volatile uint32_t mtime = 0;

static void systick_init(void)
{
    /* 1000Hz */
    SysTick_Config(__SYSTEM_CLOCK/1000-1);
    NVIC_SetPriority(SysTick_IRQn, PRIO_SYSTICK);
}

void SysTick_Handler(void)
{
    ++mtime;
}

static void spin(uint32_t ms)
{
    uint32_t start = mtime;
    uint32_t stop = start + ms;
    if (stop < start) { /* overflow */
        while (mtime > start);
    }
    while (mtime < stop);
}

static void wkt_init(void)
{
#define WKT 9
    LPC_SYSCON->SYSAHBCLKCTRL |= (1 << WKT);
    NVIC_EnableIRQ(WKT_IRQn);
    NVIC_SetPriority(WKT_IRQn, PRIO_LOW);
    LPC_SYSCON->STARTERP1 |= (1 << WKT_IRQn);
    /* use 10kHz low-power oscillator for wkt */
#define LPOSCEN 2
    LPC_PMU->DPDCTRL |= (1 << LPOSCEN);
#define CLKSEL 0
    LPC_WKT->CTRL |= (1 << CLKSEL);
    /* wake up from power-down every second */
    LPC_SYSCON->PDAWAKECFG = LPC_SYSCON->PDRUNCFG;
#define MILLIS 1000
    LPC_WKT->COUNT = 10 * MILLIS;
#define SLEEPONEXIT 1
#define SLEEPDEEP 2
    SCB->SCR |= (1 << SLEEPONEXIT) | (1 << SLEEPDEEP);
#define POWER_DOWN 0x02
    LPC_PMU->PCON = POWER_DOWN;
}

static void switch_init(void)
{
#define IOCON 18
    LPC_SYSCON->SYSAHBCLKCTRL |= (1 << IOCON);
    /* UART0_TXD 9 */
    /* UART0_RXD 8 */
    LPC_SWM->PINASSIGN0 = 0xFFFF0809UL;
    /* I2C0_SDA 11 */
    LPC_SWM->PINASSIGN7 = 0x0BFFFFFFUL;
    /* disable SWCLK on PIO0_3 */
#define SWCLK_EN 2;
    LPC_SWM->PINENABLE0 |= 1 << SWCLK_EN;
    /* I2C0_SCL 10 */
    /* CLKOUT 3 */
    LPC_SWM->PINASSIGN8 = 0xFF03FF0AUL;
}

static void led_init(void)
{
#define LED_PIN 0
    LPC_GPIO_PORT->DIR0 |= (1 << LED_PIN);
#define MODE1 4
    LPC_IOCON->PIO0_0 &= ~(1 << MODE1);
}

static void led_blink(void)
{
    LPC_GPIO_PORT->SET0 = 1 << LED_PIN;
    spin(10);
    LPC_GPIO_PORT->CLR0 = 1 << LED_PIN;
} 

static void clkout_init(void)
{
#define MODE1 4
    LPC_IOCON->PIO0_3 &= ~(1 << MODE1);
    /* select main clock as CLKOUT */
    LPC_SYSCON->CLKOUTSEL = 3;
    LPC_SYSCON->CLKOUTUEN = 0;
    LPC_SYSCON->CLKOUTUEN = 1;
    LPC_SYSCON->CLKOUTDIV = 1;
}

static void uart_init(void)
{
    uart0Init(115200);
}

static void i2c_init(void)
{
#define I2C 5
    LPC_SYSCON->SYSAHBCLKCTRL |= (1 << I2C);
    ErrorCode_t err_code;
    printf("[i2c] firmware: v%u\n", (unsigned int)LPC_I2CD_API->i2c_get_firmware_version());
    printf("[i2c] memsize: %uB\n", (unsigned int)LPC_I2CD_API->i2c_get_mem_size());
    i2c.handle = LPC_I2CD_API->i2c_setup(LPC_I2C_BASE, i2c.mem);
#define I2C_CLOCKRATE 10000UL
    printf("[i2c] clk: %uHz\n", (unsigned int)I2C_CLOCKRATE);
    err_code = LPC_I2CD_API->i2c_set_bitrate(i2c.handle, __SYSTEM_CLOCK, I2C_CLOCKRATE);
    printf("[i2c] set_bitrate err: %x\n", err_code);
#define I2C_TIMEOUT 100UL
    err_code = LPC_I2CD_API->i2c_set_timeout(i2c.handle, I2C_TIMEOUT);
    printf("[i2c] set_timeout stat: %x\n", err_code);
}

#define HTU21D_ADDRESS 0x40
#define HTU21D_CMD_TEMP_HOLD 0xE3
#define HTU21D_CMD_HUMID_HOLD 0xE5
#define HTU21D_CMD_READ_USER 0xE7
#define HTU21D_CMD_SOFT_RESET 0xFE

static ErrorCode_t htu21d_cmd(uint8_t cmd, uint8_t rx_buffer[], size_t rx_count)
{
    static unsigned int i = 0;
    uint8_t tx_buffer[2];
    tx_buffer[0] = HTU21D_ADDRESS << 1;
    tx_buffer[1] = cmd;
    if (rx_count > 0) {
        rx_buffer[0] = HTU21D_ADDRESS << 1 | 0x01;
    }

    I2C_PARAM_T param = {
        .num_bytes_send = 2,
        .num_bytes_rec = rx_count,
        .buffer_ptr_send = tx_buffer,
        .buffer_ptr_rec = rx_buffer,
        .stop_flag = 1
    };
    I2C_RESULT_T result;
    ErrorCode_t err_code;

    err_code = LPC_I2CD_API->i2c_master_tx_rx_poll(i2c.handle, &param, &result);
    printf("[htu21d] i: %u cmd: 0x%02X\n", i++, cmd);
    printf("[htu21d] err: 0x%02X\n", err_code);
    if (rx_count > 0) {
        printf("[htu21d] rx: 0x");
        for (uint32_t j = 0; j < rx_count; j++) {
            printf("%02X", rx_buffer[j]);
        }
        printf("\n");
    }
    return err_code;
}

static void htu21d_soft_reset()
{
    htu21d_cmd(HTU21D_CMD_SOFT_RESET, NULL, 0);
}

static void htu21d_read_user()
{
    uint8_t rx_buffer[2];
    htu21d_cmd(HTU21D_CMD_READ_USER, rx_buffer, sizeof(rx_buffer));
}

static ErrorCode_t htu21d_measure(uint8_t cmd, uint16_t *sample)
{
    uint8_t rx_buffer[4];
    ErrorCode_t err_code;
    err_code = htu21d_cmd(cmd, rx_buffer, sizeof(rx_buffer));
    /* TODO add CRC8 checking */
    *sample = (rx_buffer[1] << 8) | (rx_buffer[2] & 0xFC);
    return err_code;
}

static void htu21d_measure_temp()
{
    uint16_t sample;
    double temp;
    ErrorCode_t err_code;
    err_code = htu21d_measure(HTU21D_CMD_TEMP_HOLD, &sample);
    if (err_code == LPC_OK) {
        temp = -46.85 + 175.72 * ((double)sample / 65536);
        printf("[temp] %dmC\n", (int)(1000 * temp));
    }
}

static void htu21d_measure_humid()
{
    uint16_t sample;
    double humid;
    ErrorCode_t err_code;
    err_code = htu21d_measure(HTU21D_CMD_HUMID_HOLD, &sample);
    if (err_code == LPC_OK) {
        humid = -6 + 125 * ((double)sample / 65536);
        printf("[humid] %dpm\n", (int)(10 * humid));
    }
}

static void ekmb_init()
{
    /* disable pull-up */
#define MODE1 4
    LPC_IOCON->PIO0_15 &= ~(1 << MODE1);
#define EKMB_PIN 15
    LPC_SYSCON->PINTSEL[0] = EKMB_PIN;
    NVIC_EnableIRQ(PININT0_IRQn);
    NVIC_SetPriority(PININT0_IRQn, PRIO_HIGH);
    /* wake-up from power-down  */
#define PINT0 0
    LPC_SYSCON->STARTERP0 |= (1 << PINT0);
     /* clear rising edge */
    LPC_PIN_INT->IST |= (1 << PINT0);
    /* enable rising edge int */
    LPC_PIN_INT->SIENR |= (1 << PINT0);
}

void PININT0_IRQHandler(void)
{
    /* clear rising edge */
    LPC_PIN_INT->IST |= (1 << PINT0);
    LPC_PMU->GPREG0 += 1;
}

void WKT_IRQHandler(void)
{
    static uint32_t time = 0, counter = 0;
#define ALARMFLAG 1
    LPC_WKT->CTRL |= (1 << ALARMFLAG);
    LPC_WKT->COUNT = 10 * MILLIS;
    if (LPC_PMU->GPREG0 != counter) {
        counter = LPC_PMU->GPREG0;
        printf("[ekmb] cntr: %u\n", (unsigned int)LPC_PMU->GPREG0);
#ifdef DEBUG
        led_blink();
#endif
    }
#define SAMPLE_PERIOD_S 16
    if (++time % SAMPLE_PERIOD_S == 0) {
        htu21d_measure_temp();
#ifndef DEBUG
        spin(1); /* needed for proper i2c operation */
#endif
        htu21d_measure_humid();
        led_blink();
    }
}

int main(void)
{
    int i = 0;
    __disable_irq();
    switch_init();
    systick_init();
    clkout_init();
#ifdef DEBUG
    uart_init();
#endif
    printf("\n--- kube boot ---\n");
    printf("[sys] clk: %uHz\n", (unsigned int)__SYSTEM_CLOCK);
    config_load(&cfg);
    i2c_init();
    led_init();
    ekmb_init();
    __enable_irq();
    htu21d_soft_reset();
    spin(100);
    htu21d_read_user();
#ifdef DEBUG
    spin(2);
#endif
    __disable_irq();
    wkt_init();
    __enable_irq();
 
    while (1) {
        printf("[sys] loop #%d\n", ++i);
#ifdef DEBUG
        spin(2);
#endif
        __WFI();
    }
    return 0;
}
