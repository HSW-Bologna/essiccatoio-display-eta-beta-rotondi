#include <stdio.h>
#include "lvgl.h"
#include "model/model.h"
#include "../view.h"
#include "../theme/style.h"
#include "../common.h"
#include "config/app_config.h"
#include "src/widgets/led/lv_led.h"
#include "../intl/intl.h"


struct page_data {
    lv_obj_t *table_digital;
    lv_obj_t *led_enable;
};


enum {
    BTN_BACK_ID,
    BTN_NEXT_ID,
    BTN_CLEAR_ID,
    BTN_ENABLE_ID,
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

    view_common_create_title(lv_scr_act(), view_intl_get_string(model, STRINGS_GETTONI), BTN_BACK_ID, BTN_NEXT_ID);

    lv_obj_t *cont = lv_obj_create(lv_scr_act());
    lv_obj_set_style_pad_ver(cont, 4, LV_STATE_DEFAULT);
    lv_obj_set_size(cont, LV_HOR_RES, LV_VER_RES - 56);
    lv_obj_align(cont, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_obj_t *table = lv_table_create(cont);
    lv_obj_set_style_text_font(table, STYLE_FONT_SMALL, LV_PART_ITEMS);
    lv_obj_set_style_pad_all(table, 2, LV_STATE_DEFAULT | LV_PART_ITEMS);

    lv_table_set_cell_value(table, 0, 0, view_intl_get_string(model, STRINGS_CASSA));
    lv_table_set_cell_value(table, 1, 0, view_intl_get_string(model, STRINGS_DIGITALE));
    lv_table_add_cell_ctrl(table, 1, 0, LV_TABLE_CELL_CTRL_MERGE_RIGHT);
    lv_table_set_cell_value_fmt(table, 2, 0, "%s 5 (2.0)", view_intl_get_string(model, STRINGS_LINEA));
    lv_table_set_cell_value_fmt(table, 3, 0, "%s 1 (1.0)", view_intl_get_string(model, STRINGS_LINEA));
    lv_table_set_cell_value_fmt(table, 4, 0, "%s 2 (0.5)", view_intl_get_string(model, STRINGS_LINEA));
    lv_table_set_cell_value_fmt(table, 5, 0, "%s 3 (0.2)", view_intl_get_string(model, STRINGS_LINEA));
    lv_table_set_cell_value_fmt(table, 6, 0, "%s 4 (0.1)", view_intl_get_string(model, STRINGS_LINEA));
    lv_table_set_cell_value(table, 7, 0, view_intl_get_string(model, STRINGS_TOTALE));
    lv_obj_align(table, LV_ALIGN_TOP_MID, 0, 0);

    pdata->table_digital = table;

    lv_obj_t *btn = lv_button_create(cont);
    lv_obj_set_size(btn, 96, 48);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, view_intl_get_string(model, STRINGS_AZZERA));
    lv_obj_set_style_text_font(lbl, STYLE_FONT_SMALL, LV_STATE_DEFAULT);
    lv_obj_center(lbl);
    view_register_object_default_callback(btn, BTN_CLEAR_ID);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    {
        lv_obj_t *btn = lv_btn_create(cont);
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_size(btn, 128, 48);
        view_register_object_default_callback(btn, BTN_ENABLE_ID);

        lv_obj_t *led = lv_led_create(btn);
        lv_obj_add_flag(led, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_led_set_color(led, VIEW_STYLE_COLOR_GREEN);
        lv_obj_set_size(led, 32, 32);
        lv_obj_align(led, LV_ALIGN_RIGHT_MID, 0, 0);
        pdata->led_enable = led;

        lv_obj_t *lbl = lv_label_create(btn);
        lv_obj_add_flag(lbl, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_set_style_text_font(lbl, STYLE_FONT_SMALL, LV_STATE_DEFAULT);
        lv_label_set_text(lbl, view_intl_get_string(model, STRINGS_ABILITA));
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 0, 0);

        lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    }

    VIEW_ADD_WATCHED_VARIABLE(&model->run.test_enable_coin_reader, 0);
    VIEW_ADD_WATCHED_VARIABLE(&model->run.minion.read.payment, 0);
    VIEW_ADD_WATCHED_ARRAY(model->run.minion.read.coins, DIGITAL_COIN_LINES_NUM, 0);

    update_page(model, pdata);
}


static pman_msg_t page_event(pman_handle_t handle, void *state, pman_event_t event) {
    struct page_data *pdata = state;

    mut_model_t *model = view_get_model(handle);

    pman_msg_t msg = PMAN_MSG_NULL;

    switch (event.tag) {
        case PMAN_EVENT_TAG_OPEN:
            view_get_protocol(handle)->set_test_mode(handle, 1);
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
                            view_get_protocol(handle)->set_test_mode(handle, 0);
                            msg.stack_msg = PMAN_STACK_MSG_BACK();
                            break;

                        case BTN_NEXT_ID:
                            view_get_protocol(handle)->digital_coin_reader_enable(handle, 0);
                            msg.stack_msg = PMAN_STACK_MSG_SWAP(&page_test_inputs);
                            break;

                        case BTN_ENABLE_ID:
                            view_get_protocol(handle)->digital_coin_reader_enable(handle,
                                                                                  !model->run.test_enable_coin_reader);
                            break;

                        case BTN_CLEAR_ID:
                            view_get_protocol(handle)->clear_coins(handle);
                            update_page(model, pdata);
                            break;
                    }
                    break;
                }

                case LV_EVENT_LONG_PRESSED: {
                    switch (obj_data->id) {
                        case BTN_NEXT_ID:
                            msg.stack_msg = PMAN_STACK_MSG_SWAP_EXTRA(&page_test_drum, (void *)(uintptr_t)1);
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
    const uint16_t values[] = {10, 20, 50, 100, 200};
    uint16_t       total    = 0;

    {
        char string[8] = {0};
        snprintf(string, sizeof(string), "%i", model->run.minion.read.payment);
        lv_table_set_cell_value(pdata->table_digital, 0, 1, string);
    }

    for (size_t i = DIGITAL_COIN_LINE_1; i <= DIGITAL_COIN_LINE_5; i++) {
        char string[8] = {0};
        snprintf(string, sizeof(string), "%i", model->run.minion.read.coins[i]);
        total += model->run.minion.read.coins[i] * values[i];
        lv_table_set_cell_value(pdata->table_digital, i + 2, 1, string);
    }

    {
        char string[8] = {0};
        snprintf(string, sizeof(string), "%1.2f", (float)total / 100.);
        lv_table_set_cell_value(pdata->table_digital, DIGITAL_COIN_LINE_5 + 3, 1, string);
    }

    if (model->run.test_enable_coin_reader) {
        lv_led_on(pdata->led_enable);
    } else {
        lv_led_off(pdata->led_enable);
    }
}


const pman_page_t page_test_coins_digital = {
    .create        = create_page,
    .destroy       = pman_destroy_all,
    .open          = open_page,
    .close         = pman_close_all,
    .process_event = page_event,
};
