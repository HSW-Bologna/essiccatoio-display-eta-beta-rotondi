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
    lv_obj_t *table;
    lv_obj_t *obj_clear;

    uint8_t clearing;
};


enum {
    BTN_BACK_ID,
    BTN_CLEAR_CYCLES_ID,
    POPUP_CANCEL_ID,
    POPUP_CONFIRM_ID,
};


static void update_page(model_t *model, struct page_data *pdata);


static void *create_page(pman_handle_t handle, void *extra) {
    (void)handle;
    (void)extra;

    struct page_data *pdata = lv_malloc(sizeof(struct page_data));
    assert(pdata != NULL);
    pdata->clearing = 0;

    return pdata;
}


static void open_page(pman_handle_t handle, void *state) {
    struct page_data *pdata = state;

    model_t *model = view_get_model(handle);

    view_title_t title = view_common_create_title(lv_screen_active(), view_intl_get_string(model, STRINGS_STATISTICHE),
                                                  BTN_BACK_ID, BTN_CLEAR_CYCLES_ID);
    lv_obj_t    *label_clear = lv_obj_get_child(title.button_next, 0);
    lv_label_set_text(label_clear, LV_SYMBOL_TRASH);

    lv_obj_t *cont = lv_obj_create(lv_scr_act());
    lv_obj_set_style_pad_ver(cont, 4, LV_STATE_DEFAULT);
    lv_obj_set_size(cont, LV_HOR_RES, LV_VER_RES - 56);
    lv_obj_align(cont, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_obj_t *table = lv_table_create(cont);
    lv_obj_set_width(table, LV_PCT(100));
    lv_obj_remove_flag(table, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_text_font(table, STYLE_FONT_SMALL, LV_PART_ITEMS);
    lv_obj_set_style_pad_all(table, 2, LV_STATE_DEFAULT | LV_PART_ITEMS);

    lv_table_set_cell_value(table, 0, 0, view_intl_get_string(model, STRINGS_CICLI_COMPLETI));
    lv_table_set_cell_value(table, 1, 0, view_intl_get_string(model, STRINGS_CICLI_PARZIALI));
    lv_table_set_cell_value(table, 2, 0, view_intl_get_string(model, STRINGS_TEMPO_DI_ATTIVITA));
    lv_table_set_cell_value(table, 3, 0, view_intl_get_string(model, STRINGS_TEMPO_DI_LAVORO));
    lv_table_set_cell_value(table, 4, 0, view_intl_get_string(model, STRINGS_TEMPO_DI_ROTAZIONE));
    lv_table_set_cell_value(table, 5, 0, view_intl_get_string(model, STRINGS_TEMPO_DI_VENTILAZIONE));
    lv_table_set_cell_value(table, 6, 0, view_intl_get_string(model, STRINGS_TEMPO_DI_RISCALDAMENTO));
    lv_obj_align(table, LV_ALIGN_TOP_MID, 0, 0);

    pdata->table = table;

    {
        lv_obj_t *obj_clear = lv_obj_create(cont);
        lv_obj_set_size(obj_clear, LV_PCT(100), LV_PCT(100));
        lv_obj_remove_flag(obj_clear, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_style(obj_clear, &style_transparent_cont, LV_STATE_DEFAULT);
        lv_obj_align(obj_clear, LV_ALIGN_CENTER, 0, -2);

        lv_obj_t *label = lv_label_create(obj_clear);
        lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(label, 300);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(label, STYLE_FONT_SMALL, LV_STATE_DEFAULT);
        lv_label_set_text(label, view_intl_get_string(model, STRINGS_AZZERARE_I_CICLI));
        lv_obj_align(label, LV_ALIGN_CENTER, 0, -32);

        lv_obj_t *button_cancel = lv_button_create(obj_clear);
        lv_obj_set_size(button_cancel, 64, 48);
        lv_obj_t *lbl = lv_label_create(button_cancel);
        lv_obj_set_style_text_font(lbl, STYLE_FONT_MEDIUM, LV_STATE_DEFAULT);
        lv_label_set_text(lbl, LV_SYMBOL_CLOSE);
        lv_obj_center(lbl);
        lv_obj_align(button_cancel, LV_ALIGN_BOTTOM_LEFT, 32, -16);
        view_register_object_default_callback(button_cancel, POPUP_CANCEL_ID);

        lv_obj_t *button_ok = lv_button_create(obj_clear);
        lv_obj_set_size(button_ok, 64, 48);
        lbl = lv_label_create(button_ok);
        lv_obj_set_style_text_font(lbl, STYLE_FONT_MEDIUM, LV_STATE_DEFAULT);
        lv_label_set_text(lbl, LV_SYMBOL_OK);
        lv_obj_center(lbl);
        lv_obj_align(button_ok, LV_ALIGN_BOTTOM_RIGHT, -32, -16);
        view_register_object_default_callback(button_ok, POPUP_CONFIRM_ID);

        pdata->obj_clear = obj_clear;
    }

    VIEW_ADD_WATCHED_VARIABLE(&model->run.minion.read.stats, 0);

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

                        case BTN_CLEAR_CYCLES_ID:
                            pdata->clearing = 1;
                            update_page(model, pdata);
                            break;

                        case POPUP_CANCEL_ID:
                            pdata->clearing = 0;
                            update_page(model, pdata);
                            break;

                        case POPUP_CONFIRM_ID:
                            view_get_protocol(handle)->clear_cycle_statistics(handle);
                            pdata->clearing = 0;
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
    if (pdata->clearing) {
        view_common_set_hidden(pdata->obj_clear, 0);
        view_common_set_hidden(pdata->table, 1);
    } else {
        view_common_set_hidden(pdata->obj_clear, 1);
        view_common_set_hidden(pdata->table, 0);

        lv_table_set_cell_value_fmt(pdata->table, 0, 1, "%i", model->run.minion.read.stats.complete_cycles);
        lv_table_set_cell_value_fmt(pdata->table, 1, 1, "%i", model->run.minion.read.stats.partial_cycles);
        {
            uint32_t seconds = model->run.minion.read.stats.active_time_seconds;
            uint32_t hours   = seconds / 3600;
            uint32_t minutes = (seconds % 3600) / 60;
            lv_table_set_cell_value_fmt(pdata->table, 2, 1, "%02" PRIdLEAST32 ":%02" PRIdLEAST32 ":%02" PRIdLEAST32,
                                        hours, minutes, seconds % 60);
        }
        {
            uint32_t seconds = model->run.minion.read.stats.work_time_seconds;
            uint32_t hours   = seconds / 3600;
            uint32_t minutes = (seconds % 3600) / 60;
            lv_table_set_cell_value_fmt(pdata->table, 3, 1, "%02" PRIdLEAST32 ":%02" PRIdLEAST32 ":%02" PRIdLEAST32,
                                        hours, minutes, seconds % 60);
        }
        {
            uint32_t seconds = model->run.minion.read.stats.rotation_time_seconds;
            uint32_t hours   = seconds / 3600;
            uint32_t minutes = (seconds % 3600) / 60;
            lv_table_set_cell_value_fmt(pdata->table, 4, 1, "%02" PRIdLEAST32 ":%02" PRIdLEAST32 ":%02" PRIdLEAST32,
                                        hours, minutes, seconds % 60);
        }
        {
            uint32_t seconds = model->run.minion.read.stats.ventilation_time_seconds;
            uint32_t hours   = seconds / 3600;
            uint32_t minutes = (seconds % 3600) / 60;
            lv_table_set_cell_value_fmt(pdata->table, 5, 1, "%02" PRIdLEAST32 ":%02" PRIdLEAST32 ":%02" PRIdLEAST32,
                                        hours, minutes, seconds % 60);
        }
        {
            uint32_t seconds = model->run.minion.read.stats.heating_time_seconds;
            uint32_t hours   = seconds / 3600;
            uint32_t minutes = (seconds % 3600) / 60;
            lv_table_set_cell_value_fmt(pdata->table, 6, 1, "%02" PRIdLEAST32 ":%02" PRIdLEAST32 ":%02" PRIdLEAST32,
                                        hours, minutes, seconds % 60);
        }
    }
}


const pman_page_t page_statistics = {
    .create        = create_page,
    .destroy       = pman_destroy_all,
    .open          = open_page,
    .close         = pman_close_all,
    .process_event = page_event,
};
