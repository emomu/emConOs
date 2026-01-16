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

/* Ekran durumları */
#define SCREEN_WELCOME  0
#define SCREEN_MAIN     1

#endif
