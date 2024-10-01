#include "lvgl.h"
#include "model/model.h"
#include "../view.h"
#include "../theme/style.h"
#include "../common.h"
#include "config/app_config.h"
#include "src/widgets/led/lv_led.h"
#include "../intl/intl.h"


struct page_data {
    uint8_t placeholder;
};


enum {
    BTN_BACK_ID,
    BTN_TEST_ID,
    BTN_ARCHIVING_ID,
    BTN_PARMAC_ID,
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


    view_common_create_title(lv_scr_act(), view_intl_get_string(model, STRINGS_IMPOSTAZIONI), BTN_BACK_ID, -1);

    lv_obj_t *cont = lv_obj_create(lv_scr_act());
    lv_obj_set_style_pad_column(cont, 8, LV_STATE_DEFAULT);
    lv_obj_set_size(cont, LV_HOR_RES, LV_VER_RES - 56);
    lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(cont, LV_ALIGN_BOTTOM_MID, 0, 0);

    {
        lv_obj_t *btn = lv_btn_create(cont);
        lv_obj_set_width(btn, 140);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, view_intl_get_string(model, STRINGS_DIAGNOSI));
        lv_obj_set_style_text_font(lbl, STYLE_FONT_SMALL, LV_STATE_DEFAULT);
        lv_obj_center(lbl);
        view_register_object_default_callback(btn, BTN_TEST_ID);
    }

    {
        lv_obj_t *btn = lv_btn_create(cont);
        lv_obj_set_width(btn, 140);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, view_intl_get_string(model, STRINGS_PARAMETRI));
        lv_obj_set_style_text_font(lbl, STYLE_FONT_SMALL, LV_STATE_DEFAULT);
        lv_obj_center(lbl);
        view_register_object_default_callback(btn, BTN_PARMAC_ID);
    }

    {
        lv_obj_t *btn = lv_btn_create(cont);
        lv_obj_set_width(btn, 140);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, view_intl_get_string(model, STRINGS_ARCHIVIAZIONE));
        lv_obj_set_style_text_font(lbl, STYLE_FONT_SMALL, LV_STATE_DEFAULT);
        lv_obj_center(lbl);
        view_register_object_default_callback(btn, BTN_ARCHIVING_ID);
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
                    switch (obj_data->id) {
                        case BTN_BACK_ID:
                            msg.stack_msg = PMAN_STACK_MSG_BACK();
                            break;

                        case BTN_TEST_ID:
                            msg.stack_msg = PMAN_STACK_MSG_PUSH_PAGE(&page_test_inputs);
                            break;

                        case BTN_ARCHIVING_ID:
                            // msg.stack_msg = PMAN_STACK_MSG_PUSH_PAGE(&page_archiving);
                            break;

                        case BTN_PARMAC_ID:
                            msg.stack_msg = PMAN_STACK_MSG_PUSH_PAGE(&page_parmac);
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
    (void)model;
    (void)pdata;
}


const pman_page_t page_menu = {
    .create        = create_page,
    .destroy       = pman_destroy_all,
    .open          = open_page,
    .close         = pman_close_all,
    .process_event = page_event,
};
