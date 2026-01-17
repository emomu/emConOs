/* transition.c - Screen transition effects implementation */
#include <ui/transition.h>
#include <ui/theme.h>
#include <graphics.h>

/* Global geçiş durumu */
Transition g_transition;

void transition_init(void) {
    g_transition.type = TRANS_NONE;
    g_transition.state = TRANS_STATE_IDLE;
    g_transition.from_screen = 0;
    g_transition.to_screen = 0;
    g_transition.duration_ms = TRANS_DURATION_NORMAL;
    g_transition.fade_alpha = 0;
    g_transition.offset_x = 0;
    g_transition.offset_y = 0;
    g_transition.scale = 1.0f;
    anim_init(&g_transition.anim);
}

void transition_start(TransitionType type, int from_screen, int to_screen) {
    transition_start_custom(type, from_screen, to_screen, TRANS_DURATION_NORMAL);
}

void transition_start_custom(TransitionType type, int from_screen, int to_screen, uint32_t duration_ms) {
    g_transition.type = type;
    g_transition.from_screen = from_screen;
    g_transition.to_screen = to_screen;
    g_transition.duration_ms = duration_ms;

    if(type == TRANS_NONE) {
        g_transition.state = TRANS_STATE_COMPLETE;
        return;
    }

    /* Çıkış animasyonu başlat */
    g_transition.state = TRANS_STATE_OUT;

    /* Animasyon başlangıç değerleri ayarla */
    switch(type) {
        case TRANS_FADE:
            /* 0 -> 255 -> 0 (ortada siyah) */
            anim_start(&g_transition.anim, 0.0f, 255.0f, duration_ms / 2, EASE_OUT_QUAD);
            break;

        case TRANS_SLIDE_LEFT:
            anim_start(&g_transition.anim, 0.0f, (float)(-SCREEN_WIDTH), duration_ms / 2, EASE_OUT_QUAD);
            break;

        case TRANS_SLIDE_RIGHT:
            anim_start(&g_transition.anim, 0.0f, (float)SCREEN_WIDTH, duration_ms / 2, EASE_OUT_QUAD);
            break;

        case TRANS_SLIDE_UP:
            anim_start(&g_transition.anim, 0.0f, (float)(-SCREEN_HEIGHT), duration_ms / 2, EASE_OUT_QUAD);
            break;

        case TRANS_SLIDE_DOWN:
            anim_start(&g_transition.anim, 0.0f, (float)SCREEN_HEIGHT, duration_ms / 2, EASE_OUT_QUAD);
            break;

        case TRANS_ZOOM_IN:
            anim_start(&g_transition.anim, 1.0f, 0.0f, duration_ms / 2, EASE_IN_QUAD);
            break;

        case TRANS_ZOOM_OUT:
            anim_start(&g_transition.anim, 1.0f, 2.0f, duration_ms / 2, EASE_IN_QUAD);
            break;

        case TRANS_WIPE_LEFT:
            anim_start(&g_transition.anim, 0.0f, (float)SCREEN_WIDTH, duration_ms, EASE_IN_OUT_QUAD);
            break;

        case TRANS_WIPE_RIGHT:
            anim_start(&g_transition.anim, (float)SCREEN_WIDTH, 0.0f, duration_ms, EASE_IN_OUT_QUAD);
            break;

        default:
            g_transition.state = TRANS_STATE_COMPLETE;
            break;
    }
}

void transition_update(void) {
    if(g_transition.state == TRANS_STATE_IDLE || g_transition.state == TRANS_STATE_COMPLETE) {
        return;
    }

    /* Animasyonu güncelle */
    anim_update(&g_transition.anim);

    /* Değerleri güncelle */
    float value = anim_get_value(&g_transition.anim);

    switch(g_transition.type) {
        case TRANS_FADE:
            g_transition.fade_alpha = (uint8_t)value;
            break;

        case TRANS_SLIDE_LEFT:
        case TRANS_SLIDE_RIGHT:
        case TRANS_WIPE_LEFT:
        case TRANS_WIPE_RIGHT:
            g_transition.offset_x = (int)value;
            break;

        case TRANS_SLIDE_UP:
        case TRANS_SLIDE_DOWN:
            g_transition.offset_y = (int)value;
            break;

        case TRANS_ZOOM_IN:
        case TRANS_ZOOM_OUT:
            g_transition.scale = value;
            break;

        default:
            break;
    }

    /* Animasyon tamamlandı mı? */
    if(anim_is_complete(&g_transition.anim)) {
        if(g_transition.state == TRANS_STATE_OUT) {
            /* Çıkış tamamlandı, giriş başlat */
            g_transition.state = TRANS_STATE_IN;

            switch(g_transition.type) {
                case TRANS_FADE:
                    anim_start(&g_transition.anim, 255.0f, 0.0f,
                              g_transition.duration_ms / 2, EASE_IN_QUAD);
                    break;

                case TRANS_SLIDE_LEFT:
                    anim_start(&g_transition.anim, (float)SCREEN_WIDTH, 0.0f,
                              g_transition.duration_ms / 2, EASE_OUT_BACK);
                    break;

                case TRANS_SLIDE_RIGHT:
                    anim_start(&g_transition.anim, (float)(-SCREEN_WIDTH), 0.0f,
                              g_transition.duration_ms / 2, EASE_OUT_BACK);
                    break;

                case TRANS_SLIDE_UP:
                    anim_start(&g_transition.anim, (float)SCREEN_HEIGHT, 0.0f,
                              g_transition.duration_ms / 2, EASE_OUT_BACK);
                    break;

                case TRANS_SLIDE_DOWN:
                    anim_start(&g_transition.anim, (float)(-SCREEN_HEIGHT), 0.0f,
                              g_transition.duration_ms / 2, EASE_OUT_BACK);
                    break;

                case TRANS_ZOOM_IN:
                    anim_start(&g_transition.anim, 0.0f, 1.0f,
                              g_transition.duration_ms / 2, EASE_OUT_BACK);
                    break;

                case TRANS_ZOOM_OUT:
                    anim_start(&g_transition.anim, 2.0f, 1.0f,
                              g_transition.duration_ms / 2, EASE_OUT_QUAD);
                    break;

                case TRANS_WIPE_LEFT:
                case TRANS_WIPE_RIGHT:
                    /* Wipe tek aşamalı */
                    g_transition.state = TRANS_STATE_COMPLETE;
                    break;

                default:
                    g_transition.state = TRANS_STATE_COMPLETE;
                    break;
            }
        } else if(g_transition.state == TRANS_STATE_IN) {
            /* Giriş tamamlandı */
            g_transition.state = TRANS_STATE_COMPLETE;
            g_transition.offset_x = 0;
            g_transition.offset_y = 0;
            g_transition.fade_alpha = 0;
            g_transition.scale = 1.0f;
        }
    }
}

int transition_is_active(void) {
    return g_transition.state == TRANS_STATE_OUT || g_transition.state == TRANS_STATE_IN;
}

int transition_is_complete(void) {
    return g_transition.state == TRANS_STATE_COMPLETE;
}

TransitionState transition_get_state(void) {
    return g_transition.state;
}

int transition_get_offset_x(void) {
    return g_transition.offset_x;
}

int transition_get_offset_y(void) {
    return g_transition.offset_y;
}

uint8_t transition_get_alpha(void) {
    return g_transition.fade_alpha;
}

float transition_get_scale(void) {
    return g_transition.scale;
}

void transition_draw_fade_overlay(void) {
    if(g_transition.type != TRANS_FADE || g_transition.fade_alpha == 0) {
        return;
    }

    /* Basit alpha blend için her satırı çiz (draw_buffer kullan) */
    for(int y = 0; y < SCREEN_HEIGHT; y++) {
        for(int x = 0; x < SCREEN_WIDTH; x++) {
            /* Mevcut piksel rengini al ve karıştır */
            uint32_t offset = (y * SCREEN_WIDTH * 4) + (x * 4);
            uint32_t *pixel_addr = (uint32_t *)(draw_buffer + offset);
            uint32_t current = *pixel_addr;

            /* Alpha blend */
            uint8_t alpha = g_transition.fade_alpha;
            uint8_t inv_alpha = 255 - alpha;

            uint8_t r = (uint8_t)(((current >> 16) & 0xFF) * inv_alpha / 255);
            uint8_t g = (uint8_t)(((current >> 8) & 0xFF) * inv_alpha / 255);
            uint8_t b = (uint8_t)((current & 0xFF) * inv_alpha / 255);

            *pixel_addr = 0xFF000000 | (r << 16) | (g << 8) | b;
        }
    }
}

void transition_cancel(void) {
    g_transition.state = TRANS_STATE_IDLE;
    g_transition.offset_x = 0;
    g_transition.offset_y = 0;
    g_transition.fade_alpha = 0;
    g_transition.scale = 1.0f;
    anim_stop(&g_transition.anim);
}
