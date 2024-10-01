#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "parameter.h"
#include "descriptions/parstring.h"
#include "model.h"
#include "parmac.h"
#include "descriptions/AUTOGEN_FILE_pars.h"


#define NUM_PARAMETERS 107
#define USER_BITS      USER_ACCESS_LEVEL
#define TECH_BITS      0x02


enum {
    LIVELLO_ACCESSO_ESTESI  = 0,
    LIVELLO_ACCESSO_RIDOTTI = 1,
    NUM_LIVELLI_ACCESSO,
};

parameter_handle_t parameters[NUM_PARAMETERS];

static parameter_handle_t *get_actual_parameter(model_t *pmodel, size_t parameter, uint8_t al);


void parmac_init(mut_model_t *model, int reset) {
    size_t              i  = 0;
    parmac_t           *p  = &model->config.parmac;
    parameter_handle_t *ps = parameters;

    char *fmt_dec_sec = "%i dsec";
    char *fmt_sec     = "%i s";
    char *fmt_min     = "%i min";
    char *fmt_cm      = "%i cm";
    char *fmt_perc    = "%i %%";
    char *fmt_lt      = "%i lt";
    char *fmt_C       = "%i C";
    char *fmt_rpm     = "%i rpm";

    // clang-format off
    ps[i++] = PARAMETER(&p->language,                                 0,                              LANGUAGES_NUM - 1,          0,                              FOPT(PARS_DESCRIPTIONS_LINGUA, pars_lingue),                               USER_BITS);
    ps[i++] = PARAMETER(&p->abilita_visualizzazione_temperatura,      0,                              1,                          0,                              FOPT(PARS_DESCRIPTIONS_VISUALIZZARE_TEMPERATURA, pars_nosi),               USER_BITS);
    ps[i++] = PARAMETER(&p->abilita_tasto_menu,                       0,                              1,                          0,                              FOPT(PARS_DESCRIPTIONS_TASTO_MENU, pars_nosi),                             USER_BITS);
    //ps[i++] = PARAMETER(&p->tempo_pressione_tasto_pausa,              0,                              10,                         0,                              FTIME(PARS_DESCRIPTIONS_TEMPO_TASTO_PAUSA),                                USER_BITS);
    //ps[i++] = PARAMETER(&p->tempo_pressione_tasto_stop,               0,                              10,                         5,                              FTIME(PARS_DESCRIPTIONS_TEMPO_TASTO_STOP),                                 USER_BITS);
    ps[i++] = PARAMETER(&p->tempo_stop_automatico,                    0,                              10,                         0,                              FTIME(PARS_DESCRIPTIONS_TEMPO_STOP_AUTOMATICO),                            USER_BITS);

    // clang-format on


#if 0
    // clang-format on
    
    ps[i++] = PARAMETER(&p->stop_tempo_ciclo,                         0,                              1,                          1,                              ((pudata_t){.t = PTYPE_SWITCH, .desc = DESC[PARAMETERS_DESC_FERMA_TEMPO], .values = (const char***)parameters_abilitazione}),                               USER_BITS);
    ps[i++] = PARAMETER(&p->tempo_attesa_partenza_ciclo,              0,                              600,                        0,                              ((pudata_t){.t = PTYPE_TIME, .desc = DESC[PARAMETERS_DESC_TEMPO_ATTESA_PARTENZA], .fmt = msfmt, .min_sec = 1}),                                             USER_BITS);
    ps[i++] = PARAMETER(&p->tipo_sonda_temperatura,                   SONDA_TEMPERATURA_PTC_1,        SONDA_TEMPERATURA_SHT,      SONDA_TEMPERATURA_PTC_1,        ((pudata_t){.t = PTYPE_DROPDOWN, .desc = DESC[PARAMETERS_DESC_TIPO_SONDA_TEMPERATURA], .values = (const char***)parameters_sonde_temperatura}),             USER_BITS);
    ps[i++] = PARAMETER(&p->posizione_sonda_temperatura,              POSIZIONE_SONDA_INGRESSO,       POSIZIONE_SONDA_USCITA,     POSIZIONE_SONDA_INGRESSO,       ((pudata_t){.t = PTYPE_DROPDOWN, .desc = DESC[PARAMETERS_DESC_POSIZIONE_SONDA_TEMPERATURA], .values = (const char***)parameters_posizione}),                USER_BITS);
    ps[i++] = PARAMETER(&p->temperatura_massima_ingresso,             0,                              MAX_TEMPERATURE,            0,                              ((pudata_t){.t = PTYPE_NUMBER, .desc = DESC[PARAMETERS_DESC_TEMPERATURA_MAX_INGRESSO], .fmt = "%i C"}),                                                      USER_BITS);
    ps[i++] = PARAMETER(&p->temperatura_massima_uscita,               0,                              MAX_TEMPERATURE,            0,                              ((pudata_t){.t = PTYPE_NUMBER, .desc = DESC[PARAMETERS_DESC_TEMPERATURA_MAX_USCITA], .fmt = "%i C"}),                                                       USER_BITS);
    ps[i++] = PARAMETER(&p->abilita_espansione_rs485,                 0,                              1,                          0,                              ((pudata_t){.t = PTYPE_SWITCH, .desc = DESC[PARAMETERS_DESC_ABILITA_EXP_RS485], .values = (const char***)parameters_abilitazione}),                         USER_BITS);
    ps[i++] = PARAMETER(&p->abilita_gas,                              0,                              1,                          0,                              ((pudata_t){.t = PTYPE_SWITCH, .desc = DESC[PARAMETERS_DESC_ABILITA_GAS], .values = (const char***)parameters_abilitazione}),                               USER_BITS);
    ps[i++] = PARAMETER(&p->velocita_minima,                          0,                              MAX_SPEED,                  0,                              ((pudata_t){.t = PTYPE_NUMBER, .desc = DESC[PARAMETERS_DESC_VELOCITA_MINIMA], .fmt = "%i RPM"}),                                                            USER_BITS);
    ps[i++] = PARAMETER(&p->velocita_massima,                         0,                              MAX_SPEED,                  0,                              ((pudata_t){.t = PTYPE_NUMBER, .desc = DESC[PARAMETERS_DESC_VELOCITA_MASSIMA], .fmt = "%i RPM"}),                                                           USER_BITS);
    ps[i++] = PARAMETER(&p->tempo_gettone,                            5,                              300,                        30,                             ((pudata_t){.t = PTYPE_TIME, .desc = DESC[PARAMETERS_DESC_TEMPO_GETTONE], .fmt = msfmt, .min_sec = 1}),                                                     USER_BITS);
    ps[i++] = PARAMETER(&p->max_programs,                             1,                              MAX_PROGRAMS,               (MAX_PROGRAMS/2),               ((pudata_t){.t = PTYPE_NUMBER, .desc=DESC[PARAMETERS_DESC_MAX_PROGRAMMI]}),                                                                                 USER_BITS);
    ps[i++] = PARAMETER(&p->temperatura_sicurezza_in,                 0,                              100,                        50,                            ((pudata_t){.t = PTYPE_NUMBER, .desc=DESC[PARAMETERS_DESC_TEMPERATURA_SICUREZZA_IN], .fmt = "%i C"}),                                                       USER_BITS);
    ps[i++] = PARAMETER(&p->temperatura_sicurezza_out,                0,                              100,                        50,                            ((pudata_t){.t = PTYPE_NUMBER, .desc=DESC[PARAMETERS_DESC_TEMPERATURA_SICUREZZA_OUT], .fmt = "%i C"}),                                                      USER_BITS);
    ps[i++] = PARAMETER(&p->tempo_allarme_temperatura,                60,                             1200,                       300,                            ((pudata_t){.t = PTYPE_TIME, .desc = DESC[PARAMETERS_DESC_TEMPO_ALLARME_TEMPERATURA], .fmt = secfmt}),                                                      USER_BITS);
    ps[i++] = PARAMETER(&p->allarme_inverter_off_on,                  0,                              1,                          0,                              ((pudata_t){.t = PTYPE_SWITCH, .desc = DESC[PARAMETERS_DESC_ALLARME_INVERTER], .values = (const char***)parameters_abilitazione}),                          USER_BITS);
    ps[i++] = PARAMETER(&p->allarme_filtro_off_on,                    0,                              1,                          0,                              ((pudata_t){.t = PTYPE_SWITCH, .desc = DESC[PARAMETERS_DESC_ALLARME_FILTRO], .values = (const char***)parameters_abilitazione}),                          USER_BITS);
    ps[i++] = PARAMETER(&p->inverti_macchina_occupata,                0,                              1,                          0,                              ((pudata_t){.t = PTYPE_SWITCH, .desc = DESC[PARAMETERS_DESC_INVERTI_MACCHINA_OCCUPATA], .values = (const char***)parameters_abilitazione}),                 USER_BITS);
    ps[i++] = PARAMETER(&p->tipo_macchina_occupata,                   0,                              2,                          0,                              ((pudata_t){.t = PTYPE_DROPDOWN, .desc = DESC[PARAMETERS_DESC_TIPO_MACCHINA_OCCUPATA], .values = (const char***)parameters_tipo_macchina_occupata}),        USER_BITS);
    ps[i++] = PARAMETER(&p->tipo_riscaldamento,                       TIPO_RISCALDAMENTO_GAS,         TIPO_RISCALDAMENTO_VAPORE,  TIPO_RISCALDAMENTO_ELETTRICO,   ((pudata_t){.t = PTYPE_DROPDOWN, .desc = DESC[PARAMETERS_DESC_TIPO_RISCALDAMENTO], .values = (const char***)parameters_riscaldamento}),                     USER_BITS);
    ps[i++] = PARAMETER(&p->access_level,                             0,                              1,                          0,                              ((pudata_t){.t = PTYPE_DROPDOWN, .desc = DESC[PARAMETERS_DESC_LIVELLO_ACCESSO], .values = (const char***)parameters_livello_accesso}),                      TECH_BITS);
    ps[i++] = PARAMETER(&p->autoavvio,                                0,                              1,                          0,                              ((pudata_t){.t = PTYPE_SWITCH, .desc = DESC[PARAMETERS_DESC_AUTOAVVIO], .values = (const char***)parameters_abilitazione}),                                 USER_BITS);
    ps[i++] = PARAMETER(&p->abilita_allarmi,                          0,                              1,                          1,                              ((pudata_t){.t = PTYPE_SWITCH, .desc = DESC[PARAMETERS_DESC_GESTIONE_ALLARMI], .values = (const char***)parameters_abilitazione}),                          TECH_BITS);
#endif

    parameter_check_ranges(parameters, i);
    if (reset) {
        parameter_reset_to_defaults(parameters, i);
    }
}

void parmac_operation(model_t *pmodel, size_t parameter, int op, uint8_t al) {
    parameter_operator(get_actual_parameter(pmodel, parameter, al), op);
}


const char *parmac_get_description(model_t *pmodel, size_t parameter, uint8_t al) {
    parameter_user_data_t *data = parameter_get_user_data(get_actual_parameter(pmodel, parameter, al));

    return data->descrizione[pmodel->config.parmac.language];
}

void parmac_format_value(model_t *model, char *string, size_t parameter, uint8_t al) {
    parameter_handle_t    *par  = get_actual_parameter(model, parameter, al);
    parameter_user_data_t *data = parameter_get_user_data(par);

    data->format(string, model->config.parmac.language, model, par);
}

size_t parmac_get_tot_parameters(uint8_t al) {
    return parameter_get_count(parameters, NUM_PARAMETERS, model_get_bit_accesso(al));
}



static parameter_handle_t *get_actual_parameter(model_t *model, size_t parameter, uint8_t al) {
    (void)model;
    parameter_handle_t *par = parameter_get_handle(parameters, NUM_PARAMETERS, parameter, model_get_bit_accesso(al));
    assert(par != NULL);
    return par;
}


const char *parmac_commissioning_language_get_description(model_t *pmodel) {
    parameter_user_data_t *data = parameter_get_user_data(&parameters[PARMAC_COMMISSIONING_LINGUA]);
    return data->descrizione[pmodel->config.parmac.language];
}



void parmac_commissioning_operation(model_t *pmodel, parmac_commissioning_t parameter, int op) {
    parameter_operator(&parameters[parameter], op);
}
