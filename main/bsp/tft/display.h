#ifndef DISPLAY_H_INCLUDED
#define DISPLAY_H_INCLUDED


#include "lvgl.h"


void bsp_tft_display_lvgl_flush_cb(lv_display_t *display, const lv_area_t *area, uint8_t *px_map);
void bsp_tft_display_init(void (*display_flush_ready_cb)(void), size_t buffer_size);
void bsp_tft_display_brightness_set(uint8_t brightness_percentage);
void bsp_tft_display_reset(void);


#endif
