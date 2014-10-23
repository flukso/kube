#ifndef _UART_H_
#define _UART_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "LPC8xx.h"

void uart0Init     (uint32_t baudRate);
void uart0SendChar (char buffer);
void uart0Send     (const char *buffer, uint32_t length);
int  uart0RecvChar (void);

#ifdef __cplusplus
}
#endif

#endif