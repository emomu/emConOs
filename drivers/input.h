/* input.h - Gamepad/Button + Keyboard input driver */
#ifndef INPUT_H
#define INPUT_H

#include "../types.h"

/* Buton tanımları (hem GPIO hem klavye için) */
#define BTN_UP      (1 << 0)
#define BTN_DOWN    (1 << 1)
#define BTN_LEFT    (1 << 2)
#define BTN_RIGHT   (1 << 3)
#define BTN_A       (1 << 4)
#define BTN_B       (1 << 5)
#define BTN_START   (1 << 6)
#define BTN_SELECT  (1 << 7)

/* GPIO pin tanımları (standart düzen) */
#define GPIO_BTN_UP      5
#define GPIO_BTN_DOWN    6
#define GPIO_BTN_LEFT    13
#define GPIO_BTN_RIGHT   19
#define GPIO_BTN_A       26
#define GPIO_BTN_B       21
#define GPIO_BTN_START   20
#define GPIO_BTN_SELECT  16

/* Fonksiyonlar */
void input_init(void);
uint8_t input_read(void);
uint8_t input_read_debounced(void);

/* Tek buton kontrolü */
int btn_pressed(uint8_t state, uint8_t btn);
int btn_just_pressed(uint8_t btn);
int btn_just_released(uint8_t btn);

/* Önceki durumu güncelle (her frame sonunda çağır) */
void input_update(void);

/* Klavye desteği */
void input_enable_keyboard(int enable);
int input_keyboard_enabled(void);

#endif
