/* graphics.h - Grafik fonksiyonları */
#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "types.h"

/* Framebuffer değişkenleri (extern) */
extern uint32_t screen_width, screen_height, screen_pitch;
extern uint8_t *framebuffer;

/* Temel çizim fonksiyonları */
void draw_pixel(int x, int y, uint32_t color);
void draw_rect(int x, int y, int w, int h, uint32_t color);
void draw_rect_outline(int x, int y, int w, int h, int thickness, uint32_t color);
void clear_screen(uint32_t color);

/* Gradient arka plan */
void draw_gradient_bg(uint32_t color_top, uint32_t color_bottom);

#endif
