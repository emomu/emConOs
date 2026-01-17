/* font_inter_32.h - Inter Regular 32px */
#ifndef FONT_INTER_32_H
#define FONT_INTER_32_H

#include "types.h"

#define FONT_INTER_32_HEIGHT 39
#define FONT_INTER_32_FIRST_CHAR 32
#define FONT_INTER_32_LAST_CHAR 126
#define FONT_INTER_32_ASCII_COUNT 95
#define FONT_INTER_32_TURKISH_COUNT 12
#define FONT_INTER_32_CHAR_COUNT (95 + 12)

/* Türkçe karakter indeksleri */
#define TR_IDX_G_BREVE_LOWER  0   /* ğ */
#define TR_IDX_G_BREVE_UPPER  1   /* Ğ */
#define TR_IDX_U_UMLAUT_LOWER 2   /* ü */
#define TR_IDX_U_UMLAUT_UPPER 3   /* Ü */
#define TR_IDX_S_CEDILLA_LOWER 4  /* ş */
#define TR_IDX_S_CEDILLA_UPPER 5  /* Ş */
#define TR_IDX_I_DOTLESS      6   /* ı */
#define TR_IDX_I_DOTTED       7   /* İ */
#define TR_IDX_O_UMLAUT_LOWER 8   /* ö */
#define TR_IDX_O_UMLAUT_UPPER 9   /* Ö */
#define TR_IDX_C_CEDILLA_LOWER 10 /* ç */
#define TR_IDX_C_CEDILLA_UPPER 11 /* Ç */

/* Karakter bilgisi */
typedef struct {
    uint8_t width;      /* Karakter genişliği */
    uint32_t offset;    /* Piksel verisinin başlangıç offseti */
} font_inter_32_Glyph;

/* Font verileri */
extern const font_inter_32_Glyph font_inter_32_glyphs[95];
extern const font_inter_32_Glyph font_inter_32_turkish_glyphs[12];
extern const uint8_t font_inter_32_pixels[];

/* Karakter çizme fonksiyonu */
void font_inter_32_draw_char(int x, int y, char c, uint32_t color);
void font_inter_32_draw_turkish_char(int x, int y, int tr_index, uint32_t color);
void font_inter_32_draw_text(int x, int y, const char* text, uint32_t color);
int font_inter_32_text_width(const char* text);
int font_inter_32_turkish_char_width(int tr_index);

#endif
