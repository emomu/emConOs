/* sd.h - SD Card Driver for Raspberry Pi */
#ifndef SD_H
#define SD_H

#include <types.h>

/* SD kart durumu */
#define SD_OK          0
#define SD_ERROR      -1
#define SD_TIMEOUT    -2
#define SD_BUSY       -3

/* SD kart bilgisi */
typedef struct {
    uint32_t rca;           /* Relative Card Address */
    uint32_t ocr;           /* Operating Conditions Register */
    uint32_t cid[4];        /* Card Identification */
    uint32_t csd[4];        /* Card Specific Data */
    uint64_t capacity;      /* Kart kapasitesi (byte) */
    uint32_t block_size;    /* Blok boyutu (genelde 512) */
    uint8_t  type;          /* Kart tipi (SDv1, SDv2, SDHC) */
    uint8_t  initialized;   /* Başlatıldı mı? */
} SDCard;

/* SD kart tipleri */
#define SD_TYPE_UNKNOWN  0
#define SD_TYPE_SDV1     1
#define SD_TYPE_SDV2     2
#define SD_TYPE_SDHC     3

/* Fonksiyonlar */
int sd_init(void);
int sd_read_block(uint32_t lba, uint8_t *buffer);
int sd_write_block(uint32_t lba, const uint8_t *buffer);
int sd_read_blocks(uint32_t lba, uint32_t count, uint8_t *buffer);
int sd_write_blocks(uint32_t lba, uint32_t count, const uint8_t *buffer);

/* Bilgi fonksiyonları */
uint64_t sd_get_capacity(void);
uint32_t sd_get_block_size(void);
int sd_is_initialized(void);

#endif
