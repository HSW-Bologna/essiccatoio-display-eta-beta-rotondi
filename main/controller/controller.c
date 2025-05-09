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
#include "bsp/system.h"
#include "bsp/tft/display.h"
#include "services/fup.h"
#include "observer.h"


static struct {
    timestamp_t minion_sync_ts;
    uint8_t     handshook;
    uint16_t    postpone_alarms_counter;
} state = {0};


static const char *TAG = "Controller";


void controller_init(mut_model_t *model) {
    configuration_init(model);
    configuration_load_all_data(model);
    model_check_parameters(model);
    observer_init(model);

    minion_init();

    view_change_page(&page_splash);
}


void controller_process_message(pman_handle_t handle, void *msg) {
    (void)handle;
    (void)msg;
}


void controller_manage(mut_model_t *model) {
    observer_manage(model);
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
                    uint16_t      previous_credit      = model_get_credit(model);
                    cycle_state_t previous_cycle_state = model->run.minion.read.cycle_state;

                    model->run.minion.read.firmware_version_major = response.as.sync.firmware_version_major;
                    model->run.minion.read.firmware_version_minor = response.as.sync.firmware_version_minor;
                    model->run.minion.read.firmware_version_patch = response.as.sync.firmware_version_patch;
                    model->run.minion.read.build_day              = response.as.sync.build_day;
                    model->run.minion.read.build_month            = response.as.sync.build_month;
                    model->run.minion.read.build_year             = response.as.sync.build_year;
                    model->run.minion.read.inputs                 = response.as.sync.inputs;
                    model->run.minion.read.outputs                = response.as.sync.outputs;
                    model->run.minion.read.fan_speed              = response.as.sync.fan_speed;
                    model->run.minion.read.drum_speed             = response.as.sync.drum_speed;
                    model->run.minion.read.heating                = response.as.sync.heating;
                    model->run.minion.read.held_by_temperature    = response.as.sync.held_by_temperature;
                    model->run.minion.read.held_by_humidity       = response.as.sync.held_by_humidity;
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
                    model->run.minion.read.elapsed_time_seconds   = response.as.sync.elapsed_time_seconds;
                    model->run.minion.read.remaining_time_seconds = response.as.sync.remaining_time_seconds;
                    model->run.minion.read.alarms                 = response.as.sync.alarms;
                    model->run.minion.read.payment                = response.as.sync.payment;

                    model->run.minion.read.stats.complete_cycles          = response.as.sync.complete_cycles;
                    model->run.minion.read.stats.partial_cycles           = response.as.sync.partial_cycles;
                    model->run.minion.read.stats.active_time_seconds      = response.as.sync.active_time_seconds;
                    model->run.minion.read.stats.work_time_seconds        = response.as.sync.work_time_seconds;
                    model->run.minion.read.stats.rotation_time_seconds    = response.as.sync.rotation_time_seconds;
                    model->run.minion.read.stats.ventilation_time_seconds = response.as.sync.ventilation_time_seconds;
                    model->run.minion.read.stats.heating_time_seconds     = response.as.sync.heating_time_seconds;

                    const uint16_t tf[] = {0, 4, 3, 2, 1};
                    for (size_t i = DIGITAL_COIN_LINE_1; i <= DIGITAL_COIN_LINE_5; i++) {
                        model->run.minion.read.coins[tf[i]] = response.as.sync.coins[i];
                    }
                    uint16_t current_credit = model_get_credit(model);

                    // A coin was received, increase the running time
                    if (current_credit > previous_credit) {
                        if (!model_is_cycle_stopped(model)) {
                            switch (model_get_current_step(model).type) {
                                // If we are already drying just increase the duration
                                case PROGRAM_STEP_TYPE_DRYING: {
                                    minion_increase_duration((current_credit - previous_credit) *
                                                             model->config.parmac.time_per_coin);
                                    minion_sync(model);
                                    break;
                                }

                                // In cooling go back to drying with the specified credit
                                case PROGRAM_STEP_TYPE_COOLING: {
                                    model_return_to_drying(model);
                                    minion_change_step(model);
                                    minion_sync(model);
                                    break;
                                }

                                // In antifold do nothing; the credit should be kept for the next cycle
                                case PROGRAM_STEP_TYPE_ANTIFOLD: {
                                    break;
                                }
                            }
                        }
                    }

                    // The cycle has started
                    if (previous_cycle_state != model->run.minion.read.cycle_state) {
                        switch (previous_cycle_state) {
                            case CYCLE_STATE_STOPPED:
                                model->run.starting_temperature = model->run.minion.read.default_temperature;
                                break;

                            default:
                                break;
                        }
                    }

                    if (model_is_porthole_open(model)) {
                        model->run.should_open_porthole = 0;
                    }

                    // Program finished
                    if (model_is_program_done(model)) {
                        ESP_LOGI(TAG, "Program done!");
                        minion_program_done(model);
                    }
                    // Currently waiting
                    else if (model_waiting_for_next_step(model)) {
                        ESP_LOGI(TAG, "Moving to next step");
                        model_move_to_next_step(model);
                        minion_resume_program(model, 0);
                    }

                    if (model_is_cycle_stopped(model)) {
                        model_reset_program(model);
                    }
                    break;
                }

                case MINION_RESPONSE_TAG_HANDSHAKE: {
                    ESP_LOGI(TAG, "Handshake: %i %i %i", response.as.handshake.cycle_state,
                             response.as.handshake.program_index, response.as.handshake.step_index);
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
                    if (msc_response.error) {
                        model->run.storage_status = STORAGE_STATUS_ERROR;
                    } else {
                        model->run.storage_status = STORAGE_STATUS_DONE;
                    }
                    break;

                case MSC_RESPONSE_CODE_ARCHIVE_SAVING_COMPLETE:
                    ESP_LOGI(TAG, "Configuration saved!");
                    model->run.removable_drive_state = msc_is_device_mounted();
                    if (msc_response.error) {
                        model->run.storage_status = STORAGE_STATUS_ERROR;
                    } else {
                        model->run.storage_status = STORAGE_STATUS_DONE;
                    }
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
            minion_handshake(model);
            initialized          = 1;
            state.minion_sync_ts = timestamp_get();
        }
        // Wait a few seconds before starting to query the minion in order for it to boot properly
        else {
            boot_wait_counter++;
            state.minion_sync_ts = timestamp_get();
        }

        model->run.removable_drive_state = msc_is_device_mounted();
        msc_read_archives(model);
    }

    {
        static unsigned long ts = 0;
        if (timestamp_is_expired(ts, 10000)) {
            bsp_system_memory_dump();
            ts = timestamp_get();
        }
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
