/* src/ui/filemgr.c - Tam ve Düzeltilmiş Dosya Yöneticisi */
#include <ui/filemgr.h>
#include <ui/theme.h>
#include <fs/fat32.h>
#include <graphics.h>
#include <fonts/fonts.h>
#include <types.h>

/* --- AYARLAR --- */
#define ITEM_HEIGHT      50
#define HEADER_HEIGHT    60
#define FOOTER_HEIGHT    45
#define MAX_FILES        256

/* --- RENKLER (Flat Design) --- */
#define COL_BG           0xFF1A1A1A
#define COL_HEADER       0xFF151515
#define COL_ITEM_BG      0xFF252525
#define COL_ITEM_SEL     0xFFE91E63 // Veya g_theme.accent
#define COL_TEXT_PRI     0xFFFFFFFF
#define COL_TEXT_SEC     0xFF888888
#define COL_FOLDER       0xFFE6B800
#define COL_FILE         0xFFAAAAAA

/* --- YAPILAR --- */
static struct {
    FileInfo files[MAX_FILES];
    int count;
    int selected;
    
    /* Scroll Yönetimi */
    float current_pixel_y;    // Animasyonlu anlık pozisyon
    float target_pixel_y;     // Hedef pozisyon
    
    FileMgrState state;
    char current_path[MAX_PATH];
} fm;

/* --- YARDIMCI FONKSİYONLAR --- */

/* Matematik kütüphanesiz Lerp (Yumuşak Geçiş) */
static float fm_lerp(float start, float end, float t) {
    return start + (end - start) * t;
}

/* Basit string kopyalama */
static void str_copy_simple(char *dest, const char *src) {
    while(*src) *dest++ = *src++;
    *dest = 0;
}

/* Demo dosyaları (SD kart yoksa gösterilecekler) */
static void add_demo_files(void) {
    fm.count = 0;
    const char *demos[] = { 
        "NES", "SNES", "GBA", "Mega Drive", "MAME", "NeoGeo", 
        "mario.nes", "zelda.smc", "sonic.md", "pokemon.gb", 
        "doom.exe", "readme.txt", "system.cfg", "bios.bin" 
    };
    
    for(int i = 0; i < 14; i++) {
        str_copy_simple(fm.files[i].name, demos[i]);
        fm.files[i].is_dir = (i < 6); // İlk 6 tanesi klasör
        fm.files[i].size = 1024 * (i * i + 1); // Rastgele boyut
        fm.count++;
    }
}

/* Flat İkon Çizimi */
static void draw_flat_icon(int x, int y, int is_dir) {
    if(is_dir) {
        /* Klasör (Sarı) */
        draw_rect(x, y + 2, 24, 18, COL_FOLDER);
        draw_rect(x, y, 10, 4, COL_FOLDER);
    } else {
        /* Dosya (Gri) */
        draw_rect(x + 4, y, 16, 22, COL_FILE);
        draw_rect(x + 16, y, 4, 4, 0xFF555555); // Kıvrık köşe
    }
}

/* --- INIT --- */

void filemgr_init(void) {
    fm.count = 0;
    fm.selected = 0;
    fm.current_pixel_y = 0;
    fm.target_pixel_y = 0;
    fm.state = FM_STATE_LOADING;
    
    fm.current_path[0] = '/';
    fm.current_path[1] = 0;

    /* FAT32 Başlatmayı Dene */
    if(fat32_init() != FAT_OK) {
        add_demo_files();
        fm.state = FM_STATE_READY;
        return;
    }

    if(fat32_open_dir("/") != FAT_OK) {
        add_demo_files();
        fm.state = FM_STATE_READY;
        return;
    }

    /* Dosyaları Oku */
    FileInfo info;
    while(fat32_read_dir(&info) == FAT_OK && fm.count < MAX_FILES) {
        int i;
        for(i = 0; i < MAX_FILENAME && info.name[i]; i++) {
            fm.files[fm.count].name[i] = info.name[i];
        }
        fm.files[fm.count].name[i] = 0;
        fm.files[fm.count].size = info.size;
        fm.files[fm.count].is_dir = info.is_dir;
        fm.count++;
    }
    fat32_close_dir();

    if(fm.count == 0) add_demo_files();
    fm.state = FM_STATE_READY;
}

/* --- DRAW --- */

void filemgr_draw(void) {
    /* 1. Arka Planı Temizle */
    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COL_BG);

    /* --- LİSTE ÇİZİMİ (EN ALTA) --- */
    /* Header ve Footer'ın arkasında kalsın diye önce çiziyoruz */

    int list_start_y = HEADER_HEIGHT + 10;
    int list_visible_h = SCREEN_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT; 
    
    /* Animasyon */
    fm.current_pixel_y = fm_lerp(fm.current_pixel_y, fm.target_pixel_y, 0.2f);

    /* Layout (Hizalama) */
    int side_padding = 15;
    int scrollbar_width = 8;
    int scrollbar_gap = 5;
    
    // Öğelerin genişliğini scrollbar'a çarpmayacak şekilde ayarla
    int item_w = SCREEN_WIDTH - (side_padding * 2) - scrollbar_width - scrollbar_gap;
    int start_x = side_padding;

    for(int i = 0; i < fm.count; i++) {
        /* Y Koordinatı Hesabı */
        float y = (i * ITEM_HEIGHT) - fm.current_pixel_y + list_start_y;

        /* Ekrana girmeyenleri çizme (Culling) */
        /* Performans için kritik */
        if(y < -ITEM_HEIGHT || y > SCREEN_HEIGHT) continue;

        int is_selected = (i == fm.selected);
        FileInfo *file = &fm.files[i];

        /* Kutu Arka Planı */
        if(is_selected) {
            draw_rect(start_x, (int)y, item_w, ITEM_HEIGHT - 5, g_theme.accent);
            draw_rect_outline(start_x, (int)y, item_w, ITEM_HEIGHT - 5, 2, 0xFFFFFFFF);
        } else {
            draw_rect(start_x, (int)y, item_w, ITEM_HEIGHT - 5, COL_ITEM_BG);
        }

        /* İkon */
        draw_flat_icon(start_x + 10, (int)y + 10, file->is_dir);

        /* Dosya Adı */
        uint32_t text_color = is_selected ? COL_TEXT_PRI : 0xFFCCCCCC;
        draw_text_16(start_x + 45, (int)y + 14, file->name, text_color);

        /* Dosya Bilgisi (Sağa Dayalı) */
        char info[16];
        if(file->is_dir) {
            str_copy_simple(info, "<DIR>");
        } else {
            /* Boyut Formatla (KB/MB) */
            int k = file->size / 1024;
            if(k == 0 && file->size > 0) k = 1; // En az 1KB göster
            
            // Basit int -> string çevrimi
            if(k < 1000) {
                // 000KB formatı hizalama için güzel durur
                info[0] = '0' + (k/100)%10; 
                info[1] = '0' + (k/10)%10; 
                info[2] = '0' + k%10; 
                info[3] = 'K'; info[4] = 'B'; info[5] = 0;
            } else {
                int m = k / 1024;
                info[0] = '0' + (m/100)%10; 
                info[1] = '0' + (m/10)%10; 
                info[2] = '0' + m%10; 
                info[3] = 'M'; info[4] = 'B'; info[5] = 0;
            }
        }
        
        uint32_t info_color = is_selected ? 0xFFDDDDDD : COL_TEXT_SEC;
        draw_text_16(start_x + item_w - 70, (int)y + 14, info, info_color);
    }

    /* --- HEADER (ÜSTE BİNER) --- */
    /* Liste yukarı kayarken çirkin görünmemesi için opak arka plan çiziyoruz */
    draw_rect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, COL_HEADER);
    draw_rect(0, HEADER_HEIGHT, SCREEN_WIDTH, 1, 0xFF444444); // Alt çizgi
    
    draw_text_20_bold(20, 18, "Dosya Yöneticisi", COL_TEXT_PRI);
    
    /* Path Kutusu */
    int path_w = 140;
    draw_rect(SCREEN_WIDTH - path_w - 20, 15, path_w, 30, COL_ITEM_BG);
    draw_text_16(SCREEN_WIDTH - path_w - 10, 20, fm.current_path, COL_TEXT_SEC);

    /* --- FOOTER (ÜSTE BİNER) --- */
    int footer_y = SCREEN_HEIGHT - FOOTER_HEIGHT;
    draw_rect(0, footer_y, SCREEN_WIDTH, FOOTER_HEIGHT, COL_HEADER);
    // draw_rect(0, footer_y, SCREEN_WIDTH, 1, 0xFF333333); // İsteğe bağlı üst çizgi

    /* Öğe Sayısı */
    // Basitçe statik yazıyoruz, istenirse intToStr eklenebilir
    draw_text_16(20, footer_y + 12, "Liste Sonu", COL_TEXT_SEC);

    /* Kontroller */
    draw_text_16(SCREEN_WIDTH - 200, footer_y + 12, "[A] Aç   [B] Geri", COL_TEXT_SEC);

    /* --- SCROLL BAR (EN ÜSTE) --- */
    int visible_count = list_visible_h / ITEM_HEIGHT;
    
    if(fm.count > visible_count) {
        int bar_h = list_visible_h - 20;
        int bar_x = SCREEN_WIDTH - side_padding - scrollbar_width + 5; // En sağa
        int bar_y = list_start_y + 10;

        /* Ray (Track) */
        draw_rect(bar_x, bar_y, scrollbar_width, bar_h, COL_ITEM_BG);

        /* Tutacak (Thumb) Boyutu */
        float ratio = (float)visible_count / fm.count;
        int thumb_h = (int)(bar_h * ratio);
        if(thumb_h < 30) thumb_h = 30; // Minimum yükseklik

        /* Tutacak Pozisyonu */
        float scroll_max = (fm.count * ITEM_HEIGHT) - list_visible_h;
        if(scroll_max <= 0) scroll_max = 1;
        
        float scroll_percent = fm.current_pixel_y / scroll_max;
        if(scroll_percent < 0) scroll_percent = 0;
        if(scroll_percent > 1) scroll_percent = 1;

        int thumb_y = bar_y + (int)((bar_h - thumb_h) * scroll_percent);
        
        /* Tutacağı Çiz */
        draw_rect(bar_x, thumb_y, scrollbar_width, thumb_h, g_theme.accent);
    }

    /* Memory Barrier (Görüntü yırtılmasını önler) */
    __asm__ volatile("dsb sy");
}

/* --- GÜNCELLEME ve KONTROL --- */

/* Scroll Hedefini Güncelle (Smart Scrolling) */
static void update_scroll_target(void) {
    int list_h = SCREEN_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT - 20;
    
    int selected_y_top = fm.selected * ITEM_HEIGHT;
    int selected_y_bottom = selected_y_top + ITEM_HEIGHT;

    int view_top = (int)fm.target_pixel_y;
    int view_bottom = view_top + list_h;

    /* Görünür alanda tut */
    if(selected_y_top < view_top) {
        fm.target_pixel_y = selected_y_top;
    } else if(selected_y_bottom > view_bottom) {
        fm.target_pixel_y = selected_y_bottom - list_h;
    }

    /* Sınırları aşma */
    int max_scroll = (fm.count * ITEM_HEIGHT) - list_h;
    if(max_scroll < 0) max_scroll = 0;
    
    if(fm.target_pixel_y < 0) fm.target_pixel_y = 0;
    if(fm.target_pixel_y > max_scroll) fm.target_pixel_y = max_scroll;
}

void filemgr_update(void) {
    /* Animasyonlar draw içinde lerp ile hallediliyor */
}

void filemgr_up(void) {
    if(fm.selected > 0) {
        fm.selected--;
        update_scroll_target();
    }
}

void filemgr_down(void) {
    if(fm.selected < fm.count - 1) {
        fm.selected++;
        update_scroll_target();
    }
}

void filemgr_enter(void) {
    /* Seçim işlemi (TODO) */
}

void filemgr_back(void) {
    /* Geri gitme işlemi (TODO) */
}

/* --- GETTERLAR --- */
FileMgrState filemgr_get_state(void) { return fm.state; }
int filemgr_get_file_count(void) { return fm.count; }
int filemgr_get_selected(void) { return fm.selected; }