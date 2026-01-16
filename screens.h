/* screens.h - Ekran çizim fonksiyonları */
#ifndef SCREENS_H
#define SCREENS_H

#include "types.h"

/* Ekran tipleri */
#define SCREEN_WELCOME    0
#define SCREEN_MAIN       1
#define SCREEN_FILES      2
#define SCREEN_SETTINGS   3
#define SCREEN_ABOUT      4

/* Ekran durumu */
extern int current_screen;

/* Ekran çizim fonksiyonları */
void draw_welcome_screen(void);
void draw_main_screen(void);
void draw_files_screen(void);
void draw_settings_screen(void);
void draw_about_screen(void);

/* Ekran geçişi */
void switch_screen(int screen);

#endif
