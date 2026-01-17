/* filemgr.c - RetroArch-style File Manager UI */
#include <ui/filemgr.h>
#include <ui/theme.h>
#include <ui/animation.h>
#include <fs/fat32.h>
#include <graphics.h>
#include <fonts/fonts.h>
#include <types.h>

/* Maksimum değerler */
#define MAX_VISIBLE_FILES   10
#define MAX_FILES           256

/* Dosya listesi */
static struct {
    FileInfo files[MAX_FILES];
    int count;
    int selected;
    int scroll_offset;
    FileMgrState state;
    char current_path[MAX_PATH];

    /* Animasyonlar */
    Animation scroll_anim;
    Animation select_anim;
    float current_scroll;
    float target_scroll;
} fm;

/* Dosya tipi renkleri */
#define COLOR_FOLDER    0xFF4A90D9  /* Mavi */
#define COLOR_FILE      0xFFCCCCCC  /* Gri */
#define COLOR_IMAGE     0xFF9B59B6  /* Mor */
#define COLOR_AUDIO     0xFF27AE60  /* Yeşil */
#define COLOR_VIDEO     0xFFE74C3C  /* Kırmızı */
#define COLOR_TEXT      0xFFF39C12  /* Turuncu */
#define COLOR_ROM       0xFFE91E63  /* Pembe - ROM dosyaları */

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

    /* ROM dosyaları */
    if(ends_with(info->name, ".nes") || ends_with(info->name, ".smc") ||
       ends_with(info->name, ".sfc") || ends_with(info->name, ".gb") ||
       ends_with(info->name, ".gbc") || ends_with(info->name, ".gba") ||
       ends_with(info->name, ".md") || ends_with(info->name, ".bin")) {
        return COLOR_ROM;
    }

    /* Resim dosyaları */
    if(ends_with(info->name, ".png") || ends_with(info->name, ".jpg") ||
       ends_with(info->name, ".bmp") || ends_with(info->name, ".gif")) {
        return COLOR_IMAGE;
    }

    /* Ses dosyaları */
    if(ends_with(info->name, ".mp3") || ends_with(info->name, ".wav") ||
       ends_with(info->name, ".ogg") || ends_with(info->name, ".flac")) {
        return COLOR_AUDIO;
    }

    /* Video dosyaları */
    if(ends_with(info->name, ".mp4") || ends_with(info->name, ".avi") ||
       ends_with(info->name, ".mkv") || ends_with(info->name, ".mov")) {
        return COLOR_VIDEO;
    }

    /* Metin dosyaları */
    if(ends_with(info->name, ".txt") || ends_with(info->name, ".cfg") ||
       ends_with(info->name, ".ini") || ends_with(info->name, ".log")) {
        return COLOR_TEXT;
    }

    return COLOR_FILE;
}

/* Klasör ikonu çiz (RetroArch style) */
static void draw_folder_icon(int x, int y, uint32_t color) {
    /* Klasör şekli */
    draw_rect(x, y + 6, 22, 16, color);
    draw_rect(x, y + 3, 10, 5, color);

    /* Gölge */
    draw_rect(x + 2, y + 20, 20, 2, theme_color_darken(color, 0.3f));
}

/* Dosya ikonu çiz (RetroArch style) */
static void draw_file_icon(int x, int y, uint32_t color) {
    Theme *t = theme_get();

    /* Dosya şekli */
    draw_rect(x + 3, y + 2, 16, 20, color);

    /* Köşe katlanması */
    draw_rect(x + 15, y + 2, 4, 5, t->bg_dark);
    draw_rect(x + 15, y + 2, 4, 1, theme_color_lighten(color, 0.2f));
    draw_rect(x + 18, y + 2, 1, 5, theme_color_lighten(color, 0.2f));
}

/* FileInfo kopyala */
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

/* String kopyalama */
static void str_copy_simple(char *dest, const char *src) {
    while(*src) *dest++ = *src++;
    *dest = 0;
}

/* Demo dosyaları ekle (QEMU test için) */
static void add_demo_files(void) {
    /* Klasörler */
    str_copy_simple(fm.files[0].name, "NES");
    fm.files[0].is_dir = 1; fm.files[0].size = 0; fm.count++;

    str_copy_simple(fm.files[1].name, "SNES");
    fm.files[1].is_dir = 1; fm.files[1].size = 0; fm.count++;

    str_copy_simple(fm.files[2].name, "Mega Drive");
    fm.files[2].is_dir = 1; fm.files[2].size = 0; fm.count++;

    str_copy_simple(fm.files[3].name, "Game Boy");
    fm.files[3].is_dir = 1; fm.files[3].size = 0; fm.count++;

    str_copy_simple(fm.files[4].name, "GBA");
    fm.files[4].is_dir = 1; fm.files[4].size = 0; fm.count++;

    /* ROM dosyaları */
    str_copy_simple(fm.files[5].name, "mario.nes");
    fm.files[5].is_dir = 0; fm.files[5].size = 40976; fm.count++;

    str_copy_simple(fm.files[6].name, "zelda.smc");
    fm.files[6].is_dir = 0; fm.files[6].size = 1048576; fm.count++;

    str_copy_simple(fm.files[7].name, "sonic.md");
    fm.files[7].is_dir = 0; fm.files[7].size = 524288; fm.count++;

    str_copy_simple(fm.files[8].name, "pokemon.gb");
    fm.files[8].is_dir = 0; fm.files[8].size = 1048576; fm.count++;

    str_copy_simple(fm.files[9].name, "metroid.gba");
    fm.files[9].is_dir = 0; fm.files[9].size = 8388608; fm.count++;

    /* Diğer dosyalar */
    str_copy_simple(fm.files[10].name, "readme.txt");
    fm.files[10].is_dir = 0; fm.files[10].size = 1024; fm.count++;

    str_copy_simple(fm.files[11].name, "config.ini");
    fm.files[11].is_dir = 0; fm.files[11].size = 256; fm.count++;
}

/* Dosya yöneticisi başlat */
void filemgr_init(void) {
    fm.count = 0;
    fm.selected = 0;
    fm.scroll_offset = 0;
    fm.state = FM_STATE_LOADING;
    fm.current_path[0] = '/';
    fm.current_path[1] = 0;
    fm.current_scroll = 0;
    fm.target_scroll = 0;

    anim_init(&fm.scroll_anim);
    anim_init(&fm.select_anim);

    /* FAT32 başlat */
    if(fat32_init() != FAT_OK) {
        /* SD kart yok - demo dosyaları göster */
        add_demo_files();
        fm.state = FM_STATE_READY;
        return;
    }

    /* Dizini oku */
    if(fat32_open_dir("/") != FAT_OK) {
        add_demo_files();
        fm.state = FM_STATE_READY;
        return;
    }

    FileInfo info;
    while(fat32_read_dir(&info) == FAT_OK && fm.count < MAX_FILES) {
        copy_fileinfo(&fm.files[fm.count], &info);
        fm.count++;
    }

    fat32_close_dir();

    if(fm.count == 0) {
        add_demo_files();
    }

    fm.state = FM_STATE_READY;
}

/* Dosya yöneticisi çiz (RetroArch style) */
void filemgr_draw(void) {
    Theme *t = theme_get();

    /* Arka plan */
    clear_screen(t->bg_dark);

    /* Header */
    draw_rect(0, 0, SCREEN_WIDTH, 50, t->header_bg);
    draw_text_20_bold(20, 12, "Dosya Yoneticisi", t->text_primary);

    /* Breadcrumb (path) */
    draw_text_16(SCREEN_WIDTH - 100, 17, fm.current_path, t->text_secondary);

    /* Accent line */
    draw_rect(0, 49, SCREEN_WIDTH, 2, t->accent);

    /* Hata durumu */
    if(fm.state == FM_STATE_ERROR) {
        draw_text_20(50, 200, "SD kart okunamadi!", t->error);
        draw_text_16(50, 240, "Lutfen SD karti kontrol edin.", t->text_secondary);
        return;
    }

    /* Yükleniyor */
    if(fm.state == FM_STATE_LOADING) {
        draw_text_20(50, 200, "Yukleniyor...", t->text_primary);
        return;
    }

    /* Boş dizin */
    if(fm.count == 0) {
        draw_text_20(50, 200, "Dizin bos", t->text_disabled);
        return;
    }

    /* Dosya listesi alanı */
    int list_x = 25;
    int list_y = 65;
    int item_height = 38;

    /* Smooth scroll */
    float scroll = fm.current_scroll;
    if(anim_is_running(&fm.scroll_anim)) {
        scroll = anim_get_value(&fm.scroll_anim);
        fm.current_scroll = scroll;
    }

    /* Görünür dosyaları çiz */
    for(int i = 0; i < MAX_VISIBLE_FILES + 2 && (fm.scroll_offset + i) < fm.count; i++) {
        int idx = fm.scroll_offset + i;
        if(idx < 0) continue;

        FileInfo *file = &fm.files[idx];
        int y = list_y + i * item_height - (int)(scroll * item_height);

        /* Ekran dışındaysa atla */
        if(y < 50 || y > SCREEN_HEIGHT - 60) continue;

        int is_selected = (idx == fm.selected);

        /* Seçili satır arka planı */
        if(is_selected) {
            /* Gradient-like selection */
            draw_rect(list_x - 10, y - 3, SCREEN_WIDTH - 50, item_height, t->selected_bg);

            /* Left accent bar */
            draw_rect(list_x - 10, y - 3, 4, item_height, t->accent);

            /* Outline */
            draw_rect_outline(list_x - 10, y - 3, SCREEN_WIDTH - 50, item_height, 1, t->accent);
        }

        /* İkon */
        uint32_t icon_color = get_file_color(file);
        if(file->is_dir) {
            draw_folder_icon(list_x, y, icon_color);
        } else {
            draw_file_icon(list_x, y, icon_color);
        }

        /* Dosya adı */
        uint32_t text_color = is_selected ? t->text_highlight : t->text_primary;
        if(!file->is_dir) {
            /* ROM dosyaları için özel renk */
            if(ends_with(file->name, ".nes") || ends_with(file->name, ".smc") ||
               ends_with(file->name, ".gba") || ends_with(file->name, ".gb")) {
                if(!is_selected) text_color = t->accent_alt;
            }
        }
        draw_text_16(list_x + 35, y + 8, file->name, text_color);

        /* Boyut / Tip (sağda) */
        char info_str[20];
        if(file->is_dir) {
            str_copy_simple(info_str, "<DIR>");
        } else {
            fat32_format_size(file->size, info_str);
        }
        int info_w = text_width_16(info_str);
        draw_text_16(SCREEN_WIDTH - 60 - info_w, y + 8, info_str, t->text_secondary);
    }

    /* Scroll bar */
    if(fm.count > MAX_VISIBLE_FILES) {
        int bar_x = SCREEN_WIDTH - 18;
        int bar_y = 65;
        int bar_height = SCREEN_HEIGHT - 120;

        /* Track */
        draw_rect(bar_x, bar_y, 10, bar_height, t->bg_medium);

        /* Thumb */
        int thumb_height = (MAX_VISIBLE_FILES * bar_height) / fm.count;
        if(thumb_height < 30) thumb_height = 30;
        int thumb_y = bar_y + (fm.selected * (bar_height - thumb_height)) / (fm.count - 1);

        draw_rect(bar_x, thumb_y, 10, thumb_height, t->accent);
    }

    /* Footer */
    draw_rect(0, SCREEN_HEIGHT - 45, SCREEN_WIDTH, 45, t->footer_bg);
    draw_rect(0, SCREEN_HEIGHT - 45, SCREEN_WIDTH, 1, t->accent);

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
    int len = 8;
    while(j > 0) {
        count_str[len++] = tmp[--j];
    }
    count_str[len++] = ' ';
    count_str[len++] = 'o';
    count_str[len++] = 'g';
    count_str[len++] = 'e';
    count_str[len] = 0;

    draw_text_16(20, SCREEN_HEIGHT - 30, count_str, t->text_secondary);

    /* Kontrol ipuçları */
    draw_text_16(SCREEN_WIDTH - 180, SCREEN_HEIGHT - 30, "A:Ac  B:Geri", t->text_secondary);

    /* Memory barrier */
    __asm__ volatile("dsb sy");
}

/* Yukarı git */
void filemgr_up(void) {
    if(fm.selected > 0) {
        fm.selected--;

        /* Scroll animasyonu */
        if(fm.selected < fm.scroll_offset) {
            fm.scroll_offset = fm.selected;
            float current = fm.current_scroll;
            anim_start(&fm.scroll_anim, current, 0, 150, EASE_OUT_QUAD);
        }
    }
}

/* Aşağı git */
void filemgr_down(void) {
    if(fm.selected < fm.count - 1) {
        fm.selected++;

        /* Scroll animasyonu */
        if(fm.selected >= fm.scroll_offset + MAX_VISIBLE_FILES) {
            fm.scroll_offset = fm.selected - MAX_VISIBLE_FILES + 1;
            float current = fm.current_scroll;
            anim_start(&fm.scroll_anim, current, 0, 150, EASE_OUT_QUAD);
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
        /* Dosyayı aç */
        /* TODO: ROM başlatma */
    }
}

/* Geri git */
void filemgr_back(void) {
    /* Üst dizine git */
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
