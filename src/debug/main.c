#include "LPC8xx.h"
#include <stdio.h>
#include "uart.h"

const int ledPin = 0;

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

static void init(void)
{
    /* Enable clocks to IOCON & SWM */
    LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 18) | (1 << 7);
    /* UART0_TXD 9 */
    /* UART0_RXD 8 */
    LPC_SWM->PINASSIGN0 = 0xFFFF0809UL;
    /* disable SWCLK on PIO0_3 */
#define SWCLK_EN 2;
    LPC_SWM->PINENABLE0 |= 1 << SWCLK_EN;
    /* connect CLKOUT to PIO0_3 */
    LPC_SWM->PINASSIGN8 = 0xFF03FFFFUL;
    /* select main clock as CLKOUT */
    LPC_SYSCON->CLKOUTSEL = 3;
    LPC_SYSCON->CLKOUTUEN = 0;
    LPC_SYSCON->CLKOUTUEN = 1;
    LPC_SYSCON->CLKOUTDIV = 1;
}

int main(void)
{
    init();
    uart0Init(115200);

    SysTick_Config(__SYSTEM_CLOCK/1000-1);   // 1000 Hz
    LPC_GPIO_PORT->DIR0 |= (1 << ledPin);

    printf("sys clk %uHz\n", (unsigned int)__SYSTEM_CLOCK);

    while (1) {
        LPC_GPIO_PORT->NOT0 = 1 << ledPin;
        sleep(100);
        LPC_GPIO_PORT->NOT0 = 1 << ledPin;
        sleep(900);
    }

    return 0;
}
