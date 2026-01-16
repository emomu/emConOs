/* kernel.c - Ana kernel dosyası */
#include "types.h"
#include "hw.h"
#include "graphics.h"
#include "screens.h"
#include "drivers/input.h"
#include "ui/filemgr.h"
#include "fonts/fonts.h"

/* Ana menü seçimi */
static int menu_selection = 0;
#define MENU_ITEMS 4

/* Kısa gecikme (tekrarlı basımı önlemek için) */
static void short_delay(void) {
    for(volatile int i = 0; i < 5000000; i++) {
        __asm__ volatile("nop");
    }
}

/* Ana menüyü seçimle çiz */
static void draw_main_menu_with_selection(void) {
    /* Arka plan */
    clear_screen(COLOR_DARK_BLUE);

    /* Üst bar */
    draw_rect(0, 0, SCREEN_WIDTH, 50, COLOR_BLUE);
    draw_text_20_bold(15, 12, "EmConOs", COLOR_WHITE);
    draw_text_16(SCREEN_WIDTH - 70, 17, "12:00", COLOR_WHITE);

    /* Ana menü başlığı */
    const char *menu_title = "Ana Menu";
    int mt_w = text_width_32(menu_title);
    draw_text_32_bold((SCREEN_WIDTH - mt_w) / 2, 70, menu_title, COLOR_WHITE);

    /* Menü kutuları */
    int box_w = 140;
    int box_h = 100;
    int gap = 20;
    int start_x = (SCREEN_WIDTH - (4 * box_w + 3 * gap)) / 2;
    int start_y = 160;

    const char *labels[] = {"Oyunlar", "Dosyalar", "Ayarlar", "Hakkinda"};

    for(int i = 0; i < 4; i++) {
        int x = start_x + i * (box_w + gap);
        uint32_t outline_color = (i == menu_selection) ? COLOR_WHITE : COLOR_LIGHT_GRAY;
        int outline_width = (i == menu_selection) ? 4 : 2;

        draw_rect(x, start_y, box_w, box_h, COLOR_BLUE);
        draw_rect_outline(x, start_y, box_w, box_h, outline_width, outline_color);

        int tw = text_width_20(labels[i]);
        draw_text_20_medium(x + (box_w - tw) / 2, start_y + (box_h - FONT_HEIGHT_20) / 2, labels[i], COLOR_WHITE);
    }

    /* Alt bilgi çubuğu */
    draw_rect(0, SCREEN_HEIGHT - 45, SCREEN_WIDTH, 45, COLOR_BLUE);
    draw_text_16(15, SCREEN_HEIGHT - 32, "A: Sec   B: Geri   D-Pad: Gezin", COLOR_WHITE);

    __asm__ volatile("dsb sy");
}

/* Ana menü input işleme */
static void handle_main_menu_input(void) {
    if(btn_just_pressed(BTN_LEFT)) {
        uart_puts("[INPUT] LEFT basildi\n");
        if(menu_selection > 0) {
            menu_selection--;
            draw_main_menu_with_selection();
        }
    }
    if(btn_just_pressed(BTN_RIGHT)) {
        uart_puts("[INPUT] RIGHT basildi\n");
        if(menu_selection < MENU_ITEMS - 1) {
            menu_selection++;
            draw_main_menu_with_selection();
        }
    }
    if(btn_just_pressed(BTN_UP)) {
        uart_puts("[INPUT] UP basildi\n");
    }
    if(btn_just_pressed(BTN_DOWN)) {
        uart_puts("[INPUT] DOWN basildi\n");
    }
    if(btn_just_pressed(BTN_A)) {
        uart_puts("[INPUT] A basildi\n");
        switch(menu_selection) {
            case 0: /* Oyunlar - henüz yok */
                break;
            case 1: /* Dosyalar */
                switch_screen(SCREEN_FILES);
                break;
            case 2: /* Ayarlar */
                switch_screen(SCREEN_SETTINGS);
                break;
            case 3: /* Hakkında */
                switch_screen(SCREEN_ABOUT);
                break;
        }
    }
    if(btn_just_pressed(BTN_B)) {
        uart_puts("[INPUT] B basildi\n");
    }
}

/* Dosya yöneticisi input işleme */
static void handle_files_input(void) {
    if(btn_just_pressed(BTN_UP)) {
        filemgr_up();
        filemgr_draw();
        __asm__ volatile("dsb sy");
    }
    if(btn_just_pressed(BTN_DOWN)) {
        filemgr_down();
        filemgr_draw();
        __asm__ volatile("dsb sy");
    }
    if(btn_just_pressed(BTN_A)) {
        filemgr_enter();
        filemgr_draw();
        __asm__ volatile("dsb sy");
    }
    if(btn_just_pressed(BTN_B)) {
        current_screen = SCREEN_MAIN;
        draw_main_menu_with_selection();
    }
}

/* Ayarlar input işleme */
static void handle_settings_input(void) {
    if(btn_just_pressed(BTN_B)) {
        current_screen = SCREEN_MAIN;
        draw_main_menu_with_selection();
    }
}

/* Hakkında input işleme */
static void handle_about_input(void) {
    if(btn_just_pressed(BTN_B)) {
        current_screen = SCREEN_MAIN;
        draw_main_menu_with_selection();
    }
}

void kernel_main(void) {
    uart_init();
    uart_puts("\nEmConOs v1.0 - Game Console\n");
    init_screen();

    if(framebuffer) {
        uart_puts("Hosgeldiniz ekrani gosteriliyor...\n");

        /* Hoşgeldiniz ekranını göster */
        draw_welcome_screen();

        /* 3 saniye bekle */
        uart_puts("Bekleniyor...\n");
        wait_seconds(3);

        /* Input sistemini başlat */
        uart_puts("Input baslatiliyor...\n");
        input_init();

        /* Ana ekrana geç */
        uart_puts("Ana ekrana geciliyor...\n");
        current_screen = SCREEN_MAIN;
        menu_selection = 0;
        draw_main_menu_with_selection();

        uart_puts("Sistem hazir.\n");
        uart_puts("Kontroller: W/A/S/D=Yon, Z=Sec, X=Geri\n");
    }

    /* Ana döngü */
    while(1) {
        /* Input güncelle */
        input_update();

        /* Mevcut ekrana göre input işle */
        switch(current_screen) {
            case SCREEN_MAIN:
                handle_main_menu_input();
                break;
            case SCREEN_FILES:
                handle_files_input();
                break;
            case SCREEN_SETTINGS:
                handle_settings_input();
                break;
            case SCREEN_ABOUT:
                handle_about_input();
                break;
        }

        /* Kısa gecikme (CPU kullanımını azalt) */
        short_delay();
    }
}
