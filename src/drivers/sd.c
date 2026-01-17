/* sd.c - SD Card Driver for Raspberry Pi (EMMC) */
#include <drivers/sd.h>
#include <hw.h>

/* EMMC Register adresleri (BCM2835/2837) */
#define MMIO_BASE       0x3F000000
#define EMMC_BASE       (MMIO_BASE + 0x300000)

#define EMMC_ARG2       ((volatile uint32_t*)(EMMC_BASE + 0x00))
#define EMMC_BLKSIZECNT ((volatile uint32_t*)(EMMC_BASE + 0x04))
#define EMMC_ARG1       ((volatile uint32_t*)(EMMC_BASE + 0x08))
#define EMMC_CMDTM      ((volatile uint32_t*)(EMMC_BASE + 0x0C))
#define EMMC_RESP0      ((volatile uint32_t*)(EMMC_BASE + 0x10))
#define EMMC_RESP1      ((volatile uint32_t*)(EMMC_BASE + 0x14))
#define EMMC_RESP2      ((volatile uint32_t*)(EMMC_BASE + 0x18))
#define EMMC_RESP3      ((volatile uint32_t*)(EMMC_BASE + 0x1C))
#define EMMC_DATA       ((volatile uint32_t*)(EMMC_BASE + 0x20))
#define EMMC_STATUS     ((volatile uint32_t*)(EMMC_BASE + 0x24))
#define EMMC_CONTROL0   ((volatile uint32_t*)(EMMC_BASE + 0x28))
#define EMMC_CONTROL1   ((volatile uint32_t*)(EMMC_BASE + 0x2C))
#define EMMC_INTERRUPT  ((volatile uint32_t*)(EMMC_BASE + 0x30))
#define EMMC_IRPT_MASK  ((volatile uint32_t*)(EMMC_BASE + 0x34))
#define EMMC_IRPT_EN    ((volatile uint32_t*)(EMMC_BASE + 0x38))
#define EMMC_CONTROL2   ((volatile uint32_t*)(EMMC_BASE + 0x3C))
#define EMMC_SLOTISR_VER ((volatile uint32_t*)(EMMC_BASE + 0xFC))

/* Status register bitleri */
#define SR_READ_AVAILABLE   0x00000800
#define SR_WRITE_AVAILABLE  0x00000400
#define SR_DAT_INHIBIT      0x00000002
#define SR_CMD_INHIBIT      0x00000001
#define SR_APP_CMD          0x00000020

/* Interrupt bitleri */
#define INT_DATA_TIMEOUT    0x00100000
#define INT_CMD_TIMEOUT     0x00010000
#define INT_READ_RDY        0x00000020
#define INT_WRITE_RDY       0x00000010
#define INT_DATA_DONE       0x00000002
#define INT_CMD_DONE        0x00000001
#define INT_ERROR_MASK      0x017E8000

/* Control1 bitleri */
#define C1_SRST_DATA        0x04000000
#define C1_SRST_CMD         0x02000000
#define C1_SRST_HC          0x01000000
#define C1_TOUNIT_MAX       0x000E0000
#define C1_CLK_EN           0x00000004
#define C1_CLK_STABLE       0x00000002
#define C1_CLK_INTLEN       0x00000001

/* SD Komutları */
#define CMD_GO_IDLE         0x00000000
#define CMD_ALL_SEND_CID    0x02010000
#define CMD_SEND_REL_ADDR   0x03020000
#define CMD_CARD_SELECT     0x07030000
#define CMD_SEND_IF_COND    0x08020000
#define CMD_STOP_TRANS      0x0C030000
#define CMD_READ_SINGLE     0x11220010
#define CMD_READ_MULTI      0x12220032
#define CMD_WRITE_SINGLE    0x18220000
#define CMD_WRITE_MULTI     0x19220022
#define CMD_APP_CMD         0x37000000
#define CMD_SET_BLOCKCNT    0x17020000
#define CMD_SEND_OP_COND    0x29020000
#define CMD_SEND_SCR        0x33220010

/* SD kart global değişkeni */
static SDCard sd_card;

/* Yardımcı fonksiyonlar */
static void sd_delay(int count) {
    while(count--) {
        __asm__ volatile("nop");
    }
}

static int sd_wait_for_cmd(void) {
    int timeout = 1000000;
    while((*EMMC_STATUS & SR_CMD_INHIBIT) && timeout--) {
        sd_delay(10);
    }
    return (timeout > 0) ? SD_OK : SD_TIMEOUT;
}

static int sd_wait_for_data(void) {
    int timeout = 1000000;
    while((*EMMC_STATUS & SR_DAT_INHIBIT) && timeout--) {
        sd_delay(10);
    }
    return (timeout > 0) ? SD_OK : SD_TIMEOUT;
}

static int sd_wait_for_interrupt(uint32_t mask) {
    int timeout = 1000000;
    uint32_t irq;

    while(timeout--) {
        irq = *EMMC_INTERRUPT;
        if(irq & (mask | INT_ERROR_MASK)) {
            break;
        }
        sd_delay(10);
    }

    if(timeout <= 0 || (irq & INT_CMD_TIMEOUT) || (irq & INT_DATA_TIMEOUT)) {
        return SD_TIMEOUT;
    }

    if(irq & INT_ERROR_MASK) {
        return SD_ERROR;
    }

    return SD_OK;
}

static int sd_send_command(uint32_t cmd, uint32_t arg) {
    /* Önceki interrupt'ları temizle */
    *EMMC_INTERRUPT = *EMMC_INTERRUPT;

    /* Komut hazır olana kadar bekle */
    if(sd_wait_for_cmd() != SD_OK) {
        return SD_TIMEOUT;
    }

    /* Komutu gönder */
    *EMMC_ARG1 = arg;
    *EMMC_CMDTM = cmd;

    /* Komutun tamamlanmasını bekle */
    if(sd_wait_for_interrupt(INT_CMD_DONE) != SD_OK) {
        return SD_ERROR;
    }

    /* Interrupt'ı temizle */
    *EMMC_INTERRUPT = INT_CMD_DONE;

    return SD_OK;
}

static int sd_send_app_command(uint32_t cmd, uint32_t arg) {
    /* Önce APP_CMD gönder */
    if(sd_send_command(CMD_APP_CMD, sd_card.rca) != SD_OK) {
        return SD_ERROR;
    }

    /* Sonra asıl komutu gönder */
    return sd_send_command(cmd, arg);
}

static void sd_set_clock(uint32_t freq) {
    uint32_t div;
    uint32_t c1 = *EMMC_CONTROL1;

    /* Clock'u kapat */
    c1 &= ~C1_CLK_EN;
    *EMMC_CONTROL1 = c1;
    sd_delay(10000);

    /* Divider hesapla (base clock 41.6MHz varsayılarak) */
    div = 41666666 / freq;
    if(div < 2) div = 2;
    if(div > 2046) div = 2046;
    div = (div >> 1);

    /* Yeni clock ayarla */
    c1 &= ~0xFFE0;
    c1 |= (div << 8);
    c1 |= C1_CLK_INTLEN;
    *EMMC_CONTROL1 = c1;
    sd_delay(10000);

    /* Clock'un stabil olmasını bekle */
    int timeout = 10000;
    while(!(*EMMC_CONTROL1 & C1_CLK_STABLE) && timeout--) {
        sd_delay(10);
    }

    /* Clock'u aç */
    *EMMC_CONTROL1 |= C1_CLK_EN;
    sd_delay(10000);
}

/* SD kart başlatma */
int sd_init(void) {
    uint32_t resp;
    int timeout;

    uart_puts("SD kart baslatiliyor...\n");

    /* Yapıyı sıfırla */
    sd_card.initialized = 0;
    sd_card.rca = 0;
    sd_card.type = SD_TYPE_UNKNOWN;

    /* Host controller'ı resetle */
    *EMMC_CONTROL0 = 0;
    *EMMC_CONTROL1 = C1_SRST_HC;

    timeout = 10000;
    while((*EMMC_CONTROL1 & C1_SRST_HC) && timeout--) {
        sd_delay(100);
    }
    if(timeout <= 0) {
        uart_puts("SD: Reset timeout\n");
        return SD_TIMEOUT;
    }

    /* Clock ayarla (400kHz - tanımlama için) */
    *EMMC_CONTROL1 = C1_CLK_INTLEN | C1_TOUNIT_MAX;
    sd_set_clock(400000);

    /* Tüm interrupt'ları etkinleştir */
    *EMMC_IRPT_EN = 0xFFFFFFFF;
    *EMMC_IRPT_MASK = 0xFFFFFFFF;

    /* GO_IDLE_STATE (CMD0) */
    if(sd_send_command(CMD_GO_IDLE, 0) != SD_OK) {
        uart_puts("SD: CMD0 failed\n");
        return SD_ERROR;
    }

    /* SEND_IF_COND (CMD8) - SD v2 kontrolü */
    if(sd_send_command(CMD_SEND_IF_COND, 0x1AA) == SD_OK) {
        resp = *EMMC_RESP0;
        if((resp & 0xFFF) != 0x1AA) {
            uart_puts("SD: CMD8 response error\n");
            return SD_ERROR;
        }
        sd_card.type = SD_TYPE_SDV2;
    } else {
        sd_card.type = SD_TYPE_SDV1;
    }

    /* ACMD41 - Kartın hazır olmasını bekle */
    timeout = 100;
    do {
        if(sd_send_app_command(CMD_SEND_OP_COND,
            (sd_card.type == SD_TYPE_SDV2) ? 0x40FF8000 : 0x00FF8000) != SD_OK) {
            uart_puts("SD: ACMD41 failed\n");
            return SD_ERROR;
        }
        resp = *EMMC_RESP0;
        sd_delay(10000);
    } while(!(resp & 0x80000000) && timeout--);

    if(timeout <= 0) {
        uart_puts("SD: Card not ready\n");
        return SD_TIMEOUT;
    }

    sd_card.ocr = resp;

    /* SDHC kontrolü */
    if((resp & 0x40000000) && sd_card.type == SD_TYPE_SDV2) {
        sd_card.type = SD_TYPE_SDHC;
    }

    /* ALL_SEND_CID (CMD2) */
    if(sd_send_command(CMD_ALL_SEND_CID, 0) != SD_OK) {
        uart_puts("SD: CMD2 failed\n");
        return SD_ERROR;
    }
    sd_card.cid[0] = *EMMC_RESP0;
    sd_card.cid[1] = *EMMC_RESP1;
    sd_card.cid[2] = *EMMC_RESP2;
    sd_card.cid[3] = *EMMC_RESP3;

    /* SEND_RELATIVE_ADDR (CMD3) */
    if(sd_send_command(CMD_SEND_REL_ADDR, 0) != SD_OK) {
        uart_puts("SD: CMD3 failed\n");
        return SD_ERROR;
    }
    sd_card.rca = *EMMC_RESP0 & 0xFFFF0000;

    /* Clock'u yükselt (25MHz) */
    sd_set_clock(25000000);

    /* SELECT_CARD (CMD7) */
    if(sd_send_command(CMD_CARD_SELECT, sd_card.rca) != SD_OK) {
        uart_puts("SD: CMD7 failed\n");
        return SD_ERROR;
    }

    /* Blok boyutunu 512 yap */
    *EMMC_BLKSIZECNT = (1 << 16) | 512;
    sd_card.block_size = 512;

    /* Kapasiteyi hesapla (basitleştirilmiş) */
    if(sd_card.type == SD_TYPE_SDHC) {
        sd_card.capacity = 4ULL * 1024 * 1024 * 1024; /* Varsayılan 4GB */
    } else {
        sd_card.capacity = 2ULL * 1024 * 1024 * 1024; /* Varsayılan 2GB */
    }

    sd_card.initialized = 1;

    uart_puts("SD kart baslatildi! Tip: ");
    if(sd_card.type == SD_TYPE_SDHC) uart_puts("SDHC\n");
    else if(sd_card.type == SD_TYPE_SDV2) uart_puts("SDv2\n");
    else uart_puts("SDv1\n");

    return SD_OK;
}

/* Tek blok oku */
int sd_read_block(uint32_t lba, uint8_t *buffer) {
    if(!sd_card.initialized) return SD_ERROR;

    /* SDHC için LBA, SDv1/v2 için byte adresi */
    uint32_t addr = (sd_card.type == SD_TYPE_SDHC) ? lba : (lba * 512);

    /* Veri hazır olana kadar bekle */
    if(sd_wait_for_data() != SD_OK) {
        return SD_TIMEOUT;
    }

    /* Blok sayısını ayarla */
    *EMMC_BLKSIZECNT = (1 << 16) | 512;

    /* READ_SINGLE_BLOCK (CMD17) */
    if(sd_send_command(CMD_READ_SINGLE, addr) != SD_OK) {
        return SD_ERROR;
    }

    /* Veri okumayı bekle */
    if(sd_wait_for_interrupt(INT_READ_RDY) != SD_OK) {
        return SD_ERROR;
    }

    /* Veriyi oku */
    uint32_t *buf32 = (uint32_t*)buffer;
    for(int i = 0; i < 128; i++) {
        buf32[i] = *EMMC_DATA;
    }

    /* Data done bekle */
    if(sd_wait_for_interrupt(INT_DATA_DONE) != SD_OK) {
        return SD_ERROR;
    }

    *EMMC_INTERRUPT = INT_DATA_DONE | INT_READ_RDY;

    return SD_OK;
}

/* Tek blok yaz */
int sd_write_block(uint32_t lba, const uint8_t *buffer) {
    if(!sd_card.initialized) return SD_ERROR;

    uint32_t addr = (sd_card.type == SD_TYPE_SDHC) ? lba : (lba * 512);

    if(sd_wait_for_data() != SD_OK) {
        return SD_TIMEOUT;
    }

    *EMMC_BLKSIZECNT = (1 << 16) | 512;

    /* WRITE_SINGLE_BLOCK (CMD24) */
    if(sd_send_command(CMD_WRITE_SINGLE, addr) != SD_OK) {
        return SD_ERROR;
    }

    if(sd_wait_for_interrupt(INT_WRITE_RDY) != SD_OK) {
        return SD_ERROR;
    }

    /* Veriyi yaz */
    const uint32_t *buf32 = (const uint32_t*)buffer;
    for(int i = 0; i < 128; i++) {
        *EMMC_DATA = buf32[i];
    }

    if(sd_wait_for_interrupt(INT_DATA_DONE) != SD_OK) {
        return SD_ERROR;
    }

    *EMMC_INTERRUPT = INT_DATA_DONE | INT_WRITE_RDY;

    return SD_OK;
}

/* Birden fazla blok oku */
int sd_read_blocks(uint32_t lba, uint32_t count, uint8_t *buffer) {
    for(uint32_t i = 0; i < count; i++) {
        if(sd_read_block(lba + i, buffer + i * 512) != SD_OK) {
            return SD_ERROR;
        }
    }
    return SD_OK;
}

/* Birden fazla blok yaz */
int sd_write_blocks(uint32_t lba, uint32_t count, const uint8_t *buffer) {
    for(uint32_t i = 0; i < count; i++) {
        if(sd_write_block(lba + i, buffer + i * 512) != SD_OK) {
            return SD_ERROR;
        }
    }
    return SD_OK;
}

/* Kapasite al */
uint64_t sd_get_capacity(void) {
    return sd_card.capacity;
}

/* Blok boyutu al */
uint32_t sd_get_block_size(void) {
    return sd_card.block_size;
}

/* Başlatıldı mı? */
int sd_is_initialized(void) {
    return sd_card.initialized;
}
