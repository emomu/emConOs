/* filemgr.c - File Manager UI */
#include "filemgr.h"
#include "../fs/fat32.h"
#include "../graphics.h"
#include "../fonts/fonts.h"
#include "../types.h"

/* Maksimum değerler */
#define MAX_VISIBLE_FILES   12
#define MAX_FILES           256

/* Dosya listesi */
static struct {
    FileInfo files[MAX_FILES];
    int count;
    int selected;
    int scroll_offset;
    FileMgrState state;
    char current_path[MAX_PATH];
} fm;

/* Dosya ikonları için renkler */
#define COLOR_FOLDER    0xFF4A90D9  /* Mavi */
#define COLOR_FILE      0xFFCCCCCC  /* Gri */
#define COLOR_IMAGE     0xFF9B59B6  /* Mor */
#define COLOR_AUDIO     0xFF27AE60  /* Yeşil */
#define COLOR_VIDEO     0xFFE74C3C  /* Kırmızı */
#define COLOR_TEXT      0xFFF39C12  /* Turuncu */

/* String karşılaştır (case-insensitive, sondan) */
static int ends_with(const char *str, const char *suffix) {
    int str_len = 0, suf_len = 0;
    while(str[str_len]) str_len++;
    while(suffix[suf_len]) suf_len++;

    if(suf_len > str_len) return 0;

    for(int i = 0; i < suf_len; i++) {
        char a = str[str_len - suf_len + i];
        char b = suffix[i];
        if(a >= 'A' && a <= 'Z') a += 32;
        if(b >= 'A' && b <= 'Z') b += 32;
        if(a != b) return 0;
    }
    return 1;
}

/* Dosya tipi rengini al */
static uint32_t get_file_color(const FileInfo *info) {
    if(info->is_dir) return COLOR_FOLDER;

    /* Uzantıya göre renk */
    if(ends_with(info->name, ".png") || ends_with(info->name, ".jpg") ||
       ends_with(info->name, ".bmp") || ends_with(info->name, ".gif")) {
        return COLOR_IMAGE;
    }
    if(ends_with(info->name, ".mp3") || ends_with(info->name, ".wav") ||
       ends_with(info->name, ".ogg") || ends_with(info->name, ".flac")) {
        return COLOR_AUDIO;
    }
    if(ends_with(info->name, ".mp4") || ends_with(info->name, ".avi") ||
       ends_with(info->name, ".mkv") || ends_with(info->name, ".mov")) {
        return COLOR_VIDEO;
    }
    if(ends_with(info->name, ".txt") || ends_with(info->name, ".cfg") ||
       ends_with(info->name, ".ini") || ends_with(info->name, ".log")) {
        return COLOR_TEXT;
    }

    return COLOR_FILE;
}

/* Basit klasör ikonu çiz */
static void draw_folder_icon(int x, int y, uint32_t color) {
    /* Klasör şekli */
    draw_rect(x, y + 4, 18, 14, color);
    draw_rect(x, y + 2, 8, 4, color);
}

/* Basit dosya ikonu çiz */
static void draw_file_icon(int x, int y, uint32_t color) {
    /* Dosya şekli */
    draw_rect(x + 2, y + 2, 14, 18, color);
    /* Köşe katlanması */
    draw_rect(x + 12, y + 2, 4, 4, COLOR_DARK_BLUE);
}

/* FileInfo kopyala (memcpy yerine) */
static void copy_fileinfo(FileInfo *dest, const FileInfo *src) {
    int i;
    for(i = 0; i < MAX_FILENAME && src->name[i]; i++) {
        dest->name[i] = src->name[i];
    }
    dest->name[i] = 0;
    dest->size = src->size;
    dest->cluster = src->cluster;
    dest->attr = src->attr;
    dest->is_dir = src->is_dir;
    dest->date = src->date;
    dest->time = src->time;
}

/* String kopyalama yardımcısı */
static void str_copy_simple(char *dest, const char *src) {
    while(*src) *dest++ = *src++;
    *dest = 0;
}

/* Demo dosyaları ekle (QEMU test için) */
static void add_demo_files(void) {
    /* Klasörler */
    str_copy_simple(fm.files[0].name, "Games");
    fm.files[0].is_dir = 1; fm.files[0].size = 0; fm.count++;

    str_copy_simple(fm.files[1].name, "Music");
    fm.files[1].is_dir = 1; fm.files[1].size = 0; fm.count++;

    str_copy_simple(fm.files[2].name, "Videos");
    fm.files[2].is_dir = 1; fm.files[2].size = 0; fm.count++;

    str_copy_simple(fm.files[3].name, "Documents");
    fm.files[3].is_dir = 1; fm.files[3].size = 0; fm.count++;

    /* Dosyalar */
    str_copy_simple(fm.files[4].name, "readme.txt");
    fm.files[4].is_dir = 0; fm.files[4].size = 1024; fm.count++;

    str_copy_simple(fm.files[5].name, "config.ini");
    fm.files[5].is_dir = 0; fm.files[5].size = 256; fm.count++;

    str_copy_simple(fm.files[6].name, "photo.png");
    fm.files[6].is_dir = 0; fm.files[6].size = 524288; fm.count++;

    str_copy_simple(fm.files[7].name, "song.mp3");
    fm.files[7].is_dir = 0; fm.files[7].size = 4194304; fm.count++;

    str_copy_simple(fm.files[8].name, "video.mp4");
    fm.files[8].is_dir = 0; fm.files[8].size = 104857600; fm.count++;

    str_copy_simple(fm.files[9].name, "game.nes");
    fm.files[9].is_dir = 0; fm.files[9].size = 262144; fm.count++;

    str_copy_simple(fm.files[10].name, "backup.zip");
    fm.files[10].is_dir = 0; fm.files[10].size = 15728640; fm.count++;
}

/* Dosya yöneticisi başlat */
void filemgr_init(void) {
    fm.count = 0;
    fm.selected = 0;
    fm.scroll_offset = 0;
    fm.state = FM_STATE_LOADING;
    fm.current_path[0] = '/';
    fm.current_path[1] = 0;

    /* FAT32 başlat */
    if(fat32_init() != FAT_OK) {
        /* SD kart yok - demo dosyaları göster (QEMU test için) */
        add_demo_files();
        fm.state = FM_STATE_READY;
        return;
    }

    /* Dizini oku */
    if(fat32_open_dir("/") != FAT_OK) {
        /* Dizin açılamadı - demo dosyaları göster */
        add_demo_files();
        fm.state = FM_STATE_READY;
        return;
    }

    FileInfo info;
    while(fat32_read_dir(&info) == FAT_OK && fm.count < MAX_FILES) {
        /* Dosyayı listeye ekle */
        copy_fileinfo(&fm.files[fm.count], &info);
        fm.count++;
    }

    fat32_close_dir();

    /* Hiç dosya yoksa demo ekle */
    if(fm.count == 0) {
        add_demo_files();
    }

    fm.state = FM_STATE_READY;
}

/* Dosya yöneticisi çiz */
void filemgr_draw(void) {
    /* Arka plan */
    clear_screen(COLOR_DARK_BLUE);

    /* Başlık çubuğu */
    draw_rect(0, 0, SCREEN_WIDTH, 45, COLOR_BLUE);
    draw_text_20_bold(15, 10, "Dosya Yoneticisi", COLOR_WHITE);

    /* Mevcut yol */
    draw_text_16(SCREEN_WIDTH - 100, 14, fm.current_path, COLOR_LIGHT_GRAY);

    /* Hata durumu */
    if(fm.state == FM_STATE_ERROR) {
        draw_text_20(50, 200, "SD kart okunamadi!", COLOR_RED);
        draw_text_16(50, 240, "Lutfen SD karti kontrol edin.", COLOR_WHITE);
        return;
    }

    /* Yükleniyor durumu */
    if(fm.state == FM_STATE_LOADING) {
        draw_text_20(50, 200, "Yukleniyor...", COLOR_WHITE);
        return;
    }

    /* Boş dizin */
    if(fm.count == 0) {
        draw_text_20(50, 200, "Dizin bos", COLOR_LIGHT_GRAY);
        return;
    }

    /* Dosya listesi alanı */
    int list_x = 20;
    int list_y = 55;
    int item_height = 32;

    /* Görünür dosyaları çiz */
    for(int i = 0; i < MAX_VISIBLE_FILES && (fm.scroll_offset + i) < fm.count; i++) {
        int idx = fm.scroll_offset + i;
        FileInfo *file = &fm.files[idx];
        int y = list_y + i * item_height;

        /* Seçili satır arka planı */
        if(idx == fm.selected) {
            draw_rect(list_x - 5, y - 2, SCREEN_WIDTH - 40, item_height, 0xFF1A3A5C);
            draw_rect_outline(list_x - 5, y - 2, SCREEN_WIDTH - 40, item_height, 2, COLOR_WHITE);
        }

        /* İkon */
        uint32_t icon_color = get_file_color(file);
        if(file->is_dir) {
            draw_folder_icon(list_x, y + 2, icon_color);
        } else {
            draw_file_icon(list_x, y, icon_color);
        }

        /* Dosya adı */
        uint32_t text_color = (idx == fm.selected) ? COLOR_WHITE : COLOR_LIGHT_GRAY;
        draw_text_16(list_x + 28, y + 6, file->name, text_color);

        /* Boyut (dosyalar için) */
        if(!file->is_dir) {
            char size_str[16];
            fat32_format_size(file->size, size_str);
            draw_text_16(SCREEN_WIDTH - 100, y + 6, size_str, COLOR_LIGHT_GRAY);
        } else {
            draw_text_16(SCREEN_WIDTH - 100, y + 6, "<DIR>", COLOR_LIGHT_GRAY);
        }
    }

    /* Scroll indicator */
    if(fm.count > MAX_VISIBLE_FILES) {
        int bar_height = 300;
        int bar_y = 60;
        int thumb_height = (MAX_VISIBLE_FILES * bar_height) / fm.count;
        if(thumb_height < 20) thumb_height = 20;
        int thumb_y = bar_y + (fm.scroll_offset * (bar_height - thumb_height)) / (fm.count - MAX_VISIBLE_FILES);

        /* Scroll track */
        draw_rect(SCREEN_WIDTH - 15, bar_y, 8, bar_height, 0xFF1A3A5C);
        /* Scroll thumb */
        draw_rect(SCREEN_WIDTH - 15, thumb_y, 8, thumb_height, COLOR_LIGHT_GRAY);
    }

    /* Alt bilgi çubuğu */
    draw_rect(0, SCREEN_HEIGHT - 45, SCREEN_WIDTH, 45, COLOR_BLUE);

    /* Dosya sayısı */
    char count_str[32] = "Toplam: ";
    int num = fm.count;
    char tmp[16];
    int j = 0;
    if(num == 0) {
        tmp[j++] = '0';
    } else {
        while(num > 0) {
            tmp[j++] = '0' + (num % 10);
            num /= 10;
        }
    }
    int len = 8; /* "Toplam: " uzunluğu */
    while(j > 0) {
        count_str[len++] = tmp[--j];
    }
    count_str[len++] = ' ';
    count_str[len++] = 'd';
    count_str[len++] = 'o';
    count_str[len++] = 's';
    count_str[len++] = 'y';
    count_str[len++] = 'a';
    count_str[len] = 0;

    draw_text_16(15, SCREEN_HEIGHT - 30, count_str, COLOR_WHITE);

    /* Kontroller */
    draw_text_16(SCREEN_WIDTH - 200, SCREEN_HEIGHT - 30, "A:Ac  B:Geri", COLOR_WHITE);
}

/* Yukarı git */
void filemgr_up(void) {
    if(fm.selected > 0) {
        fm.selected--;
        if(fm.selected < fm.scroll_offset) {
            fm.scroll_offset = fm.selected;
        }
    }
}

/* Aşağı git */
void filemgr_down(void) {
    if(fm.selected < fm.count - 1) {
        fm.selected++;
        if(fm.selected >= fm.scroll_offset + MAX_VISIBLE_FILES) {
            fm.scroll_offset = fm.selected - MAX_VISIBLE_FILES + 1;
        }
    }
}

/* Seçili öğeyi aç */
void filemgr_enter(void) {
    if(fm.count == 0) return;

    FileInfo *file = &fm.files[fm.selected];

    if(file->is_dir) {
        /* Dizine gir (şimdilik sadece root destekleniyor) */
        /* TODO: Alt dizin desteği */
    } else {
        /* Dosyayı aç (şimdilik bir şey yapmıyor) */
        /* TODO: Dosya önizleme */
    }
}

/* Geri git */
void filemgr_back(void) {
    /* Üst dizine git (şimdilik sadece root) */
    /* TODO: Navigasyon geçmişi */
}

/* Durum al */
FileMgrState filemgr_get_state(void) {
    return fm.state;
}

/* Dosya sayısı al */
int filemgr_get_file_count(void) {
    return fm.count;
}

/* Seçili indeks al */
int filemgr_get_selected(void) {
    return fm.selected;
}
