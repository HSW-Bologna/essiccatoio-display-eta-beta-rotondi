#include "lvgl.h"
#include "model/model.h"
#include "../view.h"
#include "../theme/style.h"
#include "../common.h"
#include "config/app_config.h"
#include "src/widgets/led/lv_led.h"
#include "../intl/intl.h"


struct page_data {
    lv_obj_t *btn_import;
    lv_obj_t *btn_export;
    lv_obj_t *btn_fup;

    lv_obj_t *button_ok;

    lv_obj_t *spinner;

    lv_obj_t *obj_buttons;

    lv_obj_t *label_status;
};


enum {
    BTN_BACK_ID,
    BTN_IMPORT_CONFIGURATION_ID,
    BTN_EXPORT_CONFIGURATION_ID,
    BTN_FUP_ID,
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

    lv_obj_t *background = lv_obj_create(lv_screen_active());
    lv_obj_set_size(background, LV_HOR_RES, LV_VER_RES - 56);
    lv_obj_align(background, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_remove_flag(background, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *cont = lv_obj_create(background);
    lv_obj_add_style(cont, &style_transparent_cont, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(cont, 16, LV_STATE_DEFAULT);
    lv_obj_set_size(cont, LV_HOR_RES, LV_VER_RES - 56);
    lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(cont, LV_ALIGN_BOTTOM_MID, 0, 0);
    pdata->obj_buttons = cont;

    {
        lv_obj_t *btn = lv_btn_create(cont);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, view_intl_get_string(model, STRINGS_IMPORTA_UNA_CONFIGURAZIONE));
        lv_obj_set_style_text_font(lbl, STYLE_FONT_MEDIUM, LV_STATE_DEFAULT);
        lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
        lv_obj_set_width(lbl, 260);
        lv_obj_center(lbl);
        view_register_object_default_callback(btn, BTN_IMPORT_CONFIGURATION_ID);

        pdata->btn_import = btn;
    }

    {
        lv_obj_t *btn = lv_btn_create(cont);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, view_intl_get_string(model, STRINGS_ESPORTA_LA_CONFIGURAZIONE));
        lv_obj_set_style_text_font(lbl, STYLE_FONT_MEDIUM, LV_STATE_DEFAULT);
        lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
        lv_obj_set_width(lbl, 260);
        lv_obj_center(lbl);
        view_register_object_default_callback(btn, BTN_EXPORT_CONFIGURATION_ID);

        pdata->btn_export = btn;
    }

    {
        lv_obj_t *btn = lv_btn_create(cont);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, view_intl_get_string(model, STRINGS_AGGIORNA));
        lv_obj_set_style_text_font(lbl, STYLE_FONT_MEDIUM, LV_STATE_DEFAULT);
        lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
        lv_obj_set_width(lbl, 260);
        lv_obj_center(lbl);
        view_register_object_default_callback(btn, BTN_FUP_ID);

        pdata->btn_fup = btn;
    }

    lv_obj_t *label = lv_label_create(background);
    lv_obj_set_style_text_font(label, STYLE_FONT_MEDIUM, LV_STATE_DEFAULT);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label, LV_PCT(90));
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, -64);
    pdata->label_status = label;

    {
        lv_obj_t *button = lv_button_create(background);
        lv_obj_t *label  = lv_label_create(button);
        lv_label_set_text(label, LV_SYMBOL_OK);
        lv_obj_center(label);
        lv_obj_align(button, LV_ALIGN_BOTTOM_MID, 0, 0);
        view_register_object_default_callback(button, BTN_OK_ID);
        pdata->button_ok = button;
    }

    lv_obj_t *spinner = lv_spinner_create(background);
    lv_obj_set_size(spinner, 64, 64);
    lv_obj_align(spinner, LV_ALIGN_CENTER, 0, 64);
    pdata->spinner = spinner;

    VIEW_ADD_WATCHED_VARIABLE(&model->run.removable_drive_state, 0);
    // VIEW_ADD_WATCHED_VARIABLE(&model->run.storage_status, 0);
    VIEW_ADD_WATCHED_VARIABLE(&model->run.firmware_update_state, 0);

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

                        case BTN_IMPORT_CONFIGURATION_ID:
                            // msg.stack_msg = PMAN_STACK_MSG_PUSH_PAGE(&page_import_configuration);
                            break;

                        case BTN_EXPORT_CONFIGURATION_ID:
                            // view_get_protocol(handle)->export_configuration(handle, model->prog.parmac.nome);
                            update_page(model, pdata);
                            break;

                        case BTN_FUP_ID:
                            view_get_protocol(handle)->update_firmware(handle);
                            break;

                        case BTN_OK_ID: {
                            if (model->run.firmware_update_state == FIRMWARE_UPDATE_STATE_SUCCESS) {
                                view_get_protocol(handle)->reset(handle);
                            } else {
                                // model_reset_storage_operation(model);
                                update_page(model, pdata);
                            }
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
    if (model->run.removable_drive_state != REMOVABLE_DRIVE_STATE_MOUNTED) {
        lv_obj_add_state(pdata->btn_import, LV_STATE_DISABLED);
        lv_obj_add_state(pdata->btn_export, LV_STATE_DISABLED);
        lv_obj_add_state(pdata->btn_fup, LV_STATE_DISABLED);
    } else {
        lv_obj_remove_state(pdata->btn_import, LV_STATE_DISABLED);
        lv_obj_remove_state(pdata->btn_export, LV_STATE_DISABLED);
        lv_obj_remove_state(pdata->btn_fup, LV_STATE_DISABLED);
    }

    switch (model->run.firmware_update_state) {
        case FIRMWARE_UPDATE_STATE_AVAILABLE:
            lv_obj_remove_state(pdata->btn_fup, LV_STATE_DISABLED);
            view_common_set_hidden(pdata->spinner, 1);
            view_common_set_hidden(pdata->button_ok, 1);
            view_common_set_hidden(pdata->label_status, 1);
            view_common_set_hidden(pdata->obj_buttons, 0);
            break;

        case FIRMWARE_UPDATE_STATE_SUCCESS:
            view_common_set_hidden(pdata->spinner, 1);
            view_common_set_hidden(pdata->button_ok, 0);
            view_common_set_hidden(pdata->label_status, 0);
            lv_label_set_text(
                pdata->label_status,
                view_intl_get_string(model, STRINGS_AGGIORNAMENTO_FIRMWARE_CONCLUSO_RESETTARE_IL_DISPOSITIVO));
            view_common_set_hidden(pdata->obj_buttons, 1);
            break;

        case FIRMWARE_UPDATE_STATE_UPDATING:
            view_common_set_hidden(pdata->spinner, 0);
            view_common_set_hidden(pdata->button_ok, 1);
            view_common_set_hidden(pdata->label_status, 0);
            lv_label_set_text(pdata->label_status,
                              view_intl_get_string(model, STRINGS_AGGIORNAMENTO_FIRMWARE_IN_CORSO));
            view_common_set_hidden(pdata->obj_buttons, 1);
            break;

        case FIRMWARE_UPDATE_STATE_FAILURE:
            view_common_set_hidden(pdata->spinner, 1);
            view_common_set_hidden(pdata->button_ok, 0);
            view_common_set_hidden(pdata->label_status, 0);
            lv_label_set_text(pdata->label_status, view_intl_get_string(model, STRINGS_AGGIORNAMENTO_FIRMWARE_FALLITO));
            view_common_set_hidden(pdata->obj_buttons, 1);
            break;

        case FIRMWARE_UPDATE_STATE_NONE: {
            lv_obj_add_state(pdata->btn_fup, LV_STATE_DISABLED);
            view_common_set_hidden(pdata->spinner, 1);
            view_common_set_hidden(pdata->button_ok, 1);
            view_common_set_hidden(pdata->label_status, 1);
            view_common_set_hidden(pdata->obj_buttons, 0);

            /*
            switch (model->system.storage_status) {
                case STORAGE_STATUS_READY: {
                    view_common_set_hidden(pdata->spinner, 1);
                    view_common_set_hidden(pdata->button_ok, 1);
                    view_common_set_hidden(pdata->label_status, 1);
                    view_common_set_hidden(pdata->obj_buttons, 0);
                    break;
                }

                case STORAGE_STATUS_LOADING: {
                    view_common_set_hidden(pdata->spinner, 0);
                    view_common_set_hidden(pdata->obj_buttons, 1);
                    view_common_set_hidden(pdata->label_status, 1);
                    view_common_set_hidden(pdata->button_ok, 1);
                    break;
                }

                case STORAGE_STATUS_DONE: {
                    view_common_set_hidden(pdata->spinner, 1);
                    view_common_set_hidden(pdata->obj_buttons, 1);
                    view_common_set_hidden(pdata->label_status, 0);
                    lv_label_set_text(pdata->label_status,
                                      view_intl_get_string(model, STRINGS_OPERAZIONE_CONCLUSA_CON_SUCCESSO));
                    view_common_set_hidden(pdata->button_ok, 0);
                    break;
                }

                case STORAGE_STATUS_ERROR: {
                    view_common_set_hidden(pdata->spinner, 1);
                    view_common_set_hidden(pdata->obj_buttons, 1);
                    view_common_set_hidden(pdata->label_status, 0);
                    lv_label_set_text(pdata->label_status, view_intl_get_string(model, STRINGS_OPERAZIONE_FALLITA));
                    view_common_set_hidden(pdata->button_ok, 0);
                    break;
                }
            }
            */
            break;
        }
    }
}


const pman_page_t page_drive = {
    .create        = create_page,
    .destroy       = pman_destroy_all,
    .open          = open_page,
    .close         = pman_close_all,
    .process_event = page_event,
};
