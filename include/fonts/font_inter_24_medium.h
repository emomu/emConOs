/* font_inter_24_medium.h - Inter Medium 24px */
#ifndef FONT_INTER_24_MEDIUM_H
#define FONT_INTER_24_MEDIUM_H

#include <types.h>

#define FONT_INTER_24_MEDIUM_HEIGHT 30
#define FONT_INTER_24_MEDIUM_FIRST_CHAR 32
#define FONT_INTER_24_MEDIUM_LAST_CHAR 126
#define FONT_INTER_24_MEDIUM_CHAR_COUNT 95

/* Karakter bilgisi */
typedef struct {
    uint8_t width;      /* Karakter genişliği */
    uint32_t offset;    /* Piksel verisinin başlangıç offseti */
} font_inter_24_medium_Glyph;

/* Font verileri */
extern const font_inter_24_medium_Glyph font_inter_24_medium_glyphs[95];
extern const uint8_t font_inter_24_medium_pixels[];

/* Karakter çizme fonksiyonu */
void font_inter_24_medium_draw_char(int x, int y, char c, uint32_t color);
void font_inter_24_medium_draw_text(int x, int y, const char* text, uint32_t color);
int font_inter_24_medium_text_width(const char* text);

#endif
