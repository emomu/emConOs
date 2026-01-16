/* timer.c - System Timer and Clock */
#include "timer.h"

/* Raspberry Pi System Timer adresleri */
#define MMIO_BASE       0x3F000000
#define TIMER_BASE      (MMIO_BASE + 0x3000)

#define TIMER_CS        ((volatile uint32_t*)(TIMER_BASE + 0x00))
#define TIMER_CLO       ((volatile uint32_t*)(TIMER_BASE + 0x04))
#define TIMER_CHI       ((volatile uint32_t*)(TIMER_BASE + 0x08))

/* System Timer 1MHz'de çalışır */
#define TIMER_FREQ      1000000

/* Dahili saat durumu */
static struct {
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint64_t last_tick;
    uint32_t tick_accumulator;
} clock_state;

/* Timer başlat */
void timer_init(void) {
    /* System timer otomatik çalışır, başlatma gerekmez */
    clock_state.last_tick = timer_get_ticks();
    clock_state.tick_accumulator = 0;
}

/* 64-bit tick değeri al */
uint64_t timer_get_ticks(void) {
    uint32_t hi, lo;

    /* Önce yüksek, sonra düşük oku, taşma kontrolü */
    hi = *TIMER_CHI;
    lo = *TIMER_CLO;

    /* Yüksek byte değiştiyse tekrar oku */
    if(*TIMER_CHI != hi) {
        hi = *TIMER_CHI;
        lo = *TIMER_CLO;
    }

    return ((uint64_t)hi << 32) | lo;
}

/* Saniye cinsinden süre */
uint32_t timer_get_seconds(void) {
    return (uint32_t)(timer_get_ticks() / TIMER_FREQ);
}

/* Saat başlat */
void clock_init(uint8_t hour, uint8_t minute, uint8_t second) {
    clock_state.hour = hour;
    clock_state.minute = minute;
    clock_state.second = second;
    clock_state.last_tick = timer_get_ticks();
    clock_state.tick_accumulator = 0;
}

/* Tarih ayarla */
void clock_set_date(uint16_t year, uint8_t month, uint8_t day) {
    clock_state.year = year;
    clock_state.month = month;
    clock_state.day = day;
}

/* Saat güncelle (her frame çağrılmalı) */
void clock_update(void) {
    uint64_t now = timer_get_ticks();
    uint64_t delta = now - clock_state.last_tick;
    clock_state.last_tick = now;

    /* Mikrosaniye biriktir */
    clock_state.tick_accumulator += (uint32_t)delta;

    /* 1 saniye geçti mi? */
    while(clock_state.tick_accumulator >= TIMER_FREQ) {
        clock_state.tick_accumulator -= TIMER_FREQ;
        clock_state.second++;

        if(clock_state.second >= 60) {
            clock_state.second = 0;
            clock_state.minute++;

            if(clock_state.minute >= 60) {
                clock_state.minute = 0;
                clock_state.hour++;

                if(clock_state.hour >= 24) {
                    clock_state.hour = 0;
                    clock_state.day++;

                    /* Basit ay sonu kontrolü */
                    uint8_t days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
                    if(clock_state.month >= 1 && clock_state.month <= 12) {
                        if(clock_state.day > days_in_month[clock_state.month - 1]) {
                            clock_state.day = 1;
                            clock_state.month++;

                            if(clock_state.month > 12) {
                                clock_state.month = 1;
                                clock_state.year++;
                            }
                        }
                    }
                }
            }
        }
    }
}

/* Zaman yapısını al */
void clock_get_time(DateTime *dt) {
    dt->hour = clock_state.hour;
    dt->minute = clock_state.minute;
    dt->second = clock_state.second;
    dt->year = clock_state.year;
    dt->month = clock_state.month;
    dt->day = clock_state.day;
}

/* "HH:MM" formatı */
void clock_format_time(char *buffer) {
    buffer[0] = '0' + (clock_state.hour / 10);
    buffer[1] = '0' + (clock_state.hour % 10);
    buffer[2] = ':';
    buffer[3] = '0' + (clock_state.minute / 10);
    buffer[4] = '0' + (clock_state.minute % 10);
    buffer[5] = '\0';
}

/* "HH:MM:SS" formatı */
void clock_format_full(char *buffer) {
    buffer[0] = '0' + (clock_state.hour / 10);
    buffer[1] = '0' + (clock_state.hour % 10);
    buffer[2] = ':';
    buffer[3] = '0' + (clock_state.minute / 10);
    buffer[4] = '0' + (clock_state.minute % 10);
    buffer[5] = ':';
    buffer[6] = '0' + (clock_state.second / 10);
    buffer[7] = '0' + (clock_state.second % 10);
    buffer[8] = '\0';
}

/* "DD/MM/YYYY" formatı */
void clock_format_date(char *buffer) {
    buffer[0] = '0' + (clock_state.day / 10);
    buffer[1] = '0' + (clock_state.day % 10);
    buffer[2] = '/';
    buffer[3] = '0' + (clock_state.month / 10);
    buffer[4] = '0' + (clock_state.month % 10);
    buffer[5] = '/';
    buffer[6] = '0' + (clock_state.year / 1000);
    buffer[7] = '0' + ((clock_state.year / 100) % 10);
    buffer[8] = '0' + ((clock_state.year / 10) % 10);
    buffer[9] = '0' + (clock_state.year % 10);
    buffer[10] = '\0';
}

/* Milisaniye bekle */
void timer_wait_ms(uint32_t ms) {
    uint64_t target = timer_get_ticks() + (ms * 1000);
    while(timer_get_ticks() < target) {
        __asm__ volatile("nop");
    }
}

/* Mikrosaniye bekle */
void timer_wait_us(uint32_t us) {
    uint64_t target = timer_get_ticks() + us;
    while(timer_get_ticks() < target) {
        __asm__ volatile("nop");
    }
}
