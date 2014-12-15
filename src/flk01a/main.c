#include "LPC8xx.h"
#include "romapi_8xx.h"
#include "rom_pwr_8xx.h"
#include "rom_i2c_8xx.h"
#include <stdio.h>
#include "uart.h"
#include "pkt.h"
#include "rf69_12.h"

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

static volatile struct {
    I2C_HANDLE_T *handle;
    uint32_t mem[24];
    ErrorCode_t err_code;
    uint8_t ready;
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
    /* I2C0_SCL 10 */
    /* CLKOUT 3 */
#ifdef DEBUG
    /* disable SWCLK on PIO0_3 */
#define SWCLK_EN 2;
    LPC_SWM->PINENABLE0 |= 1 << SWCLK_EN;
    LPC_SWM->PINASSIGN8 = 0xFF03FF0AUL;
#else
    LPC_SWM->PINASSIGN8 = 0xFFFFFF0AUL;
#endif
#define MODE0 3
#define MODE1 4
  /* SPI0_SCK 7 */
  LPC_IOCON->PIO0_7 &= ~(1 << MODE1);
  LPC_SWM->PINASSIGN3 = 0x07ffffffUL;
  /* SPI0_MOSI 1 */
  LPC_IOCON->PIO0_1 &= ~(1 << MODE1);
  /* SPI0_MISO 13 (repeater mode) */
  LPC_IOCON->PIO0_13 |= (1 << MODE0);
  /* SPI0_SSEL 14 */
  LPC_IOCON->PIO0_14 &= ~(1 << MODE1);
  LPC_SWM->PINASSIGN4 = 0xff0e0d01UL;
  /* IRQ 17 */
  LPC_IOCON->PIO0_17 &= ~(1 << MODE1);
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

static void pwr_init(void)
{
    uint32_t cmd[] = {12, PWR_LOW_CURRENT, 12};
    uint32_t result[1];
    LPC_PWRD_API->set_power(cmd, result);
    printf("[pwr] result[0]: 0x%02X\n", (unsigned int)result[0]);
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

static void acmp_init(void)
{
#define ACMPS 19
    LPC_SYSCON->SYSAHBCLKCTRL |= (1 << ACMPS);
#define ACMP_RST_N 12
    LPC_SYSCON->PRESETCTRL &= ~(1 << ACMP_RST_N);
    LPC_SYSCON->PRESETCTRL |= (1 << ACMP_RST_N);
#define COMP_VP_SEL 8
    LPC_CMP->CTRL = (0x6 << COMP_VP_SEL);
}

static uint8_t acmp_compare(uint8_t ladder)
{
#define LADMASK 0x1F
    ladder &= LADMASK;
#define LADEN 0
#define LADSEL 1
    LPC_CMP->LAD = (1 << LADEN) | (ladder << LADSEL);
    spin(2);
#define COMPSTAT 21
    return (LPC_CMP->CTRL >> COMPSTAT) & 0x1;
}

static uint8_t acmp_sample()
{
#define ACMP 15
    LPC_SYSCON->PDRUNCFG &= ~(1 << ACMP);
    uint8_t ladder = 0;
    for (int i=0; i<5; i++) {
        ladder += (acmp_compare(ladder + (1 << (4 - i))) << (4 - i));
    }
    LPC_SYSCON->PDRUNCFG |= (1 << ACMP);
    printf("[acmp] ladder: %u\n", ladder); 
    return ladder;
}

static void i2c_init(void)
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
#define I2C_CLOCKRATE 100000UL
    printf("[i2c] clk: %uHz\n", (unsigned int)I2C_CLOCKRATE);
    err_code = LPC_I2CD_API->i2c_set_bitrate(i2c.handle, __SYSTEM_CLOCK, I2C_CLOCKRATE);
    printf("[i2c] set_bitrate err: %x\n", err_code);
#define I2C_TIMEOUT 1000UL
}

void I2C_IRQHandler(void)
{
    LPC_I2CD_API->i2c_isr_handler(i2c.handle);
}

static void i2c_callback(uint32_t err_code, uint32_t n)
{
    i2c.err_code = err_code; 
    i2c.ready = 1;
}

static void i2c_bus_clear(void)
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
    switch_init();
}

#define HTU21D_ADDRESS 0x40
#define HTU21D_CMD_TEMP_HOLD 0xE3
#define HTU21D_CMD_HUMID_HOLD 0xE5
#define HTU21D_CMD_TEMP_NO_HOLD 0xF3
#define HTU21D_CMD_HUMID_NO_HOLD 0xF5
#define HTU21D_CMD_READ_USER 0xE7
#define HTU21D_CMD_SOFT_RESET 0xFE
#define HTU21D_SAMPLE_ERR 0xFFFF

static ErrorCode_t htu21d_write(uint8_t cmd)
{
    static unsigned int i = 0;
    uint8_t tx_buffer[2];
    tx_buffer[0] = HTU21D_ADDRESS << 1;
    tx_buffer[1] = cmd;

    I2C_PARAM_T param = {
        .num_bytes_send = 2,
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
    printf("[htu21d][w] i: %u err: 0x%02X tx: 0x%02X\n", i++, i2c.err_code, cmd);
    return i2c.err_code;
}

static ErrorCode_t htu21d_read(uint8_t rx_buffer[], size_t rx_count)
{
    static unsigned int i = 0;
    rx_buffer[0] = HTU21D_ADDRESS << 1 | 0x01;

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
    printf("[htu21d][r] i: %u err: 0x%02X rx: 0x", i++, i2c.err_code);
    for (uint32_t j = 0; j < rx_count; j++) {
        printf("%02X", rx_buffer[j]);
    }
    printf("\n");
    return i2c.err_code;
}

static ErrorCode_t htu21d_soft_reset()
{
    return htu21d_write(HTU21D_CMD_SOFT_RESET);
}

static ErrorCode_t htu21d_read_user()
{
    uint8_t rx_buffer[2];
    ErrorCode_t err_code;
    err_code = htu21d_write(HTU21D_CMD_READ_USER);
    if (err_code != LPC_OK) {
        return err_code;
    }
    return htu21d_read(rx_buffer, sizeof(rx_buffer));
}

static ErrorCode_t htu21d_sample(uint8_t cmd, uint16_t *sample)
{
    uint8_t rx_buffer[4];
    ErrorCode_t err_code;
    err_code = htu21d_write(cmd);
    if (err_code != LPC_OK) {
        return err_code;
    }
    spin(50);
    err_code = htu21d_read(rx_buffer, sizeof(rx_buffer));
    /* TODO add CRC8 checking */
    *sample = (rx_buffer[1] << 8) | (rx_buffer[2] & 0xFC);
    return err_code;
}

static uint8_t htu21d_sample_temp(uint16_t *sample)
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

static uint8_t htu21d_sample_humid(uint16_t *sample)
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
    static uint32_t time = 0;
    static struct pkt_counter_s pkt_counter = {
        .cntr = 0
    };
    static struct pkt_gauge_s pkt_gauge = {
        .padding = 0
    };
#define ALARMFLAG 1
    LPC_WKT->CTRL |= (1 << ALARMFLAG);
    LPC_WKT->COUNT = 10 * MILLIS;

    if (LPC_PMU->GPREG0 != pkt_counter.cntr) {
        pkt_counter.cntr = LPC_PMU->GPREG0;
        printf("[ekmb] cntr: %u\n", (unsigned int)pkt_counter.cntr);
        rf12_sendNow(0, &pkt_counter, sizeof(pkt_counter));
        rf12_sendWait(3);
#ifdef DEBUG
        led_blink();
#endif
    }

#define SAMPLE_PERIOD_S 64
    if (time % SAMPLE_PERIOD_S == 0) {
        pkt_gauge.batt = acmp_sample();
        pkt_gauge.temp_err = htu21d_sample_temp(&pkt_gauge.temp);
#ifndef DEBUG
        spin(2); /* needed for proper i2c operation */
#endif
        pkt_gauge.humid_err = htu21d_sample_humid(&pkt_gauge.humid);
        rf12_sendNow(0, &pkt_gauge, sizeof(pkt_gauge));
        rf12_sendWait(3);
        if (pkt_gauge.temp_err || pkt_gauge.humid_err) {
            i2c_bus_clear();
        }
        led_blink();
    }

#define RESET_PERIOD_S 65536UL
    if (time == RESET_PERIOD_S) {
        printf("[sys] resetting...\n");
#ifdef DEBUG
        spin(2);
#endif
        NVIC_SystemReset();
    }

    time++;
}

int main(void)
{
    int i = 0;
    __disable_irq();
    switch_init();
    systick_init();
#ifdef DEBUG
    clkout_init();
    uart_init();
#endif
    printf("\n--- kube boot ---\n");
    printf("[sys] clk: %uHz\n", (unsigned int)__SYSTEM_CLOCK);
    config_load(&cfg);
    pwr_init();
    i2c_init();
    led_init();
    acmp_init();
    ekmb_init();
    rf12_initialize(cfg.nid, RF12_868MHZ, cfg.grp);
    rf12_sleep();
    __enable_irq();
    spin(15);
    i2c_bus_clear();
    htu21d_soft_reset();
    spin(15);
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
