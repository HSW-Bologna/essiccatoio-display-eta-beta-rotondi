#include "controller.h"
#include "model/model.h"
#include "adapters/view/view.h"
#include "gui.h"
#include "minion.h"
#include "services/timestamp.h"
#include "esp_log.h"


static struct {
    timestamp_t minion_sync_ts;
    uint8_t     handshook;
} state = {0};


static const char *TAG = "Controller";


void controller_init(mut_model_t *model) {
    (void)model;

    minion_init();
    minion_handshake();


    view_change_page(&page_main);
}


void controller_process_message(pman_handle_t handle, void *msg) {
    (void)handle;
    (void)msg;
}


void controller_manage(mut_model_t *model) {
    (void)model;

    controller_gui_manage();
    view_manage(model);

    {
        minion_response_t response = {0};
        if (minion_get_response(&response)) {
            switch (response.tag) {
                case MINION_RESPONSE_TAG_ERROR:
                    model->run.minion.communication_error = 1;
                    break;

                case MINION_RESPONSE_TAG_SYNC: {
                    model->run.minion.read.firmware_version_major = response.as.sync.firmware_version_major;
                    model->run.minion.read.firmware_version_minor = response.as.sync.firmware_version_minor;
                    model->run.minion.read.firmware_version_patch = response.as.sync.firmware_version_patch;
                    model->run.minion.read.inputs                 = response.as.sync.inputs;
                    model->run.minion.read.temperature_1          = response.as.sync.temperature_1;
                    model->run.minion.read.temperature_1_adc      = response.as.sync.temperature_1_adc;
                    model->run.minion.read.temperature_2          = response.as.sync.temperature_2;
                    model->run.minion.read.temperature_2_adc      = response.as.sync.temperature_2_adc;
                    model->run.minion.read.pressure_adc           = response.as.sync.pressure_adc;
                    model->run.minion.read.pressure               = response.as.sync.pressure;
                    model->run.minion.read.cycle_state            = response.as.sync.cycle_state;
                    model->run.minion.read.step_type              = response.as.sync.step_type;
                    model->run.minion.read.default_temperature    = response.as.sync.default_temperature;
                    model->run.minion.read.remaining_time_seconds = response.as.sync.remaining_time_seconds;
                    model->run.minion.read.alarms                 = response.as.sync.alarms;

                    if (model_is_program_done(model)) {
                        ESP_LOGI(TAG, "Program done!");
                        minion_program_done();
                    }
                    break;
                }

                case MINION_RESPONSE_TAG_HANDSHAKE: {
                    if (response.as.handshake.cycle_state != CYCLE_STATE_STOPPED) {
                        model_select_program(model, response.as.handshake.program_index);
                        model_select_step(model, response.as.handshake.step_index);
                        model->run.minion.read.cycle_state = response.as.handshake.cycle_state;
                    }

                    state.handshook = 1;
                    break;
                }

                default:
                    break;
            }
        }
    }

    if (timestamp_is_expired(state.minion_sync_ts, 400)) {
        controller_sync_minion(model);
    }
}


void controller_sync_minion(model_t *model) {
    if (state.handshook) {
        minion_sync(model);
        state.minion_sync_ts = timestamp_get();
    }
}
