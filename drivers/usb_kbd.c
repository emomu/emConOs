/* usb_kbd.c - USB Keyboard driver for Raspberry Pi */
#include "usb_kbd.h"

/*
 * Raspberry Pi USB Controller (DWC2 - DesignWare USB 2.0)
 * USB klavye bare-metal'da oldukça karmaşık, bu basitleştirilmiş bir versiyon.
 * Gerçek implementasyon için USB stack gerekir.
 *
 * Bu driver şimdilik polling-based basit bir yaklaşım kullanıyor.
 */

#define MMIO_BASE       0x3F000000
#define USB_BASE        (MMIO_BASE + 0x980000)

/* DWC2 USB Controller registers */
#define USB_CORE_GAHBCFG    ((volatile uint32_t*)(USB_BASE + 0x008))
#define USB_CORE_GUSBCFG    ((volatile uint32_t*)(USB_BASE + 0x00C))
#define USB_CORE_GRSTCTL    ((volatile uint32_t*)(USB_BASE + 0x010))
#define USB_CORE_GINTSTS    ((volatile uint32_t*)(USB_BASE + 0x014))
#define USB_CORE_GINTMSK    ((volatile uint32_t*)(USB_BASE + 0x018))
#define USB_CORE_HCFG       ((volatile uint32_t*)(USB_BASE + 0x400))
#define USB_CORE_HPRT       ((volatile uint32_t*)(USB_BASE + 0x440))

/* Klavye durumu */
static struct {
    uint8_t keys[6];        /* Basılı tuşlar (max 6) */
    uint8_t modifiers;      /* Shift, Ctrl, Alt, vb. */
    uint8_t prev_keys[6];   /* Önceki frame'deki tuşlar */
    int initialized;
} kbd_state;

/* Gecikme */
static void usb_delay(int count) {
    while(count--) {
        __asm__ volatile("nop");
    }
}

/* USB controller'ı sıfırla */
static void usb_reset(void) {
    /* Core soft reset */
    *USB_CORE_GRSTCTL = (1 << 0);  /* CSftRst */
    usb_delay(100000);

    /* Reset bitinin temizlenmesini bekle */
    while(*USB_CORE_GRSTCTL & (1 << 0)) {
        usb_delay(1000);
    }

    /* AHB idle bekle */
    while(!(*USB_CORE_GRSTCTL & (1 << 31))) {
        usb_delay(1000);
    }
}

/* USB host modunu başlat */
static void usb_host_init(void) {
    uint32_t cfg;

    /* Force host mode */
    cfg = *USB_CORE_GUSBCFG;
    cfg &= ~(1 << 30);  /* Clear ForceDevMode */
    cfg |= (1 << 29);   /* Set ForceHostMode */
    *USB_CORE_GUSBCFG = cfg;

    usb_delay(100000);

    /* Host config */
    *USB_CORE_HCFG = 1;  /* FS/LS only */

    /* Enable DMA and global interrupts */
    *USB_CORE_GAHBCFG = (1 << 0) | (1 << 5);  /* GlblIntrMsk, DMAEn */
}

/* Port durumunu kontrol et */
static int usb_port_connected(void) {
    uint32_t hprt = *USB_CORE_HPRT;
    return (hprt & (1 << 0)) ? 1 : 0;  /* PrtConnSts */
}

/* USB klavyeyi başlat */
void usb_kbd_init(void) {
    /* State'i temizle */
    for(int i = 0; i < 6; i++) {
        kbd_state.keys[i] = 0;
        kbd_state.prev_keys[i] = 0;
    }
    kbd_state.modifiers = 0;
    kbd_state.initialized = 0;

    /* USB controller'ı başlat */
    usb_reset();
    usb_host_init();

    /* Port bağlantısını kontrol et */
    if(usb_port_connected()) {
        kbd_state.initialized = 1;
    }

    /*
     * NOT: Gerçek bir USB HID driver için:
     * 1. Device enumeration
     * 2. Configuration descriptor okuma
     * 3. HID report descriptor okuma
     * 4. Interrupt endpoint setup
     * 5. Polling veya interrupt handling
     *
     * Bu basitleştirilmiş versiyon sadece temel yapıyı gösteriyor.
     * Tam USB stack için USPi veya Circle kütüphanesi önerilir.
     */
}

/* Klavyeden tuş oku (polling) */
uint8_t usb_kbd_read(void) {
    if(!kbd_state.initialized) return KEY_NONE;

    /* İlk basılı tuşu döndür */
    for(int i = 0; i < 6; i++) {
        if(kbd_state.keys[i] != KEY_NONE) {
            return kbd_state.keys[i];
        }
    }

    return KEY_NONE;
}

/* Klavyede veri var mı? */
int usb_kbd_available(void) {
    if(!kbd_state.initialized) return 0;

    for(int i = 0; i < 6; i++) {
        if(kbd_state.keys[i] != KEY_NONE) {
            return 1;
        }
    }
    return 0;
}

/* Belirli bir tuş basılı mı? */
int key_pressed(uint8_t key) {
    for(int i = 0; i < 6; i++) {
        if(kbd_state.keys[i] == key) return 1;
    }
    return 0;
}

/* Tuş yeni mi basıldı? */
int key_just_pressed(uint8_t key) {
    int now_pressed = 0, was_pressed = 0;

    for(int i = 0; i < 6; i++) {
        if(kbd_state.keys[i] == key) now_pressed = 1;
        if(kbd_state.prev_keys[i] == key) was_pressed = 1;
    }

    return now_pressed && !was_pressed;
}

/* Tuş yeni mi bırakıldı? */
int key_just_released(uint8_t key) {
    int now_pressed = 0, was_pressed = 0;

    for(int i = 0; i < 6; i++) {
        if(kbd_state.keys[i] == key) now_pressed = 1;
        if(kbd_state.prev_keys[i] == key) was_pressed = 1;
    }

    return !now_pressed && was_pressed;
}

/* Klavye durumunu güncelle */
void usb_kbd_update(void) {
    /* Önceki durumu kaydet */
    for(int i = 0; i < 6; i++) {
        kbd_state.prev_keys[i] = kbd_state.keys[i];
    }

    /*
     * Gerçek implementasyonda burası:
     * - USB interrupt endpoint'ten HID report okur
     * - Report'u parse eder
     * - kbd_state.keys[] ve kbd_state.modifiers günceller
     *
     * Şimdilik sadece state'i koruyoruz.
     * Tam USB HID için ayrı bir USB stack gerekli.
     */

    if(!kbd_state.initialized) {
        /* Tekrar bağlantı kontrolü */
        if(usb_port_connected()) {
            kbd_state.initialized = 1;
        }
    }
}

/*
 * Test için simüle edilmiş tuş basımı
 * (Gerçek donanımda bu fonksiyon kullanılmaz)
 */
void usb_kbd_simulate_key(uint8_t key) {
    kbd_state.keys[0] = key;
}

void usb_kbd_simulate_release(void) {
    for(int i = 0; i < 6; i++) {
        kbd_state.keys[i] = KEY_NONE;
    }
}
