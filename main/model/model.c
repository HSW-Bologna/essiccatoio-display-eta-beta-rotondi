#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "model.h"
#include "services/serializer.h"
#include "services/timestamp.h"
#include "parmac.h"
#include "program.h"
#include "adapters/view/intl/intl.h"


#define INPUT_FILTER_FEEDBACK 1
#define INPUT_EMERGENCY       5
#define INPUT_PORTHOLE        6


void model_init(mut_model_t *model) {
    assert(model != NULL);
    memset(model, 0, sizeof(mut_model_t));

    model->config.num_programs = BASE_PROGRAMS_NUM;
    for (size_t i = 0; i < BASE_PROGRAMS_NUM; i++) {
        model->config.programs[i].steps[0]         = program_default_drying_parameters[i];
        model->config.programs[i].num_drying_steps = 1;
    }

    model->run.minion.communication_enabled = 1;

    parmac_init(model, 1);
}


void model_init_default_programs(mut_model_t *model) {
    const strings_t names[BASE_PROGRAMS_NUM] = {
        STRINGS_CALDO, STRINGS_MEDIO, STRINGS_TIEPIDO, STRINGS_FREDDO, STRINGS_LANA,
    };

    for (uint16_t i = model->config.num_programs; i < BASE_PROGRAMS_NUM; i++) {
        program_t *program        = &model->config.programs[i];
        program->steps[0]         = program_default_drying_parameters[i];
        program->num_drying_steps = 1;
        snprintf(program->filename, sizeof(program->filename), "%i.bin", i + 1);
        program->cooling_enabled  = 0;
        program->antifold_enabled = 0;

        for (uint16_t j = 0; j < MAX_LINGUE; j++) {
            uint16_t language = j >= LANGUAGES_NUM ? LANGUAGES_NUM - 1 : j;
            snprintf(program->nomi[j], sizeof(program->nomi[j]), "%s",
                     view_intl_get_string_in_language(language, names[i]));
        }

        model->config.num_programs++;
    }
}


uint8_t model_is_porthole_open(model_t *model) {
    assert(model != NULL);
    return model_is_alarm_active(model, ALARM_PORTHOLE);
}


uint8_t model_is_any_alarm_active(model_t *model) {
    assert(model != NULL);
    return model->run.minion.read.alarms > 0;
}


uint8_t model_is_alarm_active(model_t *model, alarm_t alarm) {
    assert(model != NULL);
    return model->run.minion.read.alarms & (1 << alarm);
}


int16_t model_get_current_setpoint(model_t *model) {
    assert(model != NULL);
    const program_t *program = model_get_program(model, model->run.current_program_index);
    if (model->run.current_step_index < program->num_drying_steps) {
        return program->steps[model->run.current_step_index].temperature;
    } else {
        return 0;
    }
}


int16_t model_get_program_display_temperature(model_t *model, uint16_t program_index) {
    assert(model != NULL);
    if (program_index < model->config.num_programs) {
        const program_t *program = &model->config.programs[program_index];
        if (0 < program->num_drying_steps) {
            return program->steps[0].temperature;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}


void model_select_program(mut_model_t *model, uint16_t program_index) {
    assert(model != NULL);

    if (program_index < model->config.num_programs) {
        model->run.current_program_index = program_index;
        model->run.current_program       = model->config.programs[program_index];
    }
}


void model_select_step(mut_model_t *model, uint16_t step_index) {
    assert(model != NULL);

    if (step_index < model->run.current_program.num_drying_steps) {
        model->run.current_step_index = step_index;
    }
}


uint8_t model_is_cycle_active(model_t *model) {
    assert(model != NULL);

    switch (model->run.minion.read.cycle_state) {
        case CYCLE_STATE_ACTIVE:
        case CYCLE_STATE_RUNNING:
        case CYCLE_STATE_WAIT_START:
        case CYCLE_STATE_PAUSED:
            return 1;

        default:
            return 0;
    }

    return 0;
}


uint8_t model_is_cycle_paused(model_t *model) {
    assert(model != NULL);
    return model->run.minion.read.cycle_state == CYCLE_STATE_PAUSED;
}


uint8_t model_is_cycle_running(model_t *model) {
    assert(model != NULL);
    return model->run.minion.read.cycle_state == CYCLE_STATE_RUNNING;
}

uint8_t model_is_cycle_stopped(model_t *model) {
    assert(model != NULL);
    return model->run.minion.read.cycle_state == CYCLE_STATE_STOPPED;
}


program_step_t model_get_current_step(model_t *model) {
    assert(model != NULL);
    const program_t *program = model_get_program(model, model->run.current_program_index);

    if (model->run.current_step_index < program->num_drying_steps) {
        return (program_step_t){.type   = PROGRAM_STEP_TYPE_DRYING,
                                .drying = program->steps[model->run.current_step_index]};
    } else if (model->run.current_step_index == program->num_drying_steps && program->cooling_enabled) {
        return (program_step_t){.type = PROGRAM_STEP_TYPE_COOLING, .cooling = program->cooling_step};
    } else {
        return (program_step_t){.type = PROGRAM_STEP_TYPE_ANTIFOLD, .antifold = program->antifold_step};
    }
}


void model_clear_test_outputs(mut_model_t *model) {
    assert(model != NULL);

    model->run.minion.write.test_outputs = 0;
}


void model_set_test_output(mut_model_t *model, uint16_t output_index) {
    assert(model != NULL);

    model->run.minion.write.test_outputs = 1 << output_index;
}


uint8_t model_get_bit_accesso(uint8_t al) {
    if (al) {
        return TECHNICIAN_ACCESS_LEVEL;
    } else {
        return USER_ACCESS_LEVEL;
    }
}


char *model_new_unique_filename(model_t *model, char *filename, unsigned long seed) {
    unsigned long now = seed;
    int           found;

    do {
        found = 0;
        snprintf(filename, STRING_NAME_SIZE, "%lu.bin", now);

        for (size_t i = 0; i < model->config.num_programs; i++) {
            if (strcmp(filename, model->config.programs[i].filename) == 0) {
                now++;
                found = 1;
                break;
            }
        }
    } while (found);

    return filename;
}


const program_t *model_get_program(model_t *model, uint16_t program_index) {
    assert(model != NULL);
    return &model->config.programs[program_index];
}


program_t *model_get_mut_program(mut_model_t *model, uint16_t program_index) {
    assert(model != NULL);
    return &model->config.programs[program_index];
}


void model_reset_temporary_language(mut_model_t *model) {
    assert(model != NULL);
    model->run.temporary_language = model->config.parmac.language;
}


size_t model_serialize_parmac(uint8_t *buffer, parmac_t *p) {
    assert(p != NULL);
    size_t i = 0;

    memcpy(&buffer[i], p->nome, sizeof(name_t));
    i += sizeof(name_t);

    uint16_t flags = ((p->stop_time_in_pause > 0) << 0) | ((p->disabilita_allarmi > 0) << 1) |
                     ((p->gas_preemptive_reset > 0) << 2) | ((p->abilita_visualizzazione_temperatura > 0) << 3) |
                     ((p->abilita_tasto_menu > 0) << 4) | ((p->allarme_inverter_off_on > 0) << 5) |
                     ((p->allarme_filtro_off_on > 0) << 6)     // |
        ;

    i += serialize_uint16_be(&buffer[i], flags);
    i += serialize_uint16_be(&buffer[i], (uint16_t)p->language);
    i += serialize_uint16_be(&buffer[i], (uint16_t)p->tempo_stop_automatico);
    i += serialize_uint16_be(&buffer[i], (uint16_t)p->tipo_macchina_occupata);
    i += serialize_uint16_be(&buffer[i], (uint16_t)p->temperature_probe);
    i += serialize_uint16_be(&buffer[i], (uint16_t)p->temperatura_sicurezza);
    i += serialize_uint16_be(&buffer[i], (uint16_t)p->tempo_allarme_temperatura);
    i += serialize_uint16_be(&buffer[i], (uint16_t)p->heating_type);
    i += serialize_uint16_be(&buffer[i], (uint16_t)p->gas_ignition_attempts);
    i += serialize_uint16_be(&buffer[i], (uint16_t)p->busy_signal_nc_na);
    i += serialize_uint16_be(&buffer[i], (uint16_t)p->porthole_nc_na);
    i += serialize_uint16_be(&buffer[i], (uint16_t)p->air_flow_alarm_time);
    i += serialize_uint16_be(&buffer[i], (uint16_t)p->fan_with_open_porthole_time);
    i += serialize_uint8(&buffer[i], (uint16_t)p->access_level);

    // assert(i == PARMAC_SIZE);
    return i;
}


size_t model_deserialize_parmac(parmac_t *p, uint8_t *buffer) {
    assert(p != NULL);
    size_t i = 0;

    memcpy(p->nome, &buffer[i], sizeof(name_t));
    i += sizeof(name_t);

    uint16_t flags = 0;
    i += UNPACK_UINT16_BE(flags, &buffer[i]);
    p->stop_time_in_pause                  = (flags & (1 << 0)) > 0;
    p->disabilita_allarmi                  = (flags & (1 << 1)) > 0;
    p->gas_preemptive_reset                = (flags & (1 << 2)) > 0;
    p->abilita_visualizzazione_temperatura = (flags & (1 << 3)) > 0;
    p->abilita_tasto_menu                  = (flags & (1 << 4)) > 0;
    p->allarme_inverter_off_on             = (flags & (1 << 5)) > 0;
    p->allarme_filtro_off_on               = (flags & (1 << 6)) > 0;

    i += UNPACK_UINT16_BE(p->language, &buffer[i]);
    i += UNPACK_UINT16_BE(p->tempo_stop_automatico, &buffer[i]);
    i += UNPACK_UINT16_BE(p->tipo_macchina_occupata, &buffer[i]);
    i += UNPACK_UINT16_BE(p->temperature_probe, &buffer[i]);
    i += UNPACK_UINT16_BE(p->temperatura_sicurezza, &buffer[i]);
    i += UNPACK_UINT16_BE(p->tempo_allarme_temperatura, &buffer[i]);
    i += UNPACK_UINT16_BE(p->heating_type, &buffer[i]);
    i += UNPACK_UINT16_BE(p->gas_ignition_attempts, &buffer[i]);
    i += UNPACK_UINT16_BE(p->busy_signal_nc_na, &buffer[i]);
    i += UNPACK_UINT16_BE(p->porthole_nc_na, &buffer[i]);
    i += UNPACK_UINT16_BE(p->air_flow_alarm_time, &buffer[i]);
    i += UNPACK_UINT16_BE(p->fan_with_open_porthole_time, &buffer[i]);
    i += UNPACK_UINT8(p->access_level, &buffer[i]);

    // assert(i == PARMAC_SIZE);
    return i;
}


uint8_t model_waiting_for_next_step(model_t *model) {
    assert(model != NULL);
    const program_t *program = model_get_program(model, model->run.current_program_index);

    return model->run.minion.read.cycle_state == CYCLE_STATE_ACTIVE &&
           model->run.minion.read.remaining_time_seconds == 0 &&
           model->run.current_step_index < program_num_steps(program);
}


uint8_t model_is_program_done(model_t *model) {
    assert(model != NULL);
    const program_t *program = model_get_program(model, model->run.current_program_index);

    return model->run.minion.read.cycle_state == CYCLE_STATE_ACTIVE &&
           model->run.minion.read.remaining_time_seconds == 0 &&
           model->run.current_step_index + 1 >= program_num_steps(program);
}


uint8_t model_should_enable_coin_reader(model_t *model) {
    assert(model != NULL);
    if (model->run.minion.write.test_mode) {
        return model->run.test_enable_coin_reader;
    } else {
        return 0;
    }
}


void model_move_to_next_step(mut_model_t *model) {
    assert(model);
    const program_t *program = model_get_program(model, model->run.current_program_index);
    if (model->run.current_step_index < program_num_steps(program)) {
        model->run.current_step_index++;
    }
}


void model_reset_program(mut_model_t *model) {
    assert(model);
    model->run.current_step_index = 0;
}


uint8_t model_is_step_endless(model_t *model) {
    assert(model);
    return model->run.minion.read.remaining_time_seconds == 0xFFFF;
}


uint8_t model_swap_programs(mut_model_t *model, size_t first, size_t second) {
    assert(model != NULL);

    if (first >= model->config.num_programs) {
        return 0;
    }
    if (second >= model->config.num_programs) {
        return 0;
    }
    if (first == second) {
        return 0;
    }

    program_t p                    = model->config.programs[first];
    model->config.programs[first]  = model->config.programs[second];
    model->config.programs[second] = p;

    return 1;
}


void model_create_new_program(mut_model_t *model, uint16_t program_index) {
    assert(model != NULL);

    if (program_index > model->config.num_programs || model->config.num_programs >= MAX_PROGRAMMI) {
        return;
    }

    if (model->config.num_programs > 0) {
        // Make room
        for (uint16_t i = model->config.num_programs - 1; i > program_index; i--) {
            model->config.programs[i] = model->config.programs[i - 1];
        }
    }

    program_t *program = &model->config.programs[program_index];
    model_new_unique_filename(model, program->filename, timestamp_get());

    const char *nuovo_programma[MAX_LINGUE] = {"Nuovo programma", "New program", "New program", "New program",
                                               "New program",     "New program", "New program", "New program",
                                               "New program",     "New program"};
    for (uint16_t i = 0; i < MAX_LINGUE; i++) {
        strcpy(program->nomi[i], nuovo_programma[i]);
    }

    program->antifold_enabled = 0;
    program->cooling_enabled  = 0;
    program->num_drying_steps = 0;

    model->config.num_programs++;
}


void model_clone_program(mut_model_t *model, uint16_t source_program_index, uint16_t destination_program_index) {
    assert(model != NULL);

    if (source_program_index > model->config.num_programs || destination_program_index > model->config.num_programs ||
        model->config.num_programs >= MAX_PROGRAMMI) {
        return;
    }

    program_t source_program = model->config.programs[source_program_index];
    model_create_new_program(model, destination_program_index);
    model->config.programs[destination_program_index] = source_program;
}


void model_delete_program(mut_model_t *model, uint16_t program_index) {
    assert(model != NULL);

    for (uint16_t i = program_index; i < model->config.num_programs - 1; i++) {
        model->config.programs[i] = model->config.programs[i + 1];
    }
    model->config.num_programs--;
}
