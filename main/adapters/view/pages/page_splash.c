#include "lvgl.h"
#include "model/model.h"
#include "../view.h"
#include "../theme/style.h"
#include "../common.h"
#include "config/app_config.h"
#include "src/widgets/led/lv_led.h"
#include "../intl/intl.h"
#include "config/app_config.h"


struct page_data {
    pman_timer_t *timer;
};


static void update_page(model_t *model, struct page_data *pdata);


static void *create_page(pman_handle_t handle, void *extra) {
    (void)handle;
    (void)extra;

    struct page_data *pdata = lv_malloc(sizeof(struct page_data));
    assert(pdata != NULL);

    pdata->timer = PMAN_REGISTER_TIMER_ID(handle, 3000, 0);

    return pdata;
}


static void open_page(pman_handle_t handle, void *state) {
    struct page_data *pdata = state;
    pman_timer_resume(pdata->timer);

    model_t *model = view_get_model(handle);

    LV_IMG_DECLARE(img_logo_rotondi);
    LV_IMG_DECLARE(img_logo_ms);
    LV_IMG_DECLARE(img_logo_hoover);
    LV_IMG_DECLARE(img_logo_unity_laundry);

    const lv_img_dsc_t *img_dsc_logos[] = {
        &img_logo_ms,
        &img_logo_rotondi,
        &img_logo_hoover,
        &img_logo_unity_laundry,
    };

    lv_obj_t *cont = lv_obj_create(lv_screen_active());
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_center(cont);
    lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(cont, LV_HOR_RES, LV_VER_RES);
    lv_obj_add_style(cont, &style_padless_cont, LV_STATE_DEFAULT);
    lv_obj_add_style(cont, &style_borderless_cont, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cont, VIEW_STYLE_COLOR_WHITE, LV_STATE_DEFAULT);

    if (model->config.parmac.logo > 0 &&
        model->config.parmac.logo <= (sizeof(img_dsc_logos) / sizeof(img_dsc_logos[0]))) {
        lv_obj_t *image = lv_image_create(cont);
        lv_img_set_src(image, img_dsc_logos[model->config.parmac.logo - 1]);
        lv_obj_center(image);
    }

    update_page(model, pdata);
}


static pman_msg_t page_event(pman_handle_t handle, void *state, pman_event_t event) {
    struct page_data *pdata = state;
    (void)pdata;

    mut_model_t *model = view_get_model(handle);

    pman_msg_t msg = PMAN_MSG_NULL;

    switch (event.tag) {
        case PMAN_EVENT_TAG_OPEN:
            break;

        case PMAN_EVENT_TAG_TIMER: {
            msg.stack_msg =
                PMAN_STACK_MSG_SWAP(model->config.commissioned ? view_common_main_page(model) : &page_commissioning);
            break;
        }

        case PMAN_EVENT_TAG_USER: {
            view_event_t *view_event = event.as.user;
            switch (view_event->tag) {
                case VIEW_EVENT_TAG_PAGE_WATCHER: {
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
    (void)model;
    (void)pdata;
}


static void close_page(void *state) {
    struct page_data *pdata = state;
    pman_timer_pause(pdata->timer);
    lv_obj_clean(lv_scr_act());
}


void destroy_page(void *state, void *extra) {
    (void)extra;
    //struct page_data *pdata = state;
    //pman_timer_delete(pdata->timer);
    lv_free(state);
}


const pman_page_t page_splash = {
    .create        = create_page,
    .destroy       = destroy_page,
    .open          = open_page,
    .close         = close_page,
    .process_event = page_event,
};
