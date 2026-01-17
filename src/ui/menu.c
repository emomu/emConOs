/* menu.c - XMB-style menu implementation */
#include <ui/menu.h>
#include <ui/theme.h>
#include <ui/animation.h>
#include <ui/transition.h>
#include <graphics.h>
#include <fonts/fonts.h>
#include <drivers/timer.h>
#include <hw.h>

/* Global menü */
XMBMenu g_menu;

/* String kopyalama yardımcısı */
static void str_copy(char *dest, const char *src, int max_len) {
    int i = 0;
    while(src[i] && i < max_len - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

/* --- Menü Başlatma --- */

void menu_init(void) {
    /* Sıfırla */
    for(int i = 0; i < MENU_MAX_CATEGORIES; i++) {
        g_menu.categories[i].item_count = 0;
        g_menu.categories[i].selected_item = 0;
        g_menu.categories[i].scroll_x = 0;
        g_menu.categories[i].target_scroll_x = 0;
        anim_init(&g_menu.categories[i].scroll_anim);
    }

    g_menu.category_count = 0;
    g_menu.selected_category = 0;
    g_menu.scroll_y = 0;
    g_menu.target_scroll_y = 0;

    /* Animasyonları başlat */
    anim_init(&g_menu.cat_anim);
    anim_init(&g_menu.item_anim);
    anim_init(&g_menu.pulse_anim);

    /* Görsel ayarlar */
    g_menu.item_height = 45;
    g_menu.category_spacing = 80;
    g_menu.content_start_x = 200;
    g_menu.content_start_y = 120;

    g_menu.initialized = 1;
    g_menu.input_locked = 0;

    /* Pulse animasyonunu sürekli çalıştır */
    anim_start(&g_menu.pulse_anim, 0.0f, 1.0f, 1500, EASE_IN_OUT_QUAD);
}

/* --- Menü Güncelleme --- */

void menu_update(void) {
    if(!g_menu.initialized) return;

    /* Animasyonları güncelle */
    anim_update(&g_menu.cat_anim);
    anim_update(&g_menu.item_anim);
    anim_update(&g_menu.pulse_anim);

    /* Pulse döngüsü */
    if(anim_is_complete(&g_menu.pulse_anim)) {
        float start = g_menu.pulse_anim.end_value;
        float end = (start > 0.5f) ? 0.0f : 1.0f;
        anim_start(&g_menu.pulse_anim, start, end, 1500, EASE_IN_OUT_QUAD);
    }

    /* Kategori scroll animasyonları */
    for(int i = 0; i < g_menu.category_count; i++) {
        anim_update(&g_menu.categories[i].scroll_anim);
        if(anim_is_running(&g_menu.categories[i].scroll_anim)) {
            g_menu.categories[i].scroll_x = anim_get_value(&g_menu.categories[i].scroll_anim);
        }
    }

    /* Input kilidini kontrol et */
    if(g_menu.input_locked) {
        if(!anim_is_running(&g_menu.cat_anim) && !anim_is_running(&g_menu.item_anim)) {
            g_menu.input_locked = 0;
        }
    }
}

/* --- Navigasyon --- */

void menu_move_up(void) {
    if(g_menu.input_locked || g_menu.category_count == 0) return;

    if(g_menu.selected_category > 0) {
        g_menu.selected_category--;

        /* Animasyon başlat */
        float current = g_menu.scroll_y;
        float target = -(float)(g_menu.selected_category * g_menu.category_spacing);
        anim_start(&g_menu.cat_anim, current, target, 200, EASE_OUT_BACK);

        g_menu.input_locked = 1;
    }
}

void menu_move_down(void) {
    if(g_menu.input_locked || g_menu.category_count == 0) return;

    if(g_menu.selected_category < g_menu.category_count - 1) {
        g_menu.selected_category++;

        /* Animasyon başlat */
        float current = g_menu.scroll_y;
        float target = -(float)(g_menu.selected_category * g_menu.category_spacing);
        anim_start(&g_menu.cat_anim, current, target, 200, EASE_OUT_BACK);

        g_menu.input_locked = 1;
    }
}

void menu_move_left(void) {
    if(g_menu.input_locked || g_menu.category_count == 0) return;

    MenuCategory *cat = &g_menu.categories[g_menu.selected_category];
    if(cat->item_count == 0) return;

    if(cat->selected_item > 0) {
        cat->selected_item--;

        /* Yatay scroll animasyonu */
        float current = cat->scroll_x;
        float target = -(float)(cat->selected_item * 160);  /* Her öğe 160px */
        anim_start(&cat->scroll_anim, current, target, 150, EASE_OUT_QUAD);
    }
}

void menu_move_right(void) {
    if(g_menu.input_locked || g_menu.category_count == 0) return;

    MenuCategory *cat = &g_menu.categories[g_menu.selected_category];
    if(cat->item_count == 0) return;

    if(cat->selected_item < cat->item_count - 1) {
        cat->selected_item++;

        /* Yatay scroll animasyonu */
        float current = cat->scroll_x;
        float target = -(float)(cat->selected_item * 160);
        anim_start(&cat->scroll_anim, current, target, 150, EASE_OUT_QUAD);
    }
}

void menu_select(void) {
    if(g_menu.input_locked || g_menu.category_count == 0) return;

    MenuCategory *cat = &g_menu.categories[g_menu.selected_category];
    if(cat->item_count == 0) return;

    MenuItem *item = &cat->items[cat->selected_item];
    if(!item->enabled) return;

    switch(item->type) {
        case MENU_ITEM_ACTION:
        case MENU_ITEM_SUBMENU:
            if(item->action) {
                item->action();
            }
            break;

        case MENU_ITEM_TOGGLE:
            item->value = !item->value;
            if(item->action) {
                item->action();
            }
            break;

        case MENU_ITEM_SLIDER:
            /* Slider değerini artır */
            item->value++;
            if(item->value > item->max_value) {
                item->value = item->min_value;
            }
            if(item->action) {
                item->action();
            }
            break;

        default:
            break;
    }
}

void menu_back(void) {
    /* Geri tuşu - şimdilik bir şey yapmıyor */
}

/* --- Kategori ve Öğe Ekleme --- */

int menu_add_category(const char *label, char icon_char, uint32_t color) {
    if(g_menu.category_count >= MENU_MAX_CATEGORIES) return -1;

    MenuCategory *cat = &g_menu.categories[g_menu.category_count];
    str_copy(cat->label, label, MENU_MAX_LABEL_LEN);
    cat->icon_char = icon_char;
    cat->icon_color = color;
    cat->item_count = 0;
    cat->selected_item = 0;
    cat->scroll_x = 0;
    cat->target_scroll_x = 0;
    anim_init(&cat->scroll_anim);

    return g_menu.category_count++;
}

int menu_add_item(int category, const char *label, const char *desc,
                  MenuItemType type, void (*action)(void)) {
    if(category < 0 || category >= g_menu.category_count) return -1;

    MenuCategory *cat = &g_menu.categories[category];
    if(cat->item_count >= MENU_MAX_ITEMS) return -1;

    MenuItem *item = &cat->items[cat->item_count];
    str_copy(item->label, label, MENU_MAX_LABEL_LEN);
    if(desc) {
        str_copy(item->description, desc, MENU_MAX_DESC_LEN);
    } else {
        item->description[0] = '\0';
    }
    item->type = type;
    item->action = action;
    item->value = 0;
    item->min_value = 0;
    item->max_value = 1;
    item->enabled = 1;
    item->icon_color = g_theme.text_primary;

    return cat->item_count++;
}

int menu_add_toggle(int category, const char *label, const char *desc,
                    int *value_ptr, void (*on_change)(void)) {
    int idx = menu_add_item(category, label, desc, MENU_ITEM_TOGGLE, on_change);
    if(idx >= 0 && value_ptr) {
        g_menu.categories[category].items[idx].value = *value_ptr;
    }
    return idx;
}

int menu_add_slider(int category, const char *label, const char *desc,
                    int *value_ptr, int min, int max, void (*on_change)(void)) {
    int idx = menu_add_item(category, label, desc, MENU_ITEM_SLIDER, on_change);
    if(idx >= 0) {
        MenuItem *item = &g_menu.categories[category].items[idx];
        item->min_value = min;
        item->max_value = max;
        if(value_ptr) {
            item->value = *value_ptr;
        }
    }
    return idx;
}

/* --- Yardımcı Fonksiyonlar --- */

MenuCategory *menu_get_current_category(void) {
    if(g_menu.category_count == 0) return (void*)0;
    return &g_menu.categories[g_menu.selected_category];
}

MenuItem *menu_get_current_item(void) {
    MenuCategory *cat = menu_get_current_category();
    if(!cat || cat->item_count == 0) return (void*)0;
    return &cat->items[cat->selected_item];
}

int menu_get_selected_category(void) {
    return g_menu.selected_category;
}

int menu_get_selected_item(void) {
    MenuCategory *cat = menu_get_current_category();
    if(!cat) return -1;
    return cat->selected_item;
}

void menu_set_item_enabled(int category, int item, int enabled) {
    if(category < 0 || category >= g_menu.category_count) return;
    if(item < 0 || item >= g_menu.categories[category].item_count) return;
    g_menu.categories[category].items[item].enabled = enabled;
}

/* --- Render Fonksiyonları --- */

void menu_draw_header(void) {
    /* Başlık bar */
    draw_rect(0, 0, SCREEN_WIDTH, 50, g_theme.header_bg);

    /* Logo/Başlık */
    draw_text_20_bold(20, 12, "EmConOs", g_theme.text_primary);

    /* Saat */
    char time_str[6];
    clock_format_time(time_str);
    draw_text_16(SCREEN_WIDTH - 70, 17, time_str, g_theme.text_secondary);

    /* Alt çizgi */
    draw_rect(0, 49, SCREEN_WIDTH, 1, g_theme.accent);
}

void menu_draw_footer(void) {
    /* Alt bar */
    draw_rect(0, SCREEN_HEIGHT - 40, SCREEN_WIDTH, 40, g_theme.footer_bg);

    /* Üst çizgi */
    draw_rect(0, SCREEN_HEIGHT - 40, SCREEN_WIDTH, 1, g_theme.accent);

    /* Kontrol ipuçları */
    int x = 20;
    draw_text_16(x, SCREEN_HEIGHT - 28, "A: Sec", g_theme.text_secondary);
    x += 80;
    draw_text_16(x, SCREEN_HEIGHT - 28, "B: Geri", g_theme.text_secondary);
    x += 90;
    draw_text_16(x, SCREEN_HEIGHT - 28, "D-Pad: Gezin", g_theme.text_secondary);
}

void menu_draw_category_icons(void) {
    /* Sol taraftaki kategori ikonları */
    int icon_x = 40;
    int icon_size = 50;
    int base_y = 140;

    /* Scroll offset'i al */
    float scroll_offset = anim_is_running(&g_menu.cat_anim) ?
                          anim_get_value(&g_menu.cat_anim) : g_menu.scroll_y;

    for(int i = 0; i < g_menu.category_count; i++) {
        MenuCategory *cat = &g_menu.categories[i];
        int y = base_y + i * g_menu.category_spacing + (int)scroll_offset;

        /* Ekran dışındaysa atla */
        if(y < 50 || y > SCREEN_HEIGHT - 50) continue;

        int is_selected = (i == g_menu.selected_category);

        /* İkon arka planı */
        uint32_t bg_color = is_selected ? g_theme.selected_bg : g_theme.bg_medium;
        if(is_selected) {
            /* Pulse efekti */
            float pulse = anim_get_value(&g_menu.pulse_anim);
            bg_color = lerp_color(g_theme.selected_bg, g_theme.accent, pulse * 0.3f);
        }

        draw_rect(icon_x - 5, y - 5, icon_size + 10, icon_size + 10, bg_color);

        /* İkon (basit karakter olarak) */
        char icon_str[2] = {cat->icon_char, '\0'};
        uint32_t icon_color = is_selected ? g_theme.accent : cat->icon_color;
        draw_text_32_bold(icon_x + 12, y + 8, icon_str, icon_color);

        /* Kategori etiketi (sağda) */
        uint32_t label_color = is_selected ? g_theme.text_primary : g_theme.text_secondary;
        draw_text_20_medium(icon_x + icon_size + 20, y + 15, cat->label, label_color);

        /* Seçili ise ok işareti */
        if(is_selected) {
            draw_text_20(icon_x + icon_size + 15 + text_width_20(cat->label) + 10,
                        y + 15, ">", g_theme.accent);
        }
    }
}

void menu_draw_items(void) {
    if(g_menu.category_count == 0) return;

    MenuCategory *cat = &g_menu.categories[g_menu.selected_category];
    if(cat->item_count == 0) {
        draw_text_20(g_menu.content_start_x, g_menu.content_start_y + 50,
                    "Oge yok", g_theme.text_disabled);
        return;
    }

    /* Yatay öğe listesi */
    int item_width = 140;
    int item_height = 100;
    int gap = 20;
    int start_x = g_menu.content_start_x;
    int start_y = g_menu.content_start_y + 30;

    /* Scroll offset */
    float scroll = cat->scroll_x;

    for(int i = 0; i < cat->item_count; i++) {
        MenuItem *item = &cat->items[i];
        int x = start_x + i * (item_width + gap) + (int)scroll;

        /* Ekran dışındaysa atla */
        if(x + item_width < 0 || x > SCREEN_WIDTH) continue;

        int is_selected = (i == cat->selected_item);

        /* Öğe kutusu */
        uint32_t box_color = is_selected ? g_theme.selected_bg : g_theme.bg_light;
        draw_rect(x, start_y, item_width, item_height, box_color);

        /* Seçili ise kenarlık */
        if(is_selected) {
            float pulse = anim_get_value(&g_menu.pulse_anim);
            uint32_t border_color = lerp_color(g_theme.accent, g_theme.accent_glow, pulse);
            draw_rect_outline(x, start_y, item_width, item_height, 3, border_color);
        } else {
            draw_rect_outline(x, start_y, item_width, item_height, 1, g_theme.bg_medium);
        }

        /* Öğe etiketi */
        uint32_t text_color = item->enabled ?
            (is_selected ? g_theme.text_highlight : g_theme.text_primary) :
            g_theme.text_disabled;

        /* Metni ortalayarak yaz */
        int tw = text_width_16(item->label);
        int tx = x + (item_width - tw) / 2;
        draw_text_16(tx, start_y + 40, item->label, text_color);

        /* Toggle/Slider değeri */
        if(item->type == MENU_ITEM_TOGGLE) {
            const char *val_str = item->value ? "Acik" : "Kapali";
            uint32_t val_color = item->value ? g_theme.success : g_theme.text_secondary;
            int vw = text_width_16(val_str);
            draw_text_16(x + (item_width - vw) / 2, start_y + 65, val_str, val_color);
        } else if(item->type == MENU_ITEM_SLIDER) {
            /* Slider bar */
            int bar_w = item_width - 20;
            int bar_h = 8;
            int bar_x = x + 10;
            int bar_y = start_y + 70;

            draw_rect(bar_x, bar_y, bar_w, bar_h, g_theme.bg_dark);

            float ratio = (float)(item->value - item->min_value) /
                         (float)(item->max_value - item->min_value);
            int fill_w = (int)(bar_w * ratio);
            draw_rect(bar_x, bar_y, fill_w, bar_h, g_theme.accent);
        }
    }
}

void menu_draw_info_panel(void) {
    MenuItem *item = menu_get_current_item();
    if(!item || item->description[0] == '\0') return;

    /* Bilgi paneli */
    int panel_x = g_menu.content_start_x;
    int panel_y = SCREEN_HEIGHT - 120;
    int panel_w = SCREEN_WIDTH - panel_x - 20;
    int panel_h = 60;

    draw_rect(panel_x, panel_y, panel_w, panel_h, g_theme.bg_medium);
    draw_rect_outline(panel_x, panel_y, panel_w, panel_h, 1, g_theme.bg_light);

    /* Açıklama metni */
    draw_text_16(panel_x + 15, panel_y + 20, item->description, g_theme.text_secondary);
}

void menu_render(void) {
    if(!g_menu.initialized) return;

    /* Arka plan */
    clear_screen(g_theme.bg_dark);

    /* Header */
    menu_draw_header();

    /* Kategori ikonları (sol) */
    menu_draw_category_icons();

    /* Öğe listesi (sağ) */
    menu_draw_items();

    /* Bilgi paneli (alt) */
    menu_draw_info_panel();

    /* Footer */
    menu_draw_footer();

    /* Memory barrier */
    __asm__ volatile("dsb sy");
}

/* --- Varsayılan Menü --- */

/* Callback fonksiyonları (placeholder) */
static void action_games(void) { /* TODO */ }
static void action_files(void) { /* TODO */ }
static void action_settings_display(void) { /* TODO */ }
static void action_settings_audio(void) { /* TODO */ }
static void action_about(void) { /* TODO */ }

void menu_create_default(void) {
    menu_init();

    /* Oyunlar kategorisi */
    int cat_games = menu_add_category("Oyunlar", 'G', g_theme.icon_games);
    menu_add_item(cat_games, "NES", "Nintendo Entertainment System oyunlari", MENU_ITEM_ACTION, action_games);
    menu_add_item(cat_games, "SNES", "Super Nintendo oyunlari", MENU_ITEM_ACTION, action_games);
    menu_add_item(cat_games, "Mega Drive", "Sega Mega Drive oyunlari", MENU_ITEM_ACTION, action_games);
    menu_add_item(cat_games, "Game Boy", "Game Boy oyunlari", MENU_ITEM_ACTION, action_games);
    menu_add_item(cat_games, "GBA", "Game Boy Advance oyunlari", MENU_ITEM_ACTION, action_games);

    /* Dosyalar kategorisi */
    int cat_files = menu_add_category("Dosyalar", 'F', g_theme.icon_files);
    menu_add_item(cat_files, "SD Kart", "SD karttaki dosyalara goz at", MENU_ITEM_ACTION, action_files);
    menu_add_item(cat_files, "Favoriler", "Favori oyunlarin", MENU_ITEM_ACTION, action_files);
    menu_add_item(cat_files, "Son Oynan", "Son oynanan oyunlar", MENU_ITEM_ACTION, action_files);

    /* Ayarlar kategorisi */
    int cat_settings = menu_add_category("Ayarlar", 'S', g_theme.icon_settings);
    menu_add_item(cat_settings, "Goruntu", "Ekran ve grafik ayarlari", MENU_ITEM_ACTION, action_settings_display);
    menu_add_item(cat_settings, "Ses", "Ses ayarlari", MENU_ITEM_ACTION, action_settings_audio);
    menu_add_item(cat_settings, "Kontroller", "Kumanda ayarlari", MENU_ITEM_ACTION, (void*)0);
    menu_add_item(cat_settings, "Sistem", "Sistem ayarlari", MENU_ITEM_ACTION, (void*)0);

    /* Hakkında kategorisi */
    int cat_about = menu_add_category("Hakkinda", 'i', g_theme.icon_about);
    menu_add_item(cat_about, "Bilgi", "EmConOs hakkinda bilgi", MENU_ITEM_ACTION, action_about);
    menu_add_item(cat_about, "Surum", "v1.0 - Raspberry Pi Zero 2W", MENU_ITEM_ACTION, (void*)0);
    menu_add_item(cat_about, "Gelistirici", "Emirhan Soylu", MENU_ITEM_ACTION, (void*)0);
}
