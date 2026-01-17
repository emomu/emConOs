/* transition.h - Screen transition effects */
#ifndef TRANSITION_H
#define TRANSITION_H

#include <types.h>
#include <ui/animation.h>

/* Geçiş efekt türleri */
typedef enum {
    TRANS_NONE,             /* Geçiş yok, anında değiş */
    TRANS_FADE,             /* Fade in/out */
    TRANS_SLIDE_LEFT,       /* Sola kayarak */
    TRANS_SLIDE_RIGHT,      /* Sağa kayarak */
    TRANS_SLIDE_UP,         /* Yukarı kayarak */
    TRANS_SLIDE_DOWN,       /* Aşağı kayarak */
    TRANS_ZOOM_IN,          /* Zoom in */
    TRANS_ZOOM_OUT,         /* Zoom out */
    TRANS_WIPE_LEFT,        /* Silme efekti sola */
    TRANS_WIPE_RIGHT        /* Silme efekti sağa */
} TransitionType;

/* Geçiş durumu */
typedef enum {
    TRANS_STATE_IDLE,
    TRANS_STATE_OUT,        /* Çıkış animasyonu (eski ekran) */
    TRANS_STATE_IN,         /* Giriş animasyonu (yeni ekran) */
    TRANS_STATE_COMPLETE
} TransitionState;

/* Geçiş yapısı */
typedef struct {
    TransitionType type;
    TransitionState state;
    Animation anim;

    int from_screen;
    int to_screen;

    uint32_t duration_ms;   /* Toplam geçiş süresi */

    /* Fade için */
    uint8_t fade_alpha;

    /* Slide için offset */
    int offset_x;
    int offset_y;

    /* Zoom için scale */
    float scale;
} Transition;

/* Global geçiş durumu */
extern Transition g_transition;

/* Geçiş fonksiyonları */
void transition_init(void);
void transition_start(TransitionType type, int from_screen, int to_screen);
void transition_start_custom(TransitionType type, int from_screen, int to_screen, uint32_t duration_ms);
void transition_update(void);
int transition_is_active(void);
int transition_is_complete(void);
TransitionState transition_get_state(void);

/* Render yardımcıları */
int transition_get_offset_x(void);
int transition_get_offset_y(void);
uint8_t transition_get_alpha(void);
float transition_get_scale(void);

/* Fade overlay çizimi */
void transition_draw_fade_overlay(void);

/* Geçişi iptal et */
void transition_cancel(void);

/* Varsayılan süreler (ms) */
#define TRANS_DURATION_FAST     150
#define TRANS_DURATION_NORMAL   250
#define TRANS_DURATION_SLOW     400

#endif
