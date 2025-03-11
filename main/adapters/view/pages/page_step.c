#include <inttypes.h>
#include "lvgl.h"
#include "model/model.h"
#include "../view.h"
#include "../theme/style.h"
#include "../common.h"
#include "config/app_config.h"
#include "src/widgets/led/lv_led.h"
#include "../intl/intl.h"
#include "model/parlav.h"


struct page_data {
    lv_obj_t *label_number;
    lv_obj_t *label_description;
    lv_obj_t *label_value;

    uint16_t parameter;
    uint16_t num_parameters;

    step_modification_t *meta;

    pman_timer_t *timer;
};


enum {
    BTN_BACK_ID,
    BTN_LEFT_ID,
    BTN_RIGHT_ID,
    BTN_MINUS_ID,
    BTN_PLUS_ID,
};


static void update_page(model_t *model, struct page_data *pdata);


static void *create_page(pman_handle_t handle, void *extra) {
    model_t *model = view_get_model(handle);

    struct page_data *pdata = lv_malloc(sizeof(struct page_data));
    assert(pdata != NULL);

    pdata->timer = PMAN_REGISTER_TIMER_ID(handle, model->config.parmac.reset_page_time * 1000UL, 0);
    pdata->meta  = extra;

    return pdata;
}


static void open_page(pman_handle_t handle, void *state) {
    struct page_data *pdata = state;

    pman_timer_reset(pdata->timer);
    pman_timer_resume(pdata->timer);

    model_t *model = view_get_model(handle);

    view_title_t title =
        view_common_create_title(lv_scr_act(), view_common_step2str(model, pdata->meta->type), BTN_BACK_ID, -1);

    lv_obj_t *cont = lv_obj_create(lv_scr_act());
    lv_obj_set_size(cont, LV_HOR_RES, LV_VER_RES - 56);
    lv_obj_align(cont, LV_ALIGN_BOTTOM_MID, 0, 0);

    pdata->num_parameters = parlav_get_tot_parameters(model->config.parmac.access_level);
    pdata->parameter      = 0;

    pdata->label_number = title.label_title;

    lv_obj_t *ldesc = lv_label_create(cont);
    lv_obj_set_style_text_align(ldesc, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
    lv_label_set_long_mode(ldesc, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(ldesc, LV_PCT(95));
    lv_obj_align(ldesc, LV_ALIGN_TOP_MID, 0, 64);
    pdata->label_description = ldesc;

    {
        lv_obj_t *btn = lv_button_create(cont);
        lv_obj_set_size(btn, 96, 48);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, LV_SYMBOL_LEFT);
        lv_obj_center(lbl);
        view_register_object_default_callback(btn, BTN_LEFT_ID);
        lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 0, 0);
    }

    {
        lv_obj_t *btn = lv_button_create(cont);
        lv_obj_set_size(btn, 96, 48);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, LV_SYMBOL_RIGHT);
        lv_obj_center(lbl);
        view_register_object_default_callback(btn, BTN_RIGHT_ID);
        lv_obj_align(btn, LV_ALIGN_TOP_RIGHT, 0, 0);
    }

    lv_obj_t *lval = lv_label_create(cont);
    lv_obj_set_style_text_align(lval, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lval, STYLE_FONT_MEDIUM, LV_STATE_DEFAULT);
    lv_label_set_long_mode(lval, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lval, LV_PCT(95));
    lv_obj_align(lval, LV_ALIGN_BOTTOM_MID, 0, -64);
    pdata->label_value = lval;

    {
        lv_obj_t *btn = lv_button_create(cont);
        lv_obj_set_size(btn, 96, 48);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, LV_SYMBOL_MINUS);
        lv_obj_center(lbl);
        view_register_object_default_callback(btn, BTN_MINUS_ID);
        lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    }

    {
        lv_obj_t *btn = lv_button_create(cont);
        lv_obj_set_size(btn, 96, 48);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, LV_SYMBOL_PLUS);
        lv_obj_center(lbl);
        view_register_object_default_callback(btn, BTN_PLUS_ID);
        lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    }

    update_page(model, pdata);
}


static pman_msg_t page_event(pman_handle_t handle, void *state, pman_event_t event) {
    struct page_data *pdata = state;

    mut_model_t *model = view_get_model(handle);

    pman_msg_t msg = PMAN_MSG_NULL;

    switch (event.tag) {
        case PMAN_EVENT_TAG_OPEN:
            break;

        case PMAN_EVENT_TAG_TIMER: {
            msg.stack_msg = PMAN_STACK_MSG_REBASE(view_common_main_page(model));
            break;
        }

        case PMAN_EVENT_TAG_USER: {
            view_event_t *view_event = event.as.user;
            switch (view_event->tag) {
                case VIEW_EVENT_TAG_PAGE_WATCHER: {
                    update_page(model, pdata);
                    break;
                }

                default:
                    break;
            }

            break;
        }

        case PMAN_EVENT_TAG_LVGL: {
            lv_obj_t           *target   = lv_event_get_current_target_obj(event.as.lvgl);
            view_object_data_t *obj_data = lv_obj_get_user_data(target);

            switch (lv_event_get_code(event.as.lvgl)) {
                case LV_EVENT_CLICKED: {
                    pman_timer_reset(pdata->timer);

                    switch (obj_data->id) {
                        case BTN_BACK_ID:
                            msg.stack_msg = PMAN_STACK_MSG_BACK();
                            break;

                        case BTN_LEFT_ID:
                            if (pdata->parameter > 0) {
                                pdata->parameter--;
                            } else {
                                pdata->parameter = pdata->num_parameters - 1;
                            }
                            update_page(model, pdata);
                            break;

                        case BTN_RIGHT_ID:
                            pdata->parameter = (pdata->parameter + 1) % pdata->num_parameters;
                            update_page(model, pdata);
                            break;

                        case BTN_MINUS_ID:
                            parlav_operation(model, pdata->parameter, -1, model->config.parmac.access_level);
                            update_page(model, pdata);
                            pdata->meta->changed = 1;
                            break;

                        case BTN_PLUS_ID: {
                            parlav_operation(model, pdata->parameter, +1, model->config.parmac.access_level);
                            update_page(model, pdata);
                            pdata->meta->changed = 1;
                            break;
                        }
                    }
                    break;
                }

                case LV_EVENT_LONG_PRESSED_REPEAT: {
                    switch (obj_data->id) {
                        case BTN_LEFT_ID:
                            if (pdata->parameter > 0) {
                                pdata->parameter--;
                            } else {
                                pdata->parameter = pdata->num_parameters - 1;
                            }
                            update_page(model, pdata);
                            break;

                        case BTN_RIGHT_ID:
                            pdata->parameter = (pdata->parameter + 1) % pdata->num_parameters;
                            update_page(model, pdata);
                            break;

                        case BTN_MINUS_ID:
                            parlav_operation(model, pdata->parameter, -10, model->config.parmac.access_level);
                            update_page(model, pdata);
                            pdata->meta->changed = 1;
                            break;

                        case BTN_PLUS_ID:
                            parlav_operation(model, pdata->parameter, +10, model->config.parmac.access_level);
                            update_page(model, pdata);
                            pdata->meta->changed = 1;
                            break;
                    }
                    break;
                }

                default:
                    break;
            }
            break;
        }

        default:
            break;
    }

    return msg;
}


static void update_page(model_t *model, struct page_data *pdata) {
    char string[64] = {0};

    if (pdata->num_parameters > 0) {
        view_common_set_hidden(pdata->label_description, 0);
        view_common_set_hidden(pdata->label_value, 0);

        lv_label_set_text_fmt(pdata->label_number, "Param. %2" PRIuLEAST16 "/%" PRIuLEAST16, pdata->parameter + 1,
                              pdata->num_parameters);

        const char *description = parlav_get_description(model, pdata->parameter, model->config.parmac.access_level);
        if (strcmp(lv_label_get_text(pdata->label_description), description)) {
            lv_label_set_text(pdata->label_description, description);
        }
        parlav_format_value(model, string, pdata->parameter, model->config.parmac.access_level);
        lv_label_set_text(pdata->label_value, string);
    } else {
        lv_label_set_text(pdata->label_number, view_intl_get_string(model, STRINGS_NESSUN_PARAMETRO));
        view_common_set_hidden(pdata->label_description, 1);
        view_common_set_hidden(pdata->label_value, 1);
    }
}


static void destroy_page(void *state, void *extra) {
    struct page_data *pdata = state;
    (void)extra;
    pman_timer_delete(pdata->timer);
    lv_free(pdata);
}


static void close_page(void *state) {
    struct page_data *pdata = state;
    pman_timer_pause(pdata->timer);
    lv_obj_clean(lv_screen_active());
}


const pman_page_t page_step = {
    .create        = create_page,
    .destroy       = destroy_page,
    .open          = open_page,
    .close         = close_page,
    .process_event = page_event,

};
