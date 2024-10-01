#include "lvgl.h"
#include "model/model.h"
#include "../view.h"
#include "../theme/style.h"
#include "../common.h"
#include "config/app_config.h"
#include "src/widgets/led/lv_led.h"
#include "../intl/intl.h"


#define NUM_OUTPUTS 6


struct page_data {
    lv_obj_t *led_outputs[NUM_OUTPUTS];
};


enum {
    BTN_BACK_ID,
    BTN_NEXT_ID,
    BTN_OUTPUT_ID,
};


static void update_page(model_t *model, struct page_data *pdata);


static void *create_page(pman_handle_t handle, void *extra) {
    (void)handle;
    (void)extra;

    struct page_data *pdata = lv_malloc(sizeof(struct page_data));
    assert(pdata != NULL);

    return pdata;
}


static void open_page(pman_handle_t handle, void *state) {
    struct page_data *pdata = state;

    model_t *model = view_get_model(handle);

    view_common_create_title(lv_scr_act(), view_intl_get_string(model, STRINGS_USCITE), BTN_BACK_ID, BTN_NEXT_ID);

    lv_obj_t *cont = lv_obj_create(lv_scr_act());
    lv_obj_set_style_pad_column(cont, 6, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(cont, 6, LV_STATE_DEFAULT);
    lv_obj_set_size(cont, LV_HOR_RES, LV_VER_RES - 56);
    lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_align(cont, LV_ALIGN_BOTTOM_MID, 0, 0);

    for (size_t i = 0; i < NUM_OUTPUTS; i++) {
        lv_obj_t *btn = lv_btn_create(cont);
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(btn, 4, LV_STATE_DEFAULT);
        lv_obj_set_size(btn, 52, 52);
        view_register_object_default_callback_with_number(btn, BTN_OUTPUT_ID, i);

        lv_obj_t *led = lv_led_create(btn);
        lv_obj_add_flag(led, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_led_set_color(led, VIEW_STYLE_COLOR_GREEN);
        lv_obj_set_size(led, 32, 32);
        lv_obj_center(led);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_obj_set_style_text_color(lbl, VIEW_STYLE_COLOR_BLACK, LV_STATE_DEFAULT);
        lv_obj_add_flag(lbl, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_set_style_text_font(lbl, STYLE_FONT_SMALL, LV_STATE_DEFAULT);
        lv_label_set_text_fmt(lbl, "%02zu", i + 1);
        lv_obj_center(lbl);

        pdata->led_outputs[i] = led;
    }

    update_page(model, pdata);
}


static pman_msg_t page_event(pman_handle_t handle, void *state, pman_event_t event) {
    struct page_data *pdata = state;

    mut_model_t *model = view_get_model(handle);

    pman_msg_t msg = PMAN_MSG_NULL;

    switch (event.tag) {
        case PMAN_EVENT_TAG_OPEN:
            model->run.minion.write.test_mode = 1;
            break;

        case PMAN_EVENT_TAG_USER: {
            view_event_t *view_event = event.as.user;
            switch (view_event->tag) {
                case VIEW_EVENT_TAG_PAGE_WATCHER: {
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
                        case BTN_BACK_ID:
                            model_clear_test_outputs(model);
                            view_get_protocol(handle)->clear_test_outputs(handle);
                            msg.stack_msg = PMAN_STACK_MSG_BACK();
                            break;

                        case BTN_NEXT_ID:
                            view_get_protocol(handle)->clear_test_outputs(handle);
                            msg.stack_msg = PMAN_STACK_MSG_SWAP(&page_test_temperature);
                            break;

                        case BTN_OUTPUT_ID:
                            view_get_protocol(handle)->test_output(handle, obj_data->number);
                            update_page(model, pdata);
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
    for (int16_t i = 0; i < NUM_OUTPUTS; i++) {
        if ((model->run.minion.write.test_outputs & (1 << i)) > 0) {
            lv_led_on(pdata->led_outputs[i]);
        } else {
            lv_led_off(pdata->led_outputs[i]);
        }
    }
}


const pman_page_t page_test_outputs = {
    .create        = create_page,
    .destroy       = pman_destroy_all,
    .open          = open_page,
    .close         = pman_close_all,
    .process_event = page_event,
};
