/*

  pkt.c - packet handling

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

#include "rf69_12.h"

#include "debug.h"
#include "spin.h"
#include "pkt.h"
#include "led.h"
#include "i2c.h"
#include "htu21d.h"
#include "vcnl4k.h"
#include "mpl3115.h"
#include "mma8452.h"
#include "acmp.h"

void pkt_tx_ekmb(void)
{
    static struct pkt_ekmb_s pkt_ekmb = {
        .cntr = 0
    };

    if (LPC_PMU->GPREG0 != pkt_ekmb.cntr) {
        pkt_ekmb.cntr = LPC_PMU->GPREG0;
        printf("[ekmb] cntr: %u\n", (unsigned int)pkt_ekmb.cntr);
        rf12_sendNow(0, &pkt_ekmb, sizeof(pkt_ekmb));
        rf12_sendWait(3);
#ifdef DEBUG
        led_blink();
#endif
    }
}

void pkt_tx_mma8452(void)
{
    static struct pkt_mma8452_s pkt_mma8452 = {
        .cntr = 0
    };

    if (LPC_PMU->GPREG1 % 0x10000 != pkt_mma8452.cntr) {
        pkt_mma8452.cntr = LPC_PMU->GPREG1;
        mma8452_trans_clear();
        printf("[%s] cntr: %u\n", MMA8452_ID, (unsigned int)pkt_mma8452.cntr);
        rf12_sendNow(0, &pkt_mma8452, sizeof(pkt_mma8452));
        rf12_sendWait(3);
#ifdef DEBUG
        led_blink();
#endif
    }
}

void pkt_tx_gauge(uint8_t wwdt_event)
{
    static struct pkt_gauge_s pkt_gauge = {
        .reserved = 0
    };
    /* bit-fields are not addressable */
    uint32_t tmp32;
    uint16_t tmp16;
    uint8_t tmp8;

    pkt_gauge.wwdt_event = wwdt_event;
    pkt_gauge.bod_event = acmp_sample(&tmp8);
    pkt_gauge.batt = tmp8;
    pkt_gauge.temp_err = htu21d_sample_temp(&pkt_gauge.temp);
#ifndef DEBUG
    spin(2);                /* needed for proper i2c operation */
#endif
    pkt_gauge.humid_err = htu21d_sample_humid(&tmp16);
    pkt_gauge.humid = tmp16;
    pkt_gauge.light_err = vcnl4k_sample_light(&pkt_gauge.light);
    pkt_gauge.pressure_err = mpl3115_sample_pressure(&tmp32);
    pkt_gauge.pressure = tmp32;
    pkt_gauge.accel_err = mma8452_whoami();
    rf12_sendNow(0, &pkt_gauge, sizeof(pkt_gauge));
    rf12_sendWait(3);
    if (pkt_gauge.temp_err || pkt_gauge.humid_err) {
        i2c_bus_clear();
    }
    led_blink();
}
