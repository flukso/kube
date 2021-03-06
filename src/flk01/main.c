/*

  main.c - FluksoKube firmware

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

#include <stdio.h>
#include "LPC8xx.h"
#include "rom_pwr_8xx.h"
#include "rf69_12.h"

#include "main.h"
#include "debug.h"
#include "spin.h"
#include "config.h"
#include "pkt.h"
#include "led.h"
#include "i2c.h"
#include "htu21d.h"
#include "vcnl4k.h"
#include "mpl3115.h"
#include "mma8452.h"
#include "ekmb.h"
#include "acmp.h"

static struct config_s cfg;

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
    LPC_SWM->PINASSIGN3 = 0x07FFFFFFUL;
    /* SPI0_MOSI 4 */
    LPC_IOCON->PIO0_4 &= ~(1 << MODE1);
    /* SPI0_MISO 13 (repeater mode) */
    LPC_IOCON->PIO0_13 |= (1 << MODE0);
    /* SPI0_SSEL 14 */
    LPC_IOCON->PIO0_14 &= ~(1 << MODE1);
    LPC_SWM->PINASSIGN4 = 0xFF0E0D04UL;
    /* IRQ 17 */
    LPC_IOCON->PIO0_17 &= ~(1 << MODE1);
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

static void wwdt_init(void)
{
#define WWDT 17
    LPC_SYSCON->SYSAHBCLKCTRL |= (1 << WWDT);
    /* set WDT_OSC_CLK to 10kHz */
#define DIVSEL 0
#define FREQSEL 5
    LPC_SYSCON->WDTOSCCTRL = (0x01 << FREQSEL) | (0x1D << DIVSEL);
#define WDTOSC_PD 6
    LPC_SYSCON->PDRUNCFG &= ~(1 << WDTOSC_PD);
    LPC_SYSCON->PDAWAKECFG &= ~(1 << WDTOSC_PD);
    LPC_SYSCON->PDSLEEPCFG &= ~(1 << WDTOSC_PD);
#define WDT_1SEC 2500UL
    /* watchdog feed should occur between quarter and double the SAMPLE_PERIOD */
    LPC_WWDT->TC = 2 * SAMPLE_PERIOD_S * WDT_1SEC;
    LPC_WWDT->WINDOW =
        (LPC_WWDT->TC & 0xFFFFFF) - SAMPLE_PERIOD_S * WDT_1SEC / 4;
#define WDEN 0
#define WDRESET 1
#define WDTOF 2
#define WDPROTECT 4
#define LOCK 5
    /* clear wdt time-out flag */
    LPC_WWDT->MOD &= ~(1 << WDTOF);
    LPC_WWDT->MOD = (1 << WDEN) | (1 << WDRESET) | (1 << LOCK);
}

static void wwdt_feed(void)
{
    LPC_WWDT->FEED = 0xAA;
    LPC_WWDT->FEED = 0x55;
}

static void pwr_init(void)
{
    uint32_t cmd[] = { 12, PWR_LOW_CURRENT, 12 };
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

void WKT_IRQHandler(void)
{
    static uint32_t time = 0;
    uint8_t wwdt_event = 0;
#define ALARMFLAG 1
    LPC_WKT->CTRL |= (1 << ALARMFLAG);
    LPC_WKT->COUNT = 10 * MILLIS;

    if (time % EVENT_PERIOD_S == 0) {
        pkt_tx_ekmb();
        pkt_tx_mma8452();
    }
    if (time % SAMPLE_PERIOD_S == 0) {
        /* do not feed twice at startup or the second feed
         * will occur outside the wdt window */
        if (time) {
            wwdt_feed();
        } else {
#define WDT 2
            wwdt_event = LPC_SYSCON->SYSRSTSTAT >> WDT;
            LPC_SYSCON->SYSRSTSTAT = 0;
        }
        pkt_tx_gauge(wwdt_event);
    }
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
    wwdt_init();
    wwdt_feed();
    switch_init();
    systick_init();
#ifdef DEBUG
    clkout_init();
    uart0Init(115200);
#endif
    printf("\n--- kube boot ---\n");
    printf("[sys] clk: %uHz\n", (unsigned int)__SYSTEM_CLOCK);
    config_load(&cfg);
    pwr_init();
    i2c_init();
    led_init();
    acmp_init();
    ekmb_init();
    vcnl4k_init();
    mma8452_init();
    rf12_initialize(cfg.nid, RF12_868MHZ, cfg.grp);
    rf12_sleep();
    __enable_irq();
    spin(15);
    i2c_bus_clear();
    htu21d_soft_reset();
    spin(15);
    htu21d_read_user();
    vcnl4k_read_pid();
    mpl3115_whoami();
    mma8452_whoami();
    mma8452_trans_init();
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
