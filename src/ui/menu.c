/* src/ui/menu.c - Optimized Animation Logic */
#include <ui/menu.h>
#include <ui/theme.h>
#include <graphics.h>
#include <fonts/fonts.h>
#include <drivers/timer.h>
#include <hw.h>

/* Global menü yapısı */
XMBMenu g_menu;

/* --- KRİTİK DÜZELTME: SNAP-TO-TARGET LERP --- */
/* Bu fonksiyon, hedefe çok yaklaşıldığında animasyonu bitirir. 
   Böylece "sonsuz kayma" hissi ve titreme yok olur. */
static float menu_lerp(float start, float end, float speed) {
    float diff = end - start;
    
    /* Eğer fark çok azsa (0.5 pikselden az), direkt hedefe ışınla */
    if (diff > -0.5f && diff < 0.5f) {
        return end;
    }
    
    return start + (diff * speed);
}

static void str_copy(char *dest, const char *src, int max_len) {
    int i = 0;
    while(src[i] && i < max_len - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

/* --- BAŞLATMA --- */

void menu_init(void) {
    for(int i = 0; i < MENU_MAX_CATEGORIES; i++) {
        g_menu.categories[i].item_count = 0;
        g_menu.categories[i].selected_item = 0;
        g_menu.categories[i].scroll_x = 0;
        g_menu.categories[i].target_scroll_x = 0;
    }

    g_menu.category_count = 0;
    g_menu.selected_category = 0;
    
    g_menu.scroll_y = 0;
    g_menu.target_scroll_y = 0;

    g_menu.category_spacing = 60;
    /* Content başlangıcı Sidebar'ın (200px) biraz içine (220px) */
    g_menu.content_start_x = 220; 
    g_menu.item_height = 100;
    
    g_menu.initialized = 1;
    g_menu.input_locked = 0;
}

/* --- GÜNCELLEME (ANIMASYON MANTIĞI) --- */

void menu_update(void) {
    if(!g_menu.initialized) return;

    /* Hız Faktörü: 0.1 (Yavaş/Yumuşak) - 0.4 (Hızlı/Keskin) */
    /* Pi Zero için 0.3 ideal bir dengedir */
    float speed = 0.3f; 

    /* Dikey Scroll (Kategoriler) */
    g_menu.scroll_y = menu_lerp(g_menu.scroll_y, g_menu.target_scroll_y, speed);

    /* Yatay Scroll (Tüm kategoriler için güncelle) */
    for(int i = 0; i < g_menu.category_count; i++) {
        MenuCategory *cat = &g_menu.categories[i];
        
        /* Sadece görünür olan veya hareket halinde olanları güncelle (CPU Tasarrufu) */
        if(cat->scroll_x != cat->target_scroll_x) {
            cat->scroll_x = menu_lerp(cat->scroll_x, cat->target_scroll_x, speed);
        }
    }
}

/* --- NAVİGASYON --- */

void menu_move_up(void) {
    if(g_menu.selected_category > 0) {
        g_menu.selected_category--;
        g_menu.target_scroll_y = -(float)(g_menu.selected_category * g_menu.category_spacing);
        
        /* Kategori değiştiğinde, yeni kategorinin item pozisyonunu kontrol et */
        MenuCategory *cat = &g_menu.categories[g_menu.selected_category];
        cat->target_scroll_x = -(float)(cat->selected_item * 180);
    }
}

void menu_move_down(void) {
    if(g_menu.selected_category < g_menu.category_count - 1) {
        g_menu.selected_category++;
        g_menu.target_scroll_y = -(float)(g_menu.selected_category * g_menu.category_spacing);
        
        MenuCategory *cat = &g_menu.categories[g_menu.selected_category];
        cat->target_scroll_x = -(float)(cat->selected_item * 180);
    }
}

void menu_move_left(void) {
    MenuCategory *cat = &g_menu.categories[g_menu.selected_category];
    if(cat->item_count == 0) return;

    if(cat->selected_item > 0) {
        cat->selected_item--;
        /* 160px Genişlik + 20px Boşluk = 180px */
        cat->target_scroll_x = -(float)(cat->selected_item * 180);
    }
}

void menu_move_right(void) {
    MenuCategory *cat = &g_menu.categories[g_menu.selected_category];
    if(cat->item_count == 0) return;

    if(cat->selected_item < cat->item_count - 1) {
        cat->selected_item++;
        cat->target_scroll_x = -(float)(cat->selected_item * 180);
    }
}

void menu_select(void) {
    MenuCategory *cat = &g_menu.categories[g_menu.selected_category];
    if(cat->item_count == 0) return;
    
    MenuItem *item = &cat->items[cat->selected_item];
    
    if(item->type == MENU_ITEM_TOGGLE) {
        item->value = !item->value;
        if(item->action) item->action();
        return;
    }
    
    if(item->enabled && item->action) {
        item->action();
    }
}

void menu_back(void) { }

/* --- GETTER --- */

MenuItem *menu_get_current_item(void) {
    if(g_menu.category_count == 0) return (void*)0;
    MenuCategory *cat = &g_menu.categories[g_menu.selected_category];
    if(cat->item_count == 0) return (void*)0;
    return &cat->items[cat->selected_item];
}

int menu_get_selected_category(void) { return g_menu.selected_category; }

int menu_get_selected_item(void) {
    MenuCategory *cat = &g_menu.categories[g_menu.selected_category];
    if(!cat) return -1;
    return cat->selected_item;
}

/* --- VERİ EKLEME --- */

int menu_add_category(const char *label, char icon_char, uint32_t color) {
    if(g_menu.category_count >= MENU_MAX_CATEGORIES) return -1;
    MenuCategory *cat = &g_menu.categories[g_menu.category_count];
    str_copy(cat->label, label, MENU_MAX_LABEL_LEN);
    cat->icon_char = icon_char;
    cat->icon_color = color;
    return g_menu.category_count++;
}

int menu_add_item(int category, const char *label, const char *desc, MenuItemType type, void (*action)(void)) {
    if(category < 0 || category >= g_menu.category_count) return -1;
    MenuCategory *cat = &g_menu.categories[category];
    if(cat->item_count >= MENU_MAX_ITEMS) return -1;
    
    MenuItem *item = &cat->items[cat->item_count];
    str_copy(item->label, label, MENU_MAX_LABEL_LEN);
    str_copy(item->description, desc ? desc : "", MENU_MAX_DESC_LEN);
    item->type = type;
    item->action = action;
    item->value = 0;
    item->enabled = 1;
    return cat->item_count++;
}

/* --- DEFAULT MENÜ --- */

static void action_games(void) { }
static void action_files(void) { } // switch_screen(SCREEN_FILES) bağlanacak
static void action_settings(void) { } // switch_screen(SCREEN_SETTINGS) bağlanacak
static void action_dummy(void) { }

void menu_create_default(void) {
    menu_init();

    int cat_games = menu_add_category("Oyunlar", 'G', g_theme.icon_games);
    menu_add_item(cat_games, "NES", "Nintendo Entertainment System", MENU_ITEM_ACTION, action_games);
    menu_add_item(cat_games, "SNES", "Super Nintendo", MENU_ITEM_ACTION, action_games);
    menu_add_item(cat_games, "GBA", "Game Boy Advance", MENU_ITEM_ACTION, action_games);

    int cat_files = menu_add_category("Dosyalar", 'F', g_theme.icon_files);
    menu_add_item(cat_files, "SD Kart", "Dosyalarına göz at", MENU_ITEM_ACTION, action_files);
    menu_add_item(cat_files, "Favoriler", "Sık kullanılanlar", MENU_ITEM_ACTION, action_files);

    int cat_settings = menu_add_category("Ayarlar", 'S', g_theme.icon_settings);
    menu_add_item(cat_settings, "Ekran", "Parlaklık ve goruntu", MENU_ITEM_ACTION, action_settings);
    menu_add_item(cat_settings, "Ses", "Ses duzeyi", MENU_ITEM_ACTION, action_settings);
    
    int cat_about = menu_add_category("Hakkında", 'i', g_theme.icon_about);
    menu_add_item(cat_about, "Sistem", "EmConOs v1.0", MENU_ITEM_ACTION, action_dummy);
}

/* --- ÇİZİM FONKSİYONLARI --- */

void menu_draw_header(void) {
    draw_rect(0, 0, SCREEN_WIDTH, 50, 0xFF151515);
    draw_rect(0, 50, SCREEN_WIDTH, 1, 0xFF333333); 
    draw_text_20_bold(20, 15, "EmConOs", 0xFFFFFFFF);
    draw_text_16(SCREEN_WIDTH - 70, 18, "12:00", 0xFFAAAAAA);
}

void menu_draw_sidebar(void) {
    /* Sidebar OPAK olmalı (Transparency YOK) */
    /* Böylece içerik soldan kayarken bunun arkasına girer */
    int sidebar_width = 200;
    int sidebar_height = SCREEN_HEIGHT - 51;
    
    draw_rect(0, 51, sidebar_width, sidebar_height, 0xFF101010); 
    draw_rect(sidebar_width, 51, 1, sidebar_height, 0xFF333333); 

    int base_y = 200; 
    int icon_x = 50;

    for(int i = 0; i < g_menu.category_count; i++) {
        MenuCategory *cat = &g_menu.categories[i];
        
        float y = base_y + (i * g_menu.category_spacing) + g_menu.scroll_y;

        /* Ekran dışı kontrolü */
        if(y < 50 || y > SCREEN_HEIGHT - 30) continue;

        int is_selected = (i == g_menu.selected_category);
        char icon_str[2] = {cat->icon_char, '\0'};
        uint32_t icon_color = is_selected ? g_theme.accent : 0xFF555555;
        uint32_t text_color = is_selected ? 0xFFFFFFFF : 0xFF777777;

        if(is_selected) {
            draw_rect(0, (int)y - 15, 4, 30, g_theme.accent);
            draw_text_32_bold(icon_x, (int)y - 16, icon_str, icon_color);
            draw_text_20_bold(icon_x + 40, (int)y - 10, cat->label, text_color);
        } else {
            draw_text_24(icon_x + 10, (int)y - 12, icon_str, icon_color);
        }
    }
}

void menu_draw_content(void) {
    if(g_menu.category_count == 0) return;

    MenuCategory *cat = &g_menu.categories[g_menu.selected_category];
    
    if(cat->item_count == 0) {
        draw_text_20(g_menu.content_start_x + 20, 200, "Bu klasor bos.", 0xFF666666);
        return;
    }

    int item_w = 160;
    int item_h = 100;
    int gap = 20;
    int start_y = 150; 

    for(int i = 0; i < cat->item_count; i++) {
        MenuItem *item = &cat->items[i];

        /* Pozisyon Hesabı */
        float x = g_menu.content_start_x + (i * (item_w + gap)) + cat->scroll_x;

        /* Rendering Optimization (Culling) */
        /* Sidebar'ın arkasına geçenleri çizme (190px altı) */
        /* Ekranın sağından çıkanları çizme */
        if(x < 150 || x > SCREEN_WIDTH) continue;

        int is_selected = (i == cat->selected_item);

        /* KUTU */
        if(is_selected) {
            draw_rect((int)x, start_y, item_w, item_h, g_theme.accent); 
            draw_rect_outline((int)x, start_y, item_w, item_h, 2, 0xFFFFFFFF);
            
            /* Açıklama metni */
            draw_text_16(220, SCREEN_HEIGHT - 40, item->description, 0xFFCCCCCC);
        } else {
            draw_rect((int)x, start_y, item_w, item_h, 0xFF252525);
            draw_rect_outline((int)x, start_y, item_w, item_h, 1, 0xFF444444);
        }

        /* METİN */
        uint32_t label_color = is_selected ? 0xFFFFFFFF : 0xFFAAAAAA;
        draw_text_16((int)x + 10, start_y + item_h - 30, item->label, label_color);

        if(item->type == MENU_ITEM_TOGGLE) {
            uint32_t status_color = item->value ? 0xFF00FF00 : 0xFFFF0000; 
            draw_rect((int)x + item_w - 20, start_y + 10, 10, 10, status_color);
        }
    }
}



void menu_render(void) {
    if(!g_menu.initialized) return;

    /* 1. Arka planı temizle */
    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0xFF1A1A1A); 

    /* 2. ÖNCE İçeriği çiz */
    /* Böylece içerik kayarken Sidebar'ın altına girer */
    menu_draw_content();

    /* 3. SONRA Sidebar'ı çiz (Üste biner) */
    menu_draw_sidebar();

    /* 4. En üste Header ve Footer */
    menu_draw_header();
    

    __asm__ volatile("dsb sy");
}