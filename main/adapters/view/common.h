#ifndef VIEW_COMMON_H_INCLUDED
#define VIEW_COMMON_H_INCLUDED


#include <stdint.h>
#include "lvgl.h"
#include "view.h"


#define MENU_BUTTON_WIDTH LV_PCT(48);


typedef struct {
    const char      *password;
    pman_stack_msg_t msg;
} password_page_options_t;


typedef struct {
    lv_obj_t *blanket;

    lv_obj_t *lbl_msg;
    lv_obj_t *lbl_retry;
    lv_obj_t *lbl_disable;
    lv_obj_t *btn_retry;
    lv_obj_t *btn_disable;
} communication_error_popup_t;


typedef struct {
    lv_obj_t *obj_title;
    lv_obj_t *label_title;
    lv_obj_t *button_back;
    lv_obj_t *button_next;
} view_title_t;


typedef struct {
    program_step_type_t type;
    uint8_t             changed;
    uint16_t            program_index;
} step_modification_t;


typedef struct {
    lv_obj_t *blanket;

    lv_obj_t *lbl_description;

    lv_obj_t *btn_cancel;
    lv_obj_t *btn_ok;
} popup_t;


void                        view_common_set_hidden(lv_obj_t *obj, uint8_t hidden);
password_page_options_t    *view_common_default_password_page_options(pman_stack_msg_t msg, const char *password);
view_title_t                view_common_create_title(lv_obj_t *root, const char *text, int back_id, int next_id);
void                        view_common_image_set_src(lv_obj_t *img, const lv_image_dsc_t *img_dsc);
communication_error_popup_t view_common_communication_error_popup(lv_obj_t *parent);
void                        view_common_set_disabled(lv_obj_t *obj, uint8_t disabled);
void                        view_common_format_alarm(lv_obj_t *label, model_t *model, language_t language);
popup_t                     view_common_popup_create(lv_obj_t *parent, const char *text, int ok_id, int cancel_id);
popup_t                     view_common_alarm_popup_create(lv_obj_t *parent, int id);
void                        view_common_alarm_popup_update(model_t *model, popup_t *alarm_popup, uint16_t language);
lv_obj_t                   *view_common_icon_button_create(lv_obj_t *parent, const char *icon, int id);
const pman_page_t          *view_common_main_page(model_t *model);
const char                 *view_common_step2str(model_t *model, program_step_type_t type);


#endif
