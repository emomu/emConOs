/* kernel.c - EmConOs Main Kernel with RetroArch-style UI */
#include <types.h>
#include <hw.h>
#include <graphics.h>
#include <screens.h>
#include <drivers/input.h>
#include <drivers/timer.h>
#include <ui/filemgr.h>
#include <ui/theme.h>
#include <ui/animation.h>
#include <ui/transition.h>
#include <ui/menu.h>
#include <fonts/fonts.h>

/* Frame rate kontrolü */
#define TARGET_FPS          30
#define FRAME_TIME_MS       (1000 / TARGET_FPS)

/* Input işleme (ana menü) */
static void handle_main_menu_input(void) {
    if(btn_just_pressed(BTN_UP)) {
        menu_move_up();
    }
    if(btn_just_pressed(BTN_DOWN)) {
        menu_move_down();
    }
    if(btn_just_pressed(BTN_LEFT)) {
        menu_move_left();
    }
    if(btn_just_pressed(BTN_RIGHT)) {
        menu_move_right();
    }
    if(btn_just_pressed(BTN_A)) {
        /* Seçili öğenin action'ını çalıştır */
        MenuItem *item = menu_get_current_item();
        if(item) {
            int cat_idx = menu_get_selected_category();

            /* Kategoriye göre ekran geçişi */
            if(cat_idx == 1) {  /* Dosyalar */
                switch_screen(SCREEN_FILES);
            } else if(cat_idx == 2) {  /* Ayarlar */
                switch_screen(SCREEN_SETTINGS);
            } else if(cat_idx == 3) {  /* Hakkında */
                switch_screen(SCREEN_ABOUT);
            } else {
                menu_select();
            }
        }
    }
    if(btn_just_pressed(BTN_B)) {
        menu_back();
    }
}

/* Input işleme (dosya yöneticisi) */
static void handle_files_input(void) {
    if(btn_just_pressed(BTN_UP)) {
        filemgr_up();
    }
    if(btn_just_pressed(BTN_DOWN)) {
        filemgr_down();
    }
    if(btn_just_pressed(BTN_A)) {
        filemgr_enter();
    }
    if(btn_just_pressed(BTN_B)) {
        /* Root'taysak ana menüye dön, değilse üst dizine git */
        if(filemgr_is_at_root()) {
            switch_screen(SCREEN_MAIN);
        } else {
            filemgr_back();
        }
    }
}

/* Input işleme (ayarlar) */
static void handle_settings_input(void) {
    if(btn_just_pressed(BTN_B)) {
        switch_screen(SCREEN_MAIN);
    }
}

/* Input işleme (hakkında) */
static void handle_about_input(void) {
    if(btn_just_pressed(BTN_B)) {
        switch_screen(SCREEN_MAIN);
    }
}

/* Input işleme (mevcut ekrana göre) */
static void handle_input(void) {
    /* Geçiş sırasında input'u kilitle */
    if(transition_is_active()) {
        return;
    }

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
        default:
            break;
    }
}

/* Ana kernel fonksiyonu */
void kernel_main(void) {
    /* UART başlat */
    uart_init();
    uart_puts("\n");
    uart_puts("========================================\n");
    uart_puts("  EmConOs v1.0 - RetroArch Style UI\n");
    uart_puts("  Raspberry Pi Zero 2W Game Console\n");
    uart_puts("========================================\n");
    uart_puts("\n");

    /* Timer başlat */
    timer_init();
    clock_init(12, 0, 0);  /* 12:00:00 başlangıç */

    /* Ekran başlat */
    uart_puts("[INIT] Ekran baslatiliyor...\n");
    init_screen();

    if(!framebuffer) {
        uart_puts("[ERROR] Framebuffer alinamadi!\n");
        while(1) { __asm__ volatile("wfe"); }
    }

    uart_puts("[INIT] Framebuffer hazir: ");
    uart_hex((uint32_t)(uint64_t)framebuffer);
    uart_puts("\n");

    /* Çift tamponlama başlat */
    uart_puts("[INIT] Cift tamponlama baslatiliyor...\n");
    graphics_init_buffers();

    /* Tema başlat */
    uart_puts("[INIT] Tema sistemi baslatiliyor...\n");
    theme_init();

    /* Animasyon sistemi başlat */
    uart_puts("[INIT] Animasyon sistemi baslatiliyor...\n");
    anim_system_init();

    /* Geçiş sistemi başlat */
    uart_puts("[INIT] Gecis sistemi baslatiliyor...\n");
    transition_init();

    /* Input sistemi başlat */
    uart_puts("[INIT] Input sistemi baslatiliyor...\n");
    input_init();

    /* Hoşgeldiniz ekranını göster */
    uart_puts("[INIT] Hosgeldiniz ekrani...\n");
    current_screen = SCREEN_WELCOME;

    uart_puts("[INIT] Sistem hazir!\n");
    uart_puts("\n");
    uart_puts("Kontroller:\n");
    uart_puts("  W/S veya Yukari/Asagi : Kategori degistir\n");
    uart_puts("  A/D veya Sol/Sag     : Oge sec\n");
    uart_puts("  Z/Enter              : Onayla (A)\n");
    uart_puts("  X/Escape             : Geri (B)\n");
    uart_puts("\n");

    /* Frame zamanlama */
    uint32_t frame_count = 0;
    uint32_t fps_timer = timer_get_ms();

    /* Ana döngü */
    while(1) {
        uint32_t frame_start = timer_get_ms();

        /* Animasyon sistemi güncelle */
        anim_system_update();

        /* Saat güncelle */
        clock_update();

        /* Input güncelle */
        input_update();

        /* Input işle */
        handle_input();

        /* Mevcut ekranı güncelle */
        update_current_screen();

        /* Render (back buffer'a çiz) */
        render_current_screen();

        /* Buffer swap (back buffer'ı framebuffer'a kopyala) */
        graphics_swap_buffers();

        /* FPS sayacı */
        frame_count++;
        if(timer_get_ms() - fps_timer >= 1000) {
            /* Her saniye FPS'i logla (debug için) */
            /* uart_puts("FPS: "); uart_hex(frame_count); uart_puts("\n"); */
            frame_count = 0;
            fps_timer = timer_get_ms();
        }

        /* Frame rate limiter */
        uint32_t frame_time = timer_get_ms() - frame_start;
        if(frame_time < FRAME_TIME_MS) {
            timer_wait_ms(FRAME_TIME_MS - frame_time);
        }
    }
}
