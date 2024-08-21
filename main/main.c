#include <driver/i2c.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "model/model.h"
#include "adapters/view/view.h"
#include "controller/controller.h"
#include "controller/gui.h"
#include "bsp/system.h"
#include "bsp/tft/display.h"
#include "services/timestamp.h"
#include "bsp/tft/touch.h"


static const char *TAG = "Main";


void app_main(void) {
    vTaskDelay(pdMS_TO_TICKS(500));
    ESP_LOGI(TAG, "Main");

    mut_model_t model = {0};

    bsp_system_init();
    bsp_tft_display_init(view_display_flush_ready, VIEW_LVGL_BUFFER_SIZE);
    bsp_tft_touch_init();

    model_init(&model);
    view_init(&model, controller_process_message, bsp_tft_display_lvgl_flush_cb, bsp_tft_touch_read);
    controller_init(&model);

    ESP_LOGI(TAG, "Begin main loop");
    for (;;) {
        controller_manage(&model);

        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
