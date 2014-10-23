#include "uart_int.h"

#define UART_ENABLE          (1 << 0)
#define UART_DATA_LENGTH_8   (1 << 2)
#define UART_PARITY_NONE     (0 << 4)
#define UART_STOP_BIT_1      (0 << 6)

#define UART_STATUS_RXRDY    (1 << 0)
#define UART_STATUS_TXRDY    (1 << 2)
#define UART_STATUS_CTSDEL   (1 << 5)
#define UART_STATUS_RXBRKDEL (1 << 11)

#define UART_INTEN_RXRDY     (1 << 0)
#define UART_INTEN_TXRDY     (1 << 2)

typedef struct {
  volatile uint8_t fill, take;
  uint8_t buffer[64];
} RingBuffer;

static RingBuffer rxBuf, txBuf;

static void rbInit (RingBuffer* rb) {
  // rb->fill = rb->take = 0;
}

static int rbIsEmpty (RingBuffer* rb) {
  return rb->fill == rb->take;
}

static int isFull (RingBuffer* rb) {
  return (rb->fill + 1) % sizeof rb->buffer == rb->take;
}
  
static void rbAdd (RingBuffer* rb, int c) {
  int n = rb->fill;
  rb->buffer[n] = c;
  rb->fill = (n + 1) % sizeof rb->buffer;
}

static uint8_t rbPop (RingBuffer* rb) {
  int n = rb->take;
  uint8_t c = rb->buffer[n];
  rb->take = (n + 1) % sizeof rb->buffer;
  return c;
}

void uart0Init (uint32_t baudRate) {
  rbInit(&rxBuf);
  rbInit(&txBuf);
  
  const uint32_t UARTCLKDIV=1;

  /* Setup the clock and reset UART0 */
  LPC_SYSCON->UARTCLKDIV = UARTCLKDIV;
  // NVIC_DisableIRQ(UART0_IRQn);
  LPC_SYSCON->SYSAHBCLKCTRL |=  (1 << 14);
  LPC_SYSCON->PRESETCTRL    &= ~(1 << 3);
  LPC_SYSCON->PRESETCTRL    |=  (1 << 3);

  /* Configure UART0 */
  SystemCoreClockUpdate();
  uint32_t clk = SystemCoreClock / UARTCLKDIV;
  LPC_USART0->CFG = UART_DATA_LENGTH_8 | UART_PARITY_NONE | UART_STOP_BIT_1;
  LPC_USART0->BRG = clk / 16 / baudRate - 1;
  LPC_SYSCON->UARTFRGDIV = 0xFF;
  LPC_SYSCON->UARTFRGMULT = (((clk / 16) * (LPC_SYSCON->UARTFRGDIV + 1)) /
    (baudRate * (LPC_USART0->BRG + 1))) - (LPC_SYSCON->UARTFRGDIV + 1);

  /* Clear the status bits */
  LPC_USART0->STAT = UART_STATUS_CTSDEL | UART_STATUS_RXBRKDEL;

  /* Enable UART0 interrupt */
  NVIC_EnableIRQ(UART0_IRQn);

  /* Enable UART0 */
  LPC_USART0->CFG |= UART_ENABLE;
}

void uart0SendChar (char buffer) {
  while (isFull(&txBuf))
    ;
  rbAdd(&txBuf, buffer);
  LPC_USART0->INTENSET = UART_INTEN_TXRDY;
}

void uart0Send (const char *buffer, uint32_t length) {
  while (length--)
    uart0SendChar(*buffer++);
}

int uart0RecvChar (void) {
  int result = -1;
  LPC_USART0->INTENCLR = UART_INTEN_RXRDY;
  if (!rbIsEmpty(&rxBuf))
    result = rbPop(&rxBuf);
  LPC_USART0->INTENSET = UART_INTEN_RXRDY;
  return result;
}

void UART0_IRQHandler () {
  uint32_t stat = LPC_USART0->STAT;
  if (stat & UART_STATUS_RXRDY)
    rbAdd(&rxBuf, LPC_USART0->RXDATA);
  if (stat & UART_STATUS_TXRDY) {
    if (rbIsEmpty(&txBuf))
      LPC_USART0->INTENCLR = UART_INTEN_TXRDY;
    else
      LPC_USART0->TXDATA = rbPop(&txBuf);
  }
}
