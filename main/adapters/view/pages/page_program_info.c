#include "lvgl.h"
#include "model/model.h"
#include "../view.h"
#include "../theme/style.h"
#include "../common.h"
#include "config/app_config.h"
#include "src/widgets/led/lv_led.h"
#include "../intl/intl.h"


struct page_data {
    step_modification_t *meta;

    keyboard_page_options_t keyboard_options;

    pman_timer_t *timer;

    lv_obj_t *checkbox_cooling;
    lv_obj_t *checkbox_antifold;
};


enum {
    BTN_BACK_ID,
    BTN_NAME_ID,
    CHECKBOX_COOLING_ID,
    CHECKBOX_ANTIFOLD_ID,
};


static void update_page(model_t *model, struct page_data *pdata);


static void *create_page(pman_handle_t handle, void *extra) {
    (void)extra;

    model_t *model = view_get_model(handle);

    struct page_data *pdata = lv_malloc(sizeof(struct page_data));
    assert(pdata != NULL);
    pdata->meta  = (step_modification_t *)extra;
    pdata->timer = PMAN_REGISTER_TIMER_ID(handle, model->config.parmac.reset_page_time * 1000UL, 0);

    return pdata;
}


static void open_page(pman_handle_t handle, void *state) {
    struct page_data *pdata = state;

    pman_timer_reset(pdata->timer);
    pman_timer_resume(pdata->timer);

    model_t         *model   = view_get_model(handle);
    const program_t *program = model_get_program(model, pdata->meta->program_index);

    view_common_create_title(lv_scr_act(), view_intl_get_string(model, STRINGS_INFORMAZIONI), BTN_BACK_ID, -1);

    lv_obj_t *cont = lv_obj_create(lv_scr_act());
    lv_obj_set_style_pad_column(cont, 8, LV_STATE_DEFAULT);
    lv_obj_set_size(cont, LV_HOR_RES, LV_VER_RES - 56);
    lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(cont, LV_ALIGN_BOTTOM_MID, 0, 0);

    {
        lv_obj_t *obj = lv_obj_create(cont);
        lv_obj_set_size(obj, LV_PCT(100), 64);
        lv_obj_add_style(obj, &style_transparent_cont, LV_STATE_DEFAULT);

        lv_obj_t *label_name = lv_label_create(obj);
        lv_obj_set_style_text_font(label_name, STYLE_FONT_SMALL, LV_STATE_DEFAULT);
        lv_label_set_long_mode(label_name, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_width(label_name, 230);
        lv_label_set_text(label_name, program->nomi[model->config.parmac.language]);
        lv_obj_align(label_name, LV_ALIGN_LEFT_MID, 0, 0);

        lv_obj_t *button = view_common_icon_button_create(obj, LV_SYMBOL_EDIT, BTN_NAME_ID);
        lv_obj_align(button, LV_ALIGN_RIGHT_MID, 0, 0);
    }

    {
        lv_obj_t *obj = lv_obj_create(cont);
        lv_obj_set_size(obj, LV_PCT(100), 64);
        lv_obj_add_style(obj, &style_transparent_cont, LV_STATE_DEFAULT);

        lv_obj_t *label = lv_label_create(obj);
        lv_obj_set_style_text_font(label, STYLE_FONT_SMALL, LV_STATE_DEFAULT);
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_width(label, 230);
        lv_label_set_text(label, view_intl_get_string(model, STRINGS_RAFFREDDAMENTO));
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);

        lv_obj_t *checkbox = lv_checkbox_create(obj);
        lv_checkbox_set_text(checkbox, "");
        lv_obj_align(checkbox, LV_ALIGN_RIGHT_MID, 0, 0);
        view_register_object_default_callback(checkbox, CHECKBOX_COOLING_ID);
        pdata->checkbox_cooling = checkbox;
    }

    {
        lv_obj_t *obj = lv_obj_create(cont);
        lv_obj_set_size(obj, LV_PCT(100), 64);
        lv_obj_add_style(obj, &style_transparent_cont, LV_STATE_DEFAULT);

        lv_obj_t *label = lv_label_create(obj);
        lv_obj_set_style_text_font(label, STYLE_FONT_SMALL, LV_STATE_DEFAULT);
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_width(label, 230);
        lv_label_set_text(label, view_intl_get_string(model, STRINGS_ANTIPIEGA));
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);

        lv_obj_t *checkbox = lv_checkbox_create(obj);
        lv_checkbox_set_text(checkbox, "");
        lv_obj_align(checkbox, LV_ALIGN_RIGHT_MID, 0, 0);
        view_register_object_default_callback(checkbox, CHECKBOX_ANTIFOLD_ID);
        pdata->checkbox_antifold = checkbox;
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

        case PMAN_EVENT_TAG_TIMER: {
            msg.stack_msg = PMAN_STACK_MSG_REBASE(view_common_main_page(model));
            break;
        }

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
                    pman_timer_reset(pdata->timer);

                    switch (obj_data->id) {
                        case BTN_BACK_ID:
                            msg.stack_msg = PMAN_STACK_MSG_BACK();
                            break;

                        case BTN_NAME_ID: {
                            pdata->keyboard_options.max_length = MAX_NAME_LENGTH;
                            pdata->keyboard_options.numeric    = 0;
                            pdata->keyboard_options.string =
                                (void *)model_get_program(model, pdata->meta->program_index)
                                    ->nomi[model->config.parmac.language];

                            pdata->meta->changed = 1;
                            msg.stack_msg = PMAN_STACK_MSG_PUSH_PAGE_EXTRA(&page_keyboard, &pdata->keyboard_options);
                            break;
                        }
                    }
                    break;
                }

                case LV_EVENT_VALUE_CHANGED: {
                    pman_timer_reset(pdata->timer);

                    switch (obj_data->id) {
                        case CHECKBOX_COOLING_ID: {
                            program_t *program = model_get_mut_program(model, pdata->meta->program_index);
                            program->cooling_enabled =
                                (lv_obj_get_state(pdata->checkbox_cooling) & LV_STATE_CHECKED) > 0;
                            pdata->meta->changed = 1;
                            update_page(model, pdata);
                            break;
                        }

                        case CHECKBOX_ANTIFOLD_ID: {
                            program_t *program = model_get_mut_program(model, pdata->meta->program_index);
                            program->antifold_enabled =
                                (lv_obj_get_state(pdata->checkbox_antifold) & LV_STATE_CHECKED) > 0;
                            pdata->meta->changed = 1;
                            update_page(model, pdata);
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
    const program_t *program = model_get_program(model, pdata->meta->program_index);

    if (program->cooling_enabled) {
        lv_obj_add_state(pdata->checkbox_cooling, LV_STATE_CHECKED);
    } else {
        lv_obj_remove_state(pdata->checkbox_cooling, LV_STATE_CHECKED);
    }

    if (program->antifold_enabled) {
        lv_obj_add_state(pdata->checkbox_antifold, LV_STATE_CHECKED);
    } else {
        lv_obj_remove_state(pdata->checkbox_antifold, LV_STATE_CHECKED);
    }
}


static void close_page(void *args) {
    struct page_data *pdata = args;
    pman_timer_pause(pdata->timer);
    lv_obj_clean(lv_scr_act());
}


static void destroy_page(void *args, void *extra) {
    (void)extra;
    struct page_data *pdata = args;
    pman_timer_delete(pdata->timer);
    lv_free(pdata);
}


const pman_page_t page_program_info = {
    .create        = create_page,
    .destroy       = destroy_page,
    .open          = open_page,
    .close         = close_page,
    .process_event = page_event,
};
