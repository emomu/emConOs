/* graphics.c - Grafik fonksiyonları */
#include "graphics.h"

/* Framebuffer değişkenleri */
uint32_t screen_width, screen_height, screen_pitch;
uint8_t *framebuffer;

void draw_pixel(int x, int y, uint32_t color) {
    if(x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;

    uint32_t offset = (y * screen_pitch) + (x * 4);
    volatile uint32_t *pixel_addr = (volatile uint32_t *)(framebuffer + offset);
    *pixel_addr = color;
}

void draw_rect(int x, int y, int w, int h, uint32_t color) {
    for(int j = y; j < y + h; j++) {
        for(int i = x; i < x + w; i++) {
            draw_pixel(i, j, color);
        }
    }
}

void draw_rect_outline(int x, int y, int w, int h, int thickness, uint32_t color) {
    draw_rect(x, y, w, thickness, color);
    draw_rect(x, y + h - thickness, w, thickness, color);
    draw_rect(x, y, thickness, h, color);
    draw_rect(x + w - thickness, y, thickness, h, color);
}

void clear_screen(uint32_t color) {
    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, color);
}

void draw_gradient_bg(uint32_t color_top, uint32_t color_bottom) {
    uint8_t r1 = (color_top >> 16) & 0xFF;
    uint8_t g1 = (color_top >> 8) & 0xFF;
    uint8_t b1 = color_top & 0xFF;

    uint8_t r2 = (color_bottom >> 16) & 0xFF;
    uint8_t g2 = (color_bottom >> 8) & 0xFF;
    uint8_t b2 = color_bottom & 0xFF;

    for(int y = 0; y < SCREEN_HEIGHT; y++) {
        uint8_t r = r1 + (r2 - r1) * y / SCREEN_HEIGHT;
        uint8_t g = g1 + (g2 - g1) * y / SCREEN_HEIGHT;
        uint8_t b = b1 + (b2 - b1) * y / SCREEN_HEIGHT;
        uint32_t color = 0xFF000000 | (r << 16) | (g << 8) | b;

        for(int x = 0; x < SCREEN_WIDTH; x++) {
            draw_pixel(x, y, color);
        }
    }
}
