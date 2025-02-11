#include "model/model.h"
#include "adapters/view/view.h"
#include "controller.h"
#include "esp_log.h"
#include "services/timestamp.h"
#include "configuration/configuration.h"
#include "minion.h"
#include "lvgl.h"
#include "minion.h"
#include "services/fup.h"
#include "bsp/system.h"


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
static void clear_coins(pman_handle_t handle);
static void digital_coin_reader_enable(pman_handle_t handle, uint8_t enable);
static void save_program_index(pman_handle_t handle);
static void create_new_program(pman_handle_t handle, uint16_t program_index);
static void clone_program(pman_handle_t handle, uint16_t source_program_index, uint16_t destination_program_index);
static void delete_program(pman_handle_t handle, uint16_t program_index);
static void save_program(pman_handle_t handle, uint16_t program_index);
static void commissioning_done(pman_handle_t handle);
static void factory_reset(pman_handle_t handle);
static void update_firmware(pman_handle_t handle);
static void reset(pman_handle_t handle);


static const char *TAG = "Gui";


view_protocol_t controller_gui_protocol = {
    .retry_communication        = retry_communication,
    .refresh_minion             = refresh_minion,
    .set_test_mode              = set_test_mode,
    .test_output                = test_output,
    .test_drum                  = test_drum,
    .clear_test_outputs         = clear_test_outputs,
    .save_parmac                = save_parmac,
    .start_program              = start_program,
    .resume_cycle               = resume_cycle,
    .pause_cycle                = pause_cycle,
    .stop_cycle                 = stop_cycle,
    .digital_coin_reader_enable = digital_coin_reader_enable,
    .clear_coins                = clear_coins,
    .save_program_index         = save_program_index,
    .create_new_program         = create_new_program,
    .save_program               = save_program,
    .clone_program              = clone_program,
    .delete_program             = delete_program,
    .commissioning_done         = commissioning_done,
    .factory_reset              = factory_reset,
    .update_firmware            = update_firmware,
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
    mut_model_t *model                 = view_get_model(handle);
    model->run.minion.write.test_mode  = test_mode;
    model->run.test_enable_coin_reader = 0;
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
        uint16_t output_index = forward ? 0 : 1;
        model_clear_test_outputs(model);
        model_set_test_output(model, output_index);
    } else {
        model_clear_test_outputs(model);
    }

    model->run.minion.write.test_pwm2 = percentage;

    controller_sync_minion(model);
}


static void clear_test_outputs(pman_handle_t handle) {
    mut_model_t *model = view_get_model(handle);
    model_clear_test_outputs(model);
    controller_sync_minion(model);
}


static void save_parmac(pman_handle_t handle) {
    mut_model_t *model = view_get_model(handle);

    model_reset_temporary_language(model);
    configuration_save_parmac(&model->config.parmac);
    controller_sync_minion(model);
}


static void start_program(pman_handle_t handle, uint16_t program_index) {
    mut_model_t *model = view_get_model(handle);

    ESP_LOGI(TAG, "Starting program %i", program_index);
    model_select_program(model, program_index);
    controller_sync_minion(model);
    if (model_is_cycle_stopped(model) && model->run.minion.communication_enabled) {
        minion_resume_program(model, 1);
    }
}


static void resume_cycle(pman_handle_t handle) {
    mut_model_t *model = view_get_model(handle);
    if (model->run.minion.communication_enabled) {
        minion_resume_program(model, 1);
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
        model_reset_program(model);
        minion_program_done(model);
    }
}


static void clear_coins(pman_handle_t handle) {
    mut_model_t *model = view_get_model(handle);
    memset(model->run.minion.read.coins, 0, sizeof(model->run.minion.read.coins));
    minion_clear_coins();
}


static void digital_coin_reader_enable(pman_handle_t handle, uint8_t enable) {
    mut_model_t *model                 = view_get_model(handle);
    model->run.test_enable_coin_reader = enable;
    minion_sync(model);
}


static void save_program_index(pman_handle_t handle) {
    mut_model_t *model = view_get_model(handle);
    configuration_update_index(model->config.programs, model->config.num_programs);
}


static void create_new_program(pman_handle_t handle, uint16_t program_index) {
    mut_model_t *model = view_get_model(handle);
    configuration_create_empty_program(model);
    model->config.num_programs = configuration_load_programs(model, model->config.programs);
    configuration_clear_orphan_programs(model->config.programs, model->config.num_programs);

    program_t new_program = model->config.programs[model->config.num_programs - 1];
    for (uint16_t i = model->config.num_programs - 1; i > program_index; i--) {
        model->config.programs[i] = model->config.programs[i - 1];
    }
    model->config.programs[program_index] = new_program;

    ESP_LOGI(TAG, "Created new program at index %i", program_index);
}


static void clone_program(pman_handle_t handle, uint16_t source_program_index, uint16_t destination_program_index) {
    mut_model_t *model = view_get_model(handle);
    configuration_clone_program(model, source_program_index, destination_program_index);
    ESP_LOGI(TAG, "Clone program at %i to %i", source_program_index, destination_program_index);
}


static void delete_program(pman_handle_t handle, uint16_t program_index) {
    mut_model_t *model = view_get_model(handle);
    if (model->config.num_programs > BASE_PROGRAMS_NUM) {
        configuration_remove_program(model->config.programs, model->config.num_programs, program_index);
        model->config.num_programs--;
    }
}


static void save_program(pman_handle_t handle, uint16_t program_index) {
    mut_model_t *model = view_get_model(handle);
    configuration_update_program(model_get_program(model, program_index));
    configuration_update_index(model->config.programs, model->config.num_programs);
}


static void commissioning_done(pman_handle_t handle) {
    mut_model_t *model = view_get_model(handle);
    configuration_save_parmac(&model->config.parmac);
    configuration_commissioned(1);
}


static void factory_reset(pman_handle_t handle) {
    mut_model_t *model = view_get_model(handle);
    model_init(model);
    configuration_commissioned(0);
    configuration_save_parmac(&model->config.parmac);
    configuration_update_index(model->config.programs, model->config.num_programs);
    bsp_system_reset();
}


static void update_firmware(pman_handle_t handle) {
    (void)handle;
    fup_proceed();
}


static void reset(pman_handle_t handle) {
    (void)handle;
    bsp_system_reset();
}
