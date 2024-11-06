#include <assert.h>
#include <stdlib.h>
#include "lvgl.h"
#include "model/model.h"
#include "../view.h"
#include "../theme/style.h"
#include "../intl/intl.h"
#include <esp_log.h>
#include "bsp/tft/display.h"
#include "../theme/style.h"
#include "../common.h"
#include "config/app_config.h"


LV_IMG_DECLARE(img_wool_demo);
LV_IMG_DECLARE(img_cold_demo);
LV_IMG_DECLARE(img_lukewarm_demo);
LV_IMG_DECLARE(img_warm_demo);
LV_IMG_DECLARE(img_hot_demo);
LV_IMG_DECLARE(img_marble_english);
LV_IMG_DECLARE(img_marble_italiano);
LV_IMG_DECLARE(img_pause_pressed_demo);
LV_IMG_DECLARE(img_pause_released_demo);
LV_IMG_DECLARE(img_stop_pressed_demo);
LV_IMG_DECLARE(img_stop_released_demo);


#define SETTINGS_DRAG_WIDTH    32
#define SETTINGS_DRAG_HEIGHT   100
#define SETTINGS_DRAWER_WIDTH  128
#define SETTINGS_DRAWER_HEIGHT 100
#define SETTINGS_BTN_WIDTH     64

#define SCALE_SMALL_ICON      160
#define SCALE_SELECTED_ICON   280
#define SCALE_CONTROL         180
#define SCALE_EMPTY_CORNER    220
#define SCALE_CLICK_ANIMATION 280

#define BUTTON_INDEX_HOT      0
#define BUTTON_INDEX_WARM     1
#define BUTTON_INDEX_LUKEWARM 2
#define BUTTON_INDEX_WOOL     3
#define BUTTON_INDEX_LANGUAGE 4
#define BUTTON_INDEX_COLD     5

#define MAX_IMAGES 6


enum {
    BTN_PROGRAM_ID,
    BTN_LANGUAGE_ID,
    BTN_STOP_ID,
    BTN_ALARM_ID,
    TIMER_CHANGE_PAGE_ID,
    TIMER_RESTORE_LANGUAGE_ID,
    OBJ_SETTINGS_ID,
    WATCH_STATE_ID,
    WATCH_INFO_ID,
};


static const char *TAG = "PageMain";


struct page_data {
    lv_obj_t *buttons[BASE_PROGRAMS_NUM];
    lv_obj_t *images[BASE_PROGRAMS_NUM];
    lv_obj_t *labels[BASE_PROGRAMS_NUM];

    lv_obj_t *image_language;
    lv_obj_t *image_stop;

    lv_obj_t *label_status;

    lv_obj_t *obj_handle;
    lv_obj_t *obj_drawer;

    popup_t alarm_popup;

    uint8_t alarm_pacified;
    int16_t last_alarm;

    pman_timer_t *timer_change_page;
    pman_timer_t *timer_restore_language;
};


static void update_page(model_t *model, struct page_data *pdata);
static void handle_alarm(model_t *model, struct page_data *pdata);


static void *create_page(pman_handle_t handle, void *extra) {
    (void)handle;
    (void)extra;

    struct page_data *pdata = lv_malloc(sizeof(struct page_data));
    assert(pdata != NULL);

    pdata->timer_change_page      = pman_timer_create(handle, 250, (void *)(uintptr_t)TIMER_CHANGE_PAGE_ID);
    pdata->timer_restore_language = pman_timer_create(handle, 10000, (void *)(uintptr_t)TIMER_RESTORE_LANGUAGE_ID);
    pdata->last_alarm             = 0;
    pdata->alarm_pacified         = 0;

    return pdata;
}


static void open_page(pman_handle_t handle, void *state) {
    struct page_data *pdata = state;
    (void)pdata;

    model_t *model = pman_get_user_data(handle);

    lv_obj_t *cont = lv_obj_create(lv_screen_active());
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(cont, LV_HOR_RES, LV_VER_RES);
    lv_obj_add_style(cont, &style_padless_cont, LV_STATE_DEFAULT);
    lv_obj_add_style(cont, &style_borderless_cont, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cont, VIEW_STYLE_COLOR_WHITE, LV_STATE_DEFAULT);

    lv_obj_t *top_cont = lv_obj_create(cont);
    lv_obj_remove_flag(top_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(top_cont, &style_transparent_cont, LV_STATE_DEFAULT);
    lv_obj_set_size(top_cont, LV_HOR_RES, 164);
    lv_obj_align(top_cont, LV_ALIGN_TOP_MID, 0, -28);
    lv_obj_set_style_pad_column(top_cont, 8, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_hor(top_cont, 8, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(top_cont, 4, LV_STATE_DEFAULT);
    lv_obj_set_layout(top_cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(top_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(top_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);

    const lv_img_dsc_t *program_icons[BASE_PROGRAMS_NUM] = {
        &img_hot_demo, &img_warm_demo, &img_lukewarm_demo, &img_wool_demo, &img_cold_demo,
    };

    for (size_t i = 0; i < MAX_IMAGES / 2; i++) {
        lv_obj_t *button = lv_button_create(top_cont);
        lv_obj_set_flex_grow(button, 1);
        lv_obj_set_height(button, LV_PCT(100));
        lv_obj_add_style(button, &style_tall_button, LV_STATE_DEFAULT);
        lv_obj_add_style(button, &style_tall_button_checked, LV_STATE_CHECKED);
        lv_obj_set_style_pad_top(button, 28, LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(button, 12, LV_STATE_DEFAULT);
        lv_obj_set_style_radius(button, 24, 0);
        view_register_object_default_callback_with_number(button, BTN_PROGRAM_ID, i);

        lv_obj_t *image = lv_image_create(button);
        lv_image_set_src(image, program_icons[i]);
        lv_image_set_scale(image, 280);
        lv_obj_align(image, LV_ALIGN_BOTTOM_MID, 0, 0);

        lv_obj_t *label = lv_label_create(button);
        lv_obj_set_style_text_font(label, STYLE_FONT_BIG, LV_STATE_DEFAULT);
        lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);

        pdata->buttons[i] = button;
        pdata->images[i]  = image;
        pdata->labels[i]  = label;
    }

    lv_obj_t *bottom_cont = lv_obj_create(cont);
    lv_obj_remove_flag(bottom_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(bottom_cont, &style_transparent_cont, LV_STATE_DEFAULT);
    lv_obj_set_size(bottom_cont, LV_HOR_RES, 164);
    lv_obj_align(bottom_cont, LV_ALIGN_BOTTOM_MID, 0, 28);
    lv_obj_set_style_pad_column(bottom_cont, 8, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_hor(bottom_cont, 8, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(bottom_cont, 4, LV_STATE_DEFAULT);
    lv_obj_set_layout(bottom_cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(bottom_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bottom_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);

    for (size_t i = MAX_IMAGES / 2; i < MAX_IMAGES; i++) {
        lv_obj_t *button = lv_button_create(bottom_cont);
        lv_obj_set_flex_grow(button, 1);
        lv_obj_set_height(button, LV_PCT(100));
        lv_obj_add_style(button, &style_tall_button, LV_STATE_DEFAULT);
        lv_obj_add_style(button, &style_tall_button_checked, LV_STATE_CHECKED);
        lv_obj_set_style_pad_top(button, 12, LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(button, 28, LV_STATE_DEFAULT);
        lv_obj_set_style_radius(button, 24, 0);

        lv_obj_t *image = lv_image_create(button);
        lv_image_set_scale(image, 280);
        lv_obj_align(image, LV_ALIGN_TOP_MID, 0, 0);
        if (i != BUTTON_INDEX_LANGUAGE) {
            lv_obj_t *label = lv_label_create(button);
            lv_obj_set_style_text_font(label, STYLE_FONT_BIG, LV_STATE_DEFAULT);
            lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, 0);

            size_t program_index = i > BUTTON_INDEX_LANGUAGE ? i - 1 : i;
            lv_image_set_src(image, program_icons[program_index]);
            lv_obj_add_flag(image, LV_OBJ_FLAG_EVENT_BUBBLE);
            view_register_object_default_callback_with_number(button, BTN_PROGRAM_ID, program_index);
            pdata->buttons[program_index] = button;
            pdata->images[program_index]  = image;
            pdata->labels[program_index]  = label;
        } else {
            {
                lv_image_set_scale(image, 180);
                lv_obj_align(image, LV_ALIGN_TOP_MID, 0, -12);
                lv_obj_remove_flag(button, LV_OBJ_FLAG_CLICKABLE);
                lv_obj_add_flag(image, LV_OBJ_FLAG_CLICKABLE);
                view_register_object_default_callback(image, BTN_LANGUAGE_ID);

                pdata->image_language = image;
            }

            {
                lv_obj_t *image = lv_image_create(button);
                lv_image_set_src(image, &img_pause_released_demo);
                lv_obj_add_flag(image, LV_OBJ_FLAG_CLICKABLE);
                view_register_object_default_callback(image, BTN_STOP_ID);

                lv_obj_align(image, LV_ALIGN_BOTTOM_MID, 0, -8);

                pdata->image_stop = image;
            }
        }
    }

    lv_obj_t *lbl = lv_label_create(cont);
    lv_obj_set_style_text_font(lbl, STYLE_FONT_MEDIUM, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lbl, lv_color_black(), LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(lbl, LV_HOR_RES);
    lv_obj_center(lbl);
    pdata->label_status = lbl;

    {
        lv_obj_t *obj = lv_obj_create(cont);
        lv_obj_set_size(obj, SETTINGS_DRAG_WIDTH, SETTINGS_DRAG_HEIGHT);
        lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_add_style(obj, (lv_style_t *)&style_transparent_cont, LV_STATE_DEFAULT);
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
        view_register_object_default_callback(obj, OBJ_SETTINGS_ID);
        pdata->obj_handle = obj;

        lv_obj_t *drawer = lv_obj_create(cont);
        lv_obj_set_size(drawer, SETTINGS_DRAWER_WIDTH, SETTINGS_DRAWER_HEIGHT);
        lv_obj_align_to(drawer, obj, LV_ALIGN_OUT_TOP_MID, 0, 0);
        pdata->obj_drawer = drawer;

        lv_obj_t *btn = lv_btn_create(drawer);
        lv_obj_set_size(btn, SETTINGS_BTN_WIDTH, SETTINGS_BTN_WIDTH);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, LV_SYMBOL_SETTINGS);
        lv_obj_center(lbl);
        lv_obj_align(btn, LV_ALIGN_CENTER, 0, 0);
    }

    pdata->alarm_popup = view_common_alarm_popup_create(lv_screen_active(), BTN_ALARM_ID);
    lv_obj_set_size(pdata->alarm_popup.blanket, LV_HOR_RES, LV_VER_RES * 3 / 4);

    ESP_LOGI(TAG, "Open");

    VIEW_ADD_WATCHED_VARIABLE(&model->run.minion.read.cycle_state, WATCH_STATE_ID);
    VIEW_ADD_WATCHED_VARIABLE(&model->run.current_program_index, WATCH_STATE_ID);
    VIEW_ADD_WATCHED_VARIABLE(&model->run.current_step_index, WATCH_STATE_ID);
    VIEW_ADD_WATCHED_VARIABLE(&model->run.minion.read.default_temperature, WATCH_INFO_ID);
    VIEW_ADD_WATCHED_VARIABLE(&model->run.minion.read.remaining_time_seconds, WATCH_INFO_ID);
    VIEW_ADD_WATCHED_VARIABLE(&model->run.minion.read.alarms, WATCH_INFO_ID);

    handle_alarm(model, pdata);
    update_page(model, pdata);
}


static pman_msg_t page_event(pman_handle_t handle, void *state, pman_event_t event) {
    (void)handle;

    pman_msg_t msg = PMAN_MSG_NULL;

    struct page_data *pdata = state;
    mut_model_t      *model = pman_get_user_data(handle);

    switch (event.tag) {
        case PMAN_EVENT_TAG_USER: {
            view_event_t *view_event = event.as.user;
            switch (view_event->tag) {
                case VIEW_EVENT_TAG_PAGE_WATCHER: {
                    switch (view_event->as.page_watcher.code) {
                        case WATCH_STATE_ID:
                            if (model_is_cycle_active(model)) {
                                pman_timer_reset(pdata->timer_restore_language);
                                pman_timer_pause(pdata->timer_restore_language);
                            } else {
                                pman_timer_resume(pdata->timer_restore_language);
                            }
                            update_page(model, pdata);
                            break;

                        case WATCH_INFO_ID:
                            handle_alarm(model, pdata);
                            update_page(model, pdata);
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

        case PMAN_EVENT_TAG_TIMER: {
            switch ((int)(uintptr_t)event.as.timer->user_data) {
                case TIMER_CHANGE_PAGE_ID: {
                    pman_stack_msg_t         pw_msg = PMAN_STACK_MSG_SWAP(&page_menu);
                    password_page_options_t *opts =
                        view_common_default_password_page_options(pw_msg, (const char *)APP_CONFIG_PASSWORD);
                    msg.stack_msg = PMAN_STACK_MSG_PUSH_PAGE_EXTRA(&page_password, opts);
                    break;
                }

                case TIMER_RESTORE_LANGUAGE_ID: {
                    model_reset_temporary_language(model);
                    break;
                }

                default:
                    break;
            }
            break;
        }

        case PMAN_EVENT_TAG_LVGL: {
            lv_obj_t           *target   = lv_event_get_target(event.as.lvgl);
            view_object_data_t *obj_data = lv_obj_get_user_data(target);

            if (obj_data == NULL) {
                break;
            }

            switch (lv_event_get_code(event.as.lvgl)) {
                case LV_EVENT_CLICKED: {
                    pman_timer_reset(pdata->timer_restore_language);

                    switch (obj_data->id) {
                        case BTN_ALARM_ID: {
                            if (!model_is_any_alarm_active(model)) {
                                pdata->alarm_pacified = 0;
                                pdata->last_alarm     = model->run.minion.read.alarms;
                            } else {
                                pdata->alarm_pacified = 1;
                            }

                            update_page(model, pdata);
                            break;
                        }

                        case BTN_PROGRAM_ID:
                            if (model_is_cycle_paused(model) && obj_data->number == model->run.current_program_index) {
                                view_get_protocol(handle)->resume_cycle(handle);
                            } else {
                                view_get_protocol(handle)->start_program(handle, obj_data->number);
                            }
                            break;

                        case BTN_STOP_ID:
                            if (model_is_cycle_paused(model)) {
                                view_get_protocol(handle)->stop_cycle(handle);
                            } else {
                                view_get_protocol(handle)->pause_cycle(handle);
                            }
                            break;

                        case BTN_LANGUAGE_ID: {
                            pman_timer_resume(pdata->timer_restore_language);
                            model->run.temporary_language = (model->run.temporary_language + 1) % LANGUAGES_NUM;
                            update_page(model, pdata);
                            break;
                        }

                        default:
                            break;
                    }
                    break;
                }

                case LV_EVENT_PRESSED: {
                    switch (obj_data->id) {
                        case BTN_STOP_ID: {
                            if (model_is_cycle_paused(model)) {
                                lv_image_set_src(pdata->image_stop, &img_stop_pressed_demo);
                            } else {
                                lv_image_set_src(pdata->image_stop, &img_pause_pressed_demo);
                            }
                            break;
                        }

                        default:
                            break;
                    }
                    break;
                }

                case LV_EVENT_PRESSING: {
                    switch (obj_data->id) {
                        case OBJ_SETTINGS_ID: {
                            lv_indev_t *indev = lv_indev_get_act();
                            if (indev != NULL) {
                                lv_point_t vect;
                                lv_indev_get_vect(indev, &vect);

                                lv_coord_t y = lv_obj_get_y_aligned(target) + vect.y;

                                if (y > 0 && y < SETTINGS_DRAWER_HEIGHT) {
                                    lv_obj_set_y(target, y);
                                    lv_obj_align_to(pdata->obj_drawer, target, LV_ALIGN_OUT_TOP_MID, 0, 0);
                                }
                            }
                            break;
                        }

                        default:
                            break;
                    }
                    break;
                }

                case LV_EVENT_RELEASED: {
                    switch (obj_data->id) {
                        case BTN_STOP_ID: {
                            if (model_is_cycle_running(model)) {
                                lv_image_set_src(pdata->image_stop, &img_pause_released_demo);
                            } else {
                                lv_image_set_src(pdata->image_stop, &img_stop_released_demo);
                            }
                            break;
                        }

                        case OBJ_SETTINGS_ID: {
                            lv_coord_t y = lv_obj_get_y_aligned(target);

                            if (y > SETTINGS_DRAWER_HEIGHT / 2) {
                                lv_obj_align(target, LV_ALIGN_TOP_MID, 0, SETTINGS_DRAWER_HEIGHT);
                                pman_timer_reset(pdata->timer_change_page);
                                pman_timer_resume(pdata->timer_change_page);
                            } else {
                                lv_obj_align(target, LV_ALIGN_TOP_MID, 0, 0);
                            }
                            lv_obj_align_to(pdata->obj_drawer, target, LV_ALIGN_OUT_TOP_MID, 0, 0);
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


static void update_page(model_t *model, struct page_data *pdata) {
    language_t language = model->run.temporary_language;

    if (model_is_cycle_paused(model)) {
        view_common_set_hidden(pdata->image_stop, 0);
        lv_image_set_src(pdata->image_stop, lv_obj_has_state(pdata->image_stop, LV_STATE_PRESSED)
                                                ? &img_stop_pressed_demo
                                                : &img_stop_released_demo);
    } else if (model_is_cycle_active(model)) {
        view_common_set_hidden(pdata->image_stop, 0);
        lv_image_set_src(pdata->image_stop, lv_obj_has_state(pdata->image_stop, LV_STATE_PRESSED)
                                                ? &img_pause_pressed_demo
                                                : &img_pause_released_demo);
    } else {
        view_common_set_hidden(pdata->image_stop, 1);
    }

    for (base_program_t program_index = 0; program_index < BASE_PROGRAMS_NUM; program_index++) {
        lv_label_set_text_fmt(pdata->labels[program_index], "%i°",
                              model_get_program_display_temperature(model, program_index));

        if (model_is_cycle_active(model)) {
            if (program_index == model->run.current_program_index) {
                lv_obj_add_state(pdata->buttons[program_index], LV_STATE_CHECKED);
            } else {
                lv_obj_remove_state(pdata->buttons[program_index], LV_STATE_CHECKED);
            }
        } else {
            lv_obj_remove_state(pdata->buttons[program_index], LV_STATE_CHECKED);
        }
    }

    if (model->run.minion.read.alarms == 0) {
        if (model_is_porthole_open(model)) {
            lv_label_set_text(pdata->label_status, view_intl_get_string_in_language(language, STRINGS_OBLO_APERTO));
        } else if (model_is_cycle_active(model)) {
            uint16_t minutes = model->run.minion.read.remaining_time_seconds / 60;
            uint16_t seconds = model->run.minion.read.remaining_time_seconds % 60;

            lv_label_set_text_fmt(pdata->label_status, "%02i:%02i", minutes, seconds);
        } else {
            lv_label_set_text(pdata->label_status,
                              view_intl_get_string_in_language(language, STRINGS_SCELTA_PROGRAMMA));
        }
    } else {
        view_common_format_alarm(pdata->label_status, model->run.minion.read.alarms, language);
    }

    const lv_img_dsc_t *language_icons[2] = {&img_marble_italiano, &img_marble_english};
    lv_img_set_src(pdata->image_language, language_icons[model->run.temporary_language]);

    if (!pdata->alarm_pacified && model_is_any_alarm_active(model)) {
        view_common_set_hidden(pdata->alarm_popup.blanket, 0);
        view_common_alarm_popup_update(model, &pdata->alarm_popup, model->run.temporary_language);
    } else {
        view_common_set_hidden(pdata->alarm_popup.blanket, 1);
    }
}


static void handle_alarm(model_t *model, struct page_data *pdata) {
    ESP_LOGI(TAG, "Alarm %i 0x%02X %i", pdata->last_alarm, model->run.minion.read.alarms, pdata->alarm_pacified);

    if (pdata->alarm_pacified && model_is_any_alarm_active(model)) {
        pdata->last_alarm = model->run.minion.read.alarms;
    } else if (pdata->last_alarm != model->run.minion.read.alarms) {
        pdata->alarm_pacified = 0;
    }
    if (model_is_any_alarm_active(model)) {
        pdata->last_alarm = model->run.minion.read.alarms;
    }
}


static void close_page(void *state) {
    struct page_data *pdata = state;
    pman_timer_pause(pdata->timer_change_page);
    pman_timer_pause(pdata->timer_restore_language);
    lv_obj_clean(lv_scr_act());
}


static void destroy_page(void *state, void *extra) {
    (void)extra;
    struct page_data *pdata = state;
    pman_timer_delete(pdata->timer_change_page);
    pman_timer_delete(pdata->timer_restore_language);
    lv_free(state);
}


const pman_page_t page_main_demo = {
    .create        = create_page,
    .destroy       = destroy_page,
    .open          = open_page,
    .close         = close_page,
    .process_event = page_event,
};
