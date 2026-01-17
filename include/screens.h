/* screens.h - Screen management and rendering */
#ifndef SCREENS_H
#define SCREENS_H

#include <types.h>

/* Ekran tipleri */
typedef enum {
    SCREEN_WELCOME = 0,
    SCREEN_MAIN,            /* XMB ana menü */
    SCREEN_FILES,           /* Dosya yöneticisi */
    SCREEN_SETTINGS,        /* Ayarlar */
    SCREEN_ABOUT,           /* Hakkında */
    SCREEN_GAME,            /* Oyun çalışıyor */
    SCREEN_COUNT
} ScreenType;

/* Ekran durumu */
extern ScreenType current_screen;
extern ScreenType previous_screen;

/* Ekran çizim fonksiyonları */
void draw_welcome_screen(void);
void draw_main_screen(void);
void draw_files_screen(void);
void draw_settings_screen(void);
void draw_about_screen(void);

/* Ekran güncelleme fonksiyonları */
void update_welcome_screen(void);
void update_main_screen(void);
void update_files_screen(void);
void update_settings_screen(void);
void update_about_screen(void);

/* Ekran geçişi (animasyonlu) */
void switch_screen(ScreenType screen);
void switch_screen_instant(ScreenType screen);

/* Mevcut ekranı render et */
void render_current_screen(void);

/* Mevcut ekranı güncelle */
void update_current_screen(void);

#endif
