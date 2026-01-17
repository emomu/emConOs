/* animation.c - Animation system implementation */
#include <ui/animation.h>
#include <drivers/timer.h>

/* Global zaman değişkenleri */
uint32_t g_current_time_ms = 0;
uint32_t g_delta_time_ms = 0;
uint32_t g_last_frame_time = 0;

/* Animasyon sistemi başlat */
void anim_system_init(void) {
    g_current_time_ms = timer_get_ms();
    g_last_frame_time = g_current_time_ms;
    g_delta_time_ms = 0;
}

/* Her frame güncelle */
void anim_system_update(void) {
    uint32_t now = timer_get_ms();
    g_delta_time_ms = now - g_last_frame_time;
    g_last_frame_time = now;
    g_current_time_ms = now;

    /* Delta time sınırla (100ms max - lag durumları için) */
    if(g_delta_time_ms > 100) {
        g_delta_time_ms = 100;
    }
}

uint32_t anim_get_time_ms(void) {
    return g_current_time_ms;
}

uint32_t anim_get_delta_ms(void) {
    return g_delta_time_ms;
}

/* --- Easing Fonksiyonları --- */

float ease_linear(float t) {
    return t;
}

float ease_in_quad(float t) {
    return t * t;
}

float ease_out_quad(float t) {
    return t * (2.0f - t);
}

float ease_in_out_quad(float t) {
    if(t < 0.5f) {
        return 2.0f * t * t;
    }
    return -1.0f + (4.0f - 2.0f * t) * t;
}

float ease_out_back(float t) {
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    float tm1 = t - 1.0f;
    return 1.0f + c3 * tm1 * tm1 * tm1 + c1 * tm1 * tm1;
}

float ease_out_elastic(float t) {
    if(t == 0.0f) return 0.0f;
    if(t == 1.0f) return 1.0f;

    const float c4 = (2.0f * 3.14159f) / 3.0f;
    float power = -10.0f * t;

    /* Basit pow yaklaşımı */
    float pow2 = 1.0f;
    for(int i = 0; i < (int)(-power); i++) {
        pow2 *= 2.0f;
    }
    pow2 = 1.0f / pow2;

    /* sin yaklaşımı (Taylor serisi) */
    float angle = (t * 10.0f - 0.75f) * c4;
    float sin_val = angle - (angle * angle * angle) / 6.0f;

    return pow2 * sin_val + 1.0f;
}

/* Easing uygula */
static float apply_ease(float t, EaseType ease) {
    switch(ease) {
        case EASE_LINEAR:      return ease_linear(t);
        case EASE_IN_QUAD:     return ease_in_quad(t);
        case EASE_OUT_QUAD:    return ease_out_quad(t);
        case EASE_IN_OUT_QUAD: return ease_in_out_quad(t);
        case EASE_OUT_BACK:    return ease_out_back(t);
        case EASE_OUT_ELASTIC: return ease_out_elastic(t);
        default:               return ease_linear(t);
    }
}

/* --- Tek Animasyon Fonksiyonları --- */

void anim_init(Animation *anim) {
    anim->start_value = 0.0f;
    anim->end_value = 0.0f;
    anim->current_value = 0.0f;
    anim->duration_ms = 0;
    anim->elapsed_ms = 0;
    anim->start_time = 0;
    anim->ease = EASE_LINEAR;
    anim->state = ANIM_IDLE;
}

void anim_start(Animation *anim, float start, float end, uint32_t duration_ms, EaseType ease) {
    anim->start_value = start;
    anim->end_value = end;
    anim->current_value = start;
    anim->duration_ms = duration_ms;
    anim->elapsed_ms = 0;
    anim->start_time = g_current_time_ms;
    anim->ease = ease;
    anim->state = ANIM_RUNNING;
}

void anim_update(Animation *anim) {
    if(anim->state != ANIM_RUNNING) {
        return;
    }

    anim->elapsed_ms = g_current_time_ms - anim->start_time;

    if(anim->elapsed_ms >= anim->duration_ms) {
        anim->elapsed_ms = anim->duration_ms;
        anim->current_value = anim->end_value;
        anim->state = ANIM_COMPLETED;
        return;
    }

    /* Progress hesapla (0.0 - 1.0) */
    float t = (float)anim->elapsed_ms / (float)anim->duration_ms;

    /* Easing uygula */
    float eased_t = apply_ease(t, anim->ease);

    /* Değeri interpolate et */
    anim->current_value = anim->start_value + (anim->end_value - anim->start_value) * eased_t;
}

float anim_get_value(Animation *anim) {
    return anim->current_value;
}

int anim_is_running(Animation *anim) {
    return anim->state == ANIM_RUNNING;
}

int anim_is_complete(Animation *anim) {
    return anim->state == ANIM_COMPLETED;
}

void anim_stop(Animation *anim) {
    anim->state = ANIM_IDLE;
}

void anim_reset(Animation *anim) {
    anim->current_value = anim->start_value;
    anim->elapsed_ms = 0;
    anim->state = ANIM_IDLE;
}

/* --- Animasyon Grubu --- */

void anim_group_init(AnimationGroup *group) {
    anim_init(&group->x);
    anim_init(&group->y);
    anim_init(&group->alpha);
    anim_init(&group->scale);
}

void anim_group_update(AnimationGroup *group) {
    anim_update(&group->x);
    anim_update(&group->y);
    anim_update(&group->alpha);
    anim_update(&group->scale);
}

int anim_group_is_running(AnimationGroup *group) {
    return anim_is_running(&group->x) ||
           anim_is_running(&group->y) ||
           anim_is_running(&group->alpha) ||
           anim_is_running(&group->scale);
}

/* --- Yardımcı Fonksiyonlar --- */

float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

int lerp_int(int a, int b, float t) {
    return (int)(a + (b - a) * t);
}

uint32_t lerp_color(uint32_t c1, uint32_t c2, float t) {
    uint8_t a1 = (c1 >> 24) & 0xFF;
    uint8_t r1 = (c1 >> 16) & 0xFF;
    uint8_t g1 = (c1 >> 8) & 0xFF;
    uint8_t b1 = c1 & 0xFF;

    uint8_t a2 = (c2 >> 24) & 0xFF;
    uint8_t r2 = (c2 >> 16) & 0xFF;
    uint8_t g2 = (c2 >> 8) & 0xFF;
    uint8_t b2 = c2 & 0xFF;

    uint8_t a = (uint8_t)lerp_int(a1, a2, t);
    uint8_t r = (uint8_t)lerp_int(r1, r2, t);
    uint8_t g = (uint8_t)lerp_int(g1, g2, t);
    uint8_t b = (uint8_t)lerp_int(b1, b2, t);

    return (a << 24) | (r << 16) | (g << 8) | b;
}
