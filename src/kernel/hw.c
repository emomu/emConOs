/* hw.c - Donanım fonksiyonları */
#include <hw.h>
#include <graphics.h>

/* Donanım adresleri */
#define MMIO_BASE       0x3F000000

/* GPIO & UART */
#define GPFSEL1         ((volatile uint32_t*)(MMIO_BASE + 0x00200004))
#define GPPUD           ((volatile uint32_t*)(MMIO_BASE + 0x00200094))
#define GPPUDCLK0       ((volatile uint32_t*)(MMIO_BASE + 0x00200098))
#define UART0_DR        ((volatile uint32_t*)(MMIO_BASE + 0x00201000))
#define UART0_FR        ((volatile uint32_t*)(MMIO_BASE + 0x00201018))
#define UART0_IBRD      ((volatile uint32_t*)(MMIO_BASE + 0x00201024))
#define UART0_FBRD      ((volatile uint32_t*)(MMIO_BASE + 0x00201028))
#define UART0_LCRH      ((volatile uint32_t*)(MMIO_BASE + 0x0020102C))
#define UART0_CR        ((volatile uint32_t*)(MMIO_BASE + 0x00201030))
#define UART0_ICR       ((volatile uint32_t*)(MMIO_BASE + 0x00201044))

/* Mailbox */
#define MAILBOX_BASE    (MMIO_BASE + 0xB880)
#define MBOX_READ       ((volatile uint32_t*)(MAILBOX_BASE + 0x00))
#define MBOX_STATUS     ((volatile uint32_t*)(MAILBOX_BASE + 0x18))
#define MBOX_WRITE      ((volatile uint32_t*)(MAILBOX_BASE + 0x20))
#define MBOX_FULL       0x80000000
#define MBOX_EMPTY      0x40000000

#define MBOX_REQUEST    0x00000000
#define MBOX_CH_PROP    8

volatile uint32_t __attribute__((aligned(16))) mbox[36];

void delay(int32_t count) {
    __asm__ volatile("__delay_%=: subs %[count], %[count], #1; bne __delay_%=\n"
         : "=r"(count): [count]"0"(count) : "cc");
}

void wait_seconds(int seconds) {
    for(int s = 0; s < seconds; s++) {
        for(volatile int i = 0; i < 50000000; i++) {
            __asm__ volatile("nop");
        }
    }
}

static void uart_send(char c) {
    while(*UART0_FR & (1<<5)) { }
    *UART0_DR = c;
}

void uart_init(void) {
    register uint32_t r;
    *UART0_CR = 0;
    r = *GPFSEL1;
    r &= ~((7<<12)|(7<<15));
    r |= (4<<12)|(4<<15);
    *GPFSEL1 = r;
    *GPPUD = 0;
    delay(150);
    *GPPUDCLK0 = (1<<14)|(1<<15);
    delay(150);
    *GPPUDCLK0 = 0;
    *UART0_ICR = 0x7FF;
    *UART0_IBRD = 1;
    *UART0_FBRD = 40;
    *UART0_LCRH = (1<<4) | (1<<5) | (1<<6);
    *UART0_CR = (1<<0) | (1<<8) | (1<<9);
}

void uart_puts(char *s) {
    while(*s) { uart_send(*s++); }
}

void uart_hex(unsigned int d) {
    unsigned int n;
    int c;
    for(c=28;c>=0;c-=4) {
        n=(d>>c)&0xF;
        n+=n>9?0x37:0x30;
        uart_send(n);
    }
}

/* UART'tan karakter oku (non-blocking) */
int uart_getc(void) {
    if(*UART0_FR & (1<<4)) {  /* RXFE - Receive FIFO Empty */
        return -1;  /* Karakter yok */
    }
    return (int)(*UART0_DR & 0xFF);
}

/* UART'ta karakter var mı? */
int uart_available(void) {
    return !(*UART0_FR & (1<<4));
}

static int mailbox_call(unsigned char ch) {
    __asm__ volatile("dsb sy");

    uint32_t r = (((uint32_t)((unsigned long)&mbox) & ~0xF) | (ch & 0xF));
    while(*MBOX_STATUS & MBOX_FULL) { __asm__ volatile("nop"); }
    *MBOX_WRITE = r;

    __asm__ volatile("dsb sy");

    while(1) {
        while(*MBOX_STATUS & MBOX_EMPTY) { __asm__ volatile("nop"); }
        if(r == *MBOX_READ) return mbox[1] == 0x80000000;
    }
    return 0;
}

void init_screen(void) {
    uart_puts("Ekran baslatiliyor...\n");

    mbox[0] = 35 * 4;
    mbox[1] = MBOX_REQUEST;

    mbox[2] = 0x48003; mbox[3] = 8; mbox[4] = 0; mbox[5] = 640; mbox[6] = 480;
    mbox[7] = 0x48004; mbox[8] = 8; mbox[9] = 0; mbox[10] = 640; mbox[11] = 480;
    mbox[12] = 0x48009; mbox[13] = 8; mbox[14] = 0; mbox[15] = 0; mbox[16] = 0;
    mbox[17] = 0x48005; mbox[18] = 4; mbox[19] = 0; mbox[20] = 32;
    mbox[21] = 0x48006; mbox[22] = 4; mbox[23] = 0; mbox[24] = 1;
    mbox[25] = 0x40001; mbox[26] = 8; mbox[27] = 0; mbox[28] = 4096; mbox[29] = 0;
    mbox[30] = 0x40008; mbox[31] = 4; mbox[32] = 0; mbox[33] = 0;
    mbox[34] = 0;

    if(mailbox_call(MBOX_CH_PROP) && mbox[28] != 0) {
        screen_width = mbox[10];
        screen_height = mbox[11];
        screen_pitch = mbox[33];

        uint32_t raw_addr = mbox[28];
        uart_puts("GPU Adresi: 0x"); uart_hex(raw_addr); uart_puts("\n");

        framebuffer = (uint8_t*)((unsigned long)(raw_addr & 0x3FFFFFFF));

        uart_puts("LFB: 0x"); uart_hex((unsigned int)((unsigned long)framebuffer));
        uart_puts(" Pitch: 0x"); uart_hex(screen_pitch);
        uart_puts("\n");
    } else {
        uart_puts("GPU Hatasi!\n");
    }
}
