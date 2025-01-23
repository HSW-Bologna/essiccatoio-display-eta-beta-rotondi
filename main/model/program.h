#ifndef PROGRAM_H_INCLUDED
#define PROGRAM_H_INCLUDED

#include <stdint.h>
#include <stdlib.h>

#define MAX_LINGUE          10
#define MAX_PROGRAMMI       5     // 10
#define MAX_NAME_LENGTH     32
#define STRING_NAME_SIZE    (MAX_NAME_LENGTH + 1)
#define MAX_STEPS           10
#define STEP_SIZE           32
#define PROGRAM_SIZE(steps) ((size_t)(338 + STEP_SIZE * steps))
#define MAX_PROGRAM_SIZE    PROGRAM_SIZE(MAX_STEPS)


typedef char name_t[STRING_NAME_SIZE];


typedef enum {
    PROGRAMS_MODE_MANUAL = 0,
    PROGRAMS_MODE_AUTO,
} programs_mode_t;


typedef enum {
    PROGRAM_STEP_TYPE_DRYING = 0,
    PROGRAM_STEP_TYPE_COOLING,
    PROGRAM_STEP_TYPE_ANTIFOLD,
#define PROGRAM_STEP_TYPE_NUM 3,
} program_step_type_t;


typedef struct {
    uint16_t duration;
    uint16_t rotation_time;
    uint16_t pause_time;
    uint16_t speed;
    uint16_t temperature;
    uint16_t humidity;
    uint16_t cooling_hysteresis;
    uint16_t heating_hysteresis;
    uint16_t progressive_heating_time;

    uint8_t type;
    uint8_t enable_waiting_for_temperature;
    uint8_t enable_reverse;
} program_drying_parameters_t;

typedef struct {
    uint16_t duration;
    uint16_t rotation_time;
    uint16_t pause_time;

    uint8_t enable_reverse;
} program_cooling_parameters_t;

typedef struct {
    uint16_t max_duration;
    uint16_t max_cycles;
    uint16_t speed;
    uint16_t rotation_time;
    uint16_t pause_time;
    uint16_t start_delay;
} program_antifold_parameters_t;


typedef struct {
    program_step_type_t type;
    union {
        program_drying_parameters_t   drying;
        program_cooling_parameters_t  cooling;
        program_antifold_parameters_t antifold;
    };
} program_step_t;


typedef struct {
    name_t filename;
    name_t nomi[MAX_LINGUE];

    uint8_t                       cooling_enabled;
    program_cooling_parameters_t  cooling_step;
    uint8_t                       antifold_enabled;
    program_antifold_parameters_t antifold_step;

    uint16_t                    num_drying_steps;
    program_drying_parameters_t steps[MAX_STEPS];
} program_t;


void    init_new_program(program_t *p, int num);
void    update_program_name(program_t *p, const char *str, int lingua);
void    update_program_type(program_t *p, unsigned char type);
void    program_add_step(program_t *p, int tipo);
uint8_t program_swap_steps(program_t *p, int first, int second);
void    program_remove_step(program_t *p, int index);
void    program_insert_step(program_t *p, int tipo, size_t index);
int     pack_step(uint8_t *buffer, const program_drying_parameters_t *step, int num);
size_t  program_deserialize(program_t *p, uint8_t *buffer);
size_t  program_serialize(uint8_t *buffer, const program_t *p);
size_t  program_serialize_empty(uint8_t *buffer, uint16_t num);
void    program_copy_step(program_t *p, uint16_t source_step_index, uint16_t destination_step_index);
program_drying_parameters_t default_step(void);
uint16_t                    program_num_steps(const program_t *program);


extern const program_drying_parameters_t program_default_drying_parameters[];

#endif
