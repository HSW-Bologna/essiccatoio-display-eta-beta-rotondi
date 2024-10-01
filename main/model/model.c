#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "model.h"
#include "services/serializer.h"
#include "parmac.h"
#include "program.h"


#define INPUT_FILTER_FEEDBACK 1
#define INPUT_EMERGENCY       5
#define INPUT_PORTHOLE        6


void model_init(mut_model_t *model) {
    assert(model != NULL);
    memset(model, 0, sizeof(mut_model_t));

    model->config.num_programs = BASE_PROGRAMS_NUM;
    for (size_t i = 0; i < BASE_PROGRAMS_NUM; i++) {
        model->config.programs[i].steps[0]  = program_default_drying_parameters;
        model->config.programs[i].num_steps = 1;
    }

    parmac_init(model, 1);
}


uint8_t model_is_drying(model_t *model) {
    assert(model != NULL);
    return model->run.minion.read.step_type == DRYER_PROGRAM_STEP_TYPE_DRYING;
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

    if (step_index < model->run.current_program.num_steps) {
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


uint8_t model_is_cycle_stopped(model_t *model) {
    assert(model != NULL);
    return model->run.minion.read.cycle_state == CYCLE_STATE_STOPPED;
}


program_drying_parameters_t model_get_current_step(model_t *model) {
    assert(model != NULL);

    return model->run.current_program.steps[model->run.current_step_index];
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


program_t *model_get_program(mut_model_t *pmodel) {
    assert(pmodel != NULL);
    return &pmodel->run.current_program;
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

    i += serialize_uint16_be(&buffer[i], (uint16_t)p->language);

    // assert(i == PARMAC_SIZE);
    return i;
}


size_t model_deserialize_parmac(parmac_t *p, uint8_t *buffer) {
    assert(p != NULL);
    size_t i = 0;

    memcpy(p->nome, &buffer[i], sizeof(name_t));
    i += sizeof(name_t);

    i += UNPACK_UINT16_BE(p->language, &buffer[i]);

    // assert(i == PARMAC_SIZE);
    return i;
}
