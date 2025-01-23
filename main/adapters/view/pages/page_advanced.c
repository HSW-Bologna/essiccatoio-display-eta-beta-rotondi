#include "lvgl.h"
#include "model/model.h"
#include "../view.h"
#include "../theme/style.h"
#include "../common.h"
#include "config/app_config.h"
#include "src/widgets/led/lv_led.h"
#include "../intl/intl.h"
#include "model/descriptions/AUTOGEN_FILE_pars.h"


struct page_data {
    lv_obj_t *label_access;

    lv_obj_t *led_communication;

    uint8_t modified;
};


enum {
    BTN_BACK_ID,
    BTN_COMMUNICATION_ID,
    BTN_FACTORY_RESET_ID,
    BTN_ACCESS_ID,
};


static void update_page(model_t *model, struct page_data *pdata);


static void *create_page(pman_handle_t handle, void *extra) {
    (void)handle;
    (void)extra;

    struct page_data *pdata = lv_malloc(sizeof(struct page_data));
    assert(pdata != NULL);
    pdata->modified = 0;

    return pdata;
}


static void open_page(pman_handle_t handle, void *state) {
    struct page_data *pdata = state;

    model_t *model = view_get_model(handle);

    view_common_create_title(lv_scr_act(), view_intl_get_string(model, STRINGS_AVANZATE), BTN_BACK_ID, -1);

    lv_obj_t *cont = lv_obj_create(lv_scr_act());
    lv_obj_set_style_pad_column(cont, 8, LV_STATE_DEFAULT);
    lv_obj_set_size(cont, LV_HOR_RES, LV_VER_RES - 56);
    lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(cont, LV_ALIGN_BOTTOM_MID, 0, 0);

    {
        lv_obj_t *btn = lv_btn_create(cont);
        lv_obj_set_width(btn, 280);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, view_intl_get_string(model, STRINGS_COMUNICAZIONE));
        lv_obj_set_style_text_font(lbl, STYLE_FONT_SMALL, LV_STATE_DEFAULT);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 16, 0);

        lv_obj_t *led = lv_led_create(btn);
        lv_obj_set_size(led, 28, 28);
        lv_led_set_color(led, VIEW_STYLE_COLOR_RED);
        lv_obj_align(lbl, LV_ALIGN_RIGHT_MID, -16, 0);
        pdata->led_communication = led;

        view_register_object_default_callback(btn, BTN_COMMUNICATION_ID);
    }

    {
        lv_obj_t *btn = lv_btn_create(cont);
        lv_obj_set_width(btn, 280);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
        lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(lbl, 270);
        lv_label_set_text(lbl, view_intl_get_string(model, STRINGS_RIPRISTINO_A_IMPOSTAZIONI_DI_FABBRICA));
        lv_obj_set_style_text_font(lbl, STYLE_FONT_SMALL, LV_STATE_DEFAULT);
        lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);

        view_register_object_default_callback(btn, BTN_COMMUNICATION_ID);
    }

    {
        lv_obj_t *btn = lv_btn_create(cont);
        lv_obj_set_width(btn, 280);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
        lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(lbl, 270);
        lv_label_set_text(lbl, view_intl_get_string(model, STRINGS_ACCESSO));
        lv_obj_set_style_text_font(lbl, STYLE_FONT_SMALL, LV_STATE_DEFAULT);
        lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
        pdata->label_access = lbl;

        view_register_object_default_callback(btn, BTN_ACCESS_ID);
    }

    VIEW_ADD_WATCHED_VARIABLE(&model->run.minion.communication_enabled, 0);

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
                            if (pdata->modified) {
                                view_get_protocol(handle)->save_parmac(handle);
                            }
                            msg.stack_msg = PMAN_STACK_MSG_BACK();
                            break;

                        case BTN_COMMUNICATION_ID:
                            model->run.minion.communication_enabled = !model->run.minion.communication_enabled;
                            update_page(model, pdata);
                            break;

                        case BTN_ACCESS_ID:
                            model->config.parmac.access_level = (model->config.parmac.access_level + 1) % 2;
                            pdata->modified                   = 1;
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
    lv_label_set_text_fmt(pdata->label_access, "%s: %s", view_intl_get_string(model, STRINGS_ACCESSO),
                          pars_livello_accesso[model->config.parmac.access_level][model->config.parmac.language]);

    lv_led_set_brightness(pdata->led_communication,
                          model->run.minion.communication_enabled ? LV_LED_BRIGHT_MAX : LV_LED_BRIGHT_MIN);
}


const pman_page_t page_advanced = {
    .create        = create_page,
    .destroy       = pman_destroy_all,
    .open          = open_page,
    .close         = pman_close_all,
    .process_event = page_event,
};
