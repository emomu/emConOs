/* animation.h - Animation system for smooth UI transitions */
#ifndef ANIMATION_H
#define ANIMATION_H

#include <types.h>

/* Easing fonksiyon tipleri */
typedef enum {
    EASE_LINEAR,
    EASE_IN_QUAD,
    EASE_OUT_QUAD,
    EASE_IN_OUT_QUAD,
    EASE_OUT_BACK,      /* Geri sekme efekti */
    EASE_OUT_ELASTIC    /* Elastik efekt */
} EaseType;

/* Animasyon durumu */
typedef enum {
    ANIM_IDLE,
    ANIM_RUNNING,
    ANIM_COMPLETED
} AnimState;

/* Tek bir animasyon */
typedef struct {
    float start_value;
    float end_value;
    float current_value;
    uint32_t duration_ms;
    uint32_t elapsed_ms;
    uint32_t start_time;
    EaseType ease;
    AnimState state;
} Animation;

/* Animasyon grubu (birden fazla değer için) */
typedef struct {
    Animation x;
    Animation y;
    Animation alpha;
    Animation scale;
} AnimationGroup;

/* Global zaman değişkeni */
extern uint32_t g_current_time_ms;
extern uint32_t g_delta_time_ms;
extern uint32_t g_last_frame_time;

/* Zaman fonksiyonları */
void anim_system_init(void);
void anim_system_update(void);
uint32_t anim_get_time_ms(void);
uint32_t anim_get_delta_ms(void);

/* Tek animasyon fonksiyonları */
void anim_init(Animation *anim);
void anim_start(Animation *anim, float start, float end, uint32_t duration_ms, EaseType ease);
void anim_update(Animation *anim);
float anim_get_value(Animation *anim);
int anim_is_running(Animation *anim);
int anim_is_complete(Animation *anim);
void anim_stop(Animation *anim);
void anim_reset(Animation *anim);

/* Animasyon grubu fonksiyonları */
void anim_group_init(AnimationGroup *group);
void anim_group_update(AnimationGroup *group);
int anim_group_is_running(AnimationGroup *group);

/* Easing fonksiyonları */
float ease_linear(float t);
float ease_in_quad(float t);
float ease_out_quad(float t);
float ease_in_out_quad(float t);
float ease_out_back(float t);
float ease_out_elastic(float t);

/* Yardımcı fonksiyonlar */
float lerp(float a, float b, float t);
int lerp_int(int a, int b, float t);
uint32_t lerp_color(uint32_t c1, uint32_t c2, float t);

#endif
