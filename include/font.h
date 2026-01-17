/* font.h - 32x32 Anti-aliased Font */
#ifndef FONT_H
#define FONT_H

#include <types.h>

/* Font boyutları */
#define FONT_WIDTH  16
#define FONT_HEIGHT 24

/* Font çizim fonksiyonları */
void draw_char_aa(int x, int y, char c, uint32_t color, int scale);
void draw_string_aa(int x, int y, const char *str, uint32_t color, int scale);
int string_width_aa(const char *str, int scale);

#endif
