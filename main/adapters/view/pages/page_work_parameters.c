#include <assert.h>
#include <stdlib.h>
#include "lvgl.h"
#include "model/model.h"
#include "../view.h"
#include "../theme/style.h"
#include "../intl/intl.h"
#include <esp_log.h>
#include "bsp/tft/display.h"
#include "../theme/style.h"
#include "../common.h"
#include "config/app_config.h"
#include "services/timestamp.h"


typedef enum {
    WORK_PARAMETER_TEMPERATURE = 0,
    WORK_PARAMETER_HUMIDITY,
    WORK_PARAMETER_SPEED,
    WORK_PARAMETER_DURATION,
#define WORK_PARAMETERS_NUM 4
} work_parameter_t;


enum {
    BTN_BACK_ID,
    BTN_TEMPERATURE_ID,
    BTN_HUMIDITY_ID,
    BTN_SPEED_ID,
    BTN_DURATION_ID,
    BTN_MODIFY_ID,
};


static const char *TAG = __FILE_NAME__;


struct page_data {
    lv_obj_t *buttons[WORK_PARAMETERS_NUM];
    lv_obj_t *button_minus;
    lv_obj_t *button_plus;
    lv_obj_t *button_back;

    lv_obj_t *labels[WORK_PARAMETERS_NUM];
    lv_obj_t *label_parameter;

    int selected_parameter;

    pman_timer_t *timer;

    uint16_t duration_seconds;
};


static void      update_page(model_t *model, struct page_data *pdata);
static lv_obj_t *button_parameter_create(lv_obj_t *parent, lv_obj_t **label_value, const char *description, int id,
                                         uint8_t top);


static void *create_page(pman_handle_t handle, void *extra) {
    model_t *model = pman_get_user_data(handle);
    (void)extra;

    struct page_data *pdata = lv_malloc(sizeof(struct page_data));
    assert(pdata != NULL);

    pdata->timer              = PMAN_REGISTER_TIMER_ID(handle, model->config.parmac.reset_page_time * 1000UL, 0);
    pdata->selected_parameter = -1;
    pdata->duration_seconds   = model->run.minion.read.remaining_time_seconds;

    return pdata;
}


static void open_page(pman_handle_t handle, void *state) {
    struct page_data *pdata = state;

    pman_timer_reset(pdata->timer);
    pman_timer_resume(pdata->timer);

    model_t *model = pman_get_user_data(handle);

    lv_obj_remove_flag(lv_screen_active(), LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *cont = lv_obj_create(lv_screen_active());
    lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(cont, LV_HOR_RES, LV_VER_RES);
    lv_obj_add_style(cont, &style_padless_cont, LV_STATE_DEFAULT);
    lv_obj_add_style(cont, &style_borderless_cont, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cont, VIEW_STYLE_COLOR_WHITE, LV_STATE_DEFAULT);

    lv_obj_t *top_cont = lv_obj_create(cont);
    lv_obj_remove_flag(top_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(top_cont, &style_transparent_cont, LV_STATE_DEFAULT);
    lv_obj_set_size(top_cont, LV_HOR_RES, 160);
    lv_obj_align(top_cont, LV_ALIGN_TOP_MID, 0, -28);
    lv_obj_set_style_pad_column(top_cont, 8, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_hor(top_cont, 8, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(top_cont, 4, LV_STATE_DEFAULT);
    lv_obj_set_layout(top_cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(top_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(top_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);

    pdata->buttons[WORK_PARAMETER_TEMPERATURE] =
        button_parameter_create(top_cont, &pdata->labels[WORK_PARAMETER_TEMPERATURE],
                                view_intl_get_string(model, STRINGS_TEMPERATURA), BTN_TEMPERATURE_ID, 1);
    pdata->buttons[WORK_PARAMETER_HUMIDITY] =
        button_parameter_create(top_cont, &pdata->labels[WORK_PARAMETER_HUMIDITY],
                                view_intl_get_string(model, STRINGS_UMIDITA), BTN_HUMIDITY_ID, 1);

    lv_obj_t *bottom_cont = lv_obj_create(cont);
    lv_obj_remove_flag(bottom_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(bottom_cont, &style_transparent_cont, LV_STATE_DEFAULT);
    lv_obj_set_size(bottom_cont, LV_HOR_RES, 160);
    lv_obj_align(bottom_cont, LV_ALIGN_BOTTOM_MID, 0, 28);
    lv_obj_set_style_pad_column(bottom_cont, 8, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_hor(bottom_cont, 8, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(bottom_cont, 4, LV_STATE_DEFAULT);
    lv_obj_set_layout(bottom_cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(bottom_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bottom_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);

    pdata->buttons[WORK_PARAMETER_SPEED] =
        button_parameter_create(bottom_cont, &pdata->labels[WORK_PARAMETER_SPEED],
                                view_intl_get_string(model, STRINGS_VELOCITA), BTN_SPEED_ID, 0);

    pdata->buttons[WORK_PARAMETER_DURATION] =
        button_parameter_create(bottom_cont, &pdata->labels[WORK_PARAMETER_DURATION],
                                view_intl_get_string(model, STRINGS_DURATA), BTN_DURATION_ID, 0);

    lv_obj_t *lbl = lv_label_create(cont);
    lv_obj_set_style_text_font(lbl, STYLE_FONT_MEDIUM, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lbl, lv_color_black(), LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
    pdata->label_parameter = lbl;

    {
        lv_obj_t *btn = lv_button_create(cont);
        lv_obj_set_size(btn, 48, 48);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, LV_SYMBOL_MINUS);
        lv_obj_center(lbl);
        view_register_object_default_callback_with_number(btn, BTN_MODIFY_ID, -1);
        lv_obj_align(btn, LV_ALIGN_LEFT_MID, 8, 0);
        pdata->button_minus = btn;
    }

    {
        lv_obj_t *btn = lv_button_create(cont);
        lv_obj_set_size(btn, 48, 48);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, LV_SYMBOL_PLUS);
        lv_obj_center(lbl);
        view_register_object_default_callback_with_number(btn, BTN_MODIFY_ID, +1);
        lv_obj_align(btn, LV_ALIGN_RIGHT_MID, -8, 0);
        pdata->button_plus = btn;
    }

    {
        LV_IMG_DECLARE(img_door);
        lv_obj_t *btn = lv_button_create(cont);
        lv_obj_add_style(btn, (lv_style_t *)&style_config_btn, LV_STATE_DEFAULT);
        lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, LV_STATE_DEFAULT);
        lv_obj_set_size(btn, 56, 56);
        lv_obj_t *img = lv_image_create(btn);
        lv_image_set_src(img, &img_door);
        lv_obj_add_style(img, &style_white_icon, LV_STATE_DEFAULT);
        lv_obj_center(img);
        lv_obj_align(btn, LV_ALIGN_CENTER, 0, 0);
        view_register_object_default_callback(btn, BTN_BACK_ID);
        pdata->button_back = btn;
    }

    ESP_LOGI(TAG, "Open");

    VIEW_ADD_WATCHED_VARIABLE(&model->run.minion.read.default_temperature, 0);
    VIEW_ADD_WATCHED_VARIABLE(&model->run.minion.read.remaining_time_seconds, 0);
    VIEW_ADD_WATCHED_VARIABLE(&model->run.minion.read.humidity_probe, 0);
    VIEW_ADD_WATCHED_VARIABLE(&model->run.minion.read.cycle_state, 0);

    update_page(model, pdata);
}


static pman_msg_t page_event(pman_handle_t handle, void *state, pman_event_t event) {
    (void)handle;

    pman_msg_t msg = PMAN_MSG_NULL;

    struct page_data *pdata = state;
    mut_model_t      *model = pman_get_user_data(handle);

    switch (event.tag) {
        case PMAN_EVENT_TAG_USER: {
            view_event_t *view_event = event.as.user;
            switch (view_event->tag) {
                case VIEW_EVENT_TAG_PAGE_WATCHER: {
                    if (model_is_cycle_stopped(model)) {
                        msg.stack_msg = PMAN_STACK_MSG_BACK();
                    } else {
                        update_page(model, pdata);
                    }
                    break;
                }

                default:
                    break;
            }

            break;
        }

        case PMAN_EVENT_TAG_TIMER: {
            switch ((int)(uintptr_t)event.as.timer->user_data) {
                default:
                    break;
            }
            break;
        }

        case PMAN_EVENT_TAG_LVGL: {
            lv_obj_t           *target   = lv_event_get_target(event.as.lvgl);
            view_object_data_t *obj_data = lv_obj_get_user_data(target);

            if (obj_data == NULL) {
                break;
            }

            switch (lv_event_get_code(event.as.lvgl)) {
                case LV_EVENT_CLICKED: {
                    pman_timer_reset(pdata->timer);

                    switch (obj_data->id) {
                        case BTN_BACK_ID: {
                            msg.stack_msg = PMAN_STACK_MSG_BACK();
                            break;
                        }

                        case BTN_MODIFY_ID: {
                            switch (pdata->selected_parameter) {
                                case WORK_PARAMETER_TEMPERATURE: {
                                    model_modify_temperature_setpoint(model, obj_data->number);
                                    update_page(model, pdata);
                                    break;
                                }
                                case WORK_PARAMETER_HUMIDITY: {
                                    model_modify_humidity_setpoint(model, obj_data->number);
                                    update_page(model, pdata);
                                    break;
                                }
                                case WORK_PARAMETER_SPEED: {
                                    model_modify_speed(model, obj_data->number);
                                    update_page(model, pdata);
                                    break;
                                }
                                case WORK_PARAMETER_DURATION: {
                                    pdata->duration_seconds += obj_data->number * 10;
                                    pdata->duration_seconds = (pdata->duration_seconds / 10) * 10;
                                    view_get_protocol(handle)->modify_duration(handle, pdata->duration_seconds);
                                    update_page(model, pdata);
                                    break;
                                }
                                default:
                                    break;
                            }
                            break;
                        }

                        case BTN_TEMPERATURE_ID: {
                            if (pdata->selected_parameter == WORK_PARAMETER_TEMPERATURE) {
                                pdata->selected_parameter = -1;
                            } else {
                                pdata->selected_parameter = WORK_PARAMETER_TEMPERATURE;
                            }
                            update_page(model, pdata);
                            break;
                        }

                        case BTN_HUMIDITY_ID: {
                            if (pdata->selected_parameter == WORK_PARAMETER_HUMIDITY) {
                                pdata->selected_parameter = -1;
                            } else {
                                pdata->selected_parameter = WORK_PARAMETER_HUMIDITY;
                            }
                            update_page(model, pdata);
                            break;
                        }

                        case BTN_SPEED_ID: {
                            if (pdata->selected_parameter == WORK_PARAMETER_SPEED) {
                                pdata->selected_parameter = -1;
                            } else {
                                pdata->selected_parameter = WORK_PARAMETER_SPEED;
                            }
                            update_page(model, pdata);
                            break;
                        }

                        case BTN_DURATION_ID: {
                            if (pdata->selected_parameter == WORK_PARAMETER_DURATION) {
                                pdata->selected_parameter = -1;
                            } else {
                                pdata->selected_parameter = WORK_PARAMETER_DURATION;
                            }
                            update_page(model, pdata);
                            break;
                        }

                        default:
                            break;
                    }
                    break;
                }

                case LV_EVENT_LONG_PRESSED_REPEAT: {
                    pman_timer_reset(pdata->timer);

                    switch (obj_data->id) {
                        case BTN_MODIFY_ID: {
                            switch (pdata->selected_parameter) {
                                case WORK_PARAMETER_TEMPERATURE: {
                                    model_modify_temperature_setpoint(model, obj_data->number);
                                    update_page(model, pdata);
                                    break;
                                }
                                case WORK_PARAMETER_HUMIDITY: {
                                    model_modify_humidity_setpoint(model, obj_data->number);
                                    update_page(model, pdata);
                                    break;
                                }
                                case WORK_PARAMETER_SPEED: {
                                    model_modify_speed(model, obj_data->number);
                                    update_page(model, pdata);
                                    break;
                                }
                                case WORK_PARAMETER_DURATION: {
                                    pdata->duration_seconds += obj_data->number * 10;
                                    pdata->duration_seconds = (pdata->duration_seconds / 10) * 10;
                                    view_get_protocol(handle)->modify_duration(handle, pdata->duration_seconds);
                                    update_page(model, pdata);
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
    if (model_is_cycle_stopped(model)) {
        return;
    }

    program_step_t step = model_get_current_step(model);

    if (pdata->selected_parameter == -1) {
        view_common_set_hidden(pdata->button_back, 0);
        view_common_set_hidden(pdata->label_parameter, 1);
        view_common_set_hidden(pdata->button_minus, 1);
        view_common_set_hidden(pdata->button_plus, 1);
    } else {
        view_common_set_hidden(pdata->button_back, 1);
        view_common_set_hidden(pdata->label_parameter, 0);
        view_common_set_hidden(pdata->button_minus, 0);
        view_common_set_hidden(pdata->button_plus, 0);

        switch (pdata->selected_parameter) {
            case WORK_PARAMETER_TEMPERATURE:
                lv_label_set_text_fmt(pdata->label_parameter, "%i °C", model_get_temperature_setpoint(model));
                break;

            case WORK_PARAMETER_HUMIDITY:
                lv_label_set_text_fmt(pdata->label_parameter, "%i%%", model_get_humidity_setpoint(model));
                break;

            case WORK_PARAMETER_SPEED:
                lv_label_set_text_fmt(pdata->label_parameter, "%i rpm", model_get_speed(model));
                break;

            case WORK_PARAMETER_DURATION:
                lv_label_set_text_fmt(pdata->label_parameter, "%02i:%02i", pdata->duration_seconds / 60,
                                      pdata->duration_seconds % 60);
                break;

            default:
                break;
        }
    }

    switch (step.type) {
        case PROGRAM_STEP_TYPE_DRYING: {
            lv_label_set_text_fmt(pdata->labels[WORK_PARAMETER_TEMPERATURE], "%i °C",
                                  model_get_temperature_setpoint(model));
            lv_obj_remove_state(pdata->buttons[WORK_PARAMETER_TEMPERATURE], LV_STATE_DISABLED);

            lv_label_set_text_fmt(pdata->labels[WORK_PARAMETER_HUMIDITY], "%i%%", model_get_humidity_setpoint(model));
            lv_obj_remove_state(pdata->buttons[WORK_PARAMETER_HUMIDITY], LV_STATE_DISABLED);
            break;
        }

        default: {
            if (pdata->selected_parameter == WORK_PARAMETER_HUMIDITY) {
                pdata->selected_parameter = -1;
            }

            lv_label_set_text(pdata->labels[WORK_PARAMETER_HUMIDITY], "---");
            lv_obj_add_state(pdata->buttons[WORK_PARAMETER_HUMIDITY], LV_STATE_DISABLED);

            lv_label_set_text(pdata->labels[WORK_PARAMETER_TEMPERATURE], "---");
            lv_obj_add_state(pdata->buttons[WORK_PARAMETER_TEMPERATURE], LV_STATE_DISABLED);
            break;
        }
    }

    lv_label_set_text_fmt(pdata->labels[WORK_PARAMETER_SPEED], "%i rpm", model_get_speed(model));
    lv_label_set_text_fmt(pdata->labels[WORK_PARAMETER_DURATION], "%02i:%02i", pdata->duration_seconds / 60,
                          pdata->duration_seconds % 60);

    for (work_parameter_t parameter_index = 0; parameter_index < WORK_PARAMETERS_NUM; parameter_index++) {
        if (parameter_index == pdata->selected_parameter) {
            lv_obj_add_state(pdata->buttons[parameter_index], LV_STATE_CHECKED);
        } else {
            lv_obj_remove_state(pdata->buttons[parameter_index], LV_STATE_CHECKED);
        }
    }
}


static void close_page(void *state) {
    struct page_data *pdata = state;
    pman_timer_pause(pdata->timer);
    lv_obj_clean(lv_screen_active());
}


static void destroy_page(void *state, void *extra) {
    (void)extra;
    struct page_data *pdata = state;
    pman_timer_delete(pdata->timer);
    lv_free(state);
}


static lv_obj_t *button_parameter_create(lv_obj_t *parent, lv_obj_t **label_value, const char *description, int id,
                                         uint8_t top) {
    lv_obj_t *button = lv_button_create(parent);
    lv_obj_set_flex_grow(button, 1);
    lv_obj_set_height(button, LV_PCT(100));
    lv_obj_add_style(button, &style_tall_button, LV_STATE_DEFAULT);
    lv_obj_add_style(button, &style_tall_button_checked, LV_STATE_CHECKED);
    lv_obj_set_style_pad_top(button, 28, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(button, 12, LV_STATE_DEFAULT);
    lv_obj_set_style_radius(button, 24, 0);
    view_register_object_default_callback(button, id);

    lv_obj_t *label_description = lv_label_create(button);
    lv_label_set_long_mode(label_description, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(label_description, LV_HOR_RES / 2 - 32);
    lv_obj_set_style_text_font(label_description, STYLE_FONT_COMPACT, LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(label_description, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
    lv_label_set_text(label_description, description);
    if (top) {
        lv_obj_align(label_description, LV_ALIGN_TOP_MID, 0, 8);
    } else {
        lv_obj_align(label_description, LV_ALIGN_BOTTOM_MID, 0, -32);
    }

    lv_obj_t *local_label_value = lv_label_create(button);
    lv_obj_set_style_text_font(local_label_value, STYLE_FONT_MEDIUM, LV_STATE_DEFAULT);
    if (top) {
        lv_obj_align(local_label_value, LV_ALIGN_BOTTOM_MID, 0, -8);
    } else {
        lv_obj_align(local_label_value, LV_ALIGN_TOP_MID, 0, 0);
    }
    *label_value = local_label_value;

    return button;
}


const pman_page_t page_work_parameters = {
    .create        = create_page,
    .destroy       = destroy_page,
    .open          = open_page,
    .close         = close_page,
    .process_event = page_event,
};
