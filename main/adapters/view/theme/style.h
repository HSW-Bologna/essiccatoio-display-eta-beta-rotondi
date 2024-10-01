#ifndef STYLE_H_INCLUDED
#define STYLE_H_INCLUDED

#include "lvgl.h"


#define STYLE_FONT_SMALL  (&lv_font_montserrat_16)
#define STYLE_FONT_MEDIUM (&lv_font_montserrat_24)
#define STYLE_FONT_BIG    (&lv_font_montserrat_48)
#define STYLE_FONT_HUGE   (&font_montserrat_48)

#define VIEW_STYLE_PRIMARY     (lv_color_darken(((lv_color_t)lv_color_make(0x9e, 0xc1, 0xe7)), LV_OPA_30))
#define VIEW_STYLE_SECONDARY   ((lv_color_t)LV_COLOR_MAKE(0x0, 0x0, 0x0))
#define VIEW_STYLE_COLOR_RED   ((lv_color_t)LV_COLOR_MAKE(0xFF, 0x0, 0x0))
#define VIEW_STYLE_COLOR_BLACK ((lv_color_t)LV_COLOR_MAKE(0x0, 0x0, 0x0))
#define VIEW_STYLE_COLOR_GREEN ((lv_color_t)LV_COLOR_MAKE(0x0, 0xFF, 0x0))
#define VIEW_STYLE_COLOR_BACKGROUND    ((lv_color_t)LV_COLOR_MAKE(0x6B, 0x6B, 0x6B))
#define STYLE_PRIMARY_DARKER   ((lv_color_t)LV_COLOR_MAKE(0x38, 0x6A, 0x80))


extern const lv_style_t style_transparent_cont;
extern const lv_style_t style_padless_cont;
extern const lv_style_t style_borderless_cont;
extern const lv_style_t style_black_border;
extern const lv_style_t style_config_btn;


void style_init(void);

#endif
