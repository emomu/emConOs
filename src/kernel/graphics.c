/* graphics.c - Modern UI grafik fonksiyonları (çift tamponlama destekli) */
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

/* Piksel okuma (alpha blending için) */
static uint32_t read_pixel(int x, int y) {
    if(x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return 0;
    uint32_t offset = (y * SCREEN_WIDTH * 4) + (x * 4);
    return *((uint32_t *)(draw_buffer + offset));
}

/* Alpha blend iki rengi */
static uint32_t blend_colors(uint32_t bg, uint32_t fg, uint8_t alpha) {
    uint8_t inv_alpha = 255 - alpha;

    uint8_t bg_r = (bg >> 16) & 0xFF;
    uint8_t bg_g = (bg >> 8) & 0xFF;
    uint8_t bg_b = bg & 0xFF;

    uint8_t fg_r = (fg >> 16) & 0xFF;
    uint8_t fg_g = (fg >> 8) & 0xFF;
    uint8_t fg_b = fg & 0xFF;

    uint8_t r = (fg_r * alpha + bg_r * inv_alpha) / 255;
    uint8_t g = (fg_g * alpha + bg_g * inv_alpha) / 255;
    uint8_t b = (fg_b * alpha + bg_b * inv_alpha) / 255;

    return 0xFF000000 | (r << 16) | (g << 8) | b;
}

void draw_pixel(int x, int y, uint32_t color) {
    if(x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;

    /* Back buffer'a çiz (sabit pitch kullan) */
    uint32_t offset = (y * SCREEN_WIDTH * 4) + (x * 4);
    uint32_t *pixel_addr = (uint32_t *)(draw_buffer + offset);
    *pixel_addr = color;
}

/* Alpha destekli piksel çizimi */
static void draw_pixel_alpha(int x, int y, uint32_t color, uint8_t alpha) {
    if(x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
    if(alpha == 0) return;

    uint32_t offset = (y * SCREEN_WIDTH * 4) + (x * 4);
    uint32_t *pixel_addr = (uint32_t *)(draw_buffer + offset);

    if(alpha == 255) {
        *pixel_addr = color;
    } else {
        *pixel_addr = blend_colors(*pixel_addr, color, alpha);
    }
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

/* Alpha destekli dikdörtgen */
void draw_rect_alpha(int x, int y, int w, int h, uint32_t color, uint8_t alpha) {
    if(alpha == 255) {
        draw_rect(x, y, w, h, color);
        return;
    }
    if(alpha == 0) return;

    /* Sınırları kontrol et */
    if(x < 0) { w += x; x = 0; }
    if(y < 0) { h += y; y = 0; }
    if(x + w > SCREEN_WIDTH) w = SCREEN_WIDTH - x;
    if(y + h > SCREEN_HEIGHT) h = SCREEN_HEIGHT - y;
    if(w <= 0 || h <= 0) return;

    for(int j = y; j < y + h; j++) {
        uint32_t *row = (uint32_t *)(draw_buffer + (j * SCREEN_WIDTH * 4) + (x * 4));
        for(int i = 0; i < w; i++) {
            row[i] = blend_colors(row[i], color, alpha);
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
    int16_t r1 = (color_top >> 16) & 0xFF;
    int16_t g1 = (color_top >> 8) & 0xFF;
    int16_t b1 = color_top & 0xFF;

    int16_t r2 = (color_bottom >> 16) & 0xFF;
    int16_t g2 = (color_bottom >> 8) & 0xFF;
    int16_t b2 = color_bottom & 0xFF;

    for(int y = 0; y < SCREEN_HEIGHT; y++) {
        int16_t r = r1 + (r2 - r1) * y / SCREEN_HEIGHT;
        int16_t g = g1 + (g2 - g1) * y / SCREEN_HEIGHT;
        int16_t b = b1 + (b2 - b1) * y / SCREEN_HEIGHT;
        uint32_t color = 0xFF000000 | (r << 16) | (g << 8) | b;

        uint32_t *row = (uint32_t *)(draw_buffer + (y * SCREEN_WIDTH * 4));
        for(int x = 0; x < SCREEN_WIDTH; x++) {
            row[x] = color;
        }
    }
}

/* Dikdörtgen içinde gradient */
void draw_gradient_rect(int x, int y, int w, int h, uint32_t color_top, uint32_t color_bottom) {
    if(x < 0) { w += x; x = 0; }
    if(y < 0) { h += y; y = 0; }
    if(x + w > SCREEN_WIDTH) w = SCREEN_WIDTH - x;
    if(y + h > SCREEN_HEIGHT) h = SCREEN_HEIGHT - y;
    if(w <= 0 || h <= 0) return;

    int16_t r1 = (color_top >> 16) & 0xFF;
    int16_t g1 = (color_top >> 8) & 0xFF;
    int16_t b1 = color_top & 0xFF;

    int16_t r2 = (color_bottom >> 16) & 0xFF;
    int16_t g2 = (color_bottom >> 8) & 0xFF;
    int16_t b2 = color_bottom & 0xFF;

    for(int j = 0; j < h; j++) {
        int16_t r = r1 + (r2 - r1) * j / h;
        int16_t g = g1 + (g2 - g1) * j / h;
        int16_t b = b1 + (b2 - b1) * j / h;
        uint32_t color = 0xFF000000 | (r << 16) | (g << 8) | b;

        uint32_t *row = (uint32_t *)(draw_buffer + ((y + j) * SCREEN_WIDTH * 4) + (x * 4));
        for(int i = 0; i < w; i++) {
            row[i] = color;
        }
    }
}

/* Cam efektli panel - yarı saydam blur benzeri efekt */
void draw_glass_panel(int x, int y, int w, int h, uint32_t tint, uint8_t alpha) {
    if(x < 0) { w += x; x = 0; }
    if(y < 0) { h += y; y = 0; }
    if(x + w > SCREEN_WIDTH) w = SCREEN_WIDTH - x;
    if(y + h > SCREEN_HEIGHT) h = SCREEN_HEIGHT - y;
    if(w <= 0 || h <= 0) return;

    uint8_t tint_r = (tint >> 16) & 0xFF;
    uint8_t tint_g = (tint >> 8) & 0xFF;
    uint8_t tint_b = tint & 0xFF;

    for(int j = y; j < y + h; j++) {
        uint32_t *row = (uint32_t *)(draw_buffer + (j * SCREEN_WIDTH * 4) + (x * 4));
        for(int i = 0; i < w; i++) {
            uint32_t bg = row[i];
            uint8_t bg_r = (bg >> 16) & 0xFF;
            uint8_t bg_g = (bg >> 8) & 0xFF;
            uint8_t bg_b = bg & 0xFF;

            /* Blur simülasyonu: arka plan rengini açıklaştır ve tint uygula */
            uint8_t light_r = bg_r + (255 - bg_r) / 4;
            uint8_t light_g = bg_g + (255 - bg_g) / 4;
            uint8_t light_b = bg_b + (255 - bg_b) / 4;

            /* Tint ile karıştır */
            uint8_t inv_alpha = 255 - alpha;
            uint8_t r = (tint_r * alpha + light_r * inv_alpha) / 255;
            uint8_t g = (tint_g * alpha + light_g * inv_alpha) / 255;
            uint8_t b = (tint_b * alpha + light_b * inv_alpha) / 255;

            row[i] = 0xFF000000 | (r << 16) | (g << 8) | b;
        }
    }

    /* Üst kenara ince parlak çizgi (cam yansıması) */
    for(int i = x; i < x + w && i < SCREEN_WIDTH; i++) {
        draw_pixel_alpha(i, y, 0xFFFFFFFF, 30);
    }

    /* Sol kenara ince parlak çizgi */
    for(int j = y; j < y + h && j < SCREEN_HEIGHT; j++) {
        draw_pixel_alpha(x, j, 0xFFFFFFFF, 15);
    }
}

/* Yuvarlak köşeli dikdörtgen */
void draw_rounded_rect(int x, int y, int w, int h, int radius, uint32_t color) {
    if(radius <= 0) {
        draw_rect(x, y, w, h, color);
        return;
    }

    if(radius > w / 2) radius = w / 2;
    if(radius > h / 2) radius = h / 2;

    /* Ana gövde */
    draw_rect(x + radius, y, w - 2 * radius, h, color);
    draw_rect(x, y + radius, radius, h - 2 * radius, color);
    draw_rect(x + w - radius, y + radius, radius, h - 2 * radius, color);

    /* Köşeler (basit yaklaşım - daire çeyrekleri) */
    for(int j = 0; j < radius; j++) {
        for(int i = 0; i < radius; i++) {
            int dx = radius - i - 1;
            int dy = radius - j - 1;
            if(dx * dx + dy * dy <= radius * radius) {
                /* Sol üst */
                draw_pixel(x + i, y + j, color);
                /* Sağ üst */
                draw_pixel(x + w - 1 - i, y + j, color);
                /* Sol alt */
                draw_pixel(x + i, y + h - 1 - j, color);
                /* Sağ alt */
                draw_pixel(x + w - 1 - i, y + h - 1 - j, color);
            }
        }
    }
}

/* Yuvarlak köşeli alpha dikdörtgen */
void draw_rounded_rect_alpha(int x, int y, int w, int h, int radius, uint32_t color, uint8_t alpha) {
    if(alpha == 255) {
        draw_rounded_rect(x, y, w, h, radius, color);
        return;
    }
    if(alpha == 0) return;

    if(radius <= 0) {
        draw_rect_alpha(x, y, w, h, color, alpha);
        return;
    }

    if(radius > w / 2) radius = w / 2;
    if(radius > h / 2) radius = h / 2;

    /* Ana gövde */
    draw_rect_alpha(x + radius, y, w - 2 * radius, h, color, alpha);
    draw_rect_alpha(x, y + radius, radius, h - 2 * radius, color, alpha);
    draw_rect_alpha(x + w - radius, y + radius, radius, h - 2 * radius, color, alpha);

    /* Köşeler */
    for(int j = 0; j < radius; j++) {
        for(int i = 0; i < radius; i++) {
            int dx = radius - i - 1;
            int dy = radius - j - 1;
            if(dx * dx + dy * dy <= radius * radius) {
                draw_pixel_alpha(x + i, y + j, color, alpha);
                draw_pixel_alpha(x + w - 1 - i, y + j, color, alpha);
                draw_pixel_alpha(x + i, y + h - 1 - j, color, alpha);
                draw_pixel_alpha(x + w - 1 - i, y + h - 1 - j, color, alpha);
            }
        }
    }
}

/* Gölge efekti */
void draw_shadow(int x, int y, int w, int h, int blur, uint8_t intensity) {
    /* Basit gölge - blur katmanları ile */
    for(int layer = blur; layer > 0; layer--) {
        uint8_t layer_alpha = intensity * layer / blur / 2;
        int offset = blur - layer + 2;
        draw_rect_alpha(x + offset, y + h, w, layer, 0xFF000000, layer_alpha);
        draw_rect_alpha(x + w, y + offset, layer, h, 0xFF000000, layer_alpha);
    }
}

/* Parlama efekti */
void draw_glow(int x, int y, int w, int h, uint32_t color, int size) {
    for(int layer = size; layer > 0; layer--) {
        uint8_t layer_alpha = 30 * layer / size;
        draw_rect_alpha(x - layer, y - layer, w + layer * 2, h + layer * 2, color, layer_alpha);
    }
}

/* Yatay çizgi */
void draw_line_h(int x, int y, int w, uint32_t color) {
    if(y < 0 || y >= SCREEN_HEIGHT) return;
    if(x < 0) { w += x; x = 0; }
    if(x + w > SCREEN_WIDTH) w = SCREEN_WIDTH - x;
    if(w <= 0) return;

    uint32_t *row = (uint32_t *)(draw_buffer + (y * SCREEN_WIDTH * 4) + (x * 4));
    for(int i = 0; i < w; i++) {
        row[i] = color;
    }
}

/* Dikey çizgi */
void draw_line_v(int x, int y, int h, uint32_t color) {
    if(x < 0 || x >= SCREEN_WIDTH) return;
    if(y < 0) { h += y; y = 0; }
    if(y + h > SCREEN_HEIGHT) h = SCREEN_HEIGHT - y;
    if(h <= 0) return;

    for(int j = y; j < y + h; j++) {
        uint32_t *pixel = (uint32_t *)(draw_buffer + (j * SCREEN_WIDTH * 4) + (x * 4));
        *pixel = color;
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
