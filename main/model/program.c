#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "program.h"
// #include "view/lingue.h"
#include "assert.h"
#include "services/serializer.h"


#define PACK_BYTE(buffer, num, i)     i += PACK_UINT8(&buffer[i], num)
#define PACK_SHORT_BE(buffer, num, i) i += PACK_UINT16_BE(&buffer[i], num)


static void init_names(name_t *names, uint16_t num);


const program_drying_parameters_t program_default_drying_parameters[] = {
    {
        .type                           = PROGRAMS_MODE_MANUAL,
        .duration                       = 60,
        .enable_waiting_for_temperature = 1,
        .enable_reverse                 = 0,
        .rotation_time                  = 32,
        .pause_time                     = 8,
        .speed                          = 40,
        .temperature                    = 80,
        .humidity                       = 40,
    },
    {
        .type                           = PROGRAMS_MODE_MANUAL,
        .duration                       = 60,
        .enable_waiting_for_temperature = 1,
        .enable_reverse                 = 0,
        .rotation_time                  = 32,
        .pause_time                     = 8,
        .speed                          = 40,
        .temperature                    = 60,
        .humidity                       = 40,
    },
    {
        .type                           = PROGRAMS_MODE_MANUAL,
        .duration                       = 60,
        .enable_waiting_for_temperature = 1,
        .enable_reverse                 = 0,
        .rotation_time                  = 32,
        .pause_time                     = 8,
        .speed                          = 40,
        .temperature                    = 40,
        .humidity                       = 40,
    },
    {
        .type                           = PROGRAMS_MODE_MANUAL,
        .duration                       = 60,
        .enable_waiting_for_temperature = 1,
        .enable_reverse                 = 0,
        .rotation_time                  = 32,
        .pause_time                     = 8,
        .speed                          = 40,
        .temperature                    = 30,
        .humidity                       = 40,
    },
    {
        .type                           = PROGRAMS_MODE_MANUAL,
        .duration                       = 60,
        .enable_waiting_for_temperature = 1,
        .enable_reverse                 = 0,
        .rotation_time                  = 32,
        .pause_time                     = 8,
        .speed                          = 40,
        .temperature                    = 20,
        .humidity                       = 40,
    },
};


void update_program_name(program_t *p, const char *str, int lingua) {
    if (strlen(str) > 0) {
        strcpy(p->nomi[lingua], str);
    }
}


void program_add_step(program_t *p, int tipo) {
    program_insert_step(p, tipo, p->num_drying_steps);
}


void program_insert_step(program_t *p, int tipo, size_t index) {
    if (p->num_drying_steps < MAX_STEPS && index <= p->num_drying_steps) {
        for (int i = (int)p->num_drying_steps - 1; i >= (int)index; i--) {
            p->steps[i + 1] = p->steps[i];
        }

        p->steps[index] = program_default_drying_parameters[tipo];
        p->num_drying_steps++;
    } else if (index > p->num_drying_steps) {
        program_add_step(p, tipo);
    }
}


void program_remove_step(program_t *p, int index) {
    int len = p->num_drying_steps;

    for (int i = index; i < len - 1; i++) {
        p->steps[i] = p->steps[i + 1];
    }

    if (p->num_drying_steps > 0) {
        p->num_drying_steps--;
    }
}


void program_copy_step(program_t *p, uint16_t source_step_index, uint16_t destination_step_index) {
    if (source_step_index < p->num_drying_steps && destination_step_index <= p->num_drying_steps &&
        p->num_drying_steps < MAX_STEPS) {
        p->steps[p->num_drying_steps] = p->steps[source_step_index];
        p->num_drying_steps++;
        program_swap_steps(p, destination_step_index, p->num_drying_steps - 1);
    }
}


uint8_t program_swap_steps(program_t *p, int first, int second) {
    if (first != second) {
        program_drying_parameters_t s = p->steps[first];
        p->steps[first]               = p->steps[second];
        p->steps[second]              = s;
        return 1;
    } else {
        return 0;
    }
}


static int serialize_drying_step(uint8_t *buffer, const program_drying_parameters_t *s) {
    int i = 0;

    i += serialize_uint16_be(&buffer[i], s->duration);
    i += serialize_uint16_be(&buffer[i], s->rotation_time);
    i += serialize_uint16_be(&buffer[i], s->pause_time);
    i += serialize_uint16_be(&buffer[i], s->speed);
    i += serialize_uint16_be(&buffer[i], s->temperature);
    i += serialize_uint16_be(&buffer[i], s->humidity);
    i += serialize_uint16_be(&buffer[i], s->cooling_hysteresis);
    i += serialize_uint16_be(&buffer[i], s->heating_hysteresis);
    i += serialize_uint16_be(&buffer[i], s->progressive_heating_time);
    i += serialize_uint8(&buffer[i], s->type);
    i += serialize_uint8(&buffer[i], s->enable_waiting_for_temperature);
    i += serialize_uint8(&buffer[i], s->enable_reverse);

    assert(i <= STEP_SIZE);
    return STEP_SIZE;     // Allow for some margin
}


int deserialize_drying_step(program_drying_parameters_t *s, uint8_t *buffer) {
    int i = 0;

    i += deserialize_uint16_be(&s->duration, &buffer[i]);
    i += deserialize_uint16_be(&s->rotation_time, &buffer[i]);
    i += deserialize_uint16_be(&s->pause_time, &buffer[i]);
    i += deserialize_uint16_be(&s->speed, &buffer[i]);
    i += deserialize_uint16_be(&s->temperature, &buffer[i]);
    i += deserialize_uint16_be(&s->humidity, &buffer[i]);
    i += deserialize_uint16_be(&s->cooling_hysteresis, &buffer[i]);
    i += deserialize_uint16_be(&s->heating_hysteresis, &buffer[i]);
    i += deserialize_uint16_be(&s->progressive_heating_time, &buffer[i]);
    i += deserialize_uint8(&s->type, &buffer[i]);
    i += deserialize_uint8(&s->enable_waiting_for_temperature, &buffer[i]);
    i += deserialize_uint8(&s->enable_reverse, &buffer[i]);

    assert(i <= STEP_SIZE);
    return STEP_SIZE;     // Allow for some margin
}


static int serialize_cooling_step(uint8_t *buffer, const program_cooling_parameters_t *s) {
    int i = 0;

    i += serialize_uint16_be(&buffer[i], s->duration);
    i += serialize_uint16_be(&buffer[i], s->rotation_time);
    i += serialize_uint16_be(&buffer[i], s->pause_time);
    i += serialize_uint8(&buffer[i], s->enable_reverse);

    assert(i <= STEP_SIZE);
    return STEP_SIZE;     // Allow for some margin
}


int deserialize_cooling_step(program_cooling_parameters_t *s, uint8_t *buffer) {
    int i = 0;

    i += deserialize_uint16_be(&s->duration, &buffer[i]);
    i += deserialize_uint16_be(&s->rotation_time, &buffer[i]);
    i += deserialize_uint16_be(&s->pause_time, &buffer[i]);
    i += deserialize_uint8(&s->enable_reverse, &buffer[i]);

    assert(i <= STEP_SIZE);
    return STEP_SIZE;     // Allow for some margin
}


static int serialize_antifold_step(uint8_t *buffer, const program_antifold_parameters_t *s) {
    int i = 0;

    i += serialize_uint16_be(&buffer[i], s->max_duration);
    i += serialize_uint16_be(&buffer[i], s->speed);
    i += serialize_uint16_be(&buffer[i], s->rotation_time);
    i += serialize_uint16_be(&buffer[i], s->pause_time);

    assert(i <= STEP_SIZE);
    return STEP_SIZE;     // Allow for some margin
}


int deserialize_antifold_step(program_antifold_parameters_t *s, uint8_t *buffer) {
    int i = 0;

    i += deserialize_uint16_be(&s->max_duration, &buffer[i]);
    i += deserialize_uint16_be(&s->speed, &buffer[i]);
    i += deserialize_uint16_be(&s->rotation_time, &buffer[i]);
    i += deserialize_uint16_be(&s->pause_time, &buffer[i]);

    assert(i <= STEP_SIZE);
    return STEP_SIZE;     // Allow for some margin
}


size_t program_deserialize(program_t *p, uint8_t *buffer) {
    size_t i = 0;

    for (int j = 0; j < MAX_LINGUE; j++) {
        memcpy(p->nomi[j], &buffer[i], STRING_NAME_SIZE);
        i += STRING_NAME_SIZE;
    }

    i += UNPACK_UINT8(p->cooling_enabled, &buffer[i]);
    i += deserialize_cooling_step(&p->cooling_step, &buffer[i]);

    i += UNPACK_UINT8(p->antifold_enabled, &buffer[i]);
    i += deserialize_antifold_step(&p->antifold_step, &buffer[i]);

    i += UNPACK_UINT16_BE(p->num_drying_steps, &buffer[i]);

    for (size_t j = 0; j < p->num_drying_steps; j++) {
        i += deserialize_drying_step(&p->steps[j], &buffer[i]);
    }

#warning "Restore"
    // assert(i == PROGRAM_SIZE(p->num_drying_steps));
    return i;
}


size_t program_serialize(uint8_t *buffer, const program_t *p) {
    size_t i = 0;

    for (int j = 0; j < MAX_LINGUE; j++) {
        memcpy(&buffer[i], p->nomi[j], STRING_NAME_SIZE);
        i += STRING_NAME_SIZE;
    }

    i += serialize_uint8(&buffer[i], p->cooling_enabled);
    i += serialize_cooling_step(&buffer[i], &p->cooling_step);

    i += serialize_uint8(&buffer[i], p->antifold_enabled);
    i += serialize_antifold_step(&buffer[i], &p->antifold_step);

    i += serialize_uint16_be(&buffer[i], p->num_drying_steps);

    for (size_t j = 0; j < p->num_drying_steps; j++) {
        i += serialize_drying_step(&buffer[i], &p->steps[j]);
    }

    // assert(i == PROGRAM_SIZE(p->num_drying_steps));
    return i;
}


size_t program_serialize_empty(uint8_t *buffer, uint16_t num) {
    size_t i = 0;
    name_t names[MAX_LINGUE];
    init_names(names, num);

    for (int j = 0; j < MAX_LINGUE; j++) {
        memcpy(&buffer[i], names[j], STRING_NAME_SIZE);
        i += STRING_NAME_SIZE;
    }

    i += serialize_uint32_be(&buffer[i], (uint32_t)0);
    i += serialize_uint16_be(&buffer[i], 0);
    i += serialize_uint16_be(&buffer[i], 0);

    // assert(i == PROGRAM_SIZE(0));
    return i;
}


uint8_t program_is_automatic(const program_t *program) {
    assert(program);
    for (uint16_t i = 0; i < program->num_drying_steps; i++) {
        if (program->steps[i].type) {
            return 1;
        }
    }
    return 0;
}


static void init_names(name_t *names, uint16_t num) {
    const char *nuovo_programma[MAX_LINGUE] = {"Nuovo programma", "New program", "New program", "New program",
                                               "New program",     "New program", "New program", "New program",
                                               "New program",     "New program"};

    for (int i = 0; i < MAX_LINGUE; i++) {
        snprintf(names[i], sizeof(names[i]), "%s %i", nuovo_programma[i], num + 1);
    }
}
