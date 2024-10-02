#include <stdio.h>
#include "view.h"
#include "common.h"
#include "theme/style.h"
#include "intl/intl.h"


#define SQUARE_BUTTON_SIZE 48


void view_common_set_hidden(lv_obj_t *obj, uint8_t hidden) {
    if (hidden) {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
}


void view_common_set_disabled(lv_obj_t *obj, uint8_t disabled) {
    if (disabled) {
        lv_obj_add_state(obj, LV_STATE_DISABLED);
    } else {
        lv_obj_clear_state(obj, LV_STATE_DISABLED);
    }
}


void view_common_format_alarm(lv_obj_t *label, uint16_t alarms, language_t language) {
    if ((alarms & (1 << ALARM_EMERGENCY)) > 0) {
        lv_label_set_text(label, view_intl_get_string_in_language(language, STRINGS_ALLARME_EMERGENZA));
    } else if ((alarms & (1 << ALARM_FILTER)) > 0) {
        lv_label_set_text(label, view_intl_get_string_in_language(language, STRINGS_CASSETTO_DEL_FILTRO_APERTO));
    } else if ((alarms & (1 << ALARM_PORTHOLE)) > 0) {
        lv_label_set_text(label, view_intl_get_string_in_language(language, STRINGS_OBLO_APERTO));
    }
}


password_page_options_t *view_common_default_password_page_options(pman_stack_msg_t msg, const char *password) {
    password_page_options_t *fence = (password_page_options_t *)lv_malloc(sizeof(password_page_options_t));
    assert(fence != NULL);
    fence->password = password;
    fence->msg      = msg;
    return fence;
}


view_title_t view_common_create_title(lv_obj_t *root, const char *text, int back_id, int next_id) {
    LV_IMG_DECLARE(img_door);

    lv_obj_t *btn, *lbl, *cont;

    cont = lv_obj_create(root);
    lv_obj_add_style(cont, (lv_style_t *)&style_padless_cont, LV_STATE_DEFAULT);
    lv_obj_set_size(cont, LV_HOR_RES, 56);

    btn = lv_button_create(cont);
    lv_obj_add_style(btn, (lv_style_t *)&style_config_btn, LV_STATE_DEFAULT);
    lv_obj_set_size(btn, SQUARE_BUTTON_SIZE, SQUARE_BUTTON_SIZE);
    lv_obj_t *img = lv_image_create(btn);
    lv_image_set_src(img, &img_door);
    lv_obj_center(img);
    lv_obj_align(btn, LV_ALIGN_LEFT_MID, 4, 0);
    view_register_object_default_callback(btn, back_id);
    lv_obj_t *button_back = btn;

    if (next_id >= 0) {
        btn = lv_button_create(cont);
        lv_obj_add_style(btn, (lv_style_t *)&style_config_btn, LV_STATE_DEFAULT);
        lv_obj_set_size(btn, SQUARE_BUTTON_SIZE, SQUARE_BUTTON_SIZE);
        lbl = lv_label_create(btn);
        lv_obj_set_style_text_font(lbl, STYLE_FONT_BIG, LV_STATE_DEFAULT);
        lv_label_set_text(lbl, LV_SYMBOL_RIGHT);
        lv_obj_center(lbl);
        lv_obj_align(btn, LV_ALIGN_RIGHT_MID, -4, 0);
        view_register_object_default_callback(btn, next_id);
    }

    lbl = lv_label_create(cont);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lbl, STYLE_FONT_MEDIUM, LV_STATE_DEFAULT);
    lv_label_set_text(lbl, text);

    if (next_id >= 0) {
        lv_obj_set_width(lbl, LV_HOR_RES - SQUARE_BUTTON_SIZE * 2 - 8);
        lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
    } else {
        lv_obj_set_width(lbl, LV_HOR_RES - SQUARE_BUTTON_SIZE - 8);
        lv_obj_align(lbl, LV_ALIGN_RIGHT_MID, 0, 0);
    }

    return (view_title_t){.label_title = lbl, .button_back = button_back, .obj_title = cont};
}


void view_common_image_set_src(lv_obj_t *img, const lv_image_dsc_t *img_dsc) {
    if (lv_image_get_src(img) != img_dsc) {
        lv_image_set_src(img, img_dsc);
    }
}


communication_error_popup_t view_common_communication_error_popup(lv_obj_t *parent) {
    lv_obj_t *blanket = lv_obj_create(parent);
    lv_obj_add_style(blanket, &style_transparent_cont, LV_STATE_DEFAULT);
    lv_obj_set_size(blanket, LV_PCT(100), LV_PCT(100));

    lv_obj_t *cont = lv_obj_create(blanket);
    lv_obj_set_size(cont, LV_PCT(90), LV_PCT(90));
    lv_obj_center(cont);

    lv_obj_t *lbl_msg = lv_label_create(cont);
    lv_label_set_long_mode(lbl_msg, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl_msg, LV_PCT(95));
    // lv_label_set_text(lbl_msg, view_intl_get_string(model, STRINGS_ERRORE_DI_COMUNICAZIONE));
    lv_obj_align(lbl_msg, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t *btn = lv_button_create(cont);
    lv_obj_set_size(btn, 96, 64);
    lv_obj_t *lbl_retry = lv_label_create(btn);
    // lv_label_set_text(lbl_retry, view_intl_get_string(model, STRINGS_RIPROVA));
    lv_obj_center(lbl_retry);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, 0);

    return (communication_error_popup_t){
        .blanket   = blanket,
        .btn_retry = btn,
        .lbl_msg   = lbl_msg,
        .lbl_retry = lbl_retry,
    };
}
