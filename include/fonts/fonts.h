/* fonts.h - Unified font system for EmConOs */
#ifndef FONTS_H
#define FONTS_H

#include "types.h"

/* Tüm font varyantlarını dahil et */
#include "font_inter_16.h"
#include "font_inter_16_medium.h"
#include "font_inter_16_bold.h"
#include "font_inter_20.h"
#include "font_inter_20_medium.h"
#include "font_inter_20_bold.h"
#include "font_inter_24.h"
#include "font_inter_24_medium.h"
#include "font_inter_24_bold.h"
#include "font_inter_32.h"
#include "font_inter_32_medium.h"
#include "font_inter_32_bold.h"

/* Font boyutları */
typedef enum {
    FONT_SIZE_16 = 16,
    FONT_SIZE_20 = 20,
    FONT_SIZE_24 = 24,
    FONT_SIZE_32 = 32
} FontSize;

/* Font kalınlıkları */
typedef enum {
    FONT_WEIGHT_REGULAR,
    FONT_WEIGHT_MEDIUM,
    FONT_WEIGHT_BOLD
} FontWeight;

/* Kolay kullanım makroları - varsayılan olarak medium kullan */

/* 16px fontlar */
#define draw_text_16(x, y, text, color)         font_inter_16_medium_draw_text(x, y, text, color)
#define draw_text_16_regular(x, y, text, color) font_inter_16_draw_text(x, y, text, color)
#define draw_text_16_medium(x, y, text, color)  font_inter_16_medium_draw_text(x, y, text, color)
#define draw_text_16_bold(x, y, text, color)    font_inter_16_bold_draw_text(x, y, text, color)
#define text_width_16(text)                      font_inter_16_medium_text_width(text)

/* 20px fontlar */
#define draw_text_20(x, y, text, color)         font_inter_20_medium_draw_text(x, y, text, color)
#define draw_text_20_regular(x, y, text, color) font_inter_20_draw_text(x, y, text, color)
#define draw_text_20_medium(x, y, text, color)  font_inter_20_medium_draw_text(x, y, text, color)
#define draw_text_20_bold(x, y, text, color)    font_inter_20_bold_draw_text(x, y, text, color)
#define text_width_20(text)                      font_inter_20_medium_text_width(text)

/* 24px fontlar */
#define draw_text_24(x, y, text, color)         font_inter_24_medium_draw_text(x, y, text, color)
#define draw_text_24_regular(x, y, text, color) font_inter_24_draw_text(x, y, text, color)
#define draw_text_24_medium(x, y, text, color)  font_inter_24_medium_draw_text(x, y, text, color)
#define draw_text_24_bold(x, y, text, color)    font_inter_24_bold_draw_text(x, y, text, color)
#define text_width_24(text)                      font_inter_24_medium_text_width(text)

/* 32px fontlar */
#define draw_text_32(x, y, text, color)         font_inter_32_medium_draw_text(x, y, text, color)
#define draw_text_32_regular(x, y, text, color) font_inter_32_draw_text(x, y, text, color)
#define draw_text_32_medium(x, y, text, color)  font_inter_32_medium_draw_text(x, y, text, color)
#define draw_text_32_bold(x, y, text, color)    font_inter_32_bold_draw_text(x, y, text, color)
#define text_width_32(text)                      font_inter_32_medium_text_width(text)

/* Font yükseklikleri */
#define FONT_HEIGHT_16 FONT_INTER_16_MEDIUM_HEIGHT
#define FONT_HEIGHT_20 FONT_INTER_20_MEDIUM_HEIGHT
#define FONT_HEIGHT_24 FONT_INTER_24_MEDIUM_HEIGHT
#define FONT_HEIGHT_32 FONT_INTER_32_MEDIUM_HEIGHT

#endif
