/* hw.h - Donanım fonksiyonları */
#ifndef HW_H
#define HW_H

#include "types.h"

/* Donanım başlatma */
void uart_init(void);
void init_screen(void);

/* UART fonksiyonları */
void uart_puts(char *s);
void uart_hex(unsigned int d);
int uart_getc(void);
int uart_available(void);

/* Yardımcı fonksiyonlar */
void delay(int32_t count);
void wait_seconds(int seconds);

#endif
