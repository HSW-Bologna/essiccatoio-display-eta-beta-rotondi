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


LV_IMG_DECLARE(img_wool);
LV_IMG_DECLARE(img_cold);
LV_IMG_DECLARE(img_lukewarm);
LV_IMG_DECLARE(img_warm);
LV_IMG_DECLARE(img_hot);
LV_IMG_DECLARE(img_wool_off);
LV_IMG_DECLARE(img_cold_off);
LV_IMG_DECLARE(img_lukewarm_off);
LV_IMG_DECLARE(img_warm_off);
LV_IMG_DECLARE(img_hot_off);
LV_IMG_DECLARE(img_stop);
LV_IMG_DECLARE(img_pause);
LV_IMG_DECLARE(img_start);
LV_IMG_DECLARE(img_italiano);
LV_IMG_DECLARE(img_english);
LV_IMG_DECLARE(img_empty);
LV_IMG_DECLARE(img_button_background);


#define SETTINGS_DRAG_WIDTH    96
#define SETTINGS_DRAG_HEIGHT   24
#define SETTINGS_DRAWER_WIDTH  96
#define SETTINGS_DRAWER_HEIGHT 96
#define SETTINGS_BTN_WIDTH     64

#define SCALE_SMALL_ICON      160
#define SCALE_SELECTED_ICON   280
#define SCALE_CONTROL         180
#define SCALE_EMPTY_CORNER    220
#define SCALE_CLICK_ANIMATION 280


enum {
    BTN_PROGRAM_ID,
    BTN_START_ID,
    BTN_STOP_ID,
    BTN_LANGUAGE_ID,
    TIMER_CHANGE_PAGE_ID,
    TIMER_RESTORE_LANGUAGE_ID,
    OBJ_SETTINGS_ID,
    WATCH_STATE_ID,
    WATCH_INFO_ID,
};


typedef struct {
    lv_obj_t *image_background;
    lv_obj_t *image_icon;
} icon_button_t;


static const char *TAG = "PageMain";


struct page_data {
    lv_obj_t     *buttons[BASE_PROGRAMS_NUM];
    lv_obj_t     *image_stop;
    lv_obj_t     *image_start;
    icon_button_t icon_button_language;

    lv_obj_t *label_time;
    lv_obj_t *label_temperature;
    lv_obj_t *label_status;

    lv_obj_t *image_empty_corner;

    lv_obj_t *obj_empty_side;
    lv_obj_t *obj_handle;
    lv_obj_t *obj_drawer;

    pman_timer_t *timer_change_page;
    pman_timer_t *timer_restore_language;
};


static void          hit_test_event_cb(lv_event_t *event);
static void          update_programs(model_t *model, struct page_data *pdata);
static void          update_info(model_t *model, struct page_data *pdata);
static lv_obj_t     *old_icon_button_create(lv_obj_t *parent, const lv_image_dsc_t *img_dsc);
static icon_button_t icon_button_create(lv_obj_t *parent, const lv_img_dsc_t *img_dsc);
static void          icon_button_scale(icon_button_t *icon_button, int32_t scale);
static lv_obj_t     *program_button_create(lv_obj_t *parent, base_program_t program);

static const lv_img_dsc_t *images_on[] = {
    &img_wool, &img_cold, &img_lukewarm, &img_warm, &img_hot,
};
static const lv_img_dsc_t *images_off[] = {
    &img_wool_off, &img_cold_off, &img_lukewarm_off, &img_warm_off, &img_hot_off,
};


static void *create_page(pman_handle_t handle, void *extra) {
    (void)handle;
    (void)extra;

    struct page_data *pdata = lv_malloc(sizeof(struct page_data));
    assert(pdata != NULL);

    pdata->timer_change_page      = pman_timer_create(handle, 250, (void *)(uintptr_t)TIMER_CHANGE_PAGE_ID);
    pdata->timer_restore_language = pman_timer_create(handle, 10000, (void *)(uintptr_t)TIMER_RESTORE_LANGUAGE_ID);

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
    lv_obj_set_style_bg_color(cont, VIEW_STYLE_COLOR_BACKGROUND, LV_STATE_DEFAULT);

    {
        lv_obj_t *img = lv_image_create(cont);
        lv_obj_set_style_image_recolor_opa(img, LV_OPA_COVER, LV_STATE_DEFAULT);
        lv_obj_set_style_image_recolor(img, lv_color_white(), LV_STATE_DEFAULT);
        lv_image_set_src(img, &img_empty);
        pdata->image_empty_corner = img;
    }

    {
        lv_obj_t *obj = lv_obj_create(cont);
        lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(obj, lv_color_white(), LV_STATE_DEFAULT);
        lv_obj_set_style_radius(obj, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(obj, 0, LV_STATE_DEFAULT);
        lv_obj_set_size(obj, LV_HOR_RES, 44);
        pdata->obj_empty_side = obj;

        lv_obj_t *lbl = lv_label_create(obj);
        lv_obj_set_style_text_font(lbl, STYLE_FONT_SMALL, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(lbl, lv_color_black(), LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
        lv_label_set_long_mode(lbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_width(lbl, LV_HOR_RES);
        lv_obj_center(lbl);
        pdata->label_status = lbl;
    }

    {
        lv_obj_t *lbl = lv_label_create(cont);
        lv_obj_set_style_text_font(lbl, STYLE_FONT_SMALL, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(lbl, lv_color_black(), LV_STATE_DEFAULT);
        pdata->label_temperature = lbl;
    }

    {
        lv_obj_t *lbl = lv_label_create(cont);
        lv_obj_set_style_text_font(lbl, STYLE_FONT_SMALL, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(lbl, lv_color_black(), LV_STATE_DEFAULT);
        pdata->label_time = lbl;
    }

    for (base_program_t i = 0; i < BASE_PROGRAMS_NUM; i++) {
        pdata->buttons[i] = program_button_create(cont, i);
    }

    pdata->image_start = old_icon_button_create(cont, &img_start);
    lv_image_set_scale(pdata->image_start, SCALE_CONTROL);
    view_register_object_default_callback(pdata->image_start, BTN_START_ID);

    pdata->image_stop = old_icon_button_create(cont, &img_stop);
    lv_image_set_scale(pdata->image_stop, SCALE_CONTROL);
    view_register_object_default_callback(pdata->image_stop, BTN_STOP_ID);

    pdata->icon_button_language = icon_button_create(cont, &img_italiano);
    icon_button_scale(&pdata->icon_button_language, SCALE_SMALL_ICON);
    view_register_object_default_callback(pdata->icon_button_language.image_background, BTN_LANGUAGE_ID);

    {
        lv_obj_t *obj = lv_obj_create(cont);
        lv_obj_set_size(obj, SETTINGS_DRAG_WIDTH, SETTINGS_DRAG_HEIGHT);
        lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_add_style(obj, (lv_style_t *)&style_transparent_cont, LV_STATE_DEFAULT);
        lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
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

    ESP_LOGI(TAG, "Main open");

    VIEW_ADD_WATCHED_VARIABLE(&model->run.minion.read.cycle_state, WATCH_STATE_ID);
    VIEW_ADD_WATCHED_VARIABLE(&model->run.current_program_index, WATCH_STATE_ID);
    VIEW_ADD_WATCHED_VARIABLE(&model->run.current_step_index, WATCH_STATE_ID);
    VIEW_ADD_WATCHED_VARIABLE(&model->run.minion.read.default_temperature, WATCH_INFO_ID);
    VIEW_ADD_WATCHED_VARIABLE(&model->run.minion.read.remaining_time_seconds, WATCH_INFO_ID);
    VIEW_ADD_WATCHED_VARIABLE(&model->run.minion.read.alarms, WATCH_INFO_ID);

    update_programs(model, pdata);
    update_info(model, pdata);
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
                            update_programs(model, pdata);
                            break;

                        case WATCH_INFO_ID:
                            update_info(model, pdata);
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
                    update_info(model, pdata);
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
                        case BTN_PROGRAM_ID:
                            view_get_protocol(handle)->start_program(handle, obj_data->number);
                            break;

                        case BTN_LANGUAGE_ID: {
                            pman_timer_resume(pdata->timer_restore_language);
                            model->run.temporary_language = (model->run.temporary_language + 1) % LANGUAGES_NUM;
                            update_info(model, pdata);
                            break;
                        }

                        case BTN_STOP_ID:
                            if (model_is_cycle_paused(model)) {
                                view_get_protocol(handle)->stop_cycle(handle);
                            } else {
                                view_get_protocol(handle)->pause_cycle(handle);
                            }
                            break;

                        case BTN_START_ID:
                            view_get_protocol(handle)->resume_cycle(handle);
                            break;

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


static void update_programs(model_t *model, struct page_data *pdata) {
#define REDUCE_BY(Value, Reduction)                                                                                    \
    if ((Value) > 0) {                                                                                                 \
        if ((Value) > (Reduction)) {                                                                                   \
            (Value) -= (Reduction);                                                                                    \
        } else {                                                                                                       \
            (Value) = 0;                                                                                               \
        }                                                                                                              \
    } else {                                                                                                           \
        if (-(Value) > (Reduction)) {                                                                                  \
            (Value) += (Reduction);                                                                                    \
        } else {                                                                                                       \
            (Value) = 0;                                                                                               \
        }                                                                                                              \
    }

#define INCREASE_BY(Value, Increase)                                                                                   \
    if ((Value) > 0) {                                                                                                 \
        (Value) += (Increase);                                                                                         \
    } else {                                                                                                           \
        (Value) -= (Increase);                                                                                         \
    }

    const int base_delta = 72;
    const int delta_xs[] = {0, base_delta, -base_delta, base_delta, -base_delta};
    const int delta_ys[] = {0, base_delta, base_delta, -base_delta, -base_delta};

    uint8_t  cycle_active          = model_is_cycle_active(model);
    uint16_t current_program_index = model->run.current_program_index;

    for (base_program_t program = 0; program < BASE_PROGRAMS_NUM; program++) {
        if (cycle_active) {
            uint8_t centered_x = delta_xs[current_program_index] == 0;
            uint8_t centered_y = delta_xs[current_program_index] == 0;

            int delta_x = delta_xs[program];
            int delta_y = delta_ys[program];

            const int shift = 60;     // If a program is selected all buttons should be shifted
                                      // towards the opposing corner
            int reduction = 22;       // If a program is selected all other buttons
                                      // should stand closer to each other
            int reduction_center = 8;

            int shift_x = delta_xs[current_program_index] > 0 ? -shift : shift;
            int shift_y = delta_ys[current_program_index] > 0 ? -shift : shift;

            if (program == current_program_index) {
                lv_image_set_scale(pdata->buttons[program], SCALE_SELECTED_ICON);
                lv_img_set_src(pdata->buttons[program], images_on[program]);

                reduction = 8;
            } else {
                lv_image_set_scale(pdata->buttons[program], SCALE_SMALL_ICON);
                lv_img_set_src(pdata->buttons[program], images_off[program]);
            }

            if (centered_x) {
                shift_x = 0;
                REDUCE_BY(delta_x, reduction_center);
            } else {
                REDUCE_BY(delta_x, reduction);
            }
            if (centered_y) {
                shift_y = 0;
                REDUCE_BY(delta_x, reduction_center);
            } else {
                REDUCE_BY(delta_y, reduction);
            }

            if (current_program_index == BASE_PROGRAM_WOOL) {
                delta_x += 56;
            } else {
                INCREASE_BY(shift_x, 4);
            }

            lv_obj_align(pdata->buttons[program], LV_ALIGN_CENTER, delta_x + shift_x, delta_y + shift_y);
        } else {
            lv_image_set_scale(pdata->buttons[program], 255);
            lv_obj_set_style_opa(pdata->buttons[program], LV_OPA_COVER, LV_STATE_DEFAULT);
            lv_img_set_src(pdata->buttons[program], images_off[program]);

            lv_obj_align(pdata->buttons[program], LV_ALIGN_CENTER, delta_xs[program], delta_ys[program] - 24);
        }
    }

    if (cycle_active) {
        view_common_set_hidden(pdata->image_stop, 0);

        lv_obj_t *img_selected = pdata->buttons[current_program_index];

        view_common_set_hidden(pdata->image_empty_corner, 0);
        view_common_set_hidden(pdata->label_temperature, 0);
        view_common_set_hidden(pdata->label_time, 0);

        const int32_t stop_shift_x     = 52;
        const int32_t stop_shift_y     = 44;
        const int32_t language_shift_x = 16;
        const int32_t language_shift_y = 10;
        const int32_t sensors_shift_y  = 52;
        const int32_t play_shift       = 52;

        switch (current_program_index) {
            case BASE_PROGRAM_WOOL:
                lv_obj_align_to(pdata->image_stop, img_selected, LV_ALIGN_OUT_LEFT_MID, 9, 21);
                lv_obj_align_to(pdata->image_start, pdata->image_stop, LV_ALIGN_CENTER, -play_shift, -play_shift);
                lv_obj_align_to(pdata->icon_button_language.image_background, pdata->image_start, LV_ALIGN_CENTER, 0,
                                -86);

                lv_image_set_scale(pdata->image_empty_corner, SCALE_EMPTY_CORNER);
                lv_image_set_rotation(pdata->image_empty_corner, 1800);
                lv_obj_align(pdata->image_empty_corner, LV_ALIGN_BOTTOM_LEFT, -12, 12);

                lv_obj_align(pdata->label_time, LV_ALIGN_BOTTOM_LEFT, 4, -sensors_shift_y - 20);
                lv_obj_align(pdata->label_temperature, LV_ALIGN_BOTTOM_LEFT, 4, -sensors_shift_y);

                lv_obj_align(pdata->obj_empty_side, LV_ALIGN_BOTTOM_MID, 0, 0);
                break;

            case BASE_PROGRAM_COLD:
                lv_obj_align_to(pdata->image_stop, img_selected, LV_ALIGN_OUT_TOP_RIGHT, stop_shift_x, stop_shift_y);
                lv_obj_align_to(pdata->image_start, pdata->image_stop, LV_ALIGN_CENTER, play_shift, play_shift);
                lv_obj_align(pdata->icon_button_language.image_background, LV_ALIGN_TOP_RIGHT, language_shift_x,
                             -language_shift_y);

                lv_image_set_scale(pdata->image_empty_corner, SCALE_EMPTY_CORNER);
                lv_image_set_rotation(pdata->image_empty_corner, 900);
                lv_obj_align(pdata->image_empty_corner, LV_ALIGN_BOTTOM_RIGHT, 12, 0);

                lv_obj_align(pdata->label_time, LV_ALIGN_BOTTOM_RIGHT, -4, -sensors_shift_y - 20);
                lv_obj_align(pdata->label_temperature, LV_ALIGN_BOTTOM_RIGHT, -4, -sensors_shift_y);

                lv_obj_align(pdata->obj_empty_side, LV_ALIGN_BOTTOM_MID, 0, 0);
                break;

            case BASE_PROGRAM_LUKEWARM:
                lv_obj_align_to(pdata->image_stop, img_selected, LV_ALIGN_OUT_TOP_LEFT, -stop_shift_x, stop_shift_y);
                lv_obj_align_to(pdata->image_start, pdata->image_stop, LV_ALIGN_CENTER, -play_shift, play_shift);
                lv_obj_align(pdata->icon_button_language.image_background, LV_ALIGN_TOP_LEFT, -language_shift_x,
                             -language_shift_y);

                lv_image_set_scale(pdata->image_empty_corner, SCALE_EMPTY_CORNER);
                lv_image_set_rotation(pdata->image_empty_corner, 1800);
                lv_obj_align(pdata->image_empty_corner, LV_ALIGN_BOTTOM_LEFT, -12, 0);

                lv_obj_align(pdata->label_time, LV_ALIGN_BOTTOM_LEFT, 4, -sensors_shift_y - 20);
                lv_obj_align(pdata->label_temperature, LV_ALIGN_BOTTOM_LEFT, 4, -sensors_shift_y);

                lv_obj_align(pdata->obj_empty_side, LV_ALIGN_BOTTOM_MID, 0, 0);
                break;


            case BASE_PROGRAM_WARM:
                lv_obj_align_to(pdata->image_stop, img_selected, LV_ALIGN_OUT_BOTTOM_RIGHT, stop_shift_x,
                                -stop_shift_y);
                lv_obj_align_to(pdata->image_start, pdata->image_stop, LV_ALIGN_CENTER, play_shift, -play_shift);
                lv_obj_align(pdata->icon_button_language.image_background, LV_ALIGN_BOTTOM_RIGHT, language_shift_x,
                             language_shift_y);

                lv_image_set_scale(pdata->image_empty_corner, SCALE_EMPTY_CORNER);
                lv_image_set_rotation(pdata->image_empty_corner, 0);
                lv_obj_align(pdata->image_empty_corner, LV_ALIGN_TOP_RIGHT, 12, 0);

                lv_obj_align(pdata->label_time, LV_ALIGN_TOP_RIGHT, -4, sensors_shift_y + 20);
                lv_obj_align(pdata->label_temperature, LV_ALIGN_TOP_RIGHT, -4, sensors_shift_y);

                lv_obj_align(pdata->obj_empty_side, LV_ALIGN_TOP_MID, 0, 0);
                break;

            case BASE_PROGRAM_HOT:
                lv_obj_align_to(pdata->image_stop, img_selected, LV_ALIGN_OUT_BOTTOM_LEFT, -stop_shift_x,
                                -stop_shift_y);
                lv_obj_align_to(pdata->image_start, pdata->image_stop, LV_ALIGN_CENTER, -play_shift, -play_shift);
                lv_obj_align(pdata->icon_button_language.image_background, LV_ALIGN_BOTTOM_LEFT, -language_shift_x,
                             language_shift_y);

                lv_image_set_scale(pdata->image_empty_corner, SCALE_EMPTY_CORNER);
                lv_image_set_rotation(pdata->image_empty_corner, 2700);
                lv_obj_align(pdata->image_empty_corner, LV_ALIGN_TOP_LEFT, -12, -12);

                lv_obj_align(pdata->label_time, LV_ALIGN_TOP_LEFT, 4, sensors_shift_y + 20);
                lv_obj_align(pdata->label_temperature, LV_ALIGN_TOP_LEFT, 4, sensors_shift_y);

                lv_obj_align(pdata->obj_empty_side, LV_ALIGN_TOP_MID, 0, 0);
                break;
        }
    } else {
        lv_obj_align_to(pdata->icon_button_language.image_background, pdata->buttons[BASE_PROGRAM_WOOL],
                        LV_ALIGN_CENTER, -120, 0);

        lv_obj_align(pdata->obj_empty_side, LV_ALIGN_BOTTOM_MID, 0, 0);

        view_common_set_hidden(pdata->image_stop, 1);
        view_common_set_hidden(pdata->image_start, 1);
        view_common_set_hidden(pdata->image_empty_corner, 1);
        view_common_set_hidden(pdata->label_temperature, 1);
        view_common_set_hidden(pdata->label_time, 1);
    }

    if (model_is_cycle_paused(model)) {
        lv_image_set_src(pdata->image_stop, &img_stop);
        view_common_set_hidden(pdata->image_start, 0);
    } else {
        lv_image_set_src(pdata->image_stop, &img_pause);
        view_common_set_hidden(pdata->image_start, 1);
    }

#undef REDUCE_BY
#undef INCREASE_BY
}


static void update_info(model_t *model, struct page_data *pdata) {
    language_t language = model->run.temporary_language;

    const lv_image_dsc_t *icons_language[LANGUAGES_NUM] = {&img_italiano, &img_english};
    lv_image_set_src(pdata->icon_button_language.image_icon, icons_language[language]);

    program_step_t step = model_get_current_step(model);

    switch (step.type) {
        case PROGRAM_STEP_TYPE_DRYING:
            lv_label_set_text_fmt(pdata->label_temperature, "%i/%i °C", model->run.minion.read.default_temperature,
                                  step.drying.temperature);
            break;
        default:
            lv_label_set_text_fmt(pdata->label_temperature, "%i °C", model->run.minion.read.default_temperature);
            break;
    }
    view_common_set_hidden(pdata->label_temperature, !model->config.parmac.abilita_visualizzazione_temperatura);

    uint16_t minutes = model->run.minion.read.remaining_time_seconds / 60;
    uint16_t seconds = model->run.minion.read.remaining_time_seconds % 60;

    lv_label_set_text_fmt(pdata->label_time, "%02i:%02i", minutes, seconds);

    if (model->run.minion.read.alarms == 0) {
        if (model_is_cycle_active(model)) {
            lv_label_set_text(pdata->label_status,
                              view_intl_get_string_in_language(language, STRINGS_ASCIUGATURA_IN_CORSO));
        } else {
            lv_label_set_text(pdata->label_status,
                              view_intl_get_string_in_language(language, STRINGS_SCELTA_PROGRAMMA));
        }
    } else {
        view_common_format_alarm(pdata->label_status, model, language);
    }
}


static lv_obj_t *program_button_create(lv_obj_t *parent, base_program_t program) {
    lv_obj_t *img = old_icon_button_create(parent, images_off[program]);
    view_register_object_default_callback_with_number(img, BTN_PROGRAM_ID, program);
    return img;
}


static lv_obj_t *old_icon_button_create(lv_obj_t *parent, const lv_img_dsc_t *img_dsc) {
    static lv_style_prop_t           tr_prop[] = {LV_STYLE_TRANSLATE_X,       LV_STYLE_TRANSLATE_Y,
                                                  LV_STYLE_TRANSFORM_SCALE_X, LV_STYLE_TRANSFORM_SCALE_Y,
                                                  LV_STYLE_IMAGE_RECOLOR_OPA, 0};
    static lv_style_transition_dsc_t tr;
    lv_style_transition_dsc_init(&tr, tr_prop, lv_anim_path_linear, 100, 0, NULL);

    static lv_style_t style_def;
    lv_style_init(&style_def);
    lv_style_set_transform_pivot_x(&style_def, LV_PCT(50));
    lv_style_set_transform_pivot_y(&style_def, LV_PCT(50));

    /*Darken the button when pressed and make it wider*/
    static lv_style_t style_pr;
    lv_style_init(&style_pr);
    lv_style_set_image_recolor_opa(&style_pr, 10);
    lv_style_set_image_recolor(&style_pr, lv_color_black());
    lv_style_set_transform_scale(&style_pr, SCALE_CLICK_ANIMATION);
    lv_style_set_transition(&style_pr, &tr);

    lv_obj_t *img = lv_image_create(parent);
    lv_image_set_pivot(img, LV_PCT(50), LV_PCT(50));
    lv_image_set_src(img, img_dsc);

    lv_obj_add_style(img, &style_def, 0);
    lv_obj_add_style(img, &style_pr, LV_STATE_PRESSED);
    lv_image_set_inner_align(img, LV_IMAGE_ALIGN_CENTER);

    lv_obj_add_flag(img, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(img, hit_test_event_cb, LV_EVENT_HIT_TEST, NULL);

    return img;
}


static void icon_button_scale(icon_button_t *icon_button, int32_t scale) {
    lv_image_set_scale(icon_button->image_background, scale);
    lv_image_set_scale(icon_button->image_icon, scale);
}


static icon_button_t icon_button_create(lv_obj_t *parent, const lv_img_dsc_t *img_dsc) {
    static lv_style_prop_t           tr_prop[] = {LV_STYLE_TRANSLATE_X,       LV_STYLE_TRANSLATE_Y,
                                                  LV_STYLE_TRANSFORM_SCALE_X, LV_STYLE_TRANSFORM_SCALE_Y,
                                                  LV_STYLE_IMAGE_RECOLOR_OPA, 0};
    static lv_style_transition_dsc_t tr;
    lv_style_transition_dsc_init(&tr, tr_prop, lv_anim_path_linear, 100, 0, NULL);

    static lv_style_t style_def;
    lv_style_init(&style_def);
    lv_style_set_transform_pivot_x(&style_def, LV_PCT(50));
    lv_style_set_transform_pivot_y(&style_def, LV_PCT(50));

    /*Darken the button when pressed and make it wider*/
    static lv_style_t style_pr;
    lv_style_init(&style_pr);
    // lv_style_set_image_recolor_opa(&style_pr, 10);
    lv_style_set_image_recolor(&style_pr, lv_color_darken(VIEW_STYLE_COLOR_WHITE, 10));
    lv_style_set_transform_scale(&style_pr, SCALE_CLICK_ANIMATION);
    lv_style_set_transition(&style_pr, &tr);

    lv_obj_t *image_background = lv_image_create(parent);
    lv_image_set_pivot(image_background, LV_PCT(50), LV_PCT(50));
    lv_image_set_src(image_background, &img_button_background);
    lv_obj_set_style_image_recolor(image_background, VIEW_STYLE_COLOR_WHITE, LV_STATE_DEFAULT);
    lv_obj_set_style_image_recolor_opa(image_background, LV_OPA_COVER, LV_STATE_DEFAULT);

    lv_obj_add_style(image_background, &style_def, LV_STATE_DEFAULT);
    lv_obj_add_style(image_background, &style_pr, LV_STATE_PRESSED);
    lv_image_set_inner_align(image_background, LV_IMAGE_ALIGN_CENTER);

    lv_obj_add_flag(image_background, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(image_background, hit_test_event_cb, LV_EVENT_HIT_TEST, NULL);

    lv_obj_t *image_icon = lv_image_create(image_background);
    lv_image_set_src(image_icon, img_dsc);
    lv_obj_add_flag(image_icon, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_center(image_icon);

    return (icon_button_t){
        .image_background = image_background,
        .image_icon       = image_icon,
    };
}

static void hit_test_event_cb(lv_event_t *event) {
    lv_hit_test_info_t *hit_test_info = lv_event_get_param(event);

    lv_obj_t   *target = lv_event_get_target(event);
    lv_indev_t *indev  = lv_indev_get_act();

    lv_point_t vect;
    lv_indev_get_point(indev, &vect);

    lv_image_dsc_t *img_dsc = (lv_img_dsc_t *)lv_img_get_src(target);

    int32_t width        = img_dsc->header.w;
    int32_t scale        = lv_image_get_scale(target);
    int32_t scaled_width = (width * scale) / 255;
    int32_t scale_shift  = width - scaled_width;

    int32_t obj_x = lv_obj_get_x(target) + scale_shift / 2;
    int32_t obj_y = lv_obj_get_y(target) + scale_shift / 2;

    int32_t zeroed_x = vect.x - obj_x;
    int32_t zeroed_y = vect.y - obj_y;

    int32_t unscaled_zeroed_x = (zeroed_x * 255) / scale;
    int32_t unscaled_zeroed_y = (zeroed_y * 255) / scale;

    size_t pixel_index = unscaled_zeroed_x + unscaled_zeroed_y * width;
    size_t pixel_size  = 1;
    switch (img_dsc->header.cf) {
        case LV_COLOR_FORMAT_RGB565A8:
            pixel_size = 2;
            break;
        default:
            pixel_size = 1;
            break;
    }

    view_object_data_t *obj_data = lv_obj_get_user_data(target);

    int32_t centered_x     = zeroed_x - scaled_width / 2;
    int32_t centered_y     = zeroed_y - scaled_width / 2;
    int32_t vector_squared = centered_x * centered_x + centered_y * centered_y;

    ESP_LOGD(TAG,
             "%i %4" PRIiLEAST32 " %4" PRIiLEAST32 " %4" PRIiLEAST32 ", (%4" PRIiLEAST32 " %4" PRIiLEAST32
             ") (%4" PRIiLEAST32 " %4" PRIiLEAST32 ") (%4" PRIiLEAST32 " %4" PRIiLEAST32 ") (%4" PRIiLEAST32
             " %4" PRIiLEAST32 ") (%4" PRIiLEAST32 " %4" PRIiLEAST32 ") (%4" PRIiLEAST32 " %4" PRIiLEAST32
             ") %4zu %4zu 0x%02X %4" PRIiLEAST32,
             obj_data->id, width, scale, scaled_width, vect.x, vect.y, obj_x, obj_y, lv_obj_get_x(target),
             lv_obj_get_y(target), zeroed_x, zeroed_y, centered_x, centered_y, unscaled_zeroed_x, unscaled_zeroed_y,
             pixel_index, pixel_size, img_dsc->data[pixel_index * pixel_size], img_dsc->data_size);

    if (pixel_index < img_dsc->data_size) {
        if (vector_squared <= (scaled_width / 2) * (scaled_width / 2)) {
            uint8_t alpha      = img_dsc->data[pixel_index * pixel_size + (pixel_size - 1)];
            hit_test_info->res = alpha > 0;
        } else {
            hit_test_info->res = 0;
        }
    } else {
        ESP_LOGW(TAG, "Out of bounds");
        hit_test_info->res = 0;
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


const pman_page_t page_main = {
    .create        = create_page,
    .destroy       = destroy_page,
    .open          = open_page,
    .close         = close_page,
    .process_event = page_event,
};
