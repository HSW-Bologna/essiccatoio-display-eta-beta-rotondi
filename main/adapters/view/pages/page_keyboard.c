#include "lvgl.h"
#include "model/model.h"
#include "../view.h"
#include "../theme/style.h"
#include "../common.h"
#include "config/app_config.h"
#include "src/widgets/led/lv_led.h"
#include "../intl/intl.h"


struct page_data {
    char     *string;
    lv_obj_t *textarea;
};


enum {
    KEYBOARD_ID,
    TEXTAREA_ID,
};


static void update_page(model_t *model, struct page_data *pdata);


static void *create_page(pman_handle_t handle, void *extra) {
    (void)handle;
    (void)extra;

    struct page_data *pdata = lv_malloc(sizeof(struct page_data));
    assert(pdata != NULL);
    pdata->string = (char *)extra;

    return pdata;
}


static void open_page(pman_handle_t handle, void *state) {
    struct page_data *pdata = state;

    model_t *model = view_get_model(handle);

    lv_obj_t *cont = lv_obj_create(lv_screen_active());
    lv_obj_add_style(cont, &style_padless_cont, LV_STATE_DEFAULT);
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_center(cont);

    lv_obj_t *textarea = lv_textarea_create(cont);
    lv_obj_set_size(textarea, 300, 64);
    lv_obj_set_scrollbar_mode(textarea, LV_SCROLLBAR_MODE_OFF);
    lv_textarea_set_one_line(textarea, 1);
    lv_textarea_set_text(textarea, pdata->string);
    lv_textarea_set_max_length(textarea, MAX_NAME_LENGTH);
    lv_obj_align(textarea, LV_ALIGN_TOP_MID, 0, 32);
    view_register_object_default_callback(textarea, TEXTAREA_ID);
    pdata->textarea = textarea;

    lv_obj_t *keyboard = lv_keyboard_create(cont);
    lv_keyboard_set_textarea(keyboard, textarea);
    lv_obj_add_style(keyboard, (lv_style_t *)&style_config_btn, LV_PART_ITEMS);
    lv_obj_set_style_text_font(keyboard, STYLE_FONT_SMALL, LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(keyboard, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_size(keyboard, LV_PCT(100), 160);
    lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    view_register_object_default_callback(keyboard, KEYBOARD_ID);

    update_page(model, pdata);
}


static pman_msg_t page_event(pman_handle_t handle, void *state, pman_event_t event) {
    struct page_data *pdata = state;

    mut_model_t *model = view_get_model(handle);

    pman_msg_t msg = PMAN_MSG_NULL;

    switch (event.tag) {
        case PMAN_EVENT_TAG_OPEN:
            lv_obj_send_event(pdata->textarea, LV_EVENT_FOCUSED, NULL);
            lv_textarea_set_cursor_pos(pdata->textarea, LV_TEXTAREA_CURSOR_LAST);
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
                case LV_EVENT_VALUE_CHANGED: {
                    switch (obj_data->id) {
                        case TEXTAREA_ID:
                            break;

                        default:
                            break;
                    }
                    break;
                }

                case LV_EVENT_READY: {
                    switch (obj_data->id) {
                        case KEYBOARD_ID:
                            strcpy(pdata->string, lv_textarea_get_text(pdata->textarea));
                            msg.stack_msg = PMAN_STACK_MSG_BACK();
                            break;

                        default:
                            break;
                    }
                    break;
                }

                case LV_EVENT_CANCEL: {
                    switch (obj_data->id) {
                        case KEYBOARD_ID:
                            msg.stack_msg = PMAN_STACK_MSG_BACK();
                            break;

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
    (void)model;
    (void)pdata;
}


const pman_page_t page_keyboard = {
    .create        = create_page,
    .destroy       = pman_destroy_all,
    .open          = open_page,
    .close         = pman_close_all,
    .process_event = page_event,
};
