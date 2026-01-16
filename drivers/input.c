/* input.c - Gamepad/Button + Keyboard + UART input driver */
#include "input.h"
#include "usb_kbd.h"
#include "../hw.h"

/* GPIO adresleri */
#define MMIO_BASE       0x3F000000
#define GPIO_BASE       (MMIO_BASE + 0x200000)

/* GPIO register offsetleri */
#define GPFSEL0         ((volatile uint32_t*)(GPIO_BASE + 0x00))
#define GPFSEL1         ((volatile uint32_t*)(GPIO_BASE + 0x04))
#define GPFSEL2         ((volatile uint32_t*)(GPIO_BASE + 0x08))
#define GPSET0          ((volatile uint32_t*)(GPIO_BASE + 0x1C))
#define GPCLR0          ((volatile uint32_t*)(GPIO_BASE + 0x28))
#define GPLEV0          ((volatile uint32_t*)(GPIO_BASE + 0x34))
#define GPPUD           ((volatile uint32_t*)(GPIO_BASE + 0x94))
#define GPPUDCLK0       ((volatile uint32_t*)(GPIO_BASE + 0x98))

/* Dahili durum */
static uint8_t prev_state = 0;
static uint8_t curr_state = 0;
static uint8_t uart_input_state = 0;  /* UART'tan okunan son giriş */
static int keyboard_enabled = 1;

/* Gecikme */
static void gpio_delay(int count) {
    while(count--) {
        __asm__ volatile("nop");
    }
}

/* GPIO pinini input olarak ayarla */
static void gpio_set_input(int pin) {
    volatile uint32_t *gpfsel;
    int shift;

    if(pin < 10) {
        gpfsel = GPFSEL0;
        shift = pin * 3;
    } else if(pin < 20) {
        gpfsel = GPFSEL1;
        shift = (pin - 10) * 3;
    } else {
        gpfsel = GPFSEL2;
        shift = (pin - 20) * 3;
    }

    uint32_t val = *gpfsel;
    val &= ~(7 << shift);
    *gpfsel = val;
}

/* Pull-up resistor etkinleştir */
static void gpio_enable_pullup(int pin) {
    *GPPUD = 2;
    gpio_delay(150);
    *GPPUDCLK0 = (1 << pin);
    gpio_delay(150);
    *GPPUD = 0;
    *GPPUDCLK0 = 0;
}

/* GPIO pinini oku */
static int gpio_read(int pin) {
    return (*GPLEV0 >> pin) & 1;
}

/* GPIO butonlarını oku */
static uint8_t read_gpio_buttons(void) {
    /* QEMU'da GPIO düzgün çalışmıyor, devre dışı bırak */
    /* Gerçek Pi'de bu satırı kaldır */
    return 0;

    /*
    uint8_t state = 0;

    if(!gpio_read(GPIO_BTN_UP))     state |= BTN_UP;
    if(!gpio_read(GPIO_BTN_DOWN))   state |= BTN_DOWN;
    if(!gpio_read(GPIO_BTN_LEFT))   state |= BTN_LEFT;
    if(!gpio_read(GPIO_BTN_RIGHT))  state |= BTN_RIGHT;
    if(!gpio_read(GPIO_BTN_A))      state |= BTN_A;
    if(!gpio_read(GPIO_BTN_B))      state |= BTN_B;
    if(!gpio_read(GPIO_BTN_START))  state |= BTN_START;
    if(!gpio_read(GPIO_BTN_SELECT)) state |= BTN_SELECT;

    return state;
    */
}

/* UART'tan giriş oku (QEMU test için) */
static uint8_t read_uart_input(void) {
    uint8_t state = 0;
    int c = uart_getc();

    if(c < 0) return 0;  /* Karakter yok */

    /* Debug: Gelen karakteri logla */
    uart_puts("[UART] Karakter: ");
    char buf[2] = {(char)c, 0};
    if(c >= 32 && c < 127) {
        uart_puts(buf);
    } else {
        uart_puts("0x");
        uart_hex(c);
    }
    uart_puts("\n");

    /*
     * Terminal kontrolleri:
     * w/W veya k/K veya 8 = Yukarı
     * s/S veya j/J veya 2 = Aşağı
     * a/A veya h/H veya 4 = Sol
     * d/D veya l/L veya 6 = Sağ
     * z/Z veya Enter veya Space = A (Seç)
     * x/X veya b/B veya Escape = B (Geri)
     * Ok tuşları: ESC [ A/B/C/D
     */

    switch(c) {
        /* Yön tuşları - WASD */
        case 'w': case 'W': case 'k': case 'K': case '8':
            state = BTN_UP;
            break;
        case 's': case 'S': case 'j': case 'J': case '2':
            state = BTN_DOWN;
            break;
        case 'a': case 'A': case 'h': case 'H': case '4':
            state = BTN_LEFT;
            break;
        case 'd': case 'D': case 'l': case 'L': case '6':
            state = BTN_RIGHT;
            break;

        /* Aksiyon tuşları */
        case 'z': case 'Z': case '\r': case '\n': case ' ':
            state = BTN_A;
            break;
        case 'x': case 'X': case 'b': case 'B':
            state = BTN_B;
            break;

        /* ESC veya ESC sequence */
        case 27:  /* Escape */
            /* Ok tuşları için ESC sequence kontrol et */
            if(uart_available()) {
                int c2 = uart_getc();
                if(c2 == '[' && uart_available()) {
                    int c3 = uart_getc();
                    switch(c3) {
                        case 'A': state = BTN_UP; break;
                        case 'B': state = BTN_DOWN; break;
                        case 'C': state = BTN_RIGHT; break;
                        case 'D': state = BTN_LEFT; break;
                    }
                } else {
                    state = BTN_B;  /* Sadece ESC = Geri */
                }
            } else {
                state = BTN_B;  /* Sadece ESC = Geri */
            }
            break;

        /* Start/Select */
        case 'p': case 'P':
            state = BTN_START;
            break;
        case 'o': case 'O':
            state = BTN_SELECT;
            break;
    }

    /* Debug: Hangi buton state döndü */
    if(state != 0) {
        uart_puts("[UART] State: ");
        uart_hex(state);
        uart_puts("\n");
    }

    return state;
}

/* USB klavye tuşlarını oku */
static uint8_t read_keyboard_buttons(void) {
    uint8_t state = 0;

    if(!keyboard_enabled) return 0;

    if(key_pressed(KEY_UP))     state |= BTN_UP;
    if(key_pressed(KEY_DOWN))   state |= BTN_DOWN;
    if(key_pressed(KEY_LEFT))   state |= BTN_LEFT;
    if(key_pressed(KEY_RIGHT))  state |= BTN_RIGHT;
    if(key_pressed(KEY_W))      state |= BTN_UP;
    if(key_pressed(KEY_S))      state |= BTN_DOWN;
    if(key_pressed(KEY_A))      state |= BTN_LEFT;
    if(key_pressed(KEY_D))      state |= BTN_RIGHT;
    if(key_pressed(KEY_Z))      state |= BTN_A;
    if(key_pressed(KEY_ENTER))  state |= BTN_A;
    if(key_pressed(KEY_X))      state |= BTN_B;
    if(key_pressed(KEY_ESC))    state |= BTN_B;
    if(key_pressed(KEY_SPACE))  state |= BTN_START;
    if(key_pressed(KEY_TAB))    state |= BTN_SELECT;

    return state;
}

/* Input sistemini başlat */
void input_init(void) {
    gpio_set_input(GPIO_BTN_UP);
    gpio_set_input(GPIO_BTN_DOWN);
    gpio_set_input(GPIO_BTN_LEFT);
    gpio_set_input(GPIO_BTN_RIGHT);
    gpio_set_input(GPIO_BTN_A);
    gpio_set_input(GPIO_BTN_B);
    gpio_set_input(GPIO_BTN_START);
    gpio_set_input(GPIO_BTN_SELECT);

    gpio_enable_pullup(GPIO_BTN_UP);
    gpio_enable_pullup(GPIO_BTN_DOWN);
    gpio_enable_pullup(GPIO_BTN_LEFT);
    gpio_enable_pullup(GPIO_BTN_RIGHT);
    gpio_enable_pullup(GPIO_BTN_A);
    gpio_enable_pullup(GPIO_BTN_B);
    gpio_enable_pullup(GPIO_BTN_START);
    gpio_enable_pullup(GPIO_BTN_SELECT);

    usb_kbd_init();

    prev_state = 0;
    curr_state = 0;
    uart_input_state = 0;
    keyboard_enabled = 1;
}

/* Tüm girişleri oku */
uint8_t input_read(void) {
    uint8_t state = 0;

    /* GPIO butonları */
    state |= read_gpio_buttons();

    /* USB Klavye */
    state |= read_keyboard_buttons();

    /* UART girişi (QEMU için) */
    state |= uart_input_state;

    return state;
}

/* Debounce ile oku */
uint8_t input_read_debounced(void) {
    return input_read();
}

/* Buton basılı mı? */
int btn_pressed(uint8_t state, uint8_t btn) {
    return (state & btn) != 0;
}

/* Buton yeni mi basıldı? */
int btn_just_pressed(uint8_t btn) {
    return (curr_state & btn) && !(prev_state & btn);
}

/* Buton yeni mi bırakıldı? */
int btn_just_released(uint8_t btn) {
    return !(curr_state & btn) && (prev_state & btn);
}

/* Durumu güncelle */
void input_update(void) {
    prev_state = curr_state;

    /* UART girişini oku - her frame yeni oku */
    uart_input_state = read_uart_input();

    /* USB klavye güncelle */
    usb_kbd_update();

    /* Birleşik durumu al */
    curr_state = uart_input_state;  /* Sadece UART (QEMU için) */
    curr_state |= read_gpio_buttons();
    curr_state |= read_keyboard_buttons();
}

/* Klavye desteğini aç/kapat */
void input_enable_keyboard(int enable) {
    keyboard_enabled = enable;
}

/* Klavye desteği açık mı? */
int input_keyboard_enabled(void) {
    return keyboard_enabled;
}
