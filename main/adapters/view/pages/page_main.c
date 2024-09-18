#include <assert.h>
#include <stdlib.h>
#include "lvgl.h"
#include "model/model.h"
#include "../view.h"
#include "../theme/style.h"
#include "../intl/intl.h"
#include <esp_log.h>
#include "bsp/tft/display.h"


enum {
    BTN_PROGRAM_ID,
};


typedef enum {
    PROGRAM_WOOL = 0,
    PROGRAM_COLD,
    PROGRAM_LUKEWARM,
    PROGRAM_WARM,
    PROGRAM_HOT,
#define NUM_PROGRAMS 5
} program_t;


static const char *TAG = "PageMain";


struct page_data {
    uint8_t   program_selected;
    program_t program;
    lv_obj_t *buttons[NUM_PROGRAMS];
};


static void      update_page(struct page_data *pdata);
static lv_obj_t *image_button_create(const lv_img_dsc_t *img_dsc, program_t program);


static void *create_page(pman_handle_t handle, void *extra) {
    (void)handle;
    (void)extra;

    struct page_data *pdata = lv_malloc(sizeof(struct page_data));
    assert(pdata != NULL);
    pdata->program_selected = 0;
    return pdata;
}


static void open_page(pman_handle_t handle, void *state) {
    LV_IMG_DECLARE(img_wool);
    LV_IMG_DECLARE(img_cold);
    LV_IMG_DECLARE(img_lukewarm);
    LV_IMG_DECLARE(img_warm);
    LV_IMG_DECLARE(img_hot);

    struct page_data *pdata = state;
    (void)pdata;

    model_t  *p_model = pman_get_user_data(handle);
    const int delta   = 96;

    lv_obj_t *img_button_cold = image_button_create(&img_cold, PROGRAM_COLD);
    lv_obj_align(img_button_cold, LV_ALIGN_CENTER, delta, delta);
    pdata->buttons[PROGRAM_COLD] = img_button_cold;

    lv_obj_t *img_button_lukewarm = image_button_create(&img_lukewarm, PROGRAM_LUKEWARM);
    lv_obj_align(img_button_lukewarm, LV_ALIGN_CENTER, delta, -delta);
    pdata->buttons[PROGRAM_LUKEWARM] = img_button_lukewarm;

    lv_obj_t *img_button_warm = image_button_create(&img_warm, PROGRAM_WARM);
    lv_obj_align(img_button_warm, LV_ALIGN_CENTER, -delta, -delta);
    pdata->buttons[PROGRAM_WARM] = img_button_warm;

    lv_obj_t *img_button_hot = image_button_create(&img_hot, PROGRAM_HOT);
    lv_obj_align(img_button_hot, LV_ALIGN_CENTER, -delta, delta);
    pdata->buttons[PROGRAM_HOT] = img_button_hot;

    lv_obj_t *img_button_wool = image_button_create(&img_wool, PROGRAM_WOOL);
    lv_obj_center(img_button_wool);
    pdata->buttons[PROGRAM_WOOL] = img_button_wool;


    ESP_LOGI(TAG, "Main open");

    update_page(pdata);
}


static pman_msg_t page_event(pman_handle_t handle, void *state, pman_event_t event) {
    (void)handle;

    pman_msg_t msg = PMAN_MSG_NULL;

    struct page_data *pdata = state;

    switch (event.tag) {
        case PMAN_EVENT_TAG_LVGL: {
            lv_obj_t           *target   = lv_event_get_target_obj(event.as.lvgl);
            view_object_data_t *obj_data = lv_obj_get_user_data(target);

            switch (lv_event_get_code(event.as.lvgl)) {
                case LV_EVENT_CLICKED: {
                    switch (obj_data->id) {
                        case BTN_PROGRAM_ID:
                            if (pdata->program == obj_data->number) {
                                pdata->program_selected = !pdata->program_selected;
                            } else {
                                pdata->program = obj_data->number;
                            }

                            update_page(pdata);
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

        default:
            break;
    }

    return msg;
}


static void update_page(struct page_data *pdata) {
    static lv_style_t style_background;
    lv_style_init(&style_background);
    lv_style_set_transform_scale(&style_background, 160);

    static lv_style_t style_foreground;
    lv_style_init(&style_foreground);
    lv_style_set_transform_scale(&style_foreground, 255);


    for (program_t program = 0; program < NUM_PROGRAMS; program++) {
        if (pdata->program_selected) {
            if (program == pdata->program) {
                //lv_obj_remove_style(pdata->buttons[program], &style_background, LV_STATE_DEFAULT);
                //lv_obj_add_style(pdata->buttons[program], &style_foreground, LV_STATE_DEFAULT);
                lv_image_set_scale(pdata->buttons[program], 260);
            } else {
                //lv_obj_remove_style(pdata->buttons[program], &style_foreground, LV_STATE_DEFAULT);
                //lv_obj_add_style(pdata->buttons[program], &style_background, LV_STATE_DEFAULT);
                lv_image_set_scale(pdata->buttons[program], 200);
            }
        } else {
            lv_obj_remove_style(pdata->buttons[program], &style_foreground, LV_STATE_DEFAULT);
            lv_obj_remove_style(pdata->buttons[program], &style_background, LV_STATE_DEFAULT);
        }
    }
}


static lv_obj_t *image_button_create(const lv_img_dsc_t *img_dsc, program_t program) {
    static lv_style_prop_t           tr_prop[] = {LV_STYLE_TRANSLATE_X,       LV_STYLE_TRANSLATE_Y,
                                                  LV_STYLE_TRANSFORM_SCALE_X, LV_STYLE_TRANSFORM_SCALE_Y,
                                                  LV_STYLE_IMAGE_RECOLOR_OPA, 0};
    static lv_style_transition_dsc_t tr;
    lv_style_transition_dsc_init(&tr, tr_prop, lv_anim_path_linear, 200, 0, NULL);

    static lv_style_t style_def;
    lv_style_init(&style_def);
    lv_style_set_transform_pivot_x(&style_def, LV_PCT(50));
    lv_style_set_transform_pivot_y(&style_def, LV_PCT(50));
    lv_style_set_transition(&style_def, &tr);

    /*Darken the button when pressed and make it wider*/
    static lv_style_t style_pr;
    lv_style_init(&style_pr);
    lv_style_set_image_recolor_opa(&style_pr, 10);
    lv_style_set_image_recolor(&style_pr, lv_color_black());
    lv_style_set_transform_scale(&style_pr, 270);

    lv_obj_t *img = lv_image_create(lv_scr_act());
    lv_image_set_pivot(img, LV_PCT(50), LV_PCT(50));
    lv_image_set_src(img, img_dsc);

    lv_obj_add_style(img, &style_def, 0);
    lv_obj_add_style(img, &style_pr, LV_STATE_PRESSED);
    lv_image_set_inner_align(img, LV_IMAGE_ALIGN_CENTER);

    lv_obj_add_flag(img, LV_OBJ_FLAG_CLICKABLE);
    view_register_object_default_callback_with_number(img, BTN_PROGRAM_ID, program);

    return img;
}


const pman_page_t page_main = {
    .create        = create_page,
    .destroy       = pman_destroy_all,
    .open          = open_page,
    .close         = pman_close_all,
    .process_event = page_event,
};
