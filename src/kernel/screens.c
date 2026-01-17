/* screens.c - Screen rendering with RetroArch-style UI */
#include <screens.h>
#include <graphics.h>
#include <fonts/fonts.h>
#include <logo.h>
#include <ui/filemgr.h>
#include <ui/theme.h>
#include <ui/animation.h>
#include <ui/transition.h>
#include <ui/menu.h>

/* Ekran durumu */
ScreenType current_screen = SCREEN_WELCOME;
ScreenType previous_screen = SCREEN_WELCOME;

/* Hoşgeldiniz ekranı animasyonları */
static Animation welcome_logo_anim;
static Animation welcome_text_anim;
static Animation welcome_fade_anim;
static int welcome_initialized = 0;


/* --- HOŞGELDINIZ EKRANI --- */

void draw_welcome_screen(void) {
    Theme *t = theme_get();

    /* Gradient arka plan */
    draw_gradient_bg(t->bg_dark, t->bg_medium);

    /* Logo ve yazı animasyonları */
    float logo_scale_anim = anim_get_value(&welcome_logo_anim);
    float text_alpha_anim = anim_get_value(&welcome_text_anim);
    float fade_anim = anim_get_value(&welcome_fade_anim);

    /* Logo */
    int logo_scale = 6;
    int logo_w = 24 * logo_scale;
    int logo_h = 16 * logo_scale;

    const char *welcome = "Hosgeldiniz!";
    int wel_w = text_width_24(welcome);
    int gap = 30;
    int total_w = logo_w + gap + wel_w;
    int start_x = (SCREEN_WIDTH - total_w) / 2;

    /* Animasyonlu Y pozisyonu */
    int base_y = (SCREEN_HEIGHT - logo_h) / 2;
    int logo_y = base_y + (int)((1.0f - logo_scale_anim) * 50);

    /* Sol: Gamepad Logo (animasyonlu giriş) */
    if(logo_scale_anim > 0.01f) {
        draw_logo(start_x, logo_y, logo_scale);
    }

    /* Sağ: Hoşgeldiniz yazısı (fade in) */
    int text_y = logo_y + (logo_h - FONT_HEIGHT_24) / 2;
    if(text_alpha_anim > 0.01f) {
        uint32_t text_color = lerp_color(t->bg_dark, t->text_primary, text_alpha_anim);
        draw_text_24(start_x + logo_w + gap, text_y, welcome, text_color);
    }

    /* Alt yazı */
    if(text_alpha_anim > 0.5f) {
        const char *subtitle = "Raspberry Pi Zero 2W Oyun Konsolu";
        int sub_w = text_width_16(subtitle);
        uint32_t sub_color = lerp_color(t->bg_dark, t->text_secondary, (text_alpha_anim - 0.5f) * 2.0f);
        draw_text_16((SCREEN_WIDTH - sub_w) / 2, base_y + logo_h + 30, subtitle, sub_color);
    }

    /* Fade out overlay */
    if(fade_anim > 0.01f) {
        uint8_t alpha = (uint8_t)(fade_anim * 255);
        for(int y = 0; y < SCREEN_HEIGHT; y++) {
            for(int x = 0; x < SCREEN_WIDTH; x++) {
                uint32_t offset = (y * SCREEN_WIDTH * 4) + (x * 4);
                uint32_t *pixel_addr = (uint32_t *)(draw_buffer + offset);
                uint32_t current = *pixel_addr;

                uint8_t inv_alpha = 255 - alpha;
                uint8_t r = (uint8_t)(((current >> 16) & 0xFF) * inv_alpha / 255);
                uint8_t g = (uint8_t)(((current >> 8) & 0xFF) * inv_alpha / 255);
                uint8_t b = (uint8_t)((current & 0xFF) * inv_alpha / 255);

                *pixel_addr = 0xFF000000 | (r << 16) | (g << 8) | b;
            }
        }
    }
}

void update_welcome_screen(void) {
    if(!welcome_initialized) {
        /* Animasyonları başlat */
        anim_init(&welcome_logo_anim);
        anim_init(&welcome_text_anim);
        anim_init(&welcome_fade_anim);

        /* Logo giriş animasyonu */
        anim_start(&welcome_logo_anim, 0.0f, 1.0f, 500, EASE_OUT_BACK);

        /* Metin fade in (gecikmeli) */
        welcome_text_anim.start_value = 0.0f;
        welcome_text_anim.end_value = 1.0f;
        welcome_text_anim.current_value = 0.0f;

        welcome_initialized = 1;
    }

    /* Animasyonları güncelle */
    anim_update(&welcome_logo_anim);

    /* Logo animasyonu bittikten sonra metin başlat */
    if(anim_is_complete(&welcome_logo_anim) && welcome_text_anim.state == ANIM_IDLE) {
        anim_start(&welcome_text_anim, 0.0f, 1.0f, 600, EASE_OUT_QUAD);
    }

    anim_update(&welcome_text_anim);

    /* Metin animasyonu bittikten sonra fade out başlat */
    if(anim_is_complete(&welcome_text_anim) && welcome_fade_anim.state == ANIM_IDLE) {
        anim_start(&welcome_fade_anim, 0.0f, 1.0f, 800, EASE_IN_QUAD);
    }

    anim_update(&welcome_fade_anim);

    /* Fade out bittikten sonra ana menüye geç */
    if(anim_is_complete(&welcome_fade_anim)) {
        switch_screen_instant(SCREEN_MAIN);
        welcome_initialized = 0;  /* Reset for next time */
    }
}

/* --- ANA MENÜ (XMB Style) --- */

void draw_main_screen(void) {
    menu_render();
}

void update_main_screen(void) {
    menu_update();
}

/* --- DOSYA YÖNETİCİSİ --- */

void draw_files_screen(void) {
    filemgr_draw();
}

void update_files_screen(void) {
    /* Dosya yöneticisi güncelleme */
}

/* --- AYARLAR --- */

void draw_settings_screen(void) {
    Theme *t = theme_get();

    clear_screen(t->bg_dark);

    /* Başlık */
    draw_rect(0, 0, SCREEN_WIDTH, 50, t->header_bg);
    draw_text_20_bold(20, 12, "Ayarlar", t->text_primary);
    draw_rect(0, 49, SCREEN_WIDTH, 1, t->accent);

    /* Ayar öğeleri */
    int y = 70;
    int item_h = 50;

    /* Ses Seviyesi */
    draw_rect(20, y, SCREEN_WIDTH - 40, item_h, t->bg_medium);
    draw_text_20(40, y + 12, "Ses Seviyesi", t->text_primary);
    draw_text_16(SCREEN_WIDTH - 100, y + 15, "80%", t->text_secondary);
    y += item_h + 10;

    /* Parlaklık */
    draw_rect(20, y, SCREEN_WIDTH - 40, item_h, t->bg_medium);
    draw_text_20(40, y + 12, "Parlaklik", t->text_primary);
    draw_text_16(SCREEN_WIDTH - 100, y + 15, "100%", t->text_secondary);
    y += item_h + 10;

    /* Tema */
    draw_rect(20, y, SCREEN_WIDTH - 40, item_h, t->bg_medium);
    draw_text_20(40, y + 12, "Tema", t->text_primary);
    draw_text_16(SCREEN_WIDTH - 150, y + 15, "RetroArch Dark", t->text_secondary);
    y += item_h + 10;

    /* WiFi */
    draw_rect(20, y, SCREEN_WIDTH - 40, item_h, t->bg_medium);
    draw_text_20(40, y + 12, "WiFi", t->text_primary);
    draw_text_16(SCREEN_WIDTH - 100, y + 15, "Kapali", t->text_secondary);

    /* Alt bar */
    draw_rect(0, SCREEN_HEIGHT - 40, SCREEN_WIDTH, 40, t->footer_bg);
    draw_rect(0, SCREEN_HEIGHT - 40, SCREEN_WIDTH, 1, t->accent);
    draw_text_16(20, SCREEN_HEIGHT - 28, "B: Geri", t->text_secondary);

}

void update_settings_screen(void) {
    /* Ayarlar güncelleme */
}

/* --- HAKKINDA --- */

void draw_about_screen(void) {
    Theme *t = theme_get();

    clear_screen(t->bg_dark);

    /* Başlık */
    draw_rect(0, 0, SCREEN_WIDTH, 50, t->header_bg);
    draw_text_20_bold(20, 12, "Hakkinda", t->text_primary);
    draw_rect(0, 49, SCREEN_WIDTH, 1, t->accent);

    /* Logo */
    draw_logo((SCREEN_WIDTH - 24 * 5) / 2, 80, 5);

    /* Bilgiler */
    int y = 180;

    const char *name = "EmConOs";
    int nw = text_width_32(name);
    draw_text_32_bold((SCREEN_WIDTH - nw) / 2, y, name, t->text_primary);
    y += 50;

    const char *ver = "Surum 1.0";
    int vw = text_width_20(ver);
    draw_text_20((SCREEN_WIDTH - vw) / 2, y, ver, t->text_secondary);
    y += 40;

    const char *desc = "RetroArch Tarzinda Oyun Konsolu";
    int dw = text_width_16(desc);
    draw_text_16((SCREEN_WIDTH - dw) / 2, y, desc, t->accent);
    y += 35;

    const char *dev = "Gelistirici: Emirhan Soylu";
    int devw = text_width_16(dev);
    draw_text_16((SCREEN_WIDTH - devw) / 2, y, dev, t->text_secondary);
    y += 30;

    const char *hw = "Platform: Raspberry Pi Zero 2W";
    int hww = text_width_16(hw);
    draw_text_16((SCREEN_WIDTH - hww) / 2, y, hw, t->text_secondary);

    /* Alt bar */
    draw_rect(0, SCREEN_HEIGHT - 40, SCREEN_WIDTH, 40, t->footer_bg);
    draw_rect(0, SCREEN_HEIGHT - 40, SCREEN_WIDTH, 1, t->accent);
    draw_text_16(20, SCREEN_HEIGHT - 28, "B: Geri", t->text_secondary);

}

void update_about_screen(void) {
    /* Hakkında güncelleme */
}

/* --- EKRAN GEÇİŞİ --- */

void switch_screen(ScreenType screen) {
    if(screen == current_screen) return;

    previous_screen = current_screen;

    /* Geçiş animasyonu başlat */
    TransitionType trans_type = TRANS_SLIDE_LEFT;

    /* Geri gidiyorsak sağa kay */
    if(screen < current_screen) {
        trans_type = TRANS_SLIDE_RIGHT;
    }

    transition_start(trans_type, current_screen, screen);
    current_screen = screen;

    /* Yeni ekranı hazırla */
    switch(screen) {
        case SCREEN_MAIN:
            if(!g_menu.initialized) {
                theme_init();
                menu_create_default();
            }
            break;
        case SCREEN_FILES:
            filemgr_init();
            break;
        default:
            break;
    }
}

void switch_screen_instant(ScreenType screen) {
    if(screen == current_screen) return;

    previous_screen = current_screen;
    current_screen = screen;

    /* Yeni ekranı hazırla */
    switch(screen) {
        case SCREEN_MAIN:
            if(!g_menu.initialized) {
                theme_init();
                menu_create_default();
            }
            break;
        case SCREEN_FILES:
            filemgr_init();
            break;
        default:
            break;
    }
}

void render_current_screen(void) {
    /* Geçiş aktifse offset uygula */
    int offset_x = transition_get_offset_x();
    int offset_y = transition_get_offset_y();

    /* TODO: Offset'i render'a uygula */
    (void)offset_x;
    (void)offset_y;

    switch(current_screen) {
        case SCREEN_WELCOME:
            draw_welcome_screen();
            break;
        case SCREEN_MAIN:
            draw_main_screen();
            break;
        case SCREEN_FILES:
            draw_files_screen();
            break;
        case SCREEN_SETTINGS:
            draw_settings_screen();
            break;
        case SCREEN_ABOUT:
            draw_about_screen();
            break;
        default:
            break;
    }

    /* Fade overlay */
    if(transition_is_active() && g_transition.type == TRANS_FADE) {
        transition_draw_fade_overlay();
    }
}

void update_current_screen(void) {
    /* Geçiş güncelle */
    transition_update();

    /* Mevcut ekranı güncelle */
    switch(current_screen) {
        case SCREEN_WELCOME:
            update_welcome_screen();
            break;
        case SCREEN_MAIN:
            update_main_screen();
            break;
        case SCREEN_FILES:
            update_files_screen();
            break;
        case SCREEN_SETTINGS:
            update_settings_screen();
            break;
        case SCREEN_ABOUT:
            update_about_screen();
            break;
        default:
            break;
    }
}
