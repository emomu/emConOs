/* filemgr.h - File Manager UI */
#ifndef FILEMGR_H
#define FILEMGR_H

#include "../types.h"

/* Dosya yöneticisi durumu */
typedef enum {
    FM_STATE_LOADING,
    FM_STATE_READY,
    FM_STATE_ERROR
} FileMgrState;

/* Fonksiyonlar */
void filemgr_init(void);
void filemgr_draw(void);
void filemgr_handle_input(int key);

/* Navigasyon */
void filemgr_up(void);
void filemgr_down(void);
void filemgr_enter(void);
void filemgr_back(void);

/* Durum */
FileMgrState filemgr_get_state(void);
int filemgr_get_file_count(void);
int filemgr_get_selected(void);

#endif
