#include <stdlib.h>
#include "lvgl.h"
#include "model/model.h"
#include "../view.h"
#include "../theme/style.h"
#include "../common.h"
#include "config/app_config.h"
#include "services/timestamp.h"
#include <esp_log.h>


LV_IMG_DECLARE(img_lock);


enum {
    BACK_BTN_ID,
    PASSWORD_TA_ID,
    PASSWORD_KB_ID,
};


struct page_data {
    pman_timer_t            *timer;
    password_page_options_t *fence;

    int           valid;
    unsigned long lockts;

    lv_obj_t *textarea;
};


static const char *TAG = __FILE_NAME__;


static void *create_page(pman_handle_t handle, void *extra) {
    model_t *model = view_get_model(handle);

    struct page_data *pdata = lv_malloc(sizeof(struct page_data));
    assert(pdata != NULL);

    pdata->timer = pman_timer_create(handle, model->config.parmac.reset_page_time * 1000UL, NULL);
    pdata->fence = extra;
    return pdata;
}


static void open_page(pman_handle_t handle, void *state) {
    (void)handle;

    struct page_data *pdata = state;

    pman_timer_reset(pdata->timer);
    pman_timer_resume(pdata->timer);

    pdata->valid  = 0;
    pdata->lockts = 0;

    view_common_create_title(lv_scr_act(), "Password", BACK_BTN_ID, -1);

    static const char *kbmap[] = {
        "1", "2", "3", "4", "5", "\n", "6", "7", "8", "9", "0", "\n", LV_SYMBOL_BACKSPACE, LV_SYMBOL_OK, "",
    };
    static const lv_btnmatrix_ctrl_t ctrl_map[] = {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    };

    lv_obj_t *cont = lv_obj_create(lv_scr_act());
    lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(cont, LV_HOR_RES, LV_VER_RES - 56);
    lv_obj_add_style(cont, (lv_style_t *)&style_padless_cont, LV_STATE_DEFAULT);
    lv_obj_align(cont, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_obj_t *ta = lv_textarea_create(cont);
    lv_textarea_set_one_line(ta, 1);
    lv_textarea_set_password_mode(ta, 1);
    lv_textarea_set_text(ta, "");
    lv_textarea_set_max_length(ta, strlen(APP_CONFIG_BACKDOOR_PASSWORD));
    lv_obj_set_width(ta, LV_PCT(50));
    lv_obj_remove_flag(ta, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 8);
    lv_obj_set_style_text_font(ta, STYLE_FONT_MEDIUM, LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ta, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
    lv_obj_add_state(ta, LV_STATE_FOCUSED);
    pdata->textarea = ta;

    view_register_object_default_callback(ta, PASSWORD_TA_ID);

    lv_obj_t *kb = lv_keyboard_create(cont);
    lv_keyboard_set_textarea(kb, ta);
    lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_NUMBER);
    lv_obj_set_size(kb, 320, 160);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_style(kb, (lv_style_t *)&style_config_btn, LV_PART_ITEMS);
    lv_obj_set_style_text_font(kb, STYLE_FONT_BIG, LV_PART_ITEMS);
    lv_obj_set_style_pad_row(kb, 16, LV_PART_MAIN);
    lv_obj_set_style_pad_column(kb, 16, LV_PART_MAIN);
    lv_obj_set_style_pad_right(kb, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_left(kb, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(kb, 8, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(kb, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_NUMBER, kbmap, ctrl_map);
    view_register_object_default_callback(kb, PASSWORD_KB_ID);
}


static pman_msg_t page_event(pman_handle_t handle, void *state, pman_event_t event) {
    model_t *model = view_get_model(handle);

    pman_msg_t msg = PMAN_MSG_NULL;

    struct page_data *pdata = state;

    switch (event.tag) {
        case PMAN_EVENT_TAG_TIMER: {
            msg.stack_msg = PMAN_STACK_MSG_REBASE(view_common_main_page(model));
            break;
        }

        case PMAN_EVENT_TAG_LVGL: {
            lv_obj_t           *target   = lv_event_get_current_target_obj(event.as.lvgl);
            view_object_data_t *obj_data = lv_obj_get_user_data(target);

            switch (lv_event_get_code(event.as.lvgl)) {
                case LV_EVENT_CLICKED: {
                    pman_timer_reset(pdata->timer);

                    switch (obj_data->id) {
                        case BACK_BTN_ID:
                            msg.stack_msg = PMAN_STACK_MSG_BACK();
                            break;
                    }
                    break;
                }

                case LV_EVENT_VALUE_CHANGED:
                case LV_EVENT_LONG_PRESSED_REPEAT:
                case LV_EVENT_PRESSED: {
                    pman_timer_reset(pdata->timer);
                    break;
                }

                case LV_EVENT_READY: {
                    pman_timer_reset(pdata->timer);
                    const char *text = lv_textarea_get_text(pdata->textarea);

                    if (strcmp(text, pdata->fence->password) == 0 || strcmp(text, APP_CONFIG_BACKDOOR_PASSWORD) == 0) {
                        pdata->valid = 1;
                    } else {
                        lv_obj_set_style_border_color(pdata->textarea, VIEW_STYLE_COLOR_RED, LV_STATE_DEFAULT);
                        lv_textarea_set_text(pdata->textarea, "");
                    }
                    break;
                }

                case LV_EVENT_RELEASED: {
                    pman_timer_reset(pdata->timer);
                    switch (obj_data->id) {
                        case PASSWORD_KB_ID:
                            if (pdata->valid) {
                                msg.stack_msg = pdata->fence->msg;
                            }
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


static void close_page(void *args) {
    struct page_data *pdata = args;
    pman_timer_pause(pdata->timer);
    lv_obj_clean(lv_scr_act());
}


static void destroy_page(void *args, void *extra) {
    struct page_data *pdata = args;
    pman_timer_delete(pdata->timer);
    lv_free(pdata);
    lv_free(extra);
}


const pman_page_t page_password = {
    .create        = create_page,
    .destroy       = destroy_page,
    .open          = open_page,
    .close         = close_page,
    .process_event = page_event,
};
