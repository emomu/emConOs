/* kernel.c - Ana kernel dosyası */
#include "types.h"
#include "hw.h"
#include "graphics.h"
#include "screens.h"

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

        /* Ana ekrana geç */
        uart_puts("Ana ekrana geciliyor...\n");
        current_screen = SCREEN_MAIN;
        draw_main_screen();

        uart_puts("Sistem hazir.\n");
    }

    /* Ana döngü */
    while(1) {
        __asm__ volatile("nop");
    }
}
