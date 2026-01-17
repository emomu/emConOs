/* theme.h - RetroArch-style theme system */
#ifndef THEME_H
#define THEME_H

#include <types.h>

/* Tema renk yapısı */
typedef struct {
    /* Arka plan renkleri */
    uint32_t bg_dark;           /* Ana arka plan (en koyu) */
    uint32_t bg_medium;         /* Panel arka planı */
    uint32_t bg_light;          /* Açık arka plan */

    /* Vurgu renkleri */
    uint32_t accent;            /* Ana vurgu rengi */
    uint32_t accent_alt;        /* Alternatif vurgu */
    uint32_t accent_glow;       /* Parlama efekti */

    /* Metin renkleri */
    uint32_t text_primary;      /* Ana metin */
    uint32_t text_secondary;    /* İkincil metin */
    uint32_t text_disabled;     /* Devre dışı metin */
    uint32_t text_highlight;    /* Vurgulu metin */

    /* UI elemanları */
    uint32_t header_bg;         /* Başlık bar arka planı */
    uint32_t footer_bg;         /* Alt bar arka planı */
    uint32_t selected_bg;       /* Seçili öğe arka planı */
    uint32_t hover_bg;          /* Hover durumu */

    /* Özel renkler */
    uint32_t icon_games;        /* Oyunlar ikonu */
    uint32_t icon_files;        /* Dosyalar ikonu */
    uint32_t icon_settings;     /* Ayarlar ikonu */
    uint32_t icon_about;        /* Hakkında ikonu */

    /* Durum renkleri */
    uint32_t success;           /* Başarı */
    uint32_t warning;           /* Uyarı */
    uint32_t error;             /* Hata */
} Theme;

/* Önceden tanımlı temalar */
typedef enum {
    THEME_RETROARCH_DARK,       /* RetroArch koyu tema */
    THEME_RETROARCH_OZONE,      /* RetroArch Ozone tema */
    THEME_CLASSIC_BLUE,         /* Klasik mavi */
    THEME_NEON_PURPLE,          /* Neon mor */
    THEME_COUNT
} ThemePreset;

/* Mevcut tema */
extern Theme g_theme;

/* Tema fonksiyonları */
void theme_init(void);
void theme_set(ThemePreset preset);
void theme_set_custom(const Theme *custom);
Theme *theme_get(void);

/* Renk yardımcıları */
uint32_t theme_color_darken(uint32_t color, float amount);
uint32_t theme_color_lighten(uint32_t color, float amount);
uint32_t theme_color_alpha(uint32_t color, uint8_t alpha);

/* Önceden tanımlı temalar */
extern const Theme THEME_PRESET_RETROARCH_DARK;
extern const Theme THEME_PRESET_RETROARCH_OZONE;
extern const Theme THEME_PRESET_CLASSIC_BLUE;
extern const Theme THEME_PRESET_NEON_PURPLE;

#endif
