/* screens.c - Ekran çizim fonksiyonları */
#include "screens.h"
#include "graphics.h"
#include "fonts/fonts.h"
#include "logo.h"
#include "ui/filemgr.h"

/* Ekran durumu */
int current_screen = SCREEN_WELCOME;

/* Hafıza bariyeri */
static void mem_barrier(void) {
    __asm__ volatile("dsb sy");
}

/* Ekran geçişi */
void switch_screen(int screen) {
    current_screen = screen;

    switch(screen) {
        case SCREEN_WELCOME:
            draw_welcome_screen();
            break;
        case SCREEN_MAIN:
            draw_main_screen();
            break;
        case SCREEN_FILES:
            filemgr_init();
            draw_files_screen();
            break;
        case SCREEN_SETTINGS:
            draw_settings_screen();
            break;
        case SCREEN_ABOUT:
            draw_about_screen();
            break;
    }
}

/* --- HOŞGELDİNİZ EKRANI --- */
void draw_welcome_screen(void) {
    /* Gradient arka plan (koyu mavi -> açık mavi) */
    draw_gradient_bg(0xFF001030, 0xFF003080);

    /* Logo ve yazı yan yana */
    const char *welcome = "Hosgeldiniz!";
    int logo_scale = 6;
    int logo_w = 24 * logo_scale;
    int logo_h = 16 * logo_scale;
    int wel_w = text_width_24(welcome);
    int gap = 30;
    int total_w = logo_w + gap + wel_w;
    int start_x = (SCREEN_WIDTH - total_w) / 2;
    int row_y = (SCREEN_HEIGHT - logo_h) / 2;

    /* Sol: Gamepad Logo */
    draw_logo(start_x, row_y, logo_scale);

    /* Sağ: Hoşgeldiniz yazısı (24px medium) */
    int text_y = row_y + (logo_h - FONT_HEIGHT_24) / 2;
    draw_text_24(start_x + logo_w + gap, text_y, welcome, COLOR_WHITE);

    mem_barrier();
}

/* --- ANA EKRAN --- */
void draw_main_screen(void) {
    /* Arka plan */
    clear_screen(COLOR_DARK_BLUE);

    /* Üst bar */
    draw_rect(0, 0, SCREEN_WIDTH, 50, COLOR_BLUE);
    draw_text_20_bold(15, 12, "EmConOs", COLOR_WHITE);

    /* Saat alanı */
    draw_text_16(SCREEN_WIDTH - 70, 17, "12:00", COLOR_WHITE);

    /* Ana menü başlığı (32px bold) */
    const char *menu_title = "Ana Menu";
    int mt_w = text_width_32(menu_title);
    draw_text_32_bold((SCREEN_WIDTH - mt_w) / 2, 70, menu_title, COLOR_WHITE);

    /* Menü kutuları */
    int box_w = 140;
    int box_h = 100;
    int gap = 20;
    int start_x = (SCREEN_WIDTH - (4 * box_w + 3 * gap)) / 2;
    int start_y = 160;

    /* Kutu 1: Oyunlar */
    draw_rect(start_x, start_y, box_w, box_h, COLOR_BLUE);
    draw_rect_outline(start_x, start_y, box_w, box_h, 3, COLOR_WHITE);
    const char *games = "Oyunlar";
    int g_w = text_width_20(games);
    draw_text_20_medium(start_x + (box_w - g_w) / 2, start_y + (box_h - FONT_HEIGHT_20) / 2, games, COLOR_WHITE);

    /* Kutu 2: Dosyalar */
    draw_rect(start_x + box_w + gap, start_y, box_w, box_h, COLOR_BLUE);
    draw_rect_outline(start_x + box_w + gap, start_y, box_w, box_h, 3, COLOR_LIGHT_GRAY);
    const char *files = "Dosyalar";
    int f_w = text_width_20(files);
    draw_text_20_medium(start_x + box_w + gap + (box_w - f_w) / 2, start_y + (box_h - FONT_HEIGHT_20) / 2, files, COLOR_WHITE);

    /* Kutu 3: Ayarlar */
    draw_rect(start_x + 2 * (box_w + gap), start_y, box_w, box_h, COLOR_BLUE);
    draw_rect_outline(start_x + 2 * (box_w + gap), start_y, box_w, box_h, 3, COLOR_LIGHT_GRAY);
    const char *settings = "Ayarlar";
    int s_w = text_width_20(settings);
    draw_text_20_medium(start_x + 2 * (box_w + gap) + (box_w - s_w) / 2, start_y + (box_h - FONT_HEIGHT_20) / 2, settings, COLOR_WHITE);

    /* Kutu 4: Hakkinda */
    draw_rect(start_x + 3 * (box_w + gap), start_y, box_w, box_h, COLOR_BLUE);
    draw_rect_outline(start_x + 3 * (box_w + gap), start_y, box_w, box_h, 3, COLOR_LIGHT_GRAY);
    const char *about = "Hakkinda";
    int a_w = text_width_20(about);
    draw_text_20_medium(start_x + 3 * (box_w + gap) + (box_w - a_w) / 2, start_y + (box_h - FONT_HEIGHT_20) / 2, about, COLOR_WHITE);

    /* Alt bilgi çubuğu */
    draw_rect(0, SCREEN_HEIGHT - 45, SCREEN_WIDTH, 45, COLOR_BLUE);
    draw_text_16(15, SCREEN_HEIGHT - 32, "A: Sec   B: Geri   D-Pad: Gezin", COLOR_WHITE);

    mem_barrier();
}

/* --- DOSYA YÖNETİCİSİ EKRANI --- */
void draw_files_screen(void) {
    filemgr_draw();
    mem_barrier();
}

/* --- AYARLAR EKRANI --- */
void draw_settings_screen(void) {
    clear_screen(COLOR_DARK_BLUE);

    /* Başlık */
    draw_rect(0, 0, SCREEN_WIDTH, 50, COLOR_BLUE);
    draw_text_20_bold(15, 12, "Ayarlar", COLOR_WHITE);

    /* Ayar öğeleri */
    int y = 80;
    int item_h = 50;

    draw_rect(20, y, SCREEN_WIDTH - 40, item_h, 0xFF1A3A5C);
    draw_text_20(40, y + 12, "Ses Seviyesi", COLOR_WHITE);
    draw_text_16(SCREEN_WIDTH - 100, y + 15, "80%", COLOR_LIGHT_GRAY);
    y += item_h + 10;

    draw_rect(20, y, SCREEN_WIDTH - 40, item_h, 0xFF1A3A5C);
    draw_text_20(40, y + 12, "Parlaklik", COLOR_WHITE);
    draw_text_16(SCREEN_WIDTH - 100, y + 15, "100%", COLOR_LIGHT_GRAY);
    y += item_h + 10;

    draw_rect(20, y, SCREEN_WIDTH - 40, item_h, 0xFF1A3A5C);
    draw_text_20(40, y + 12, "WiFi", COLOR_WHITE);
    draw_text_16(SCREEN_WIDTH - 100, y + 15, "Kapali", COLOR_LIGHT_GRAY);
    y += item_h + 10;

    draw_rect(20, y, SCREEN_WIDTH - 40, item_h, 0xFF1A3A5C);
    draw_text_20(40, y + 12, "Bluetooth", COLOR_WHITE);
    draw_text_16(SCREEN_WIDTH - 100, y + 15, "Kapali", COLOR_LIGHT_GRAY);

    /* Alt bilgi */
    draw_rect(0, SCREEN_HEIGHT - 45, SCREEN_WIDTH, 45, COLOR_BLUE);
    draw_text_16(15, SCREEN_HEIGHT - 32, "B: Geri", COLOR_WHITE);

    mem_barrier();
}

/* --- HAKKINDA EKRANI --- */
void draw_about_screen(void) {
    clear_screen(COLOR_DARK_BLUE);

    /* Başlık */
    draw_rect(0, 0, SCREEN_WIDTH, 50, COLOR_BLUE);
    draw_text_20_bold(15, 12, "Hakkinda", COLOR_WHITE);

    /* Logo */
    draw_logo((SCREEN_WIDTH - 24 * 4) / 2, 80, 4);

    /* Bilgiler */
    int y = 180;

    const char *name = "EmConOs";
    int nw = text_width_32(name);
    draw_text_32_bold((SCREEN_WIDTH - nw) / 2, y, name, COLOR_WHITE);
    y += 50;

    const char *ver = "Surum 1.0";
    int vw = text_width_20(ver);
    draw_text_20((SCREEN_WIDTH - vw) / 2, y, ver, COLOR_LIGHT_GRAY);
    y += 40;

    const char *dev = "Gelistirici: Emirhan Soylu";
    int dw = text_width_16(dev);
    draw_text_16((SCREEN_WIDTH - dw) / 2, y, dev, COLOR_LIGHT_GRAY);
    y += 30;

    const char *hw = "Platform: Raspberry Pi Zero 2W";
    int hw_w = text_width_16(hw);
    draw_text_16((SCREEN_WIDTH - hw_w) / 2, y, hw, COLOR_LIGHT_GRAY);

    /* Alt bilgi */
    draw_rect(0, SCREEN_HEIGHT - 45, SCREEN_WIDTH, 45, COLOR_BLUE);
    draw_text_16(15, SCREEN_HEIGHT - 32, "B: Geri", COLOR_WHITE);

    mem_barrier();
}
