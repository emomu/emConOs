/* graphics.c - Grafik fonksiyonları (çift tamponlama destekli) */
#include <graphics.h>

/* Framebuffer değişkenleri */
uint32_t screen_width, screen_height, screen_pitch;
uint8_t *framebuffer;

/* Çift tamponlama için statik back buffer */
/* 640 x 480 x 4 bytes = 1,228,800 bytes (~1.2MB) */
static uint8_t back_buffer_data[SCREEN_WIDTH * SCREEN_HEIGHT * 4] __attribute__((aligned(16)));

/* Buffer pointer'ları */
uint8_t *draw_buffer;      /* Çizim yapılan buffer (back buffer) */
uint8_t *display_buffer;   /* Görüntülenen buffer (framebuffer) */

/* Inline memcpy for buffer copy */
static void fast_memcpy32(uint32_t *dst, uint32_t *src, uint32_t count) {
    while(count >= 8) {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        dst[3] = src[3];
        dst[4] = src[4];
        dst[5] = src[5];
        dst[6] = src[6];
        dst[7] = src[7];
        dst += 8;
        src += 8;
        count -= 8;
    }
    while(count--) {
        *dst++ = *src++;
    }
}

void draw_pixel(int x, int y, uint32_t color) {
    if(x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;

    /* Back buffer'a çiz (sabit pitch kullan) */
    uint32_t offset = (y * SCREEN_WIDTH * 4) + (x * 4);
    uint32_t *pixel_addr = (uint32_t *)(draw_buffer + offset);
    *pixel_addr = color;
}

void draw_rect(int x, int y, int w, int h, uint32_t color) {
    /* Sınırları kontrol et */
    if(x < 0) { w += x; x = 0; }
    if(y < 0) { h += y; y = 0; }
    if(x + w > SCREEN_WIDTH) w = SCREEN_WIDTH - x;
    if(y + h > SCREEN_HEIGHT) h = SCREEN_HEIGHT - y;
    if(w <= 0 || h <= 0) return;

    /* Her satırı tek seferde doldur */
    for(int j = y; j < y + h; j++) {
        uint32_t *row = (uint32_t *)(draw_buffer + (j * SCREEN_WIDTH * 4) + (x * 4));
        for(int i = 0; i < w; i++) {
            row[i] = color;
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
    uint32_t *buf = (uint32_t *)draw_buffer;
    uint32_t total = SCREEN_WIDTH * SCREEN_HEIGHT;

    /* Hızlı doldurma */
    for(uint32_t i = 0; i < total; i++) {
        buf[i] = color;
    }
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

        uint32_t *row = (uint32_t *)(draw_buffer + (y * SCREEN_WIDTH * 4));
        for(int x = 0; x < SCREEN_WIDTH; x++) {
            row[x] = color;
        }
    }
}

/* Buffer değiştir - back buffer'ı framebuffer'a kopyala */
void graphics_swap_buffers(void) {
    if(!framebuffer) return;

    /* Memory barrier before copy */
    __asm__ volatile("dsb sy");

    /* Back buffer'ı framebuffer'a kopyala */
    /* Pitch farklı olabilir, satır satır kopyala */
    for(int y = 0; y < SCREEN_HEIGHT; y++) {
        uint32_t *src = (uint32_t *)(draw_buffer + (y * SCREEN_WIDTH * 4));
        uint32_t *dst = (uint32_t *)(framebuffer + (y * screen_pitch));
        fast_memcpy32(dst, src, SCREEN_WIDTH);
    }

    /* Memory barrier after copy */
    __asm__ volatile("dsb sy");
}

/* Grafik sistemi başlatıldığında çağrılacak */
void graphics_init_buffers(void) {
    draw_buffer = back_buffer_data;
    display_buffer = framebuffer;
}
