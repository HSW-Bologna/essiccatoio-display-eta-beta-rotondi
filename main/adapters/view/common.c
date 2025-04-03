#include <stdio.h>
#include "view.h"
#include "common.h"
#include "theme/style.h"
#include "intl/intl.h"


static const char *get_alarm_description(uint32_t alarms, uint16_t language, alarm_t *alarm_code);


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


void view_common_format_alarm(lv_obj_t *label, uint32_t alarms, language_t language) {
    lv_label_set_text(label, get_alarm_description(alarms, language, NULL));
}


lv_obj_t *view_common_icon_button_create_with_number(lv_obj_t *parent, const char *icon, int id, int number) {
    const int32_t button_height = 48;

    lv_obj_t *button = lv_button_create(parent);
    lv_obj_add_style(button, &style_icon_button, LV_STATE_DEFAULT);
    lv_obj_set_size(button, button_height, button_height);

    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, icon);
    lv_obj_center(label);

    view_register_object_default_callback_with_number(button, id, number);
    return button;
}


lv_obj_t *view_common_icon_button_create(lv_obj_t *parent, const char *icon, int id) {
    return view_common_icon_button_create_with_number(parent, icon, id, 0);
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
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, LV_STATE_DEFAULT);
    lv_obj_set_size(btn, SQUARE_BUTTON_SIZE, SQUARE_BUTTON_SIZE);
    lv_obj_t *img = lv_image_create(btn);
    lv_image_set_src(img, &img_door);
    lv_obj_add_style(img, &style_white_icon, LV_STATE_DEFAULT);
    lv_obj_center(img);
    lv_obj_align(btn, LV_ALIGN_LEFT_MID, 4, 0);
    view_register_object_default_callback(btn, back_id);
    lv_obj_t *button_back = btn;

    lv_obj_t *button_next = NULL;
    if (next_id >= 0) {
        button_next = lv_button_create(cont);
        lv_obj_add_style(button_next, (lv_style_t *)&style_config_btn, LV_STATE_DEFAULT);
        lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, LV_STATE_DEFAULT);
        lv_obj_set_size(button_next, SQUARE_BUTTON_SIZE, SQUARE_BUTTON_SIZE);
        lbl = lv_label_create(button_next);
        lv_obj_set_style_text_font(lbl, STYLE_FONT_BIG, LV_STATE_DEFAULT);
        lv_label_set_text(lbl, LV_SYMBOL_RIGHT);
        lv_obj_center(lbl);
        lv_obj_align(button_next, LV_ALIGN_RIGHT_MID, -4, 0);
        view_register_object_default_callback(button_next, next_id);
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

    return (view_title_t){
        .label_title = lbl, .button_back = button_back, .button_next = button_next, .obj_title = cont};
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
    lv_obj_set_size(btn, 104, 48);
    lv_obj_t *lbl_retry = lv_label_create(btn);
    // lv_label_set_text(lbl_retry, view_intl_get_string(model, STRINGS_RIPROVA));
    lv_obj_center(lbl_retry);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_obj_t *btn_disable = lv_button_create(cont);
    lv_obj_set_size(btn_disable, 104, 48);
    lv_obj_t *lbl_disable = lv_label_create(btn_disable);
    lv_obj_center(lbl_disable);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, 0);

    return (communication_error_popup_t){
        .blanket     = blanket,
        .btn_retry   = btn,
        .lbl_msg     = lbl_msg,
        .lbl_retry   = lbl_retry,
        .lbl_disable = lbl_disable,
        .btn_disable = btn_disable,
    };
}


void view_common_alarm_popup_update(uint32_t alarms, popup_t *alarm_popup, uint16_t language) {
    alarm_t     alarm_code  = 0;
    const char *description = get_alarm_description(alarms, language, &alarm_code);
    lv_label_set_text_fmt(alarm_popup->lbl_description, "%s\n\n%s: %i", description,
                          view_intl_get_string_in_language(language, STRINGS_CODICE), alarm_code);
}


popup_t view_common_alarm_popup_create(lv_obj_t *parent, int id) {
    return view_common_popup_create(parent, "", id, -1);
}


popup_t view_common_popup_create(lv_obj_t *parent, const char *text, int ok_id, int cancel_id) {
    lv_obj_t *blanket = lv_obj_create(parent);
    lv_obj_add_style(blanket, &style_transparent_cont, LV_STATE_DEFAULT);
    lv_obj_set_size(blanket, LV_PCT(100), LV_PCT(100));

    lv_obj_t *cont = lv_obj_create(blanket);
    lv_obj_set_size(cont, LV_PCT(90), LV_PCT(90));
    lv_obj_center(cont);
    lv_obj_set_style_radius(cont, 48, LV_STATE_DEFAULT);

    lv_obj_t *lbl_msg = lv_label_create(cont);
    lv_obj_set_style_text_align(lbl_msg, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lbl_msg, STYLE_FONT_SMALL, LV_STATE_DEFAULT);
    lv_label_set_long_mode(lbl_msg, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl_msg, LV_PCT(95));
    lv_label_set_text(lbl_msg, text);
    lv_obj_align(lbl_msg, LV_ALIGN_CENTER, 0, -32);

    lv_obj_t *btn_cancel = NULL;
    if (cancel_id >= 0) {
        lv_obj_t *btn = lv_button_create(cont);
        lv_obj_set_size(btn, 64, 48);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_obj_set_style_text_font(lbl, STYLE_FONT_SMALL, LV_STATE_DEFAULT);
        lv_label_set_text(lbl, LV_SYMBOL_CLOSE);
        lv_obj_center(lbl);
        lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 0, 0);
        view_register_object_default_callback(btn, cancel_id);
        btn_cancel = btn;
    }

    lv_obj_t *btn = lv_button_create(cont);
    lv_obj_set_size(btn, 64, 48);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_obj_set_style_text_font(lbl, STYLE_FONT_SMALL, LV_STATE_DEFAULT);
    lv_label_set_text(lbl, LV_SYMBOL_OK);
    lv_obj_center(lbl);
    if (cancel_id >= 0) {
        lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    } else {
        lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, 0);
    }
    view_register_object_default_callback(btn, ok_id);

    return (popup_t){
        .blanket         = blanket,
        .btn_ok          = btn,
        .btn_cancel      = btn_cancel,
        .lbl_description = lbl_msg,
    };
}


const pman_page_t *view_common_main_page(model_t *model) {
    (void)model;
    return &page_main_demo;
}


const char *view_common_step2str(model_t *model, program_step_type_t type) {
    strings_t step_names[] = {STRINGS_ASCIUGATURA, STRINGS_RAFFREDDAMENTO, STRINGS_ANTIPIEGA};
    return view_intl_get_string(model, step_names[type]);
}


const char *view_common_modello_str(model_t *model) {
    const char *strings_modello[] = {
        "TEST MODEL",       "EDS RE SELF C.A.", "EDS RE LAB  C.A.", "EDS RG SELF C.A.", "EDS RG LAB  C.A.",
        "EDS RV SELF C.A.", "EDS RV LAB  C.A.", "EDS RE LAB TH CA", "EDS RG LAB TH CA", "EDS RV LAB TH CA",
    };

    return strings_modello[model->config.parmac.machine_model];
}


static const char *get_alarm_description(uint32_t alarms, uint16_t language, alarm_t *alarm_code) {
    strings_t string_codes[] = {
        STRINGS_OBLO_APERTO,
        STRINGS_ALLARME_EMERGENZA,
        STRINGS_CASSETTO_DEL_FILTRO_APERTO,
        STRINGS_FLUSSO_D_ARIA_ASSENTE,
        STRINGS_BLOCCO_BRUCIATORE,
        STRINGS_SURRISCALDAMENTO,
        STRINGS_TEMPERATURA_NON_RAGGIUNTA_IN_TEMPO,
        STRINGS_ALLARME_INVERTER,
    };
    for (alarm_t code = 0; code < ALARMS_NUM; code++) {
        if (alarms & (1 << code)) {
            if (alarm_code) {
                *alarm_code = code;
            }
            return view_intl_get_string_in_language(language, string_codes[code]);
        }
    }
    return view_intl_get_string_in_language(language, STRINGS_ALLARME);
}
