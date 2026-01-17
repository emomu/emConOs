/* menu.h - XMB-style menu system (RetroArch inspired) */
#ifndef MENU_H
#define MENU_H

#include <types.h>
#include <ui/animation.h>

/* Maksimum değerler */
#define MENU_MAX_CATEGORIES     8
#define MENU_MAX_ITEMS          32
#define MENU_MAX_LABEL_LEN      32
#define MENU_MAX_DESC_LEN       64

/* Menü öğesi türü */
typedef enum {
    MENU_ITEM_ACTION,       /* Tıklanabilir eylem */
    MENU_ITEM_SUBMENU,      /* Alt menü açar */
    MENU_ITEM_TOGGLE,       /* Açık/Kapalı */
    MENU_ITEM_SLIDER,       /* Değer seçici */
    MENU_ITEM_SEPARATOR     /* Ayırıcı çizgi */
} MenuItemType;

/* Menü öğesi */
typedef struct {
    char label[MENU_MAX_LABEL_LEN];
    char description[MENU_MAX_DESC_LEN];
    MenuItemType type;
    void (*action)(void);   /* Eylem callback */
    int value;              /* Toggle/slider değeri */
    int min_value;          /* Slider min */
    int max_value;          /* Slider max */
    int enabled;            /* Aktif mi? */
    uint32_t icon_color;    /* İkon rengi */
} MenuItem;

/* Menü kategorisi */
typedef struct {
    char label[MENU_MAX_LABEL_LEN];
    char icon_char;         /* Basit ikon karakteri */
    uint32_t icon_color;
    MenuItem items[MENU_MAX_ITEMS];
    int item_count;
    int selected_item;
    Animation scroll_anim;  /* Yatay kaydırma animasyonu */
    float scroll_x;
    float target_scroll_x;
} MenuCategory;

/* Ana menü yapısı */
typedef struct {
    MenuCategory categories[MENU_MAX_CATEGORIES];
    int category_count;
    int selected_category;

    /* Animasyonlar */
    Animation cat_anim;         /* Kategori geçiş animasyonu */
    Animation item_anim;        /* Öğe seçim animasyonu */
    Animation pulse_anim;       /* Seçim pulse efekti */

    /* Scroll değerleri */
    float scroll_y;
    float target_scroll_y;

    /* Görsel ayarlar */
    int item_height;
    int category_spacing;
    int content_start_x;
    int content_start_y;

    /* Durum */
    int initialized;
    int input_locked;           /* Geçiş sırasında input kilitle */
} XMBMenu;

/* Global menü */
extern XMBMenu g_menu;

/* Menü fonksiyonları */
void menu_init(void);
void menu_update(void);
void menu_render(void);

/* Navigasyon */
void menu_move_up(void);
void menu_move_down(void);
void menu_move_left(void);
void menu_move_right(void);
void menu_select(void);
void menu_back(void);

/* Kategori yönetimi */
int menu_add_category(const char *label, char icon_char, uint32_t color);
int menu_add_item(int category, const char *label, const char *desc,
                  MenuItemType type, void (*action)(void));
int menu_add_toggle(int category, const char *label, const char *desc,
                    int *value_ptr, void (*on_change)(void));
int menu_add_slider(int category, const char *label, const char *desc,
                    int *value_ptr, int min, int max, void (*on_change)(void));

/* Yardımcı fonksiyonlar */
MenuCategory *menu_get_current_category(void);
MenuItem *menu_get_current_item(void);
int menu_get_selected_category(void);
int menu_get_selected_item(void);
void menu_set_item_enabled(int category, int item, int enabled);

/* Varsayılan menü oluştur */
void menu_create_default(void);

/* Render yardımcıları */
void menu_draw_category_icons(void);
void menu_draw_items(void);
void menu_draw_info_panel(void);
void menu_draw_header(void);
void menu_draw_footer(void);

#endif
