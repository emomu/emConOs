/* types.h - Temel tip tanımlamaları */
#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stddef.h>

/* Renkler (ARGB formatı) */
#define COLOR_BLACK       0xFF000000
#define COLOR_WHITE       0xFFFFFFFF
#define COLOR_BLUE        0xFF0066CC
#define COLOR_DARK_BLUE   0xFF003366
#define COLOR_GREEN       0xFF00AA00
#define COLOR_GRAY        0xFF808080
#define COLOR_LIGHT_GRAY  0xFFC0C0C0
#define COLOR_RED         0xFFFF0000
#define COLOR_YELLOW      0xFFFFD800
#define COLOR_ORANGE      0xFFFF8000

/* Ekran boyutları */
#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 480

/* Yol boyutu */
#define MAX_PATH 512

/* Inline memcpy ve memset */
static inline void *memcpy(void *dest, const void *src, size_t n) {
    uint8_t *d = (uint8_t*)dest;
    const uint8_t *s = (const uint8_t*)src;
    while(n--) *d++ = *s++;
    return dest;
}

static inline void *memset(void *s, int c, size_t n) {
    uint8_t *p = (uint8_t*)s;
    while(n--) *p++ = (uint8_t)c;
    return s;
}

#endif
