/* fat32.h - FAT32 Filesystem */
#ifndef FAT32_H
#define FAT32_H

#include "../types.h"

/* FAT32 hata kodları */
#define FAT_OK           0
#define FAT_ERROR       -1
#define FAT_NOT_FOUND   -2
#define FAT_EOF         -3
#define FAT_NO_SPACE    -4

/* Dosya özellikleri */
#define ATTR_READ_ONLY   0x01
#define ATTR_HIDDEN      0x02
#define ATTR_SYSTEM      0x04
#define ATTR_VOLUME_ID   0x08
#define ATTR_DIRECTORY   0x10
#define ATTR_ARCHIVE     0x20
#define ATTR_LONG_NAME   0x0F

/* Maksimum değerler */
#define MAX_FILENAME     256
#define MAX_PATH         512
#define MAX_OPEN_FILES   16

/* Dosya bilgisi */
typedef struct {
    char name[MAX_FILENAME];
    uint32_t size;
    uint32_t cluster;       /* İlk cluster */
    uint8_t  attr;          /* Dosya özellikleri */
    uint8_t  is_dir;        /* Klasör mü? */
    /* Tarih/saat bilgileri */
    uint16_t date;
    uint16_t time;
} FileInfo;

/* Açık dosya yapısı */
typedef struct {
    uint8_t  used;
    uint32_t cluster;       /* Mevcut cluster */
    uint32_t start_cluster; /* Başlangıç cluster */
    uint32_t position;      /* Dosya içindeki pozisyon */
    uint32_t size;          /* Dosya boyutu */
    uint8_t  mode;          /* Açılma modu */
} File;

/* Dosya açma modları */
#define FILE_READ   0x01
#define FILE_WRITE  0x02

/* FAT32 fonksiyonları */
int fat32_init(void);
int fat32_is_mounted(void);

/* Dizin işlemleri */
int fat32_open_dir(const char *path);
int fat32_read_dir(FileInfo *info);
int fat32_close_dir(void);

/* Dosya işlemleri */
int fat32_open(const char *path, uint8_t mode);
int fat32_read(int fd, void *buffer, uint32_t size);
int fat32_write(int fd, const void *buffer, uint32_t size);
int fat32_seek(int fd, uint32_t position);
int fat32_close(int fd);
uint32_t fat32_tell(int fd);
uint32_t fat32_size(int fd);

/* Yardımcı fonksiyonlar */
int fat32_exists(const char *path);
int fat32_is_directory(const char *path);
int fat32_get_info(const char *path, FileInfo *info);

/* Biçimlendirme yardımcıları */
void fat32_format_size(uint32_t size, char *buffer);

#endif
