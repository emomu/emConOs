/* graphics.h - Grafik fonksiyonları */
#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <types.h>

/* Framebuffer değişkenleri (extern) */
extern uint32_t screen_width, screen_height, screen_pitch;
extern uint8_t *framebuffer;

/* Çift tamponlama için back buffer */
extern uint8_t *draw_buffer;      /* Çizim yapılan buffer */
extern uint8_t *display_buffer;   /* Görüntülenen buffer */

/* Temel çizim fonksiyonları */
void draw_pixel(int x, int y, uint32_t color);
void draw_rect(int x, int y, int w, int h, uint32_t color);
void draw_rect_outline(int x, int y, int w, int h, int thickness, uint32_t color);
void clear_screen(uint32_t color);

/* Gradient arka plan */
void draw_gradient_bg(uint32_t color_top, uint32_t color_bottom);
void draw_gradient_rect(int x, int y, int w, int h, uint32_t color_top, uint32_t color_bottom);

/* Modern UI efektleri */
void draw_rect_alpha(int x, int y, int w, int h, uint32_t color, uint8_t alpha);
void draw_glass_panel(int x, int y, int w, int h, uint32_t tint, uint8_t alpha);
void draw_rounded_rect(int x, int y, int w, int h, int radius, uint32_t color);
void draw_rounded_rect_alpha(int x, int y, int w, int h, int radius, uint32_t color, uint8_t alpha);
void draw_shadow(int x, int y, int w, int h, int blur, uint8_t intensity);
void draw_glow(int x, int y, int w, int h, uint32_t color, int size);
void draw_line_h(int x, int y, int w, uint32_t color);
void draw_line_v(int x, int y, int h, uint32_t color);

/* Çift tamponlama */
void graphics_init_buffers(void);
void graphics_swap_buffers(void);

#endif
