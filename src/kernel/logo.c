/* logo.c - Gamepad Logo */
#include <logo.h>
#include <graphics.h>

/* 24x16 GAMEPAD LOGOSU (Renkli) */
/* Renk kodları: 0=şeffaf, 1=siyah(outline), 2=beyaz(body), 3=gri(gölge),
   4=mavi, 5=yeşil, 6=kırmızı, 7=sarı */
static const uint8_t gamepad_pixels[16][24] = {
    {0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0},
    {0,0,0,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,0,0,0},
    {0,0,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,0,0},
    {0,1,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,4,2,2,2,2,1,0},
    {0,1,2,2,1,1,1,2,2,1,1,2,2,2,2,5,2,2,2,6,2,2,1,0},
    {1,2,2,2,2,1,2,2,2,1,1,2,2,2,2,2,2,7,2,2,2,2,2,1},
    {1,2,2,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,2,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {0,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,0},
    {0,1,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,1,0},
    {0,0,1,3,2,2,2,2,2,1,1,1,1,1,1,2,2,2,2,2,3,1,0,0},
    {0,0,0,1,3,2,2,2,1,0,0,0,0,0,0,1,2,2,2,3,1,0,0,0},
    {0,0,0,0,1,3,3,1,0,0,0,0,0,0,0,0,1,3,3,1,0,0,0,0},
    {0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0},
};

/* Renk paleti */
static const uint32_t gamepad_palette[8] = {
    0x00000000,  /* 0: Şeffaf */
    0xFF000000,  /* 1: Siyah (outline) */
    0xFFFFFFFF,  /* 2: Beyaz (body) */
    0xFFC0C0C0,  /* 3: Gri (gölge) */
    0xFF0080FF,  /* 4: Mavi */
    0xFF00C000,  /* 5: Yeşil */
    0xFFFF0000,  /* 6: Kırmızı */
    0xFFFFD800,  /* 7: Sarı */
};

void draw_logo(int x, int y, int scale) {
    for(int row = 0; row < 16; row++) {
        for(int col = 0; col < 24; col++) {
            uint8_t color_idx = gamepad_pixels[row][col];
            if(color_idx != 0) {
                uint32_t color = gamepad_palette[color_idx];
                for(int sy = 0; sy < scale; sy++) {
                    for(int sx = 0; sx < scale; sx++) {
                        draw_pixel(x + col * scale + sx, y + row * scale + sy, color);
                    }
                }
            }
        }
    }
}
