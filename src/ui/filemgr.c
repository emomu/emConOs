/* src/ui/filemgr.c - Dosya Yöneticisi (Windows Explorer Tarzı) */
#include <ui/filemgr.h>
#include <ui/theme.h>
#include <fs/fat32.h>
#include <fs/png.h>
#include <graphics.h>
#include <fonts/fonts.h>
#include <types.h>
#include <hw.h>

/* --- AYARLAR --- */
#define ITEM_HEIGHT      50
#define HEADER_HEIGHT    60
#define FOOTER_HEIGHT    45
#define MAX_FM_FILES     16
#define MAX_FM_NAME      32
#define MAX_FM_PATH      128
#define MAX_VIEW_SIZE    4096   /* Görüntülenebilir max dosya boyutu */
#define VIEW_LINES       12     /* Ekranda görünen satır sayısı */

/* --- RENKLER --- */
#define COL_BG           0xFF1A1A1A
#define COL_HEADER       0xFF151515
#define COL_ITEM_BG      0xFF252525
#define COL_TEXT_PRI     0xFFFFFFFF
#define COL_TEXT_SEC     0xFF888888
#define COL_FOLDER       0xFFE6B800
#define COL_FILE         0xFFAAAAAA
#define COL_ERROR        0xFFFF4444

/* --- YAPILAR --- */
/* Dosya listesi - ayrı array'ler (struct alignment sorununu önle) */
static char fm_names[MAX_FM_FILES][MAX_FM_NAME];
static uint32_t fm_sizes[MAX_FM_FILES];
static uint32_t fm_clusters[MAX_FM_FILES];
static uint32_t fm_is_dirs[MAX_FM_FILES];  /* uint32_t alignment için */
static uint32_t fm_attrs[MAX_FM_FILES];    /* uint32_t alignment için */

/* Durum değişkenleri */
static int fm_count = 0;
static int fm_selected = 0;
static int fm_scroll = 0;
static int fm_target_scroll = 0;
static FileMgrState fm_state = FM_STATE_LOADING;
static int fm_sd_error = 0;
static char fm_path[MAX_FM_PATH];
static int fm_path_depth = 0;

/* Dosya görüntüleme değişkenleri */
static char view_buffer[MAX_VIEW_SIZE];
static int view_size = 0;
static int view_scroll = 0;
static int view_total_lines = 0;
static char view_filename[MAX_FM_NAME];

/* Resim görüntüleme değişkenleri */
#define MAX_IMG_WIDTH    640
#define MAX_IMG_HEIGHT   480
#define IMG_BUFFER_SIZE  (MAX_IMG_WIDTH * MAX_IMG_HEIGHT * 4)  /* BGRA */
static uint8_t img_buffer[IMG_BUFFER_SIZE];
static int img_width = 0;
static int img_height = 0;
static int img_loaded = 0;

/* BMP row buffer - stack overflow önleme */
static uint8_t bmp_row_buf[MAX_IMG_WIDTH * 4 + 4];

/* BMP header buffer - stack overflow önleme */
static uint8_t bmp_header[64];

/* --- YARDIMCI FONKSİYONLAR --- */

/* String kopyalama */
static void str_copy(char *dest, const char *src, int max_len) {
    int i = 0;
    while(src[i] && i < max_len - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = 0;
}

/* String uzunluğu */
static int str_len(const char *s) {
    int len = 0;
    while(s[len]) len++;
    return len;
}

/* String birleştirme */
static void str_append(char *dest, const char *src, int max_len) {
    int dest_len = str_len(dest);
    int i = 0;
    while(src[i] && dest_len + i < max_len - 1) {
        dest[dest_len + i] = src[i];
        i++;
    }
    dest[dest_len + i] = 0;
}

/* Son path component'i sil (geri git) */
static void path_go_up(char *path) {
    int len = str_len(path);
    if(len <= 1) return; /* Root'tayız */

    /* Sondaki / varsa sil */
    if(path[len - 1] == '/') {
        path[len - 1] = 0;
        len--;
    }

    /* Son / karakterini bul ve oradan kes */
    while(len > 0 && path[len - 1] != '/') {
        len--;
    }

    if(len == 0) {
        path[0] = '/';
        path[1] = 0;
    } else {
        path[len] = 0;
    }
}

/* İkon Çizimi */
static void draw_icon(int x, int y, int is_dir) {
    if(is_dir) {
        /* Klasör (Sarı) */
        draw_rect(x, y + 2, 24, 18, COL_FOLDER);
        draw_rect(x, y, 10, 4, COL_FOLDER);
    } else {
        /* Dosya (Gri) */
        draw_rect(x + 4, y, 16, 22, COL_FILE);
        draw_rect(x + 16, y, 4, 4, 0xFF555555);
    }
}

/* Statik FileInfo buffer - stack overflow önleme */
static FileInfo g_file_info;

/* Dosya uzantısını kontrol et (case-insensitive) */
static int has_extension(const char *name, const char *ext) {
    int name_len = str_len(name);
    int ext_len = str_len(ext);

    if(name_len < ext_len + 1) return 0;

    /* Sondaki karakterleri karşılaştır */
    for(int i = 0; i < ext_len; i++) {
        char c1 = name[name_len - ext_len + i];
        char c2 = ext[i];
        /* Küçük harfe çevir */
        if(c1 >= 'A' && c1 <= 'Z') c1 += 32;
        if(c2 >= 'A' && c2 <= 'Z') c2 += 32;
        if(c1 != c2) return 0;
    }
    return 1;
}

/* Txt dosyasını görüntüleme için aç */
static int open_text_file(const char *filename) {
    /* Full path oluştur */
    char full_path[MAX_FM_PATH];
    str_copy(full_path, fm_path, MAX_FM_PATH);
    if(full_path[str_len(full_path) - 1] != '/') {
        str_append(full_path, "/", MAX_FM_PATH);
    }
    str_append(full_path, filename, MAX_FM_PATH);

    uart_puts("[VIEWER] Opening: ");
    uart_puts(full_path);
    uart_puts("\n");

    /* Dosyayı aç */
    int fd = fat32_open(full_path, 0);
    if(fd < 0) {
        uart_puts("[VIEWER] fat32_open failed, trying root...\n");
        /* Sadece dosya adıyla dene (root için) */
        fd = fat32_open(filename, 0);
        if(fd < 0) {
            uart_puts("[VIEWER] Failed to open file\n");
            return -1;
        }
    }

    /* Dosya boyutunu al */
    uint32_t size = fat32_size(fd);
    uart_puts("[VIEWER] File size: ");
    uart_hex(size);
    uart_puts("\n");

    if(size > MAX_VIEW_SIZE - 1) {
        size = MAX_VIEW_SIZE - 1;
    }

    /* Dosyayı oku */
    int bytes_read = fat32_read(fd, view_buffer, size);
    fat32_close(fd);

    if(bytes_read <= 0) {
        uart_puts("[VIEWER] Failed to read file\n");
        return -1;
    }

    view_buffer[bytes_read] = 0;
    view_size = bytes_read;
    view_scroll = 0;

    /* Satır sayısını hesapla */
    view_total_lines = 1;
    for(int i = 0; i < view_size; i++) {
        if(view_buffer[i] == '\n') {
            view_total_lines++;
        }
    }

    /* Dosya adını kaydet */
    str_copy(view_filename, filename, MAX_FM_NAME);

    uart_puts("[VIEWER] Loaded ");
    uart_hex(bytes_read);
    uart_puts(" bytes, ");
    uart_hex(view_total_lines);
    uart_puts(" lines\n");

    return 0;
}

/* BMP dosyasını yükle */
static int open_bmp_file(const char *filename) {
    /* Full path oluştur */
    char full_path[MAX_FM_PATH];
    str_copy(full_path, fm_path, MAX_FM_PATH);
    if(full_path[str_len(full_path) - 1] != '/') {
        str_append(full_path, "/", MAX_FM_PATH);
    }
    str_append(full_path, filename, MAX_FM_PATH);

    uart_puts("[BMP] Opening: ");
    uart_puts(full_path);
    uart_puts("\n");

    /* Dosyayı aç */
    int fd = fat32_open(full_path, 0);
    if(fd < 0) {
        fd = fat32_open(filename, 0);
        if(fd < 0) {
            uart_puts("[BMP] Failed to open file\n");
            return -1;
        }
    }

    uart_puts("[BMP] fat32_open OK, fd=");
    uart_hex(fd);
    uart_puts("\n");

    /* BMP header'ı oku (54 byte) - static bmp_header kullan */
    uart_puts("[BMP] Reading header...\n");
    int hdr_read = fat32_read(fd, bmp_header, 54);
    uart_puts("[BMP] header read returned: ");
    uart_hex(hdr_read);
    uart_puts("\n");
    if(hdr_read != 54) {
        uart_puts("[BMP] Failed to read header\n");
        fat32_close(fd);
        return -1;
    }

    uart_puts("[BMP] sig: ");
    uart_hex(bmp_header[0]);
    uart_puts(" ");
    uart_hex(bmp_header[1]);
    uart_puts("\n");

    /* BMP imzasını kontrol et */
    if(bmp_header[0] != 'B' || bmp_header[1] != 'M') {
        uart_puts("[BMP] Invalid signature\n");
        fat32_close(fd);
        return -1;
    }

    uart_puts("[BMP] sig OK\n");

    /* Header'dan değerleri oku - volatile ile optimize edilmesini engelle */
    volatile uint8_t *hdr = bmp_header;

    int width = hdr[18] | (hdr[19] << 8) | (hdr[20] << 16) | (hdr[21] << 24);
    int height = hdr[22] | (hdr[23] << 8) | (hdr[24] << 16) | (hdr[25] << 24);
    int bpp = hdr[28] | (hdr[29] << 8);
    int data_offset = hdr[10] | (hdr[11] << 8) | (hdr[12] << 16) | (hdr[13] << 24);

    uart_puts("[BMP] parsed from header\n");

    uart_puts("[BMP] W=");
    uart_hex(width);
    uart_puts(" H=");
    uart_hex(height);
    uart_puts(" BPP=");
    uart_hex(bpp);
    uart_puts(" off=");
    uart_hex(data_offset);
    uart_puts("\n");

    /* Negatif height = top-down */
    int top_down = 0;
    if(height < 0) {
        height = -height;
        top_down = 1;
    }

    /* Boyut kontrolü */
    if(width > MAX_IMG_WIDTH || height > MAX_IMG_HEIGHT) {
        uart_puts("[BMP] Image too large\n");
        fat32_close(fd);
        return -1;
    }

    /* Sadece 24-bit ve 32-bit destekleniyor */
    if(bpp != 24 && bpp != 32) {
        uart_puts("[BMP] Unsupported BPP\n");
        fat32_close(fd);
        return -1;
    }

    uart_puts("[BMP] bpp OK, seeking...\n");

    /* Data offset'e git */
    fat32_seek(fd, data_offset);

    /* Row padding (4 byte boundary) */
    int row_size = ((width * (bpp / 8) + 3) / 4) * 4;

    uart_puts("[BMP] row_size=");
    uart_hex(row_size);
    uart_puts(" reading rows...\n");

    /* Her satırı oku - statik bmp_row_buf kullan */
    for(int y = 0; y < height; y++) {
        int dest_y = top_down ? y : (height - 1 - y);

        int row_read = fat32_read(fd, bmp_row_buf, row_size);
        if(row_read != row_size) {
            uart_puts("[BMP] row ");
            uart_hex(y);
            uart_puts(" read failed, got ");
            uart_hex(row_read);
            uart_puts("\n");
            fat32_close(fd);
            return -1;
        }

        /* Pikselleri buffer'a kopyala */
        for(int x = 0; x < width; x++) {
            int src_idx = x * (bpp / 8);
            int dst_idx = (dest_y * MAX_IMG_WIDTH + x) * 4;

            /* BMP: BGR(A) formatında */
            img_buffer[dst_idx + 0] = bmp_row_buf[src_idx + 2];  /* R */
            img_buffer[dst_idx + 1] = bmp_row_buf[src_idx + 1];  /* G */
            img_buffer[dst_idx + 2] = bmp_row_buf[src_idx + 0];  /* B */
            img_buffer[dst_idx + 3] = (bpp == 32) ? bmp_row_buf[src_idx + 3] : 255;  /* A */
        }
    }

    fat32_close(fd);

    img_width = width;
    img_height = height;
    img_loaded = 1;
    str_copy(view_filename, filename, MAX_FM_NAME);

    uart_puts("[BMP] Loaded successfully\n");
    return 0;
}

/* GIF dosyasını yükle (basit, sadece ilk frame) */
static int open_gif_file(const char *filename) {
    /* Full path oluştur */
    char full_path[MAX_FM_PATH];
    str_copy(full_path, fm_path, MAX_FM_PATH);
    if(full_path[str_len(full_path) - 1] != '/') {
        str_append(full_path, "/", MAX_FM_PATH);
    }
    str_append(full_path, filename, MAX_FM_PATH);

    uart_puts("[GIF] Opening: ");
    uart_puts(full_path);
    uart_puts("\n");

    /* Dosyayı aç */
    int fd = fat32_open(full_path, 0);
    if(fd < 0) {
        fd = fat32_open(filename, 0);
        if(fd < 0) {
            uart_puts("[GIF] Failed to open file\n");
            return -1;
        }
    }

    /* GIF header'ı oku (13 byte) */
    uint8_t header[13];
    if(fat32_read(fd, header, 13) != 13) {
        uart_puts("[GIF] Failed to read header\n");
        fat32_close(fd);
        return -1;
    }

    /* GIF imzasını kontrol et */
    if(header[0] != 'G' || header[1] != 'I' || header[2] != 'F') {
        uart_puts("[GIF] Invalid signature\n");
        fat32_close(fd);
        return -1;
    }

    /* Boyutları oku (little endian) */
    int width = header[6] | (header[7] << 8);
    int height = header[8] | (header[9] << 8);
    uint8_t packed = header[10];
    int has_gct = (packed >> 7) & 1;
    int gct_size = 1 << ((packed & 7) + 1);
    uint8_t bg_color = header[11];

    uart_puts("[GIF] Width: ");
    uart_hex(width);
    uart_puts(" Height: ");
    uart_hex(height);
    uart_puts("\n");

    /* Boyut kontrolü */
    if(width > MAX_IMG_WIDTH || height > MAX_IMG_HEIGHT) {
        uart_puts("[GIF] Image too large\n");
        fat32_close(fd);
        return -1;
    }

    /* Global Color Table */
    uint8_t gct[256 * 3];
    if(has_gct) {
        if(fat32_read(fd, gct, gct_size * 3) != gct_size * 3) {
            uart_puts("[GIF] Failed to read GCT\n");
            fat32_close(fd);
            return -1;
        }
    }

    /* Arka planı temizle */
    for(int i = 0; i < width * height * 4; i += 4) {
        if(has_gct && bg_color < gct_size) {
            img_buffer[i + 0] = gct[bg_color * 3 + 0];
            img_buffer[i + 1] = gct[bg_color * 3 + 1];
            img_buffer[i + 2] = gct[bg_color * 3 + 2];
        } else {
            img_buffer[i + 0] = 0;
            img_buffer[i + 1] = 0;
            img_buffer[i + 2] = 0;
        }
        img_buffer[i + 3] = 255;
    }

    /* Extension ve image descriptor'ları atla, basit görüntü yükle */
    /* GIF LZW decoding karmaşık, şimdilik sadece header bilgisi göster */

    fat32_close(fd);

    img_width = width;
    img_height = height;
    img_loaded = 1;
    str_copy(view_filename, filename, MAX_FM_NAME);

    /* GIF için basit placeholder - gerçek decode çok karmaşık */
    /* Sadece boyut bilgisi gösterilecek */
    uart_puts("[GIF] Header loaded (no LZW decode)\n");
    return 0;
}

/* PNG dosyasını yükle (basit - sadece header ve boyut bilgisi) */
/* PNG dosya okuma buffer'ı - static */
#define PNG_FILE_MAX (300 * 1024)  /* 300KB max PNG dosya boyutu */
static uint8_t png_file_buf[PNG_FILE_MAX];

static int open_png_file(const char *filename) {
    char full_path[MAX_FM_PATH];
    str_copy(full_path, fm_path, MAX_FM_PATH);
    if(full_path[str_len(full_path) - 1] != '/') {
        str_append(full_path, "/", MAX_FM_PATH);
    }
    str_append(full_path, filename, MAX_FM_PATH);

    uart_puts("[PNG] Opening: ");
    uart_puts(full_path);
    uart_puts("\n");

    int fd = fat32_open(full_path, 0);
    if(fd < 0) {
        fd = fat32_open(filename, 0);
        if(fd < 0) {
            uart_puts("[PNG] Failed to open file\n");
            return -1;
        }
    }

    /* Dosya boyutunu al */
    uint32_t file_size = fat32_size(fd);
    uart_puts("[PNG] File size: ");
    uart_hex(file_size);
    uart_puts("\n");

    if(file_size > PNG_FILE_MAX) {
        uart_puts("[PNG] File too large\n");
        fat32_close(fd);
        return -1;
    }

    /* Tüm dosyayı oku */
    uint32_t total_read = 0;
    while(total_read < file_size) {
        int chunk = fat32_read(fd, png_file_buf + total_read, file_size - total_read);
        if(chunk <= 0) break;
        total_read += chunk;
    }
    fat32_close(fd);

    uart_puts("[PNG] Read ");
    uart_hex(total_read);
    uart_puts(" bytes\n");

    if(total_read < 8) {
        uart_puts("[PNG] File too small\n");
        return -1;
    }

    /* PNG decoder'ı çağır */
    int width, height;
    if(png_decode(png_file_buf, total_read, img_buffer, MAX_IMG_WIDTH, MAX_IMG_HEIGHT, &width, &height) < 0) {
        uart_puts("[PNG] Decode failed\n");
        return -1;
    }

    img_width = width;
    img_height = height;
    img_loaded = 1;
    str_copy(view_filename, filename, MAX_FM_NAME);

    uart_puts("[PNG] Loaded successfully\n");
    return 0;
}

/* JPEG dosyasını yükle (basit - sadece header ve boyut bilgisi) */
static int open_jpeg_file(const char *filename) {
    char full_path[MAX_FM_PATH];
    str_copy(full_path, fm_path, MAX_FM_PATH);
    if(full_path[str_len(full_path) - 1] != '/') {
        str_append(full_path, "/", MAX_FM_PATH);
    }
    str_append(full_path, filename, MAX_FM_PATH);

    uart_puts("[JPEG] Opening: ");
    uart_puts(full_path);
    uart_puts("\n");

    int fd = fat32_open(full_path, 0);
    if(fd < 0) {
        fd = fat32_open(filename, 0);
        if(fd < 0) {
            uart_puts("[JPEG] Failed to open file\n");
            return -1;
        }
    }

    /* JPEG SOI marker */
    uint8_t header[2];
    if(fat32_read(fd, header, 2) != 2) {
        uart_puts("[JPEG] Failed to read header\n");
        fat32_close(fd);
        return -1;
    }

    if(header[0] != 0xFF || header[1] != 0xD8) {
        uart_puts("[JPEG] Invalid SOI marker\n");
        fat32_close(fd);
        return -1;
    }

    /* SOF0 marker'ı ara (boyut bilgisi için) */
    int width = 0, height = 0;
    uint8_t marker[4];

    for(int i = 0; i < 100; i++) {  /* Max 100 segment ara */
        if(fat32_read(fd, marker, 2) != 2) break;

        if(marker[0] != 0xFF) continue;

        /* SOF0, SOF1, SOF2 marker'ları (baseline, extended, progressive) */
        if(marker[1] == 0xC0 || marker[1] == 0xC1 || marker[1] == 0xC2) {
            uint8_t sof[7];
            if(fat32_read(fd, sof, 7) != 7) break;

            /* Boyutlar (big endian) */
            height = (sof[3] << 8) | sof[4];
            width = (sof[5] << 8) | sof[6];

            uart_puts("[JPEG] Found SOF - Width: ");
            uart_hex(width);
            uart_puts(" Height: ");
            uart_hex(height);
            uart_puts("\n");
            break;
        }

        /* Diğer segment'leri atla */
        uint8_t len_buf[2];
        if(fat32_read(fd, len_buf, 2) != 2) break;
        int len = (len_buf[0] << 8) | len_buf[1];
        if(len > 2) {
            fat32_seek(fd, fat32_tell(fd) + len - 2);
        }
    }

    fat32_close(fd);

    if(width == 0 || height == 0) {
        uart_puts("[JPEG] Could not find image dimensions\n");
        return -1;
    }

    if(width > MAX_IMG_WIDTH || height > MAX_IMG_HEIGHT) {
        uart_puts("[JPEG] Image too large\n");
        return -1;
    }

    /* JPEG DCT decode çok karmaşık - placeholder göster */
    /* Mavi gradient placeholder */
    for(int y = 0; y < height; y++) {
        for(int x = 0; x < width; x++) {
            int idx = (y * MAX_IMG_WIDTH + x) * 4;
            img_buffer[idx + 0] = (x * 100) / width;        /* R */
            img_buffer[idx + 1] = (y * 100) / height;       /* G */
            img_buffer[idx + 2] = 200;                       /* B */
            img_buffer[idx + 3] = 255;
        }
    }

    img_width = width;
    img_height = height;
    img_loaded = 1;
    str_copy(view_filename, filename, MAX_FM_NAME);

    uart_puts("[JPEG] Header loaded (no DCT decode)\n");
    return 0;
}

/* Mevcut dizini yükle */
static void load_directory(void) {
    uart_puts("[FILEMGR] load_directory: ");
    uart_puts(fm_path);
    uart_puts("\n");

    fm_count = 0;
    fm_selected = 0;
    fm_scroll = 0;
    fm_target_scroll = 0;

    /* FAT32 dizini aç */
    uart_puts("[FILEMGR] fat32_open_dir cagiriliyor...\n");
    int result = fat32_open_dir(fm_path);
    if(result != FAT_OK) {
        uart_puts("[FILEMGR] Dizin acilamadi!\n");
        fm_state = FM_STATE_ERROR;
        return;
    }

    uart_puts("[FILEMGR] Dizin acildi, dosyalar okunuyor...\n");

    /* Dosyaları oku - statik buffer kullan */
    uart_puts("[FILEMGR] Starting read loop...\n");
    int read_result;
    while((read_result = fat32_read_dir(&g_file_info)) == FAT_OK && fm_count < MAX_FM_FILES) {
        uart_puts("[FILEMGR] Got file: ");
        uart_puts(g_file_info.name);
        uart_puts("\n");

        /* "." ve ".." ve macOS metadata dosyalarını atla */
        if(g_file_info.name[0] == '.') {
            continue;
        }
        /* macOS ._* dosyalarını atla (8.3 formatında _ ile başlar) */
        if(g_file_info.name[0] == '_') {
            continue;
        }

        uart_puts("[FILEMGR] Adding file to list at index ");
        char idx_str[4];
        idx_str[0] = '0' + fm_count;
        idx_str[1] = 0;
        uart_puts(idx_str);
        uart_puts("\n");

        /* Dosyayı listeye ekle - ayrı array'lere */
        str_copy(fm_names[fm_count], g_file_info.name, MAX_FM_NAME);
        fm_sizes[fm_count] = g_file_info.size;
        fm_is_dirs[fm_count] = g_file_info.is_dir;
        fm_clusters[fm_count] = g_file_info.cluster;
        fm_attrs[fm_count] = g_file_info.attr;

        uart_puts("[FILEMGR] File added successfully\n");
        fm_count++;
    }

    fat32_close_dir();
    fm_state = FM_STATE_READY;

    uart_puts("[FILEMGR] Toplam dosya: ");
    char num[8];
    num[0] = '0' + fm_count;
    num[1] = 0;
    uart_puts(num);
    uart_puts("\n");
}

/* --- INIT --- */

void filemgr_init(void) {
    uart_puts("[FILEMGR] Init basladi\n");

    fm_count = 0;
    fm_selected = 0;
    fm_scroll = 0;
    fm_target_scroll = 0;
    fm_state = FM_STATE_LOADING;
    fm_sd_error = 0;
    fm_path_depth = 0;

    /* Root path */
    fm_path[0] = '/';
    fm_path[1] = 0;

    uart_puts("[FILEMGR] FAT32 kontrol ediliyor...\n");

    /* FAT32 zaten başlatılmış mı kontrol et */
    if(!fat32_is_mounted()) {
        uart_puts("[FILEMGR] FAT32 mount degil, baslatiliyor...\n");
        if(fat32_init() != FAT_OK) {
            uart_puts("[FILEMGR] FAT32 HATA!\n");
            fm_sd_error = 1;
            fm_state = FM_STATE_ERROR;
            return;
        }
    }

    uart_puts("[FILEMGR] Dizin yukleniyor...\n");

    /* Root dizini yükle */
    load_directory();

    uart_puts("[FILEMGR] Init tamamlandi\n");
}

/* --- DRAW --- */

/* Dosya görüntüleme ekranı */
static void draw_viewer(void) {
    /* Arka plan */
    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COL_BG);

    /* Header */
    draw_rect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, COL_HEADER);
    draw_rect(0, HEADER_HEIGHT, SCREEN_WIDTH, 1, 0xFF444444);

    /* Dosya adı */
    draw_text_20_bold(20, 18, view_filename, COL_TEXT_PRI);

    /* İçerik alanı */
    int content_y = HEADER_HEIGHT + 10;
    int line_height = 20;
    int max_chars_per_line = (SCREEN_WIDTH - 40) / 8;  /* yaklaşık karakter genişliği */

    /* Satırları çiz */
    int current_line = 0;
    int line_start = 0;
    int y = content_y;

    for(int i = 0; i <= view_size; i++) {
        /* Satır sonu veya dosya sonu */
        if(view_buffer[i] == '\n' || view_buffer[i] == 0 || (i - line_start) >= max_chars_per_line) {
            /* Bu satır görünür mü? */
            if(current_line >= view_scroll && current_line < view_scroll + VIEW_LINES) {
                /* Satırı geçici buffer'a kopyala */
                char line_buf[80];
                int len = i - line_start;
                if(len > 79) len = 79;

                for(int j = 0; j < len; j++) {
                    char c = view_buffer[line_start + j];
                    /* Yazdırılamayan karakterleri boşluk yap */
                    if(c == '\r' || c == '\t') c = ' ';
                    if(c < 32 && c != 0) c = ' ';
                    line_buf[j] = c;
                }
                line_buf[len] = 0;

                draw_text_16(20, y, line_buf, COL_TEXT_PRI);
                y += line_height;
            }

            current_line++;
            line_start = i + 1;

            if(y > SCREEN_HEIGHT - FOOTER_HEIGHT - 10) break;
        }
    }

    /* Footer */
    int footer_y = SCREEN_HEIGHT - FOOTER_HEIGHT;
    draw_rect(0, footer_y, SCREEN_WIDTH, FOOTER_HEIGHT, COL_HEADER);

    /* Satır bilgisi */
    char info[32];
    int idx = 0;
    int line_num = view_scroll + 1;

    /* Satır numarası */
    if(line_num >= 100) info[idx++] = '0' + (line_num / 100) % 10;
    if(line_num >= 10) info[idx++] = '0' + (line_num / 10) % 10;
    info[idx++] = '0' + line_num % 10;
    info[idx++] = '/';

    if(view_total_lines >= 100) info[idx++] = '0' + (view_total_lines / 100) % 10;
    if(view_total_lines >= 10) info[idx++] = '0' + (view_total_lines / 10) % 10;
    info[idx++] = '0' + view_total_lines % 10;
    info[idx] = 0;

    draw_text_16(20, footer_y + 12, info, COL_TEXT_SEC);
    draw_text_16(SCREEN_WIDTH - 150, footer_y + 12, "[B] Kapat", COL_TEXT_SEC);

    __asm__ volatile("dsb sy");
}

/* Resim görüntüleme ekranı */
static void draw_image_viewer(void) {
    /* Arka plan */
    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0xFF000000);

    if(!img_loaded) {
        draw_text_20_bold(SCREEN_WIDTH/2 - 60, SCREEN_HEIGHT/2, "Yukleniyor...", COL_TEXT_PRI);
        return;
    }

    /* Resmi ortala */
    int offset_x = (SCREEN_WIDTH - img_width) / 2;
    int offset_y = (SCREEN_HEIGHT - img_height - FOOTER_HEIGHT) / 2;
    if(offset_x < 0) offset_x = 0;
    if(offset_y < 0) offset_y = 0;

    /* Resmi çiz */
    for(int y = 0; y < img_height && (y + offset_y) < SCREEN_HEIGHT - FOOTER_HEIGHT; y++) {
        for(int x = 0; x < img_width && (x + offset_x) < SCREEN_WIDTH; x++) {
            int idx = (y * MAX_IMG_WIDTH + x) * 4;
            uint8_t r = img_buffer[idx + 0];
            uint8_t g = img_buffer[idx + 1];
            uint8_t b = img_buffer[idx + 2];
            uint32_t color = 0xFF000000 | (r << 16) | (g << 8) | b;
            draw_pixel(x + offset_x, y + offset_y, color);
        }
    }

    /* Footer */
    int footer_y = SCREEN_HEIGHT - FOOTER_HEIGHT;
    draw_rect(0, footer_y, SCREEN_WIDTH, FOOTER_HEIGHT, COL_HEADER);

    /* Dosya adı ve boyut */
    draw_text_16(20, footer_y + 12, view_filename, COL_TEXT_PRI);

    /* Boyut bilgisi */
    char size_info[32];
    int idx = 0;

    /* Width */
    if(img_width >= 100) size_info[idx++] = '0' + (img_width / 100) % 10;
    if(img_width >= 10) size_info[idx++] = '0' + (img_width / 10) % 10;
    size_info[idx++] = '0' + img_width % 10;
    size_info[idx++] = 'x';

    /* Height */
    if(img_height >= 100) size_info[idx++] = '0' + (img_height / 100) % 10;
    if(img_height >= 10) size_info[idx++] = '0' + (img_height / 10) % 10;
    size_info[idx++] = '0' + img_height % 10;
    size_info[idx] = 0;

    draw_text_16(SCREEN_WIDTH/2 - 30, footer_y + 12, size_info, COL_TEXT_SEC);
    draw_text_16(SCREEN_WIDTH - 100, footer_y + 12, "[B] Kapat", COL_TEXT_SEC);

    __asm__ volatile("dsb sy");
}

void filemgr_draw(void) {
    /* Dosya görüntüleme modunda mıyız? */
    if(fm_state == FM_STATE_VIEWING) {
        draw_viewer();
        return;
    }

    /* Resim görüntüleme modunda mıyız? */
    if(fm_state == FM_STATE_IMAGE) {
        draw_image_viewer();
        return;
    }

    /* Arka plan */
    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COL_BG);

    /* --- HEADER --- */
    draw_rect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, COL_HEADER);
    draw_rect(0, HEADER_HEIGHT, SCREEN_WIDTH, 1, 0xFF444444);

    draw_text_20_bold(20, 18, "Dosya Yoneticisi", COL_TEXT_PRI);

    /* Path göster */
    int path_box_w = 200;
    draw_rect(SCREEN_WIDTH - path_box_w - 20, 15, path_box_w, 30, COL_ITEM_BG);
    draw_text_16(SCREEN_WIDTH - path_box_w - 10, 20, fm_path, COL_TEXT_SEC);

    /* --- HATA DURUMU --- */
    if(fm_state == FM_STATE_ERROR) {
        int center_y = SCREEN_HEIGHT / 2;

        if(fm_sd_error) {
            draw_text_20_bold(SCREEN_WIDTH/2 - 80, center_y - 30, "SD Kart Hatasi", COL_ERROR);
            draw_text_16(SCREEN_WIDTH/2 - 120, center_y + 10, "SD kart takildiginden emin olun", COL_TEXT_SEC);
        } else {
            draw_text_20_bold(SCREEN_WIDTH/2 - 60, center_y - 30, "Dizin Acilamadi", COL_ERROR);
            draw_text_16(SCREEN_WIDTH/2 - 80, center_y + 10, "Dizin okunamadi", COL_TEXT_SEC);
        }

        /* Footer */
        int footer_y = SCREEN_HEIGHT - FOOTER_HEIGHT;
        draw_rect(0, footer_y, SCREEN_WIDTH, FOOTER_HEIGHT, COL_HEADER);
        draw_text_16(20, footer_y + 12, "[B] Geri", COL_TEXT_SEC);
        return;
    }

    /* --- BOŞ DİZİN --- */
    if(fm_count == 0) {
        int center_y = SCREEN_HEIGHT / 2;
        draw_text_20_medium(SCREEN_WIDTH/2 - 50, center_y, "Bos Dizin", COL_TEXT_SEC);

        /* Footer */
        int footer_y = SCREEN_HEIGHT - FOOTER_HEIGHT;
        draw_rect(0, footer_y, SCREEN_WIDTH, FOOTER_HEIGHT, COL_HEADER);

        if(fm_path_depth > 0) {
            draw_text_16(20, footer_y + 12, "[B] Ust Dizin", COL_TEXT_SEC);
        } else {
            draw_text_16(20, footer_y + 12, "[B] Geri", COL_TEXT_SEC);
        }
        return;
    }

    /* --- LİSTE --- */
    int list_start_y = HEADER_HEIGHT + 10;
    int list_h = SCREEN_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT - 20;

    /* Layout */
    int side_padding = 15;
    int scrollbar_w = 8;
    int item_w = SCREEN_WIDTH - (side_padding * 2) - scrollbar_w - 10;

    for(int i = 0; i < fm_count; i++) {
        int y = (i * ITEM_HEIGHT) - fm_scroll + list_start_y;

        /* Görünür alan kontrolü */
        if(y < list_start_y - ITEM_HEIGHT || y > SCREEN_HEIGHT - FOOTER_HEIGHT) continue;

        int is_selected = (i == fm_selected);

        /* Arka plan */
        if(is_selected) {
            draw_rect(side_padding, y, item_w, ITEM_HEIGHT - 5, g_theme.accent);
            draw_rect_outline(side_padding, y, item_w, ITEM_HEIGHT - 5, 2, 0xFFFFFFFF);
        } else {
            draw_rect(side_padding, y, item_w, ITEM_HEIGHT - 5, COL_ITEM_BG);
        }

        /* İkon */
        draw_icon(side_padding + 10, y + 10, fm_is_dirs[i]);

        /* Dosya adı */
        uint32_t text_col = is_selected ? COL_TEXT_PRI : 0xFFCCCCCC;
        draw_text_16(side_padding + 45, y + 14, fm_names[i], text_col);

        /* Boyut veya <DIR> */
        char info_str[16];
        if(fm_is_dirs[i]) {
            str_copy(info_str, "<DIR>", 16);
        } else {
            /* Boyut formatla */
            int kb = fm_sizes[i] / 1024;
            if(kb == 0 && fm_sizes[i] > 0) kb = 1;

            if(kb < 1000) {
                info_str[0] = '0' + (kb / 100) % 10;
                info_str[1] = '0' + (kb / 10) % 10;
                info_str[2] = '0' + kb % 10;
                info_str[3] = 'K';
                info_str[4] = 'B';
                info_str[5] = 0;
            } else {
                int mb = kb / 1024;
                info_str[0] = '0' + (mb / 100) % 10;
                info_str[1] = '0' + (mb / 10) % 10;
                info_str[2] = '0' + mb % 10;
                info_str[3] = 'M';
                info_str[4] = 'B';
                info_str[5] = 0;
            }
        }

        uint32_t info_col = is_selected ? 0xFFDDDDDD : COL_TEXT_SEC;
        draw_text_16(side_padding + item_w - 70, y + 14, info_str, info_col);
    }

    /* --- SCROLLBAR --- */
    int visible_count = list_h / ITEM_HEIGHT;
    if(fm_count > visible_count) {
        int bar_h = list_h - 20;
        int bar_x = SCREEN_WIDTH - side_padding - scrollbar_w;
        int bar_y = list_start_y + 10;

        /* Track */
        draw_rect(bar_x, bar_y, scrollbar_w, bar_h, COL_ITEM_BG);

        /* Thumb boyutu */
        int thumb_h = (bar_h * visible_count) / fm_count;
        if(thumb_h < 30) thumb_h = 30;

        /* Thumb pozisyonu */
        int max_scroll = (fm_count * ITEM_HEIGHT) - list_h;
        if(max_scroll < 1) max_scroll = 1;
        int thumb_y = bar_y + ((bar_h - thumb_h) * fm_scroll) / max_scroll;
        draw_rect(bar_x, thumb_y, scrollbar_w, thumb_h, g_theme.accent);
    }

    /* --- FOOTER --- */
    int footer_y = SCREEN_HEIGHT - FOOTER_HEIGHT;
    draw_rect(0, footer_y, SCREEN_WIDTH, FOOTER_HEIGHT, COL_HEADER);

    /* Dosya sayısı */
    char count_str[32];
    int c = fm_count;
    int idx = 0;
    if(c == 0) {
        count_str[idx++] = '0';
    } else {
        char tmp[16];
        int ti = 0;
        while(c > 0) {
            tmp[ti++] = '0' + (c % 10);
            c /= 10;
        }
        while(ti > 0) count_str[idx++] = tmp[--ti];
    }
    count_str[idx++] = ' ';
    count_str[idx++] = 'o';
    count_str[idx++] = 'g';
    count_str[idx++] = 'e';
    count_str[idx] = 0;

    draw_text_16(20, footer_y + 12, count_str, COL_TEXT_SEC);

    /* Kontroller */
    if(fm_path_depth > 0) {
        draw_text_16(SCREEN_WIDTH - 220, footer_y + 12, "[A] Ac   [B] Ust Dizin", COL_TEXT_SEC);
    } else {
        draw_text_16(SCREEN_WIDTH - 180, footer_y + 12, "[A] Ac   [B] Geri", COL_TEXT_SEC);
    }

    /* Memory barrier */
    __asm__ volatile("dsb sy");
}

/* --- SCROLL GÜNCELLE --- */

static void update_scroll(void) {
    int list_h = SCREEN_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT - 20;

    int sel_top = fm_selected * ITEM_HEIGHT;
    int sel_bottom = sel_top + ITEM_HEIGHT;

    int view_top = fm_target_scroll;
    int view_bottom = view_top + list_h;

    /* Görünür alanda tut */
    if(sel_top < view_top) {
        fm_target_scroll = sel_top;
    } else if(sel_bottom > view_bottom) {
        fm_target_scroll = sel_bottom - list_h;
    }

    /* Sınırlar */
    int max_scroll = (fm_count * ITEM_HEIGHT) - list_h;
    if(max_scroll < 0) max_scroll = 0;

    if(fm_target_scroll < 0) fm_target_scroll = 0;
    if(fm_target_scroll > max_scroll) fm_target_scroll = max_scroll;

    fm_scroll = fm_target_scroll;
}

/* --- NAVİGASYON --- */

void filemgr_update(void) {
    /* Şimdilik boş */
}

void filemgr_up(void) {
    /* Dosya görüntüleme modunda scroll up */
    if(fm_state == FM_STATE_VIEWING) {
        if(view_scroll > 0) {
            view_scroll--;
        }
        return;
    }

    if(fm_state != FM_STATE_READY || fm_count == 0) return;

    if(fm_selected > 0) {
        fm_selected--;
        update_scroll();
    }
}

void filemgr_down(void) {
    /* Dosya görüntüleme modunda scroll down */
    if(fm_state == FM_STATE_VIEWING) {
        if(view_scroll < view_total_lines - VIEW_LINES) {
            view_scroll++;
        }
        return;
    }

    if(fm_state != FM_STATE_READY || fm_count == 0) return;

    if(fm_selected < fm_count - 1) {
        fm_selected++;
        update_scroll();
    }
}

void filemgr_enter(void) {
    if(fm_state != FM_STATE_READY || fm_count == 0) return;

    /* Klasörse içine gir */
    if(fm_is_dirs[fm_selected]) {
        /* Path'i güncelle */
        if(fm_path[str_len(fm_path) - 1] != '/') {
            str_append(fm_path, "/", MAX_FM_PATH);
        }
        str_append(fm_path, fm_names[fm_selected], MAX_FM_PATH);

        fm_path_depth++;

        /* Yeni dizini yükle */
        load_directory();
    } else {
        /* Dosyaysa - uzantıya göre aç */
        const char *name = fm_names[fm_selected];

        if(has_extension(name, ".txt") || has_extension(name, ".TXT")) {
            /* Text dosyası */
            uart_puts("[FILEMGR] Opening text file: ");
            uart_puts(name);
            uart_puts("\n");

            if(open_text_file(name) == 0) {
                fm_state = FM_STATE_VIEWING;
            }
        } else if(has_extension(name, ".bmp") || has_extension(name, ".BMP")) {
            /* BMP resim */
            uart_puts("[FILEMGR] Opening BMP file: ");
            uart_puts(name);
            uart_puts("\n");

            img_loaded = 0;
            if(open_bmp_file(name) == 0) {
                fm_state = FM_STATE_IMAGE;
            }
        } else if(has_extension(name, ".gif") || has_extension(name, ".GIF")) {
            /* GIF resim */
            uart_puts("[FILEMGR] Opening GIF file: ");
            uart_puts(name);
            uart_puts("\n");

            img_loaded = 0;
            if(open_gif_file(name) == 0) {
                fm_state = FM_STATE_IMAGE;
            }
        } else if(has_extension(name, ".png") || has_extension(name, ".PNG")) {
            /* PNG resim */
            uart_puts("[FILEMGR] Opening PNG file: ");
            uart_puts(name);
            uart_puts("\n");

            img_loaded = 0;
            if(open_png_file(name) == 0) {
                fm_state = FM_STATE_IMAGE;
            }
        } else if(has_extension(name, ".jpg") || has_extension(name, ".JPG") ||
                  has_extension(name, ".jpeg") || has_extension(name, ".JPEG")) {
            /* JPEG resim */
            uart_puts("[FILEMGR] Opening JPEG file: ");
            uart_puts(name);
            uart_puts("\n");

            img_loaded = 0;
            if(open_jpeg_file(name) == 0) {
                fm_state = FM_STATE_IMAGE;
            }
        }
    }
}

void filemgr_back(void) {
    /* Dosya görüntüleme modundaysak kapat */
    if(fm_state == FM_STATE_VIEWING) {
        fm_state = FM_STATE_READY;
        return;
    }

    /* Resim görüntüleme modundaysak kapat */
    if(fm_state == FM_STATE_IMAGE) {
        fm_state = FM_STATE_READY;
        img_loaded = 0;
        return;
    }

    /* Hata durumunda sadece çık işareti ver */
    if(fm_state == FM_STATE_ERROR && fm_path_depth == 0) {
        return; /* kernel.c'de ekran değişimi yapılacak */
    }

    /* Üst dizine git */
    if(fm_path_depth > 0) {
        path_go_up(fm_path);
        fm_path_depth--;
        load_directory();
    }
    /* path_depth == 0 ise ana menüye dön (kernel.c'de handle edilecek) */
}

/* --- GETTER'LAR --- */

FileMgrState filemgr_get_state(void) {
    return fm_state;
}

int filemgr_get_file_count(void) {
    return fm_count;
}

int filemgr_get_path_depth(void) {
    return fm_path_depth;
}

int filemgr_is_at_root(void) {
    /* Dosya/resim görüntüleme modundaysak root'ta değiliz (B ile kapat) */
    if(fm_state == FM_STATE_VIEWING || fm_state == FM_STATE_IMAGE) {
        return 0;
    }
    return (fm_path_depth == 0);
}
