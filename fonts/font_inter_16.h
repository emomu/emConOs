/* font_inter_16.h - Inter Regular 16px */
#ifndef FONT_INTER_16_H
#define FONT_INTER_16_H

#include "../types.h"

#define FONT_INTER_16_HEIGHT 20
#define FONT_INTER_16_FIRST_CHAR 32
#define FONT_INTER_16_LAST_CHAR 126
#define FONT_INTER_16_CHAR_COUNT 95

/* Karakter bilgisi */
typedef struct {
    uint8_t width;      /* Karakter genişliği */
    uint32_t offset;    /* Piksel verisinin başlangıç offseti */
} font_inter_16_Glyph;

/* Font verileri */
extern const font_inter_16_Glyph font_inter_16_glyphs[95];
extern const uint8_t font_inter_16_pixels[];

/* Karakter çizme fonksiyonu */
void font_inter_16_draw_char(int x, int y, char c, uint32_t color);
void font_inter_16_draw_text(int x, int y, const char* text, uint32_t color);
int font_inter_16_text_width(const char* text);

#endif
