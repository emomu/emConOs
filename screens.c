/* screens.c - Ekran çizim fonksiyonları */
#include "screens.h"
#include "graphics.h"
#include "fonts/fonts.h"
#include "logo.h"

/* Ekran durumu */
int current_screen = SCREEN_WELCOME;

/* Hafıza bariyeri */
static void mem_barrier(void) {
    __asm__ volatile("dsb sy");
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
    int box_w = 160;
    int box_h = 120;
    int gap = 30;
    int start_x = (SCREEN_WIDTH - (3 * box_w + 2 * gap)) / 2;
    int start_y = 160;

    /* Kutu 1: Oyunlar (seçili) */
    draw_rect(start_x, start_y, box_w, box_h, COLOR_BLUE);
    draw_rect_outline(start_x, start_y, box_w, box_h, 3, COLOR_WHITE);
    const char *games = "Oyunlar";
    int g_w = text_width_20(games);
    draw_text_20_medium(start_x + (box_w - g_w) / 2, start_y + (box_h - FONT_HEIGHT_20) / 2, games, COLOR_WHITE);

    /* Kutu 2: Ayarlar */
    draw_rect(start_x + box_w + gap, start_y, box_w, box_h, COLOR_BLUE);
    draw_rect_outline(start_x + box_w + gap, start_y, box_w, box_h, 3, COLOR_LIGHT_GRAY);
    const char *settings = "Ayarlar";
    int s_w = text_width_20(settings);
    draw_text_20_medium(start_x + box_w + gap + (box_w - s_w) / 2, start_y + (box_h - FONT_HEIGHT_20) / 2, settings, COLOR_WHITE);

    /* Kutu 3: Hakkinda */
    draw_rect(start_x + 2 * (box_w + gap), start_y, box_w, box_h, COLOR_BLUE);
    draw_rect_outline(start_x + 2 * (box_w + gap), start_y, box_w, box_h, 3, COLOR_LIGHT_GRAY);
    const char *about = "Hakkinda";
    int a_w = text_width_20(about);
    draw_text_20_medium(start_x + 2 * (box_w + gap) + (box_w - a_w) / 2, start_y + (box_h - FONT_HEIGHT_20) / 2, about, COLOR_WHITE);

    /* Alt bilgi çubuğu */
    draw_rect(0, SCREEN_HEIGHT - 45, SCREEN_WIDTH, 45, COLOR_BLUE);
    draw_text_16(15, SCREEN_HEIGHT - 32, "A: Sec   B: Geri", COLOR_WHITE);

    mem_barrier();
}
