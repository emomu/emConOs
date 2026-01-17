/* theme.c - Theme system implementation */
#include <ui/theme.h>

/* Mevcut aktif tema */
Theme g_theme;

/* --- Önceden Tanımlı Temalar --- */

/* RetroArch Dark Theme */
const Theme THEME_PRESET_RETROARCH_DARK = {
    /* Arka plan renkleri */
    .bg_dark = 0xFF0a0e1a,
    .bg_medium = 0xFF151a2e,
    .bg_light = 0xFF1e2642,

    /* Vurgu renkleri */
    .accent = 0xFFff6b35,           /* Turuncu */
    .accent_alt = 0xFF4ecdc4,       /* Turkuaz */
    .accent_glow = 0xFFff8c5a,      /* Açık turuncu */

    /* Metin renkleri */
    .text_primary = 0xFFffffff,
    .text_secondary = 0xFFa0a8c0,
    .text_disabled = 0xFF505868,
    .text_highlight = 0xFFffcc00,

    /* UI elemanları */
    .header_bg = 0xFF0d1220,
    .footer_bg = 0xFF0d1220,
    .selected_bg = 0xFF2a3a5f,
    .hover_bg = 0xFF1a2a4a,

    /* Özel renkler */
    .icon_games = 0xFF4a9ff5,       /* Mavi */
    .icon_files = 0xFFf5a623,       /* Turuncu */
    .icon_settings = 0xFF7b68ee,    /* Mor */
    .icon_about = 0xFF50c878,       /* Yeşil */

    /* Durum renkleri */
    .success = 0xFF4caf50,
    .warning = 0xFFff9800,
    .error = 0xFFf44336
};

/* RetroArch Ozone Theme */
const Theme THEME_PRESET_RETROARCH_OZONE = {
    .bg_dark = 0xFF2d2d2d,
    .bg_medium = 0xFF383838,
    .bg_light = 0xFF454545,

    .accent = 0xFF0099db,           /* Mavi */
    .accent_alt = 0xFF00d4aa,       /* Yeşil-mavi */
    .accent_glow = 0xFF33b5e5,

    .text_primary = 0xFFffffff,
    .text_secondary = 0xFFb0b0b0,
    .text_disabled = 0xFF606060,
    .text_highlight = 0xFF00d4aa,

    .header_bg = 0xFF252525,
    .footer_bg = 0xFF252525,
    .selected_bg = 0xFF0099db,
    .hover_bg = 0xFF404040,

    .icon_games = 0xFF0099db,
    .icon_files = 0xFFffaa00,
    .icon_settings = 0xFF9966ff,
    .icon_about = 0xFF00d4aa,

    .success = 0xFF00d4aa,
    .warning = 0xFFffaa00,
    .error = 0xFFff4455
};

/* Classic Blue Theme */
const Theme THEME_PRESET_CLASSIC_BLUE = {
    .bg_dark = 0xFF001830,
    .bg_medium = 0xFF002850,
    .bg_light = 0xFF003870,

    .accent = 0xFF00aaff,
    .accent_alt = 0xFFffcc00,
    .accent_glow = 0xFF66ccff,

    .text_primary = 0xFFffffff,
    .text_secondary = 0xFFc0d0e0,
    .text_disabled = 0xFF607080,
    .text_highlight = 0xFFffcc00,

    .header_bg = 0xFF001020,
    .footer_bg = 0xFF001020,
    .selected_bg = 0xFF004080,
    .hover_bg = 0xFF003060,

    .icon_games = 0xFF00aaff,
    .icon_files = 0xFFffcc00,
    .icon_settings = 0xFFaa88ff,
    .icon_about = 0xFF00ff88,

    .success = 0xFF00cc66,
    .warning = 0xFFffaa00,
    .error = 0xFFff4444
};

/* Neon Purple Theme */
const Theme THEME_PRESET_NEON_PURPLE = {
    .bg_dark = 0xFF0f0a1a,
    .bg_medium = 0xFF1a1030,
    .bg_light = 0xFF251845,

    .accent = 0xFFff00ff,           /* Magenta */
    .accent_alt = 0xFF00ffff,       /* Cyan */
    .accent_glow = 0xFFff66ff,

    .text_primary = 0xFFffffff,
    .text_secondary = 0xFFc0a0d0,
    .text_disabled = 0xFF605070,
    .text_highlight = 0xFF00ffff,

    .header_bg = 0xFF0a0515,
    .footer_bg = 0xFF0a0515,
    .selected_bg = 0xFF4a2080,
    .hover_bg = 0xFF301060,

    .icon_games = 0xFFff00ff,
    .icon_files = 0xFF00ffff,
    .icon_settings = 0xFFffff00,
    .icon_about = 0xFF00ff00,

    .success = 0xFF00ff88,
    .warning = 0xFFffaa00,
    .error = 0xFFff0066
};

/* Modern Dark Theme (Neutral Greys) */
const Theme THEME_PRESET_MODERN_DARK = {
    /* Arka plan renkleri - Nötr Koyu Griler */
    .bg_dark = 0xFF121212,          /* Çok koyu gri / Neredeyse siyah */
    .bg_medium = 0xFF1e1e1e,        /* Koyu gri */
    .bg_light = 0xFF2d2d2d,         /* Orta gri */

    /* Vurgu renkleri - Canlı */
    .accent = 0xFFff6b35,           /* Turuncu */
    .accent_alt = 0xFF4ecdc4,       /* Turkuaz */
    .accent_glow = 0xFFff8c5a,      /* Açık turuncu */

    /* Metin renkleri */
    .text_primary = 0xFFeeeeee,     /* Kırık beyaz */
    .text_secondary = 0xFFb0b0b0,   /* Gri metin */
    .text_disabled = 0xFF606060,
    .text_highlight = 0xFFffcc00,

    /* UI elemanları */
    .header_bg = 0xFF1a1a1a,
    .footer_bg = 0xFF1a1a1a,
    .selected_bg = 0xFF333333,
    .hover_bg = 0xFF252525,

    /* Özel renkler */
    .icon_games = 0xFF4a9ff5,       /* Mavi */
    .icon_files = 0xFFf5a623,       /* Turuncu */
    .icon_settings = 0xFF7b68ee,    /* Mor */
    .icon_about = 0xFF50c878,       /* Yeşil */

    /* Durum renkleri */
    .success = 0xFF4caf50,
    .warning = 0xFFff9800,
    .error = 0xFFf44336
};

/* --- Tema Fonksiyonları --- */

void theme_init(void) {
    /* Varsayılan tema: Modern Dark (Nötr) */
    theme_set_custom(&THEME_PRESET_MODERN_DARK);
}

void theme_set(ThemePreset preset) {
    switch(preset) {
        case THEME_RETROARCH_DARK:
            g_theme = THEME_PRESET_RETROARCH_DARK;
            break;
        case THEME_RETROARCH_OZONE:
            g_theme = THEME_PRESET_RETROARCH_OZONE;
            break;
        case THEME_CLASSIC_BLUE:
            g_theme = THEME_PRESET_CLASSIC_BLUE;
            break;
        case THEME_NEON_PURPLE:
            g_theme = THEME_PRESET_NEON_PURPLE;
            break;
        default:
            g_theme = THEME_PRESET_RETROARCH_DARK;
            break;
    }
}

void theme_set_custom(const Theme *custom) {
    g_theme = *custom;
}

Theme *theme_get(void) {
    return &g_theme;
}

/* --- Renk Yardımcıları --- */

uint32_t theme_color_darken(uint32_t color, float amount) {
    uint8_t a = (color >> 24) & 0xFF;
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;

    float factor = 1.0f - amount;
    if(factor < 0.0f) factor = 0.0f;
    if(factor > 1.0f) factor = 1.0f;

    r = (uint8_t)(r * factor);
    g = (uint8_t)(g * factor);
    b = (uint8_t)(b * factor);

    return (a << 24) | (r << 16) | (g << 8) | b;
}

uint32_t theme_color_lighten(uint32_t color, float amount) {
    uint8_t a = (color >> 24) & 0xFF;
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;

    r = (uint8_t)(r + (255 - r) * amount);
    g = (uint8_t)(g + (255 - g) * amount);
    b = (uint8_t)(b + (255 - b) * amount);

    return (a << 24) | (r << 16) | (g << 8) | b;
}

uint32_t theme_color_alpha(uint32_t color, uint8_t alpha) {
    return (color & 0x00FFFFFF) | ((uint32_t)alpha << 24);
}
