/* timer.h - System Timer and Clock */
#ifndef TIMER_H
#define TIMER_H

#include "../types.h"

/* Zaman yapısı */
typedef struct {
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint16_t year;
    uint8_t month;
    uint8_t day;
} DateTime;

/* Timer fonksiyonları */
void timer_init(void);
uint64_t timer_get_ticks(void);
uint32_t timer_get_seconds(void);

/* Saat fonksiyonları */
void clock_init(uint8_t hour, uint8_t minute, uint8_t second);
void clock_set_date(uint16_t year, uint8_t month, uint8_t day);
void clock_update(void);
void clock_get_time(DateTime *dt);
void clock_format_time(char *buffer);  /* "HH:MM" formatı */
void clock_format_full(char *buffer);  /* "HH:MM:SS" formatı */
void clock_format_date(char *buffer);  /* "DD/MM/YYYY" formatı */

/* Gecikme fonksiyonları */
void timer_wait_ms(uint32_t ms);
void timer_wait_us(uint32_t us);

#endif
