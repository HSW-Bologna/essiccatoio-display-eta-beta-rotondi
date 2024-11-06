#include "lvgl.h"
#include "style.h"

static const lv_style_const_prop_t style_transparent_cont_props[] = {
    LV_STYLE_CONST_PAD_BOTTOM(0), LV_STYLE_CONST_PAD_TOP(0),      LV_STYLE_CONST_PAD_LEFT(0),
    LV_STYLE_CONST_PAD_RIGHT(0),  LV_STYLE_CONST_BORDER_WIDTH(0), LV_STYLE_CONST_BG_OPA(LV_OPA_TRANSP),
    LV_STYLE_CONST_PROPS_END,
};
LV_STYLE_CONST_INIT(style_transparent_cont, (void *)style_transparent_cont_props);


static const lv_style_const_prop_t style_padless_cont_props[] = {
    LV_STYLE_CONST_PAD_BOTTOM(0), LV_STYLE_CONST_PAD_TOP(0),    LV_STYLE_CONST_PAD_LEFT(0), LV_STYLE_CONST_PAD_RIGHT(0),
    LV_STYLE_CONST_PAD_ROW(0),    LV_STYLE_CONST_PAD_COLUMN(0), LV_STYLE_CONST_PROPS_END,
};
LV_STYLE_CONST_INIT(style_padless_cont, (void *)style_padless_cont_props);


static const lv_style_const_prop_t style_borderless_cont_props[] = {
    LV_STYLE_CONST_BORDER_WIDTH(0),
    LV_STYLE_CONST_RADIUS(0),
    LV_STYLE_CONST_PROPS_END,
};
LV_STYLE_CONST_INIT(style_borderless_cont, (void *)style_borderless_cont_props);


static const lv_style_const_prop_t style_black_border_props[] = {
    LV_STYLE_CONST_BORDER_WIDTH(3),
    LV_STYLE_CONST_BORDER_COLOR(VIEW_STYLE_COLOR_BLACK),
    LV_STYLE_CONST_RADIUS(16),
    LV_STYLE_CONST_PROPS_END,
};
LV_STYLE_CONST_INIT(style_black_border, (void *)style_black_border_props);

static const lv_style_const_prop_t style_config_btn_props[] = {
    LV_STYLE_CONST_BG_COLOR(STYLE_PRIMARY_DARKER),
    LV_STYLE_CONST_PROPS_END,
};
LV_STYLE_CONST_INIT(style_config_btn, (void *)style_config_btn_props);


lv_style_t style_tall_button         = {0};
lv_style_t style_tall_button_checked = {0};


void style_init(void) {
    lv_style_init(&style_tall_button);
    lv_style_set_radius(&style_tall_button, 0);
    lv_style_set_text_color(&style_tall_button, VIEW_STYLE_COLOR_BLACK);
    lv_style_set_bg_color(&style_tall_button, VIEW_STYLE_COLOR_WHITE);
    lv_style_set_border_width(&style_tall_button, 2);
    lv_style_set_border_color(&style_tall_button, lv_color_darken(VIEW_STYLE_COLOR_BACKGROUND, LV_OPA_20));

    lv_style_init(&style_tall_button_checked);
    lv_style_set_bg_color(&style_tall_button_checked, VIEW_STYLE_COLOR_WHITE);
    lv_style_set_text_color(&style_tall_button_checked, VIEW_STYLE_COLOR_BLACK);
    lv_style_set_bg_color(&style_tall_button_checked, lv_color_hex(0xbdbdbd));
    lv_style_set_border_width(&style_tall_button_checked, 4);
    lv_style_set_border_color(&style_tall_button_checked, lv_color_hex(0xf79410));
}
