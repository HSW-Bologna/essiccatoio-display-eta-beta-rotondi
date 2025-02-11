#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "parameter.h"
#include "descriptions/parstring.h"
#include "model.h"
#include "parmac.h"
#include "descriptions/AUTOGEN_FILE_pars.h"


#define NUM_PARAMETERS 12
#define USER_BITS      0x01
#define TECH_BITS      0x02


static parameter_handle_t parameters[NUM_PARAMETERS];


static parameter_handle_t *get_actual_parameter(model_t *model, size_t parameter, uint8_t al);


static const char *fmt_min     = "%i min";
static const char *fmt_degrees = "%i °C";


void parlav_init_drying(mut_model_t *model, program_drying_parameters_t *p) {
    memset(parameters, 0, sizeof(parameters));
    parameter_handle_t *ps = parameters;
    uint16_t            i  = 0;

    // clang-format off
    ps[i++] = PARAMETER(&p->type,                          0,     1,    0,    FOPT(PARS_DESCRIPTIONS_TIPO, pars_manuale_automatico),          USER_BITS);
    ps[i++] = PARAMETER(&p->duration,                          1,     99,    35,    FFINT(PARS_DESCRIPTIONS_DURATA, fmt_min),          USER_BITS);
    ps[i++] = PARAMETER(&p->enable_waiting_for_temperature,    0,     1,     0,     FOPT(PARS_DESCRIPTIONS_ATTESA_TEMPERATURA, pars_nosi),        USER_BITS);
    ps[i++] = PARAMETER(&p->enable_reverse,    0,     1,     0,     FOPT(PARS_DESCRIPTIONS_ABILITA_INVERSIONE, pars_nosi),        USER_BITS);
    ps[i++] = PARAMETER(&p->rotation_time,    1,     99,     95,     FTIME(PARS_DESCRIPTIONS_TEMPO_DI_ROTAZIONE),        USER_BITS);
    ps[i++] = PARAMETER(&p->pause_time,    5,     99,     6,     FTIME(PARS_DESCRIPTIONS_TEMPO_DI_PAUSA),        USER_BITS);
    ps[i++] = PARAMETER(&p->speed,    model->config.parmac.minimum_speed,     model->config.parmac.maximum_speed,     55,     FFINT(PARS_DESCRIPTIONS_VELOCITA, "%i rpm"),        USER_BITS);
    ps[i++] = PARAMETER(&p->temperature,    0,     model_get_maximum_temperature(model),     50,     FFINT(PARS_DESCRIPTIONS_TEMPERATURA, fmt_degrees),        USER_BITS);
    ps[i++] = PARAMETER(&p->cooling_hysteresis,    0,     20,     2,     FFINT(PARS_DESCRIPTIONS_ISTERESI_RAFFREDDAMENTO, fmt_degrees),        USER_BITS);
    ps[i++] = PARAMETER(&p->heating_hysteresis,    0,     20,     2,     FFINT(PARS_DESCRIPTIONS_ISTERESI_RISCALDAMENTO, fmt_degrees),        USER_BITS);
    ps[i++] = PARAMETER(&p->humidity,    5,     50,     30,     FFINT(PARS_DESCRIPTIONS_UMIDITA, "%i %%"),        USER_BITS);
    ps[i++] = PARAMETER(&p->progressive_heating_time,    0,     3600,     0,     FFINT(PARS_DESCRIPTIONS_RISCALDAMENTO_PROGRESSIVO, fmt_min),        USER_BITS);
    // clang-format on

    assert(i <= NUM_PARAMETERS);
    parameter_check_ranges(ps, i);
}


void parlav_init_cooling(mut_model_t *model, program_cooling_parameters_t *p) {
    memset(parameters, 0, sizeof(parameters));
    (void)model;
    parameter_handle_t *ps = parameters;
    uint16_t            i  = 0;

    // clang-format off
    ps[i++] = PARAMETER(&p->duration,                          1,     99,    2,    FFINT(PARS_DESCRIPTIONS_DURATA, fmt_min),          USER_BITS);
    ps[i++] = PARAMETER(&p->enable_reverse,    0,     1,     0,     FOPT(PARS_DESCRIPTIONS_ABILITA_INVERSIONE, pars_nosi),        USER_BITS);
    ps[i++] = PARAMETER(&p->rotation_time,    1,     99,     35,     FTIME(PARS_DESCRIPTIONS_TEMPO_DI_ROTAZIONE),        USER_BITS);
    ps[i++] = PARAMETER(&p->pause_time,    5,     99,     6,     FTIME(PARS_DESCRIPTIONS_TEMPO_DI_PAUSA),        USER_BITS);
    // clang-format on

    assert(i <= NUM_PARAMETERS);
    parameter_check_ranges(ps, i);
}


void parlav_init_antifold(mut_model_t *model, program_antifold_parameters_t *p) {
    memset(parameters, 0, sizeof(parameters));
    parameter_handle_t *ps = parameters;
    uint16_t            i  = 0;

    // clang-format off
    ps[i++] = PARAMETER(&p->max_duration,                          0,     250,    0,    FFINT(PARS_DESCRIPTIONS_DURATA_MASSIMA, fmt_min),          USER_BITS);
    ps[i++] = PARAMETER(&p->max_cycles,                          0,     20,    2,    FINT(PARS_DESCRIPTIONS_CICLI_MASSIMI),          USER_BITS);
    ps[i++] = PARAMETER(&p->speed,    model->config.parmac.minimum_speed,     model->config.parmac.maximum_speed,     15,     FFINT(PARS_DESCRIPTIONS_VELOCITA, "%i rpm"),        USER_BITS);
    ps[i++] = PARAMETER(&p->rotation_time,    1,     99,     4,     FTIME(PARS_DESCRIPTIONS_TEMPO_DI_ROTAZIONE),        USER_BITS);
    ps[i++] = PARAMETER(&p->pause_time,    0,     99,     1,     FTIME(PARS_DESCRIPTIONS_TEMPO_DI_PAUSA),        USER_BITS);
    ps[i++] = PARAMETER(&p->start_delay,                          0,     250,    5,    FFINT(PARS_DESCRIPTIONS_TEMPO_DI_RITARDO, fmt_min),          USER_BITS);
    // clang-format on

    assert(i <= NUM_PARAMETERS);
    parameter_check_ranges(ps, i);
}


void parlav_operation(model_t *model, size_t parameter, int op, uint8_t al) {
    parameter_operator(get_actual_parameter(model, parameter, al), op);
}


const char *parlav_get_description(model_t *model, size_t parameter, uint8_t al) {
    parameter_user_data_t *data = parameter_get_user_data(get_actual_parameter(model, parameter, al));

    return data->descrizione[model->config.parmac.language];
}


void parlav_format_value(model_t *model, char *string, size_t parameter, uint8_t al) {
    parameter_handle_t    *par  = get_actual_parameter(model, parameter, al);
    parameter_user_data_t *data = parameter_get_user_data(par);

    data->format(string, model->config.parmac.language, model, par);
}


size_t parlav_get_tot_parameters(uint8_t al) {
    return parameter_get_count(parameters, NUM_PARAMETERS, model_get_bit_accesso(al));
}


static parameter_handle_t *get_actual_parameter(model_t *model, size_t parameter, uint8_t al) {
    (void)model;
    parameter_handle_t *par = parameter_get_handle(parameters, NUM_PARAMETERS, parameter, model_get_bit_accesso(al));
    assert(par != NULL);
    return par;
}
