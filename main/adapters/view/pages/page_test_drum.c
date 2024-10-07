#include "lvgl.h"
#include "model/model.h"
#include "../view.h"
#include "../theme/style.h"
#include "../common.h"
#include "config/app_config.h"
#include "src/widgets/led/lv_led.h"
#include "../intl/intl.h"
#include "esp_log.h"


#define DA2RPM(da) ((da * 1000) / 100)


struct page_data {
    lv_obj_t *label_pwm;
    lv_obj_t *label_run;
    lv_obj_t *label_control;

    lv_obj_t *button_control;

    lv_obj_t *led_emergency;

    uint8_t pwm_percentage;
    uint8_t run;
    uint8_t forward;
};


enum {
    BTN_BACK_ID,
    BTN_NEXT_ID,
    BTN_MINUS_ID,
    BTN_PLUS_ID,
    BTN_CONTROL_ID,
};


static void      update_page(model_t *model, struct page_data *pdata);
static lv_obj_t *alarm_led_create(lv_obj_t *parent, lv_obj_t **led, const char *text);
static int       test_cesto_in_sicurezza(model_t *model, struct page_data *data);


static const char *TAG = "PageTestDrum";


static void *create_page(pman_handle_t handle, void *extra) {
    (void)handle;
    (void)extra;

    struct page_data *pdata = lv_malloc(sizeof(struct page_data));
    assert(pdata != NULL);

    pdata->pwm_percentage = 0;
    pdata->run            = 0;
    pdata->forward        = (uint8_t)(uintptr_t)extra;

    return pdata;
}


static void open_page(pman_handle_t handle, void *state) {
    struct page_data *pdata = state;

    model_t *model = view_get_model(handle);

    view_common_create_title(
        lv_scr_act(), view_intl_get_string(model, pdata->forward ? STRINGS_MOTORE_AVANTI : STRINGS_MOTORE_INDIETRO),
        BTN_BACK_ID, BTN_NEXT_ID);

    lv_obj_t *cont = lv_obj_create(lv_scr_act());
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(cont, LV_HOR_RES, LV_VER_RES - 56);
    lv_obj_align(cont, LV_ALIGN_BOTTOM_MID, 0, 0);

    {
        lv_obj_t *btn = lv_button_create(cont);
        lv_obj_set_size(btn, 80, 56);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, LV_SYMBOL_MINUS);
        lv_obj_center(lbl);
        view_register_object_default_callback(btn, BTN_MINUS_ID);
        lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 0, 0);
    }

    {
        lv_obj_t *btn = lv_button_create(cont);
        lv_obj_set_size(btn, 80, 56);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, LV_SYMBOL_PLUS);
        lv_obj_center(lbl);
        view_register_object_default_callback(btn, BTN_PLUS_ID);
        lv_obj_align(btn, LV_ALIGN_TOP_RIGHT, 0, 0);
    }

    {
        lv_obj_t *btn = lv_button_create(cont);
        lv_obj_set_size(btn, 80, 56);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_obj_set_style_text_font(lbl, STYLE_FONT_SMALL, LV_STATE_DEFAULT);
        lv_obj_center(lbl);
        view_register_object_default_callback(btn, BTN_CONTROL_ID);
        lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 0);
        pdata->label_control  = lbl;
        pdata->button_control = btn;
    }

    {
        lv_obj_t *lbl = lv_label_create(cont);
        lv_obj_set_style_text_font(lbl, STYLE_FONT_SMALL, LV_STATE_DEFAULT);
        lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 0, 64);
        pdata->label_pwm = lbl;
    }

    {
        lv_obj_t *lbl = lv_label_create(cont);
        lv_obj_set_style_text_font(lbl, STYLE_FONT_SMALL, LV_STATE_DEFAULT);
        lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 0, 64 + 48);
        pdata->label_run = lbl;
    }

    {
        lv_obj_t *bottom = lv_obj_create(cont);
        lv_obj_clear_flag(bottom, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_style(bottom, &style_transparent_cont, LV_STATE_DEFAULT);
        lv_obj_set_style_pad_column(bottom, 16, LV_STATE_DEFAULT);
        lv_obj_set_size(bottom, LV_HOR_RES, 56);
        lv_obj_set_layout(bottom, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(bottom, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(bottom, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_align(bottom, LV_ALIGN_BOTTOM_MID, 0, 0);

        alarm_led_create(bottom, &pdata->led_emergency, "EMER.");
    }

    VIEW_ADD_WATCHED_VARIABLE(&model->run.minion.read.alarms, 0);

    update_page(model, pdata);
}


static pman_msg_t page_event(pman_handle_t handle, void *state, pman_event_t event) {
    struct page_data *pdata = state;

    model_t *model = view_get_model(handle);

    pman_msg_t msg = PMAN_MSG_NULL;

    switch (event.tag) {
        case PMAN_EVENT_TAG_OPEN:
            view_get_protocol(handle)->set_test_mode(handle, 1);
            break;

        case PMAN_EVENT_TAG_USER: {
            view_event_t *view_event = event.as.user;
            switch (view_event->tag) {
                case VIEW_EVENT_TAG_PAGE_WATCHER: {
                    if (pdata->run && !test_cesto_in_sicurezza(model, pdata)) {
                        pdata->run = 0;
                        view_get_protocol(handle)->test_drum(handle, pdata->forward, pdata->run, pdata->pwm_percentage);
                    }
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
                    switch (obj_data->id) {
                        case BTN_MINUS_ID: {
                            if (pdata->pwm_percentage > 0) {
                                if (pdata->pwm_percentage > 5) {
                                    pdata->pwm_percentage -= 5;
                                } else {
                                    pdata->pwm_percentage = 0;
                                }
                            } else {
                                pdata->pwm_percentage = 100;
                            }
                            pdata->pwm_percentage -= pdata->pwm_percentage % 5;

                            view_get_protocol(handle)->test_drum(handle, pdata->forward, pdata->run,
                                                                 pdata->pwm_percentage);
                            update_page(model, pdata);
                            break;
                        }

                        case BTN_PLUS_ID: {
                            pdata->pwm_percentage = (pdata->pwm_percentage + 5) % 101;
                            pdata->pwm_percentage -= pdata->pwm_percentage % 5;

                            view_get_protocol(handle)->test_drum(handle, pdata->forward, pdata->run,
                                                                 pdata->pwm_percentage);
                            update_page(model, pdata);
                            break;
                        }

                        case BTN_BACK_ID:
                            view_get_protocol(handle)->set_test_mode(handle, 0);
                            msg.stack_msg = PMAN_STACK_MSG_BACK();
                            break;

                        case BTN_NEXT_ID:
                            if (pdata->forward) {
                                // msg.stack_msg = PMAN_STACK_MSG_SWAP(&page_test_level);
                            } else {
                                msg.stack_msg = PMAN_STACK_MSG_SWAP_EXTRA(&page_test_drum, (void *)(uintptr_t)1);
                            }
                            break;

                        case BTN_CONTROL_ID: {
                            if (pdata->run == 0 && !test_cesto_in_sicurezza(model, pdata)) {
                                ESP_LOGW(TAG, "Cannot start");
                                break;
                            }
                            pdata->run = !pdata->run;
                            view_get_protocol(handle)->test_drum(handle, pdata->forward, pdata->run,
                                                                 pdata->pwm_percentage);
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

    return msg;
}


static int test_cesto_in_sicurezza(model_t *model, struct page_data *data) {
    (void)data;
    return (model->run.minion.read.alarms & (1 << ALARM_EMERGENCY)) == 0;
}


static void update_page(model_t *model, struct page_data *pdata) {
    view_common_set_disabled(pdata->button_control, !test_cesto_in_sicurezza(model, pdata));

    lv_label_set_text_fmt(pdata->label_pwm, "PWM=[%04i]", pdata->pwm_percentage);
    lv_label_set_text_fmt(pdata->label_run, "[marcia] %s %s", pdata->run ? "on " : "off",
                          test_cesto_in_sicurezza(model, pdata) ? "ok" : "no");

    if ((model->run.minion.read.alarms & (1 << ALARM_EMERGENCY)) > 0) {
        lv_led_on(pdata->led_emergency);
    } else {
        lv_led_off(pdata->led_emergency);
    }

    if (pdata->run) {
        lv_label_set_text(pdata->label_control, "STOP");
    } else {
        lv_label_set_text(pdata->label_control, "START");
    }
}


static lv_obj_t *alarm_led_create(lv_obj_t *parent, lv_obj_t **led, const char *text) {
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(obj, 4, LV_STATE_DEFAULT);
    lv_obj_set_size(obj, 64, 52);

    *led = lv_led_create(obj);
    lv_led_set_color(*led, VIEW_STYLE_COLOR_RED);
    lv_obj_set_size(*led, 32, 32);
    lv_obj_center(*led);

    lv_obj_t *lbl = lv_label_create(obj);
    lv_obj_set_style_text_font(lbl, STYLE_FONT_SMALL, LV_STATE_DEFAULT);
    lv_label_set_text(lbl, text);
    lv_obj_center(lbl);

    return obj;
}


const pman_page_t page_test_drum = {
    .create        = create_page,
    .destroy       = pman_destroy_all,
    .open          = open_page,
    .close         = pman_close_all,
    .process_event = page_event,
};
