/* start.S */
.section ".text.boot"

.global _start

_start:
    /* Sadece Çekirdek 0 çalışsın, diğerlerini uyutalım (RPi Zero 2W 4 çekirdekli) */
    mrs     x0, mpidr_el1
    and     x0, x0, #0xFF
    cbz     x0, master
    b       hang

hang:
    wfe
    b       hang

master:
    /* Stack Pointer'ı (Yığını) ayarla */
    ldr     x0, =_start
    mov     sp, x0

    /* BSS Bölümünü Temizle (Değişkenlerin hafızasını sıfırla) */
    ldr     x0, =__bss_start
    ldr     x1, =__bss_end
    cmp     x0, x1
    b.ge    2f
1:  str     xzr, [x0], #8
    cmp     x0, x1
    b.lt    1b

2:  /* Kernel'e zıpla! */
    bl      kernel_main
    b       hang