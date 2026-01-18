/* fat32.c - FAT32 Filesystem Implementation */
#include <fs/fat32.h>
#include <drivers/sd.h>
#include <hw.h>

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
    uint32_t partition_start;     /* Partition başlangıç sektörü (MBR için) */
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
    /* LFN desteği */
    char     lfn_buffer[256];   /* Uzun dosya ismi */
    int      lfn_index;         /* LFN buffer'daki pozisyon */
    uint8_t  lfn_checksum;      /* LFN checksum doğrulama */
    int      lfn_valid;         /* LFN geçerli mi? */
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

/* Yardımcı: 8.3 ismi için checksum hesapla (LFN doğrulama) */
static uint8_t lfn_checksum(const uint8_t *name83) {
    uint8_t sum = 0;
    for(int i = 0; i < 11; i++) {
        sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + name83[i];
    }
    return sum;
}

/* Yardımcı: LFN entry'den tek karakter oku */
static char lfn_read_char(volatile uint8_t *entry, int idx) {
    /* LFN pozisyonları: 1,3,5,7,9 (5), 14,16,18,20,22,24 (6), 28,30 (2) = 13 */
    /* idx: 0-4 -> offset 1,3,5,7,9 */
    /* idx: 5-10 -> offset 14,16,18,20,22,24 */
    /* idx: 11-12 -> offset 28,30 */

    if(idx < 0 || idx > 12) return 0;

    int offset;
    if(idx < 5) {
        offset = 1 + idx * 2;
    } else if(idx < 11) {
        offset = 14 + (idx - 5) * 2;
    } else {
        offset = 28 + (idx - 11) * 2;
    }

    /* Byte-by-byte oku - alignment sorununu önle */
    uint8_t lo = entry[offset];
    uint8_t hi = entry[offset + 1];
    uint16_t ch = lo | ((uint16_t)hi << 8);

    if(ch == 0x0000 || ch == 0xFFFF) return 0;
    if(ch < 128) return (char)ch;

    /* Türkçe karakterler */
    if(ch == 0x00E7) return 'c';  /* ç */
    if(ch == 0x00C7) return 'C';  /* Ç */
    if(ch == 0x011F) return 'g';  /* ğ */
    if(ch == 0x011E) return 'G';  /* Ğ */
    if(ch == 0x0131) return 'i';  /* ı */
    if(ch == 0x0130) return 'I';  /* İ */
    if(ch == 0x00F6) return 'o';  /* ö */
    if(ch == 0x00D6) return 'O';  /* Ö */
    if(ch == 0x015F) return 's';  /* ş */
    if(ch == 0x015E) return 'S';  /* Ş */
    if(ch == 0x00FC) return 'u';  /* ü */
    if(ch == 0x00DC) return 'U';  /* Ü */

    return '_';
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

/* Statik buffer - stack overflow önleme */
static uint8_t boot_sector_buffer[512];
static uint8_t file_sector_buffer[512];  /* fat32_read için */

/* FAT32 başlat */
int fat32_init(void) {
    uart_puts("FAT32 baslatiliyor...\n");

    /* SD kart başlatılmış mı? */
    if(!sd_is_initialized()) {
        if(sd_init() != SD_OK) {
            uart_puts("FAT32: SD kart baslatilamadi\n");
            return FAT_ERROR;
        }
    }

    /* Sector 0 oku - MBR veya VBR olabilir */
    uart_puts("FAT32: Sector 0 okunuyor...\n");
    int read_result = sd_read_block(0, boot_sector_buffer);
    if(read_result != SD_OK) {
        uart_puts("FAT32: Sector 0 okunamadi\n");
        return FAT_ERROR;
    }

    volatile uint8_t *buf = boot_sector_buffer;
    uint32_t partition_start = 0;

    /* MBR mi VBR mi kontrol et */
    /* MBR signature: 0x55AA at offset 510 */
    /* MBR'da offset 0'da jump instruction (0xEB veya 0xE9) olmaz genelde */
    /* VBR'da (FAT boot sector) offset 0'da 0xEB xx 0x90 veya 0xE9 olur */

    uart_puts("FAT32: Sector0[0]: ");
    uart_hex(buf[0]);
    uart_puts(" [510-511]: ");
    uart_hex(buf[510]);
    uart_puts(" ");
    uart_hex(buf[511]);
    uart_puts("\n");

    /* MBR partition table kontrolü */
    /* Partition 1 entry: offset 446, type at offset 450 */
    uint8_t part1_type = buf[450];
    uint32_t part1_lba = buf[454] | ((uint32_t)buf[455] << 8) |
                         ((uint32_t)buf[456] << 16) | ((uint32_t)buf[457] << 24);

    uart_puts("FAT32: Part1 type: ");
    uart_hex(part1_type);
    uart_puts(" LBA: ");
    uart_hex(part1_lba);
    uart_puts("\n");

    /* FAT32 partition type: 0x0B veya 0x0C (LBA) */
    if((part1_type == 0x0B || part1_type == 0x0C) && part1_lba > 0) {
        uart_puts("FAT32: MBR bulundu, partition LBA: ");
        uart_hex(part1_lba);
        uart_puts("\n");
        partition_start = part1_lba;

        /* Partition'ın VBR'ını oku */
        read_result = sd_read_block(partition_start, boot_sector_buffer);
        if(read_result != SD_OK) {
            uart_puts("FAT32: Partition VBR okunamadi\n");
            return FAT_ERROR;
        }
        buf = boot_sector_buffer;
    } else {
        uart_puts("FAT32: Superfloppy format (MBR yok)\n");
        partition_start = 0;
    }

    fat32.partition_start = partition_start;

    /* Boot sector değerlerini oku */
    uint16_t bytes_per_sector = buf[11] | ((uint16_t)buf[12] << 8);
    uint8_t sectors_per_cluster = buf[13];
    uint16_t reserved_sectors = buf[14] | ((uint16_t)buf[15] << 8);
    uint8_t fat_count = buf[16];
    uint32_t fat_size_32 = buf[36] | ((uint32_t)buf[37] << 8) |
                           ((uint32_t)buf[38] << 16) | ((uint32_t)buf[39] << 24);
    uint32_t root_cluster = buf[44] | ((uint32_t)buf[45] << 8) |
                            ((uint32_t)buf[46] << 16) | ((uint32_t)buf[47] << 24);

    uart_puts("FAT32: bytes_per_sector: ");
    uart_hex(bytes_per_sector);
    uart_puts("\n");

    if(bytes_per_sector != 512) {
        uart_puts("FAT32: Desteklenmeyen sektor boyutu\n");
        return FAT_ERROR;
    }

    uart_puts("FAT32: sectors_per_cluster: ");
    uart_hex(sectors_per_cluster);
    uart_puts("\n");
    uart_puts("FAT32: root_cluster: ");
    uart_hex(root_cluster);
    uart_puts("\n");

    /* FAT32 bilgilerini kaydet */
    fat32.bytes_per_sector = bytes_per_sector;
    fat32.sectors_per_cluster = sectors_per_cluster;
    fat32.cluster_size = bytes_per_sector * sectors_per_cluster;
    fat32.fat_start = partition_start + reserved_sectors;
    fat32.fat_size = fat_size_32;
    fat32.data_start = fat32.fat_start + (fat_count * fat32.fat_size);
    fat32.root_cluster = root_cluster;
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

/* Yardımcı: Path'i parçala ve cluster bul */
static uint32_t find_cluster_by_path(const char *path) {
    if(!path || path[0] == 0 || (path[0] == '/' && path[1] == 0)) {
        return fat32.root_cluster;
    }

    uint32_t current_cluster = fat32.root_cluster;
    const char *p = path;
    if(*p == '/') p++;

    char component[MAX_FILENAME];

    while(*p) {
        /* Sonraki path component'i al */
        int i = 0;
        while(*p && *p != '/' && i < MAX_FILENAME - 1) {
            component[i++] = *p++;
        }
        component[i] = 0;
        if(*p == '/') p++;

        if(i == 0) continue;

        /* Bu component'i mevcut dizinde ara */
        dir_state.open = 1;
        dir_state.cluster = current_cluster;
        dir_state.entry_index = 0;

        uint32_t sector = cluster_to_sector(dir_state.cluster);
        if(sd_read_block(sector, dir_state.sector_buffer) != SD_OK) {
            return 0;
        }

        int found = 0;
        FileInfo info;
        while(fat32_read_dir(&info) == FAT_OK) {
            /* İsim karşılaştır */
            int match = 1;
            int j = 0;
            while(component[j] && info.name[j]) {
                if(!char_equal_ci(component[j], info.name[j])) {
                    match = 0;
                    break;
                }
                j++;
            }
            if(match && !component[j] && !info.name[j] && info.is_dir) {
                current_cluster = info.cluster;
                found = 1;
                break;
            }
        }
        dir_state.open = 0;

        if(!found) return 0;
    }

    return current_cluster;
}

/* Dizin aç */
int fat32_open_dir(const char *path) {
    if(!fat32.mounted) return FAT_ERROR;

    /* Path'e göre cluster bul */
    uint32_t cluster = find_cluster_by_path(path);
    if(cluster == 0) {
        return FAT_NOT_FOUND;
    }

    dir_state.open = 1;
    dir_state.cluster = cluster;
    dir_state.entry_index = 0;
    dir_state.lfn_valid = 0;
    dir_state.lfn_buffer[0] = 0;

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

    static int read_count = 0;
    read_count++;
    if(read_count < 5) {
        uart_puts("[FAT32] read_dir called #");
        uart_hex(read_count);
        uart_puts("\n");
    }

    while(1) {
        uart_puts("[FAT32] loop start\n");

        /* Cluster içindeki sektör ve entry hesapla */
        uint32_t entries_per_sector = 512 / sizeof(FAT32DirEntry);
        uint32_t entries_per_cluster = entries_per_sector * fat32.sectors_per_cluster;

        uart_puts("[FAT32] entries_per_sector: ");
        uart_hex(entries_per_sector);
        uart_puts(" entry_index: ");
        uart_hex(dir_state.entry_index);
        uart_puts("\n");

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
        uint32_t entry_offset = sector_index * sizeof(FAT32DirEntry);
        volatile uint8_t *entry_ptr = dir_state.sector_buffer + entry_offset;

        dir_state.entry_index++;

        /* İlk byte'ı güvenli şekilde oku */
        uint8_t first_byte = entry_ptr[0];

        uart_puts("[FAT32] first_byte: ");
        uart_hex(first_byte);
        uart_puts("\n");

        /* Dizin sonu */
        if(first_byte == 0x00) {
            uart_puts("[FAT32] EOF reached\n");
            return FAT_EOF;
        }

        /* Silinen dosya */
        if(first_byte == 0xE5) {
            uart_puts("[FAT32] deleted entry, skip\n");
            continue;
        }

        /* Attribute byte'ı oku (offset 11) */
        uint8_t attr = entry_ptr[11];
        uart_puts("[FAT32] attr: ");
        uart_hex(attr);
        uart_puts("\n");

        /* Long filename entry - topla */
        if(attr == ATTR_LONG_NAME) {
            uart_puts("[LFN] entry detected\n");
            uint8_t seq = entry_ptr[0];
            uint8_t chksum = entry_ptr[13];
            uart_puts("[LFN] seq=");
            uart_hex(seq);
            uart_puts(" chk=");
            uart_hex(chksum);
            uart_puts("\n");

            /* İlk LFN entry (sıra numarası 0x40 ile OR'lanmış) */
            if(seq & 0x40) {
                uart_puts("[LFN] first entry\n");
                /* Yeni LFN başlıyor */
                dir_state.lfn_index = 0;
                dir_state.lfn_checksum = chksum;
                dir_state.lfn_valid = 1;
                /* Buffer'ı temizle - sadece ilk byte yeterli */
                dir_state.lfn_buffer[0] = 0;
            }

            /* Checksum kontrol */
            if(chksum != dir_state.lfn_checksum) {
                dir_state.lfn_valid = 0;
            }

            if(dir_state.lfn_valid) {
                uart_puts("[LFN] extracting chars\n");
                /* LFN entry'ler ters sırada gelir */
                int seq_num = (seq & 0x1F) - 1;  /* 0-indexed sıra */
                int buf_offset = seq_num * 13;
                uart_puts("[LFN] seq_num=");
                uart_hex(seq_num);
                uart_puts(" buf_offset=");
                uart_hex(buf_offset);
                uart_puts("\n");

                /* Her LFN entry 13 karakter içerir */
                for(int i = 0; i < 13; i++) {
                    uart_puts("[LFN] reading char ");
                    uart_hex(i);
                    uart_puts("\n");
                    char c = lfn_read_char(entry_ptr, i);
                    uart_puts("[LFN] got: ");
                    uart_hex(c);
                    uart_puts("\n");
                    if(c == 0) break;
                    if(buf_offset + i < 255) {
                        dir_state.lfn_buffer[buf_offset + i] = c;
                    }
                }
                uart_puts("[LFN] chars done\n");
            }
            uart_puts("[LFN] continue\n");
            continue;
        }

        /* Volume label (atla) */
        if(attr & ATTR_VOLUME_ID) {
            dir_state.lfn_valid = 0;
            continue;
        }

        /* Normal dosya girişi */
        /* İsim (0-10) */
        char name83[12];
        for(int i = 0; i < 11; i++) {
            name83[i] = entry_ptr[i];
        }
        name83[11] = 0;

        /* LFN var mı ve checksum eşleşiyor mu? */
        if(dir_state.lfn_valid && dir_state.lfn_buffer[0] != 0) {
            uint8_t calc_chksum = lfn_checksum((const uint8_t*)name83);
            if(calc_chksum == dir_state.lfn_checksum) {
                /* LFN kullan */
                int i = 0;
                while(dir_state.lfn_buffer[i] && i < MAX_FILENAME - 1) {
                    info->name[i] = dir_state.lfn_buffer[i];
                    i++;
                }
                info->name[i] = 0;
            } else {
                /* Checksum eşleşmedi, 8.3 kullan */
                convert_83_to_name(name83, info->name);
            }
        } else {
            /* LFN yok, 8.3 kullan */
            convert_83_to_name(name83, info->name);
        }

        /* LFN state'i sıfırla */
        dir_state.lfn_valid = 0;
        dir_state.lfn_buffer[0] = 0;

        /* Size (offset 28-31, little endian) */
        info->size = entry_ptr[28] | ((uint32_t)entry_ptr[29] << 8) |
                     ((uint32_t)entry_ptr[30] << 16) | ((uint32_t)entry_ptr[31] << 24);

        /* Cluster (high: 20-21, low: 26-27) */
        uint16_t cluster_high = entry_ptr[20] | ((uint16_t)entry_ptr[21] << 8);
        uint16_t cluster_low = entry_ptr[26] | ((uint16_t)entry_ptr[27] << 8);
        info->cluster = ((uint32_t)cluster_high << 16) | cluster_low;

        info->attr = attr;
        info->is_dir = (attr & ATTR_DIRECTORY) ? 1 : 0;

        /* Date/time (offset 24-25: time, 22-23: date) */
        info->time = entry_ptr[22] | ((uint16_t)entry_ptr[23] << 8);
        info->date = entry_ptr[24] | ((uint16_t)entry_ptr[25] << 8);

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

    uart_puts("[FAT32] open: ");
    uart_puts((char*)path);
    uart_puts("\n");

    /* Boş slot bul */
    int fd = -1;
    for(int i = 0; i < MAX_OPEN_FILES; i++) {
        if(!open_files[i].used) {
            fd = i;
            break;
        }
    }
    if(fd < 0) return -1;

    /* Path'i parçala: dizin + dosya adı */
    const char *p = path;
    if(*p == '/') p++;

    /* Son / karakterini bul */
    const char *last_slash = 0;
    const char *tmp = p;
    while(*tmp) {
        if(*tmp == '/') last_slash = tmp;
        tmp++;
    }

    /* Dizin path'i ve dosya adı */
    char dir_path[128];
    const char *filename;

    if(last_slash) {
        /* Alt dizinde dosya var */
        int dir_len = last_slash - p;
        dir_path[0] = '/';
        for(int i = 0; i < dir_len && i < 126; i++) {
            dir_path[i + 1] = p[i];
        }
        dir_path[dir_len + 1] = 0;
        filename = last_slash + 1;
    } else {
        /* Root dizininde */
        dir_path[0] = '/';
        dir_path[1] = 0;
        filename = p;
    }

    uart_puts("[FAT32] dir: ");
    uart_puts(dir_path);
    uart_puts(" file: ");
    uart_puts((char*)filename);
    uart_puts("\n");

    /* Dizini aç */
    if(fat32_open_dir(dir_path) != FAT_OK) {
        uart_puts("[FAT32] open_dir failed\n");
        return -1;
    }

    FileInfo info;
    int found = 0;

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
        if(match && !filename[i] && !info.name[i] && !info.is_dir) {
            found = 1;
            uart_puts("[FAT32] Found file: ");
            uart_puts(info.name);
            uart_puts(" cluster=");
            uart_hex(info.cluster);
            uart_puts(" size=");
            uart_hex(info.size);
            uart_puts("\n");
            break;
        }
    }

    uart_puts("[FAT32] closing dir\n");
    fat32_close_dir();

    if(!found) {
        uart_puts("[FAT32] file not found\n");
        return -1;
    }

    uart_puts("[FAT32] setting up file handle\n");

    /* Dosyayı aç */
    open_files[fd].used = 1;
    open_files[fd].start_cluster = info.cluster;
    open_files[fd].cluster = info.cluster;
    open_files[fd].position = 0;
    open_files[fd].size = info.size;
    open_files[fd].mode = mode;

    uart_puts("[FAT32] open complete, fd=");
    uart_hex(fd);
    uart_puts("\n");

    return fd;
}

/* Dosyadan oku */
int fat32_read(int fd, void *buffer, uint32_t size) {
    uart_puts("[READ] fd=");
    uart_hex(fd);
    uart_puts(" size=");
    uart_hex(size);
    uart_puts("\n");

    if(fd < 0 || fd >= MAX_OPEN_FILES) return -1;
    if(!open_files[fd].used) return -1;

    File *f = &open_files[fd];
    uint8_t *buf = (uint8_t*)buffer;
    uint32_t bytes_read = 0;

    uart_puts("[READ] cluster=");
    uart_hex(f->cluster);
    uart_puts(" pos=");
    uart_hex(f->position);
    uart_puts("\n");

    while(bytes_read < size && f->position < f->size) {
        /* Cluster içindeki pozisyon */
        uint32_t cluster_offset = f->position % fat32.cluster_size;
        uint32_t sector_offset = cluster_offset / 512;
        uint32_t byte_offset = cluster_offset % 512;

        /* Sektörü oku - statik buffer kullan */
        uint32_t sector = cluster_to_sector(f->cluster) + sector_offset;
        uart_puts("[READ] sector=");
        uart_hex(sector);
        uart_puts("\n");

        if(sd_read_block(sector, file_sector_buffer) != SD_OK) {
            uart_puts("[READ] sd_read_block FAILED\n");
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
            buf[bytes_read++] = file_sector_buffer[byte_offset + i];
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
