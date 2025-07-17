#include "lvgl.h"
#include "model/model.h"
#include "../view.h"
#include "../theme/style.h"
#include "../common.h"
#include "config/app_config.h"
#include "src/widgets/led/lv_led.h"
#include "../intl/intl.h"
#include "model/parmac.h"
#include "model/descriptions/AUTOGEN_FILE_pars.h"


struct page_data {
    step_modification_t *meta;

    lv_obj_t *label_language_label;
    lv_obj_t *label_language;
    lv_obj_t *label_logo_label;
    lv_obj_t *label_logo;
    lv_obj_t *label_machine_model_label;
    lv_obj_t *label_machine_model;
};


enum {
    BTN_CONFIRM_ID,
    BTN_MODIFY_LANGUAGE_ID,
    BTN_MODIFY_LOGO_ID,
    BTN_MODIFY_MACHINE_MODEL_ID,
};


static void update_page(model_t *model, struct page_data *pdata);


static void *create_page(pman_handle_t handle, void *extra) {
    (void)handle;
    (void)extra;

    struct page_data *pdata = lv_malloc(sizeof(struct page_data));
    assert(pdata != NULL);
    pdata->meta = (step_modification_t *)extra;

    return pdata;
}


static void open_page(pman_handle_t handle, void *state) {
    struct page_data *pdata = state;

    model_t *model = view_get_model(handle);

    lv_obj_t *cont = lv_obj_create(lv_scr_act());
    lv_obj_set_style_pad_column(cont, 6, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_ver(cont, 4, LV_STATE_DEFAULT);
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(cont, LV_ALIGN_CENTER, 0, 0);

    {
        lv_obj_t *cont_param = lv_obj_create(cont);
        lv_obj_remove_flag(cont_param, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_size(cont_param, LV_PCT(100), 64);
        lv_obj_add_style(cont_param, &style_transparent_cont, LV_STATE_DEFAULT);
        lv_obj_set_layout(cont_param, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(cont_param, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(cont_param, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        view_common_icon_button_create_with_number(cont_param, LV_SYMBOL_MINUS, BTN_MODIFY_LANGUAGE_ID, -1);

        lv_obj_t *cont_label = lv_obj_create(cont_param);
        lv_obj_set_flex_grow(cont_label, 1);
        lv_obj_add_style(cont_label, &style_transparent_cont, LV_STATE_DEFAULT);
        lv_obj_set_layout(cont_label, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(cont_label, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(cont_label, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        lv_obj_t *label_label = lv_label_create(cont_label);
        lv_obj_set_style_text_font(label_label, STYLE_FONT_SMALL, LV_STATE_DEFAULT);
        pdata->label_language_label = label_label;

        lv_obj_t *label = lv_label_create(cont_label);
        lv_obj_set_style_text_font(label, STYLE_FONT_MEDIUM, LV_STATE_DEFAULT);
        pdata->label_language = label;

        view_common_icon_button_create_with_number(cont_param, LV_SYMBOL_PLUS, BTN_MODIFY_LANGUAGE_ID, +1);
    }

    {
        lv_obj_t *cont_param = lv_obj_create(cont);
        lv_obj_remove_flag(cont_param, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_size(cont_param, LV_PCT(100), 64);
        lv_obj_add_style(cont_param, &style_transparent_cont, LV_STATE_DEFAULT);
        lv_obj_set_layout(cont_param, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(cont_param, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(cont_param, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        view_common_icon_button_create_with_number(cont_param, LV_SYMBOL_MINUS, BTN_MODIFY_LOGO_ID, -1);

        lv_obj_t *cont_label = lv_obj_create(cont_param);
        lv_obj_set_flex_grow(cont_label, 1);
        lv_obj_add_style(cont_label, &style_transparent_cont, LV_STATE_DEFAULT);
        lv_obj_set_layout(cont_label, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(cont_label, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(cont_label, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        lv_obj_t *label_label = lv_label_create(cont_label);
        lv_obj_set_style_text_font(label_label, STYLE_FONT_SMALL, LV_STATE_DEFAULT);
        pdata->label_logo_label = label_label;

        lv_obj_t *label = lv_label_create(cont_label);
        lv_obj_set_style_text_font(label, STYLE_FONT_MEDIUM, LV_STATE_DEFAULT);
        pdata->label_logo = label;

        view_common_icon_button_create_with_number(cont_param, LV_SYMBOL_PLUS, BTN_MODIFY_LOGO_ID, +1);
    }

    {
        lv_obj_t *cont_param = lv_obj_create(cont);
        lv_obj_remove_flag(cont_param, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_size(cont_param, LV_PCT(100), 64);
        lv_obj_add_style(cont_param, &style_transparent_cont, LV_STATE_DEFAULT);
        lv_obj_set_layout(cont_param, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(cont_param, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(cont_param, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        view_common_icon_button_create_with_number(cont_param, LV_SYMBOL_MINUS, BTN_MODIFY_MACHINE_MODEL_ID, -1);

        lv_obj_t *cont_label = lv_obj_create(cont_param);
        lv_obj_set_flex_grow(cont_label, 1);
        lv_obj_add_style(cont_label, &style_transparent_cont, LV_STATE_DEFAULT);
        lv_obj_set_layout(cont_label, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(cont_label, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(cont_label, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        lv_obj_t *label_label = lv_label_create(cont_label);
        lv_obj_set_style_text_font(label_label, STYLE_FONT_SMALL, LV_STATE_DEFAULT);
        pdata->label_machine_model_label = label_label;

        lv_obj_t *label = lv_label_create(cont_label);
        lv_obj_set_style_text_font(label, STYLE_FONT_SMALL, LV_STATE_DEFAULT);
        pdata->label_machine_model = label;

        view_common_icon_button_create_with_number(cont_param, LV_SYMBOL_PLUS, BTN_MODIFY_MACHINE_MODEL_ID, +1);
    }

    view_common_icon_button_create(cont, LV_SYMBOL_OK, BTN_CONFIRM_ID);

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

                    switch (obj_data->id) {
                        case BTN_MODIFY_LANGUAGE_ID: {
                            if (obj_data->number < 0) {
                                if (model->config.parmac.language > 0) {
                                    model->config.parmac.language--;
                                } else {
                                    model->config.parmac.language = LANGUAGES_NUM - 1;
                                }
                            } else {
                                model->config.parmac.language = (model->config.parmac.language + 1) % LANGUAGES_NUM;
                            }
                            update_page(model, pdata);
                            break;
                        }

                        case BTN_MODIFY_LOGO_ID: {
                            if (obj_data->number < 0) {
                                if (model->config.parmac.logo > 0) {
                                    model->config.parmac.logo--;
                                } else {
                                    model->config.parmac.logo = 4;
                                }
                            } else {
                                model->config.parmac.logo = (model->config.parmac.logo + 1) % 5;
                            }
                            update_page(model, pdata);
                            break;
                        }

                        case BTN_MODIFY_MACHINE_MODEL_ID: {
                            if (obj_data->number < 0) {
                                if (model->config.parmac.machine_model > 0) {
                                    model->config.parmac.machine_model--;
                                } else {
                                    model->config.parmac.machine_model = MACHINE_MODELS_NUM - 1;
                                }
                            } else {
                                model->config.parmac.machine_model =
                                    (model->config.parmac.machine_model + 1) % MACHINE_MODELS_NUM;
                            }

                            update_page(model, pdata);
                            break;
                        }

                        case BTN_CONFIRM_ID: {
                            view_get_protocol(handle)->commissioning_done(handle);
                            msg.stack_msg = PMAN_STACK_MSG_REBASE(view_common_main_page(model));
                            break;
                        }
                    }
                    break;
                }

                case LV_EVENT_VALUE_CHANGED: {
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
    strings_t  string_language[] = {STRINGS_ITALIANO, STRINGS_INGLESE};
    language_t language          = model->config.parmac.language;

    lv_label_set_text(pdata->label_language_label, view_intl_get_string(model, STRINGS_LINGUA));
    lv_label_set_text(pdata->label_language, view_intl_get_string(model, string_language[language]));

    lv_label_set_text(pdata->label_logo_label, view_intl_get_string(model, STRINGS_LOGO));
    lv_label_set_text(pdata->label_logo, pars_loghi[model->config.parmac.logo][language]);

    lv_label_set_text(pdata->label_machine_model_label, view_intl_get_string(model, STRINGS_MODELLO_MACCHINA));
    lv_label_set_text(pdata->label_machine_model, view_common_modello_str(model));
}


static void close_page(void *args) {
    struct page_data *pdata = args;
    (void)pdata;
    lv_obj_clean(lv_scr_act());
}


static void destroy_page(void *args, void *extra) {
    struct page_data *pdata = args;
    lv_free(pdata);
    lv_free(extra);
}


const pman_page_t page_commissioning = {
    .create        = create_page,
    .destroy       = destroy_page,
    .open          = open_page,
    .close         = close_page,
    .process_event = page_event,
};
