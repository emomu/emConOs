/* font_inter.h - Inter Medium 20px font */
#ifndef FONT_INTER_H
#define FONT_INTER_H

#include "types.h"

#define FONT_HEIGHT 24
#define FONT_FIRST_CHAR 32
#define FONT_LAST_CHAR 126
#define FONT_CHAR_COUNT 95

/* Karakter bilgisi */
typedef struct {
    uint8_t width;      /* Karakter genişliği */
    uint16_t offset;    /* Piksel verisinin başlangıç offseti */
} FontGlyph;

/* Font verileri */
extern const FontGlyph font_glyphs[FONT_CHAR_COUNT];
extern const uint8_t font_pixels[];

/* Karakter çizme fonksiyonu */
void draw_char_inter(int x, int y, char c, uint32_t color);
void draw_text_inter(int x, int y, const char* text, uint32_t color);
int get_text_width(const char* text);

#endif
