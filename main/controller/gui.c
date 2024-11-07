#include "model/model.h"
#include "adapters/view/view.h"
#include "controller.h"
#include "esp_log.h"
#include "services/timestamp.h"
#include "configuration/configuration.h"
#include "minion.h"
#include "lvgl.h"
#include "minion.h"


static void retry_communication(pman_handle_t handle);
static void refresh_minion(pman_handle_t handle);
static void test_output(pman_handle_t handle, uint16_t output_index);
static void clear_test_outputs(pman_handle_t handle);
static void save_parmac(pman_handle_t handle);
static void set_test_mode(pman_handle_t handle, uint8_t test_mode);
static void test_drum(pman_handle_t handle, uint8_t forward, uint8_t run, uint8_t percentage);
static void start_program(pman_handle_t handle, uint16_t program_index);
static void resume_cycle(pman_handle_t handle);
static void pause_cycle(pman_handle_t handle);
static void stop_cycle(pman_handle_t handle);
static void save_programs(pman_handle_t handle);


static const char *TAG = "Gui";


view_protocol_t controller_gui_protocol = {
    .retry_communication = retry_communication,
    .refresh_minion      = refresh_minion,
    .set_test_mode       = set_test_mode,
    .test_output         = test_output,
    .test_drum           = test_drum,
    .clear_test_outputs  = clear_test_outputs,
    .save_parmac         = save_parmac,
    .start_program       = start_program,
    .resume_cycle        = resume_cycle,
    .pause_cycle         = pause_cycle,
    .stop_cycle          = stop_cycle,
    .save_programs       = save_programs,
};


void controller_gui_manage(void) {
    (void)TAG;

#ifndef BUILD_CONFIG_SIMULATED_APP
    static timestamp_t last_invoked = 0;

    if (last_invoked != timestamp_get()) {
        if (last_invoked > 0) {
            lv_tick_inc(timestamp_interval(last_invoked, timestamp_get()));
        }
        last_invoked = timestamp_get();
    }
#endif

    lv_timer_handler();
}


static void retry_communication(pman_handle_t handle) {
    ESP_LOGI(TAG, "Requesting com retry");

    minion_retry_communication();
    mut_model_t *model                      = view_get_model(handle);
    model->run.minion.communication_error   = 0;
    model->run.minion.communication_enabled = 1;
}


static void refresh_minion(pman_handle_t handle) {
    mut_model_t *model = view_get_model(handle);
    controller_sync_minion(model);
}


static void set_test_mode(pman_handle_t handle, uint8_t test_mode) {
    mut_model_t *model                = view_get_model(handle);
    model->run.minion.write.test_mode = test_mode;
    controller_sync_minion(model);
}


static void test_output(pman_handle_t handle, uint16_t output_index) {
    mut_model_t *model = view_get_model(handle);

    if (((1 << output_index) & model->run.minion.write.test_outputs) > 0) {
        model_clear_test_outputs(model);
    } else {
        model_set_test_output(model, output_index);
    }
    controller_sync_minion(model);
}


static void test_drum(pman_handle_t handle, uint8_t forward, uint8_t run, uint8_t percentage) {
    mut_model_t *model = view_get_model(handle);

    if (run) {
        uint16_t output_index = forward ? 5 : 4;
        model_clear_test_outputs(model);
        model_set_test_output(model, output_index);
    } else {
        model_clear_test_outputs(model);
    }

    model->run.minion.write.test_pwm1 = percentage;

    controller_sync_minion(model);
}


static void clear_test_outputs(pman_handle_t handle) {
    mut_model_t *model = view_get_model(handle);
    model_clear_test_outputs(model);
    controller_sync_minion(model);
}


static void save_parmac(pman_handle_t handle) {
    mut_model_t *model = view_get_model(handle);

    configuration_save_parmac(&model->config.parmac);
    controller_sync_minion(model);
}


static void start_program(pman_handle_t handle, uint16_t program_index) {
    mut_model_t *model = view_get_model(handle);

    ESP_LOGI(TAG, "Starting program %i", program_index);
    model_select_program(model, program_index);
    controller_sync_minion(model);
    if (model_is_cycle_stopped(model) && model->run.minion.communication_enabled) {
        minion_resume_program();
    }
}


static void resume_cycle(pman_handle_t handle) {
    mut_model_t *model = view_get_model(handle);
    if (model->run.minion.communication_enabled) {
        minion_resume_program();
    }
}


static void pause_cycle(pman_handle_t handle) {
    mut_model_t *model = view_get_model(handle);
    if (model->run.minion.communication_enabled) {
        minion_pause_program();
    }
}


static void stop_cycle(pman_handle_t handle) {
    mut_model_t *model = view_get_model(handle);
    if (model->run.minion.communication_enabled) {
        minion_program_done();
    }
}


static void save_programs(pman_handle_t handle) {
    mut_model_t *model = view_get_model(handle);
    configuration_update_index(model->config.programs, model->config.num_programs);
    for (uint16_t i = 0; i < model->config.num_programs; i++) {
        configuration_update_program(&model->config.programs[i]);
    }
}
