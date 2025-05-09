#include "lvgl.h"
#include "model/model.h"
#include "../view.h"
#include "../theme/style.h"
#include "../common.h"
#include "config/app_config.h"
#include "src/widgets/led/lv_led.h"
#include "../intl/intl.h"


struct page_data {
    lv_obj_t *label_status;
    lv_obj_t *obj_configurations;
    lv_obj_t *button_ok;
    lv_obj_t *spinner;
};


enum {
    BTN_BACK_ID,
    BTN_CONFIGURATION_ID,
    BTN_OK_ID,
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

    view_common_create_title(lv_scr_act(), view_intl_get_string(model, STRINGS_CHIAVETTA), BTN_BACK_ID, -1);

    lv_obj_t *cont = lv_obj_create(lv_scr_act());
    lv_obj_add_style(cont, &style_padless_cont, LV_STATE_DEFAULT);
    lv_obj_set_size(cont, LV_HOR_RES, LV_VER_RES - 56);
    lv_obj_align(cont, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_obj_t *list = lv_obj_create(cont);
    lv_obj_add_style(list, &style_transparent_cont, LV_STATE_DEFAULT);

    lv_obj_set_style_pad_column(list, 16, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(list, 16, LV_STATE_DEFAULT);
    lv_obj_set_size(list, LV_PCT(100), LV_PCT(100));
    lv_obj_set_layout(list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_center(list);
    pdata->obj_configurations = list;

    lv_obj_t *label = lv_label_create(cont);
    lv_obj_set_style_text_font(label, STYLE_FONT_MEDIUM, LV_STATE_DEFAULT);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label, LV_PCT(90));
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
    lv_obj_center(label);
    pdata->label_status = label;

    {
        lv_obj_t *button = lv_button_create(cont);
        lv_obj_t *label  = lv_label_create(button);
        lv_label_set_text(label, LV_SYMBOL_OK);
        lv_obj_center(label);
        lv_obj_align(button, LV_ALIGN_BOTTOM_MID, 0, 0);
        view_register_object_default_callback(button, BTN_OK_ID);
        pdata->button_ok = button;
    }

    lv_obj_t *spinner = lv_spinner_create(cont);
    lv_obj_center(spinner);
    pdata->spinner = spinner;

    VIEW_ADD_WATCHED_VARIABLE(&model->run.num_archivi, 0);
    VIEW_ADD_WATCHED_VARIABLE(&model->run.removable_drive_state, 0);
    VIEW_ADD_WATCHED_VARIABLE(&model->run.storage_status, 0);

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

                        case BTN_CONFIGURATION_ID:
                            view_get_protocol(handle)->import_configuration(handle,
                                                                            model->run.archivi[obj_data->number]);
                            update_page(model, pdata);
                            break;

                        case BTN_OK_ID: {
                            model->run.storage_status = STORAGE_STATUS_READY;
                            update_page(model, pdata);
                            break;
                        }
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
    switch (model->run.storage_status) {
        case STORAGE_STATUS_READY: {
            view_common_set_hidden(pdata->spinner, 1);
            view_common_set_hidden(pdata->button_ok, 1);

            if (model->run.num_archivi == 0) {
                view_common_set_hidden(pdata->label_status, 0);
                lv_label_set_text(pdata->label_status,
                                  view_intl_get_string(model, STRINGS_NESSUNA_CONFIGURAZIONE_TROVATA));
                view_common_set_hidden(pdata->obj_configurations, 1);
            } else {
                view_common_set_hidden(pdata->label_status, 1);

                lv_obj_clean(pdata->obj_configurations);
                for (size_t i = 0; i < model->run.num_archivi; i++) {
                    lv_obj_t *button = lv_button_create(pdata->obj_configurations);
                    lv_obj_t *label  = lv_label_create(button);
                    lv_obj_set_style_text_font(label, STYLE_FONT_SMALL, LV_STATE_DEFAULT);
                    lv_label_set_text(label, model->run.archivi[i]);
                    lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
                    lv_obj_set_width(label, 260);
                    lv_obj_center(label);
                    view_register_object_default_callback_with_number(button, BTN_CONFIGURATION_ID, i);
                }

                view_common_set_hidden(pdata->obj_configurations, 0);
            }
            break;
        }

        case STORAGE_STATUS_LOADING: {
            view_common_set_hidden(pdata->spinner, 0);
            view_common_set_hidden(pdata->obj_configurations, 1);
            view_common_set_hidden(pdata->label_status, 1);
            view_common_set_hidden(pdata->button_ok, 1);
            break;
        }

        case STORAGE_STATUS_DONE: {
            view_common_set_hidden(pdata->spinner, 1);
            view_common_set_hidden(pdata->obj_configurations, 1);
            view_common_set_hidden(pdata->label_status, 0);
            lv_label_set_text(pdata->label_status,
                              view_intl_get_string(model, STRINGS_OPERAZIONE_CONCLUSA_CON_SUCCESSO));
            view_common_set_hidden(pdata->button_ok, 0);
            break;
        }

        case STORAGE_STATUS_ERROR: {
            view_common_set_hidden(pdata->spinner, 1);
            view_common_set_hidden(pdata->obj_configurations, 1);
            view_common_set_hidden(pdata->label_status, 0);
            lv_label_set_text(pdata->label_status, view_intl_get_string(model, STRINGS_OPERAZIONE_FALLITA));
            view_common_set_hidden(pdata->button_ok, 0);
            break;
        }
    }
}


const pman_page_t page_import_configuration = {
    .create        = create_page,
    .destroy       = pman_destroy_all,
    .open          = open_page,
    .close         = pman_close_all,
    .process_event = page_event,
};
