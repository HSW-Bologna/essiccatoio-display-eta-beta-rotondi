#include "controller.h"
#include "model/model.h"
#include "adapters/view/view.h"
#include "gui.h"
#include "minion.h"
#include "services/timestamp.h"
#include "esp_log.h"
#include "configuration/configuration.h"
#include "adapters/view/common.h"
#include "bsp/msc.h"
#include "services/fup.h"


static struct {
    timestamp_t minion_sync_ts;
    uint8_t     handshook;
} state = {0};


static const char *TAG = "Controller";


void controller_init(mut_model_t *model) {
    configuration_init();
    configuration_load_all_data(model);
    model_check_parameters(model);

    minion_init();

    view_change_page(&page_splash);
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
                    model->run.minion.read.temperature_probe      = response.as.sync.temperature_probe;
                    model->run.minion.read.humidity_probe         = response.as.sync.humidity_probe;
                    model->run.minion.read.pressure_adc           = response.as.sync.pressure_adc;
                    model->run.minion.read.pressure               = response.as.sync.pressure;
                    model->run.minion.read.cycle_state            = response.as.sync.cycle_state;
                    model->run.minion.read.default_temperature    = response.as.sync.default_temperature;
                    model->run.minion.read.remaining_time_seconds = response.as.sync.remaining_time_seconds;
                    model->run.minion.read.alarms                 = response.as.sync.alarms;
                    model->run.minion.read.payment                = response.as.sync.payment;
                    memcpy(model->run.minion.read.coins, response.as.sync.coins, sizeof(model->run.minion.read.coins));

                    ESP_LOGI(TAG, "Cycle state %i, inputs 0x%02X, alarms 0x%02X, remaining %i, step %i",
                             model->run.minion.read.cycle_state, model->run.minion.read.inputs,
                             model->run.minion.read.alarms, model->run.minion.read.remaining_time_seconds,
                             model->run.current_step_index);

                    // Program finished
                    if (model_is_program_done(model)) {
                        ESP_LOGI(TAG, "Program done!");
                        model_reset_program(model);
                        minion_program_done(model);
                    }
                    // Currently waiting
                    else if (model_waiting_for_next_step(model)) {
                        ESP_LOGI(TAG, "Moving to next step");
                        model_move_to_next_step(model);
                        minion_resume_program(model, 0);
                    }
                    break;
                }

                case MINION_RESPONSE_TAG_HANDSHAKE: {
                    if (response.as.handshake.cycle_state != CYCLE_STATE_STOPPED) {
                        model_select_program(model, response.as.handshake.program_index);
                        model_select_step(model, response.as.handshake.step_index);
                        model->run.minion.read.cycle_state = response.as.handshake.cycle_state;

                        // If configured so start immediately
                        if (model->config.parmac.autostart) {
                            minion_resume_program(model, 0);
                        }
                    }

                    state.handshook = 1;
                    break;
                }

                default:
                    break;
            }
        }
    }

    {
        msc_response_t msc_response;
        if (msc_get_response(&msc_response)) {
            switch (msc_response.code) {
                case MSC_RESPONSE_CODE_ARCHIVE_EXTRACTION_COMPLETE:
                    configuration_load_all_data(model);
                    model->run.removable_drive_state = msc_is_device_mounted();
                    break;

                case MSC_RESPONSE_CODE_ARCHIVE_SAVING_COMPLETE:
                    ESP_LOGI(TAG, "Configuration saved!");
                    model->run.removable_drive_state = msc_is_device_mounted();
                    break;
            }

            if (msc_response.error) {
                // TODO
            } else {
                // TODO
            }
        }
    }

    if (timestamp_is_expired(state.minion_sync_ts, 400)) {
        const uint16_t  boot_target_counter = 7;
        static uint16_t boot_wait_counter   = 0;
        static uint8_t  initialized         = 0;

        // The minion has been initialized, sync periodically
        if (initialized) {
            controller_sync_minion(model);
        }
        // Handshake message
        else if (boot_wait_counter > boot_target_counter) {
            minion_handshake();
            initialized          = 1;
            state.minion_sync_ts = timestamp_get();
        }
        // Wait a few seconds before starting to query the minion in order for it to boot properly
        else {
            boot_wait_counter++;
            state.minion_sync_ts = timestamp_get();
        }

        model->run.removable_drive_state = msc_is_device_mounted();
    }

    fup_manage();
    model->run.firmware_update_state = fup_get_state();
}


void controller_sync_minion(model_t *model) {
    if (state.handshook && model->run.minion.communication_enabled) {
        minion_sync(model);
        state.minion_sync_ts = timestamp_get();
    }
}
