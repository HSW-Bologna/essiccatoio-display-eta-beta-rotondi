#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"
#include <driver/gpio.h>
#include "esp_log.h"
#include "hardwareprofile.h"
#include "heartbit.h"
#include <esp_system.h>


static void print_heap_status(void);


static const char   *TAG       = "Heartbeat";
static TimerHandle_t timer     = NULL;
static unsigned long hb_period = 1000UL;

/**
 * Timer di attivita'. Accende e spegne il led di attivita'
 */
static void bsp_heartbit_timer(TimerHandle_t timer) {
    (void)timer;
    static int    blink   = 0;
    static size_t counter = 0;

    gpio_set_level(BSP_HAP_RUN, blink);
    blink = !blink;

    if (counter++ >= 10) {
        print_heap_status();
        counter = 0;
    }

    xTimerChangePeriod(timer, pdMS_TO_TICKS(blink ? hb_period : 50UL), portMAX_DELAY);
}


void bsp_heartbit_init(size_t period_ms) {
    gpio_config_t config = {
        .mode         = GPIO_MODE_OUTPUT,
        .intr_type    = GPIO_INTR_DISABLE,
        .pin_bit_mask = BIT64(BSP_HAP_RUN),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&config));

    hb_period = period_ms;
    timer     = xTimerCreate(TAG, pdMS_TO_TICKS(hb_period), pdTRUE, NULL, bsp_heartbit_timer);
    xTimerStart(timer, portMAX_DELAY);
}


void heartbit_stop(void) {
    xTimerStop(timer, portMAX_DELAY);
}


void heartbit_resume(void) {
    xTimerStart(timer, portMAX_DELAY);
}


static void print_heap_status(void) {
    ESP_LOGI(TAG, "Internal RAM: LWM = %u, free = %u, biggest = %u",
             heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL), heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
             heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
    ESP_LOGI(TAG, "PSRAM       : LWM = %zu, free = %zu\n", heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM),
             heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}
