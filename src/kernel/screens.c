/* src/kernel/screens.c - Welcome (Old) + Settings/About (New Flat UI) */
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

/* --- HOŞGELDİNİZ EKRANI DEĞİŞKENLERİ (ESKİ HALİ) --- */
static Animation welcome_logo_anim;
static Animation welcome_text_anim;
static Animation welcome_fade_anim;
static int welcome_initialized = 0;

/* --- YARDIMCI: Header ve Footer (Yeni Tasarımlar İçin) --- */
static void draw_screen_header(const char *title) {
    draw_rect(0, 0, SCREEN_WIDTH, 60, 0xFF151515);
    draw_rect(0, 60, SCREEN_WIDTH, 2, g_theme.accent);
    draw_text_20_bold(30, 20, title, 0xFFFFFFFF);
    draw_text_16(SCREEN_WIDTH - 80, 22, "12:00", 0xFF888888);
}

static void draw_screen_footer(const char *text) {
    int h = 50;
    int y = SCREEN_HEIGHT - h;
    draw_rect(0, y, SCREEN_WIDTH, h, 0xFF151515);
    draw_text_16(30, y + 15, text, 0xFFAAAAAA);
}

/* --- HOŞGELDİNİZ EKRANI (ORİJİNAL/ESKİ HALİ) --- */

void draw_welcome_screen(void) {
    Theme *t = theme_get();

    /* Gradient arka plan */
    draw_gradient_bg(t->bg_dark, t->bg_medium);

    float logo_scale_anim = anim_get_value(&welcome_logo_anim);
    float text_alpha_anim = anim_get_value(&welcome_text_anim);
    float fade_anim = anim_get_value(&welcome_fade_anim);

    /* Logo */
    int logo_scale = 6;
    int logo_w = 24 * logo_scale;
    int logo_h = 16 * logo_scale;

    const char *welcome = "Hoşgeldiniz!";
    int gap = 30;
    int start_x = (SCREEN_WIDTH - (logo_w + gap + text_width_24(welcome))) / 2;

    int base_y = (SCREEN_HEIGHT - logo_h) / 2;
    int logo_y = base_y + (int)((1.0f - logo_scale_anim) * 50);

    if(logo_scale_anim > 0.01f) {
        draw_logo(start_x, logo_y, logo_scale);
    }

    int text_y = logo_y + (logo_h - FONT_HEIGHT_24) / 2;
    if(text_alpha_anim > 0.01f) {
        // uint32_t text_color = lerp_color(t->bg_dark, t->text_primary, text_alpha_anim);
        // Basitçe alpha simülasyonu veya direkt renk:
        draw_text_24(start_x + logo_w + gap, text_y, welcome, t->text_primary);
    }

    if(text_alpha_anim > 0.5f) {
        const char *subtitle = "Raspberry Pi Zero 2W Oyun Konsolu";
        int sub_w = text_width_16(subtitle);
        draw_text_16((SCREEN_WIDTH - sub_w) / 2, base_y + logo_h + 30, subtitle, t->text_secondary);
    }

    /* Fade out overlay */
    if(fade_anim > 0.01f) {
        // Basit fade out (ekranı siyaha boyama)
        // Eğer draw_rect alpha destekliyorsa:
        // draw_rect_alpha(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x000000, (uint8_t)(fade_anim * 255));
    }
}

void update_welcome_screen(void) {
    if(!welcome_initialized) {
        anim_init(&welcome_logo_anim);
        anim_init(&welcome_text_anim);
        anim_init(&welcome_fade_anim);

        anim_start(&welcome_logo_anim, 0.0f, 1.0f, 500, EASE_OUT_BACK);

        welcome_text_anim.start_value = 0.0f;
        welcome_text_anim.end_value = 1.0f;
        welcome_text_anim.current_value = 0.0f;
        
        welcome_initialized = 1;
    }

    anim_update(&welcome_logo_anim);

    if(anim_is_complete(&welcome_logo_anim) && welcome_text_anim.state == ANIM_IDLE) {
        anim_start(&welcome_text_anim, 0.0f, 1.0f, 600, EASE_OUT_QUAD);
    }
    anim_update(&welcome_text_anim);

    if(anim_is_complete(&welcome_text_anim) && welcome_fade_anim.state == ANIM_IDLE) {
        anim_start(&welcome_fade_anim, 0.0f, 1.0f, 800, EASE_IN_QUAD);
    }
    anim_update(&welcome_fade_anim);

    if(anim_is_complete(&welcome_fade_anim)) {
        switch_screen_instant(SCREEN_MAIN);
        welcome_initialized = 0;
    }
}

/* --- ANA MENÜ --- */

void draw_main_screen(void) { menu_render(); }
void update_main_screen(void) { menu_update(); }

/* --- DOSYA YÖNETİCİSİ --- */

void draw_files_screen(void) { filemgr_draw(); }
void update_files_screen(void) { filemgr_update(); }

/* --- AYARLAR EKRANI (YENİ FLAT TASARIM) --- */

void draw_settings_screen(void) {
    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0xFF1A1A1A);
    draw_screen_header("Ayarlar");

    int x = 30;
    int start_y = 90;
    int h = 70;
    int w = SCREEN_WIDTH - 60;
    int gap = 20;

    /* Ses Seviyesi */
    draw_rect(x, start_y, w, h, 0xFF252525);
    draw_rect(x, start_y, 4, h, g_theme.accent); // Accent Kenarlık
    draw_text_20_medium(x + 20, start_y + 25, "Ses Seviyesi", 0xFFFFFFFF);
    
    // Bar
    draw_rect(x + 300, start_y + 30, 200, 10, 0xFF101010);
    draw_rect(x + 300, start_y + 30, 160, 10, g_theme.accent); 
    draw_text_16(x + 520, start_y + 25, "80%", 0xFFAAAAAA);

    int y = start_y + h + gap;

    /* Parlaklık */
    draw_rect(x, y, w, h, 0xFF252525);
    draw_rect(x, y, 4, h, g_theme.accent);
    draw_text_20_medium(x + 20, y + 25, "Parlaklık", 0xFFFFFFFF);
    
    // Bar
    draw_rect(x + 300, y + 30, 200, 10, 0xFF101010);
    draw_rect(x + 300, y + 30, 200, 10, g_theme.accent);
    draw_text_16(x + 520, y + 25, "100%", 0xFFAAAAAA);

    y += h + gap;

    /* Tema */
    draw_rect(x, y, w, h, 0xFF252525);
    draw_text_20_medium(x + 20, y + 25, "Tema", 0xFFFFFFFF);
    draw_text_16(x + w - 160, y + 25, "RetroArch Dark >", g_theme.accent);

    y += h + gap;

    /* WiFi */
    draw_rect(x, y, w, h, 0xFF252525);
    draw_text_20_medium(x + 20, y + 25, "WiFi", 0xFFFFFFFF);
    draw_text_16(x + w - 80, y + 25, "Kapali", 0xFF555555);

    draw_screen_footer("[B] Geri");
}

void update_settings_screen(void) {
    /* Input */
}

/* --- HAKKINDA EKRANI (YENİ FLAT TASARIM) --- */

void draw_about_screen(void) {
    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0xFF1A1A1A);
    draw_screen_header("Hakkında");

    int card_w = 460;
    int card_h = 300;
    int card_x = (SCREEN_WIDTH - card_w) / 2;
    int card_y = (SCREEN_HEIGHT - card_h) / 2 + 20;

    /* Kart */
    draw_rect(card_x, card_y, card_w, card_h, 0xFF202020);
    draw_rect_outline(card_x, card_y, card_w, card_h, 2, 0xFF333333);

    /* Logo Placeholder (Piksel art çizimi) */
    int logo_y = card_y + 40;
    int logo_x = SCREEN_WIDTH / 2;
    
    draw_rect(logo_x - 30, logo_y, 60, 36, 0xFFFFFFFF); // Gövde
    draw_rect(logo_x - 24, logo_y + 10, 12, 12, 0xFF000000); // D-Pad
    draw_rect(logo_x + 10, logo_y + 12, 6, 6, 0xFFFF0000); // A Btn
    draw_rect(logo_x + 18, logo_y + 8, 6, 6, 0xFF00FF00); // B Btn

    /* Metinler */
    int ty = logo_y + 60;
    draw_text_32_bold(logo_x - 70, ty, "EmConOs", 0xFFFFFFFF); // Tahmini ortalama

    ty += 40;
    draw_text_20(logo_x - 40, ty, "Surum 1.0", 0xFF888888);

    ty += 40;
    draw_text_16(logo_x - 120, ty, "RetroArch Tarzinda Oyun Konsolu", g_theme.accent);

    ty += 40;
    draw_text_16(logo_x - 90, ty, "Gelistirici: Emirhan Soylu", 0xFF666666);

    ty += 25;
    draw_text_16(logo_x - 100, ty, "Platform: Raspberry Pi Zero 2W", 0xFF666666);

    draw_screen_footer("[B] Geri");
}

void update_about_screen(void) {
    /* Input */
}

/* --- EKRAN YÖNETİMİ --- */

void switch_screen(ScreenType screen) {
    switch_screen_instant(screen);
}

void switch_screen_instant(ScreenType screen) {
    if(screen == current_screen) return;
    previous_screen = current_screen;
    current_screen = screen;

    switch(screen) {
        case SCREEN_MAIN:
            if(!g_menu.initialized) menu_create_default();
            break;
        case SCREEN_FILES:
            filemgr_init();
            break;
        default: break;
    }
}

void render_current_screen(void) {
    switch(current_screen) {
        case SCREEN_WELCOME: draw_welcome_screen(); break;
        case SCREEN_MAIN: draw_main_screen(); break;
        case SCREEN_FILES: draw_files_screen(); break;
        case SCREEN_SETTINGS: draw_settings_screen(); break;
        case SCREEN_ABOUT: draw_about_screen(); break;
        default: break;
    }
}

void update_current_screen(void) {
    switch(current_screen) {
        case SCREEN_WELCOME: update_welcome_screen(); break;
        case SCREEN_MAIN: update_main_screen(); break;
        case SCREEN_FILES: update_files_screen(); break;
        case SCREEN_SETTINGS: update_settings_screen(); break;
        case SCREEN_ABOUT: update_about_screen(); break;
        default: break;
    }
}