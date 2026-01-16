/* fat32.c - FAT32 Filesystem Implementation */
#include "fat32.h"
#include "../drivers/sd.h"
#include "../hw.h"

/* FAT32 Boot Sector yapısı */
typedef struct __attribute__((packed)) {
    uint8_t  jump[3];
    char     oem[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  fat_count;
    uint16_t root_entry_count;    /* FAT32'de 0 */
    uint16_t total_sectors_16;    /* FAT32'de 0 */
    uint8_t  media_type;
    uint16_t fat_size_16;         /* FAT32'de 0 */
    uint16_t sectors_per_track;
    uint16_t head_count;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    /* FAT32 specific */
    uint32_t fat_size_32;
    uint16_t ext_flags;
    uint16_t fs_version;
    uint32_t root_cluster;
    uint16_t fs_info_sector;
    uint16_t backup_boot_sector;
    uint8_t  reserved[12];
    uint8_t  drive_number;
    uint8_t  reserved1;
    uint8_t  boot_sig;
    uint32_t volume_id;
    char     volume_label[11];
    char     fs_type[8];
} FAT32BootSector;

/* Dizin girişi yapısı (32 byte) */
typedef struct __attribute__((packed)) {
    char     name[11];            /* 8.3 format */
    uint8_t  attr;
    uint8_t  nt_reserved;
    uint8_t  create_time_tenth;
    uint16_t create_time;
    uint16_t create_date;
    uint16_t access_date;
    uint16_t cluster_high;
    uint16_t modify_time;
    uint16_t modify_date;
    uint16_t cluster_low;
    uint32_t size;
} FAT32DirEntry;

/* FAT32 global durumu */
static struct {
    uint8_t  mounted;
    uint32_t fat_start;           /* FAT başlangıç sektörü */
    uint32_t data_start;          /* Veri bölgesi başlangıcı */
    uint32_t root_cluster;        /* Kök dizin cluster'ı */
    uint32_t sectors_per_cluster;
    uint32_t bytes_per_sector;
    uint32_t cluster_size;
    uint32_t fat_size;
} fat32;

/* Dizin okuma durumu */
static struct {
    uint8_t  open;
    uint32_t cluster;
    uint32_t entry_index;
    uint8_t  sector_buffer[512];
} dir_state;

/* Açık dosyalar */
static File open_files[MAX_OPEN_FILES];

/* Yardımcı: Karakter karşılaştır (case-insensitive) */
static int char_equal_ci(char a, char b) {
    if(a >= 'a' && a <= 'z') a -= 32;
    if(b >= 'a' && b <= 'z') b -= 32;
    return a == b;
}

/* Yardımcı: Cluster'ı sektöre çevir */
static uint32_t cluster_to_sector(uint32_t cluster) {
    return fat32.data_start + (cluster - 2) * fat32.sectors_per_cluster;
}

/* Yardımcı: Sonraki cluster'ı FAT'tan oku */
static uint32_t get_next_cluster(uint32_t cluster) {
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = fat32.fat_start + (fat_offset / 512);
    uint32_t entry_offset = fat_offset % 512;

    uint8_t buffer[512];
    if(sd_read_block(fat_sector, buffer) != SD_OK) {
        return 0x0FFFFFFF;
    }

    uint32_t next = *(uint32_t*)(buffer + entry_offset);
    return next & 0x0FFFFFFF;
}

/* Yardımcı: 8.3 ismini düzgün isme çevir */
static void convert_83_to_name(const char *name83, char *name) {
    int i, j = 0;

    /* Dosya adı (8 karakter) */
    for(i = 0; i < 8 && name83[i] != ' '; i++) {
        name[j++] = name83[i];
    }

    /* Uzantı varsa */
    if(name83[8] != ' ') {
        name[j++] = '.';
        for(i = 8; i < 11 && name83[i] != ' '; i++) {
            name[j++] = name83[i];
        }
    }

    name[j] = 0;
}

/* FAT32 başlat */
int fat32_init(void) {
    FAT32BootSector *bs;
    uint8_t buffer[512];

    uart_puts("FAT32 baslatiliyor...\n");

    /* SD kart başlatılmış mı? */
    if(!sd_is_initialized()) {
        if(sd_init() != SD_OK) {
            uart_puts("FAT32: SD kart baslatilamadi\n");
            return FAT_ERROR;
        }
    }

    /* Boot sector oku */
    if(sd_read_block(0, buffer) != SD_OK) {
        uart_puts("FAT32: Boot sector okunamadi\n");
        return FAT_ERROR;
    }

    bs = (FAT32BootSector*)buffer;

    /* FAT32 kontrolü */
    if(bs->bytes_per_sector != 512) {
        uart_puts("FAT32: Desteklenmeyen sektor boyutu\n");
        return FAT_ERROR;
    }

    /* FAT32 bilgilerini kaydet */
    fat32.bytes_per_sector = bs->bytes_per_sector;
    fat32.sectors_per_cluster = bs->sectors_per_cluster;
    fat32.cluster_size = fat32.bytes_per_sector * fat32.sectors_per_cluster;
    fat32.fat_start = bs->reserved_sectors;
    fat32.fat_size = bs->fat_size_32;
    fat32.data_start = fat32.fat_start + (bs->fat_count * fat32.fat_size);
    fat32.root_cluster = bs->root_cluster;
    fat32.mounted = 1;

    uart_puts("FAT32 baslatildi!\n");
    uart_puts("  Cluster boyutu: ");
    uart_hex(fat32.cluster_size);
    uart_puts("\n  Root cluster: ");
    uart_hex(fat32.root_cluster);
    uart_puts("\n");

    /* Açık dosyaları sıfırla */
    for(int i = 0; i < MAX_OPEN_FILES; i++) {
        open_files[i].used = 0;
    }

    return FAT_OK;
}

/* Mount durumu */
int fat32_is_mounted(void) {
    return fat32.mounted;
}

/* Dizin aç */
int fat32_open_dir(const char *path) {
    if(!fat32.mounted) return FAT_ERROR;

    /* Şimdilik sadece root dizini destekle */
    dir_state.open = 1;
    dir_state.cluster = fat32.root_cluster;
    dir_state.entry_index = 0;

    /* İlk sektörü oku */
    uint32_t sector = cluster_to_sector(dir_state.cluster);
    if(sd_read_block(sector, dir_state.sector_buffer) != SD_OK) {
        dir_state.open = 0;
        return FAT_ERROR;
    }

    return FAT_OK;
}

/* Dizin girişi oku */
int fat32_read_dir(FileInfo *info) {
    if(!dir_state.open) return FAT_ERROR;

    while(1) {
        /* Cluster içindeki sektör ve entry hesapla */
        uint32_t entries_per_sector = 512 / sizeof(FAT32DirEntry);
        uint32_t entries_per_cluster = entries_per_sector * fat32.sectors_per_cluster;

        /* Cluster bitti mi? */
        if(dir_state.entry_index >= entries_per_cluster) {
            /* Sonraki cluster'a geç */
            uint32_t next = get_next_cluster(dir_state.cluster);
            if(next >= 0x0FFFFFF8) {
                return FAT_EOF;
            }
            dir_state.cluster = next;
            dir_state.entry_index = 0;

            /* Yeni cluster'ın ilk sektörünü oku */
            uint32_t sector = cluster_to_sector(dir_state.cluster);
            if(sd_read_block(sector, dir_state.sector_buffer) != SD_OK) {
                return FAT_ERROR;
            }
        }

        /* Sektör içindeki pozisyon */
        uint32_t sector_index = dir_state.entry_index % entries_per_sector;

        /* Yeni sektör gerekiyor mu? */
        if(sector_index == 0 && dir_state.entry_index > 0) {
            uint32_t sector_offset = dir_state.entry_index / entries_per_sector;
            uint32_t sector = cluster_to_sector(dir_state.cluster) + sector_offset;
            if(sd_read_block(sector, dir_state.sector_buffer) != SD_OK) {
                return FAT_ERROR;
            }
        }

        /* Entry'yi al */
        FAT32DirEntry *entry = (FAT32DirEntry*)(dir_state.sector_buffer + sector_index * sizeof(FAT32DirEntry));

        dir_state.entry_index++;

        /* Dizin sonu */
        if(entry->name[0] == 0x00) {
            return FAT_EOF;
        }

        /* Silinen dosya */
        if((uint8_t)entry->name[0] == 0xE5) {
            continue;
        }

        /* Long filename entry (şimdilik atla) */
        if(entry->attr == ATTR_LONG_NAME) {
            continue;
        }

        /* Volume label (atla) */
        if(entry->attr & ATTR_VOLUME_ID) {
            continue;
        }

        /* Dosya bilgisini doldur */
        convert_83_to_name(entry->name, info->name);
        info->size = entry->size;
        info->cluster = (entry->cluster_high << 16) | entry->cluster_low;
        info->attr = entry->attr;
        info->is_dir = (entry->attr & ATTR_DIRECTORY) ? 1 : 0;
        info->date = entry->modify_date;
        info->time = entry->modify_time;

        return FAT_OK;
    }
}

/* Dizin kapat */
int fat32_close_dir(void) {
    dir_state.open = 0;
    return FAT_OK;
}

/* Dosya aç */
int fat32_open(const char *path, uint8_t mode) {
    if(!fat32.mounted) return -1;

    /* Boş slot bul */
    int fd = -1;
    for(int i = 0; i < MAX_OPEN_FILES; i++) {
        if(!open_files[i].used) {
            fd = i;
            break;
        }
    }
    if(fd < 0) return -1;

    /* Dosyayı bul (basitleştirilmiş - sadece root) */
    if(fat32_open_dir("/") != FAT_OK) return -1;

    FileInfo info;
    int found = 0;

    /* Dosya adını path'ten çıkar */
    const char *filename = path;
    if(filename[0] == '/') filename++;

    while(fat32_read_dir(&info) == FAT_OK) {
        /* İsim karşılaştır (case-insensitive) */
        int match = 1;
        int i = 0;
        while(filename[i] && info.name[i]) {
            if(!char_equal_ci(filename[i], info.name[i])) {
                match = 0;
                break;
            }
            i++;
        }
        if(match && !filename[i] && !info.name[i]) {
            found = 1;
            break;
        }
    }

    fat32_close_dir();

    if(!found) return -1;

    /* Dosyayı aç */
    open_files[fd].used = 1;
    open_files[fd].start_cluster = info.cluster;
    open_files[fd].cluster = info.cluster;
    open_files[fd].position = 0;
    open_files[fd].size = info.size;
    open_files[fd].mode = mode;

    return fd;
}

/* Dosyadan oku */
int fat32_read(int fd, void *buffer, uint32_t size) {
    if(fd < 0 || fd >= MAX_OPEN_FILES) return -1;
    if(!open_files[fd].used) return -1;

    File *f = &open_files[fd];
    uint8_t *buf = (uint8_t*)buffer;
    uint32_t bytes_read = 0;
    uint8_t sector_buffer[512];

    while(bytes_read < size && f->position < f->size) {
        /* Cluster içindeki pozisyon */
        uint32_t cluster_offset = f->position % fat32.cluster_size;
        uint32_t sector_offset = cluster_offset / 512;
        uint32_t byte_offset = cluster_offset % 512;

        /* Sektörü oku */
        uint32_t sector = cluster_to_sector(f->cluster) + sector_offset;
        if(sd_read_block(sector, sector_buffer) != SD_OK) {
            return bytes_read > 0 ? bytes_read : -1;
        }

        /* Bu sektörden ne kadar okuyabiliriz? */
        uint32_t remaining_in_sector = 512 - byte_offset;
        uint32_t remaining_in_file = f->size - f->position;
        uint32_t remaining_to_read = size - bytes_read;
        uint32_t to_copy = remaining_in_sector;
        if(to_copy > remaining_in_file) to_copy = remaining_in_file;
        if(to_copy > remaining_to_read) to_copy = remaining_to_read;

        /* Kopyala */
        for(uint32_t i = 0; i < to_copy; i++) {
            buf[bytes_read++] = sector_buffer[byte_offset + i];
        }
        f->position += to_copy;

        /* Cluster sonu? */
        if(f->position % fat32.cluster_size == 0 && f->position < f->size) {
            uint32_t next = get_next_cluster(f->cluster);
            if(next >= 0x0FFFFFF8) break;
            f->cluster = next;
        }
    }

    return bytes_read;
}

/* Dosya kapat */
int fat32_close(int fd) {
    if(fd < 0 || fd >= MAX_OPEN_FILES) return -1;
    open_files[fd].used = 0;
    return FAT_OK;
}

/* Dosya boyutu */
uint32_t fat32_size(int fd) {
    if(fd < 0 || fd >= MAX_OPEN_FILES) return 0;
    if(!open_files[fd].used) return 0;
    return open_files[fd].size;
}

/* Dosya pozisyonu */
uint32_t fat32_tell(int fd) {
    if(fd < 0 || fd >= MAX_OPEN_FILES) return 0;
    if(!open_files[fd].used) return 0;
    return open_files[fd].position;
}

/* Dosya ara */
int fat32_seek(int fd, uint32_t position) {
    if(fd < 0 || fd >= MAX_OPEN_FILES) return -1;
    if(!open_files[fd].used) return -1;

    File *f = &open_files[fd];
    if(position > f->size) position = f->size;

    /* Başa dön ve cluster'ı hesapla */
    f->position = 0;
    f->cluster = f->start_cluster;

    while(f->position + fat32.cluster_size <= position) {
        f->position += fat32.cluster_size;
        uint32_t next = get_next_cluster(f->cluster);
        if(next >= 0x0FFFFFF8) break;
        f->cluster = next;
    }

    f->position = position;
    return FAT_OK;
}

/* Boyut formatlama */
void fat32_format_size(uint32_t size, char *buffer) {
    if(size < 1024) {
        /* Byte */
        int i = 0;
        if(size == 0) {
            buffer[i++] = '0';
        } else {
            char tmp[16];
            int j = 0;
            while(size > 0) {
                tmp[j++] = '0' + (size % 10);
                size /= 10;
            }
            while(j > 0) {
                buffer[i++] = tmp[--j];
            }
        }
        buffer[i++] = ' ';
        buffer[i++] = 'B';
        buffer[i] = 0;
    } else if(size < 1024 * 1024) {
        /* KB */
        uint32_t kb = size / 1024;
        int i = 0;
        char tmp[16];
        int j = 0;
        while(kb > 0) {
            tmp[j++] = '0' + (kb % 10);
            kb /= 10;
        }
        while(j > 0) {
            buffer[i++] = tmp[--j];
        }
        buffer[i++] = ' ';
        buffer[i++] = 'K';
        buffer[i++] = 'B';
        buffer[i] = 0;
    } else {
        /* MB */
        uint32_t mb = size / (1024 * 1024);
        int i = 0;
        char tmp[16];
        int j = 0;
        while(mb > 0) {
            tmp[j++] = '0' + (mb % 10);
            mb /= 10;
        }
        while(j > 0) {
            buffer[i++] = tmp[--j];
        }
        buffer[i++] = ' ';
        buffer[i++] = 'M';
        buffer[i++] = 'B';
        buffer[i] = 0;
    }
}

/* Write fonksiyonu (placeholder) */
int fat32_write(int fd, const void *buffer, uint32_t size) {
    /* Yazma şimdilik desteklenmiyor */
    (void)fd; (void)buffer; (void)size;
    return -1;
}

/* Dosya var mı? */
int fat32_exists(const char *path) {
    FileInfo info;
    return fat32_get_info(path, &info) == FAT_OK;
}

/* Dizin mi? */
int fat32_is_directory(const char *path) {
    FileInfo info;
    if(fat32_get_info(path, &info) != FAT_OK) return 0;
    return info.is_dir;
}

/* Dosya bilgisi al */
int fat32_get_info(const char *path, FileInfo *info) {
    if(fat32_open_dir("/") != FAT_OK) return FAT_ERROR;

    const char *filename = path;
    if(filename[0] == '/') filename++;

    while(fat32_read_dir(info) == FAT_OK) {
        int match = 1;
        int i = 0;
        while(filename[i] && info->name[i]) {
            if(!char_equal_ci(filename[i], info->name[i])) {
                match = 0;
                break;
            }
            i++;
        }
        if(match && !filename[i] && !info->name[i]) {
            fat32_close_dir();
            return FAT_OK;
        }
    }

    fat32_close_dir();
    return FAT_NOT_FOUND;
}
