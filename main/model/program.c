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

void update_program_price(program_t *p, const char *string) {
    char *tmp = malloc(sizeof(string) + 1);
    strcpy(tmp, string);

    char *src, *dst;
    for (src = dst = tmp; *src != '\0'; src++) {
        *dst = *src;
        if (*dst != '.')
            dst++;
    }
    *dst = '\0';

    p->prezzo = atoi(tmp);
    free(tmp);
}


void program_add_step(program_t *p, int tipo) {
    program_insert_step(p, tipo, p->num_steps);
}


void program_insert_step(program_t *p, int tipo, size_t index) {
    if (p->num_steps < MAX_STEPS && index <= p->num_steps) {
        for (int i = (int)p->num_steps - 1; i >= (int)index; i--) {
            p->steps[i + 1] = p->steps[i];
        }

        p->steps[index] = program_default_drying_parameters[tipo];
        p->num_steps++;
    } else if (index > p->num_steps) {
        program_add_step(p, tipo);
    }
}


void programs_remove_step(program_t *p, int index) {
    int len = p->num_steps;

    for (int i = index; i < len - 1; i++) {
        p->steps[i] = p->steps[i + 1];
    }

    if (p->num_steps > 0) {
        p->num_steps--;
    }
}

void swap_steps(program_t *p, int first, int second) {
    if (first != second) {
        program_drying_parameters_t s = p->steps[first];
        p->steps[first]               = p->steps[second];
        p->steps[second]              = s;
    }
}


int pack_step(uint8_t *buffer, const program_drying_parameters_t *step, int num) {
    int     i      = 0;
    uint8_t bitmap = 0;

    PACK_BYTE(buffer, num, i);

    return i;
}

static int serialize_step(uint8_t *buffer, program_drying_parameters_t *s) {
    int i = 0;

    assert(i <= STEP_SIZE);
    return STEP_SIZE;     // Allow for some margin
}


int deserialize_step(program_drying_parameters_t *s, uint8_t *buffer) {
    int i = 0;

    assert(i <= STEP_SIZE);
    return STEP_SIZE;     // Allow for some margin
}


size_t deserialize_program(program_t *p, uint8_t *buffer) {
    size_t   i = 0;
    uint32_t prezzo;

    for (int j = 0; j < MAX_LINGUE; j++) {
        memcpy(p->nomi[j], &buffer[i], STRING_NAME_SIZE);
        i += STRING_NAME_SIZE;
    }

    i += deserialize_uint32_be(&prezzo, &buffer[i]);
    p->prezzo = ((float)prezzo);
    i += UNPACK_UINT16_BE(p->num_steps, &buffer[i]);

    for (size_t j = 0; j < p->num_steps; j++) {
        i += deserialize_step(&p->steps[j], &buffer[i]);
    }

    assert(i == PROGRAM_SIZE(p->num_steps));
    return i;
}


size_t serialize_program(uint8_t *buffer, program_t *p) {
    size_t i = 0;

    for (int j = 0; j < MAX_LINGUE; j++) {
        memcpy(&buffer[i], p->nomi[j], STRING_NAME_SIZE);
        i += STRING_NAME_SIZE;
    }

    i += serialize_uint32_be(&buffer[i], (uint32_t)p->prezzo);
    i += serialize_uint16_be(&buffer[i], p->num_steps);

    for (size_t j = 0; j < p->num_steps; j++) {
        i += serialize_step(&buffer[i], &p->steps[j]);
    }

    assert(i == PROGRAM_SIZE(p->num_steps));
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

    assert(i == PROGRAM_SIZE(0));
    return i;
}


static void init_names(name_t *names, uint16_t num) {
    const char *nuovo_programma[MAX_LINGUE] = {"Nuovo programma", "New program", "New program", "New program",
                                               "New program",     "New program", "New program", "New program",
                                               "New program",     "New program"};

    for (int i = 0; i < MAX_LINGUE; i++) {
        snprintf(names[i], sizeof(names[i]), "%s %i", nuovo_programma[i], num + 1);
    }
}
