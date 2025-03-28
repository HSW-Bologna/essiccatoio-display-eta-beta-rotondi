#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "parameter.h"
#include "descriptions/parstring.h"
#include "model.h"
#include "parmac.h"
#include "descriptions/AUTOGEN_FILE_pars.h"
#include "esp_log.h"


#define NUM_PARAMETERS 106


enum {
    LIVELLO_ACCESSO_ESTESI  = 0,
    LIVELLO_ACCESSO_RIDOTTI = 1,
    NUM_LIVELLI_ACCESSO,
};

static parameter_handle_t *get_actual_parameter(model_t *pmodel, size_t parameter, uint8_t al);


static parameter_handle_t parameters[NUM_PARAMETERS];
static const char        *TAG = __FILE_NAME__;


void parmac_init(mut_model_t *model, int reset) {
    size_t              i  = 0;
    parmac_t           *p  = &model->config.parmac;
    parameter_handle_t *ps = parameters;

    const char *fmt_c   = "%i °C";
    const char *fmt_rpm = "%i rpm";
    char     ***heating_type_strings =
        (char ***)(model_is_steam_model(model) ? pars_tipo_riscaldamento_vapore : pars_tipo_riscaldamento);

    // clang-format off
    ps[i++] = PARAMETER(&p->language,                                 0,                              LANGUAGES_NUM - 1,          0,                              FOPT(PARS_DESCRIPTIONS_LINGUA, pars_lingue),                                                      USER_BITS);
    ps[i++] = PARAMETER(&p->max_user_language,                        0,                              LANGUAGES_NUM - 1,          0,                              FOPT(PARS_DESCRIPTIONS_LINGUA_MASSIMA_UTENTE, pars_lingue),                                       TECH_BITS);
    ps[i++] = PARAMETER(&p->logo,                                     0,                              NUM_LOGOS - 1,              0,                              FOPT(PARS_DESCRIPTIONS_LOGO, pars_loghi),                                                         TECH_BITS);
    ps[i++] = PARAMETER(&p->abilita_visualizzazione_temperatura,      0,                              1,                          0,                              FOPT(PARS_DESCRIPTIONS_VISUALIZZARE_TEMPERATURA, pars_nosi),                                      USER_BITS);
    ps[i++] = PARAMETER(&p->abilita_tasto_menu,                       0,                              1,                          0,                              FOPT(PARS_DESCRIPTIONS_TASTO_MENU, pars_nosi),                                                    TECH_BITS);
    ps[i++] = PARAMETER(&p->access_level,                             0,                              1,                          0,                              FOPT(PARS_DESCRIPTIONS_LIVELLO_ACCESSO, pars_livello_accesso),                                    TECH_BITS);
    ps[i++] = PARAMETER(&p->autostart,                                0,                              1,                          0,                              FOPT(PARS_DESCRIPTIONS_AUTOAVVIO, pars_nosi),                                                     TECH_BITS);
    ps[i++] = PARAMETER(&p->heating_type,                             0,                              1,                          0,                              FOPT(PARS_DESCRIPTIONS_TIPO_RISCALDAMENTO, heating_type_strings),                              TECH_BITS);
    ps[i++] = PARAMETER(&p->gas_ignition_attempts,                    0,                              9,                          1,                              FFINT(PARS_DESCRIPTIONS_TENTATIVI_ACCENSIONE_GAS, "%i"),                                          TECH_BITS);
    ps[i++] = PARAMETER(&p->reset_page_time,                          10,                             250,                        30,                             FTIME(PARS_DESCRIPTIONS_TEMPO_DI_RITORNO_PAGINA_INIZIALE),                                        TECH_BITS);
    ps[i++] = PARAMETER(&p->reset_language_time,                      1,                              250,                        5,                              FTIME(PARS_DESCRIPTIONS_TEMPO_DI_RITORNO_LINGUA_INIZIALE),                                        TECH_BITS);
    ps[i++] = PARAMETER(&p->pause_button_time,                        1,                              60,                         1,                              FTIME(PARS_DESCRIPTIONS_TEMPO_TASTO_PAUSA),                                                       USER_BITS);
    ps[i++] = PARAMETER(&p->stop_button_time,                         1,                              60,                         2,                              FTIME(PARS_DESCRIPTIONS_TEMPO_TASTO_STOP),                                                        USER_BITS);
    ps[i++] = PARAMETER(&p->temperature_probe,                        0,                              2,                          1,                              FOPT(PARS_DESCRIPTIONS_SONDA_TEMPERATURA, pars_sonda_temperatura),                                TECH_BITS);
    ps[i++] = PARAMETER(&p->max_input_temperature,                    1,                              125,                        115,                            FFINT(PARS_DESCRIPTIONS_TEMPERATURA_MASSIMA_IN_INGRESSO, fmt_c),                                  TECH_BITS);
    ps[i++] = PARAMETER(&p->safety_input_temperature,                 1,                              145,                        135,                            FFINT(PARS_DESCRIPTIONS_TEMPERATURA_DI_SICUREZZA_IN_INGRESSO, fmt_c),                             TECH_BITS);
    ps[i++] = PARAMETER(&p->max_output_temperature,                   1,                              85,                         85,                             FFINT(PARS_DESCRIPTIONS_TEMPERATURA_MASSIMA_IN_USCITA, fmt_c),                                    TECH_BITS);
    ps[i++] = PARAMETER(&p->safety_output_temperature,                1,                              90,                         90,                             FFINT(PARS_DESCRIPTIONS_TEMPERATURA_DI_SICUREZZA_IN_USCITA, fmt_c),                               TECH_BITS);
    ps[i++] = PARAMETER(&p->air_flow_alarm_time,                      1,                              250,                        10,                             FTIME(PARS_DESCRIPTIONS_TEMPO_ALLARME_FLUSSO_ARIA),                                               TECH_BITS);
    ps[i++] = PARAMETER(&p->minimum_speed,                            10,                             25,                         15,                             FFINT(PARS_DESCRIPTIONS_VELOCITA_MINIMA, fmt_rpm),                                                TECH_BITS);
    ps[i++] = PARAMETER(&p->maximum_speed,                            25,                             70,                         60,                             FFINT(PARS_DESCRIPTIONS_VELOCITA_MASSIMA, fmt_rpm),                                               TECH_BITS);
    if (model_is_self_service(model) || model_is_tech_level(model)) {
    ps[i++] = PARAMETER(&p->payment_type,                             0,                              3,                          1,                              FOPT(PARS_DESCRIPTIONS_TIPO_PAGAMENTO, pars_tipo_pagamento),                                      USER_BITS);
    ps[i++] = PARAMETER(&p->minimum_coins,                            0,                              10,                         1,                              FINT(PARS_DESCRIPTIONS_NUMERO_MINIMO_DI_GETTONI),                                                 TECH_BITS);
    ps[i++] = PARAMETER(&p->time_per_coin,                            1,                              3600,                       60,                             FTIME(PARS_DESCRIPTIONS_TEMPO_PER_GETTONE),                                                       USER_BITS);
    ps[i++] = PARAMETER(&p->credit_request_type,                      0,                              2,                          0,                              FOPT(PARS_DESCRIPTIONS_TIPO_DI_RICHIESTA_PAGAMENTO, pars_tipo_richiesta_pagamento),               USER_BITS);
    }
    ps[i++] = PARAMETER(&p->stop_time_in_pause,                       0,                              1,                          0,                              FOPT(PARS_DESCRIPTIONS_FERMA_TEMPO_IN_PAUSA, pars_nosi),                                          USER_BITS);
    ps[i++] = PARAMETER(&p->cycle_reset_time,                         0,                              3600,                       60,                             FTIME(PARS_DESCRIPTIONS_TEMPO_AZZERAMENTO_CICLO),                                                 USER_BITS);
    ps[i++] = PARAMETER(&p->busy_signal_nc_na,                        0,                              1,                          1,                              FOPT(PARS_DESCRIPTIONS_DIREZIONE_CONTATTO_MACCHINA_OCCUPATA, pars_nc_na),                         USER_BITS);
    ps[i++] = PARAMETER(&p->tipo_macchina_occupata,                   0,                              2,                          0,                              FOPT(PARS_DESCRIPTIONS_TIPO_MACCHINA_OCCUPATA, pars_tipo_macchina_occupata),                      USER_BITS);
    ps[i++] = PARAMETER(&p->tempo_allarme_temperatura,                60,                             99*60+59,                   45*60,                          FTIME(PARS_DESCRIPTIONS_TEMPO_ALLARME_TEMPERATURA),                                               TECH_BITS);
    ps[i++] = PARAMETER(&p->allarme_inverter_off_on,                  0,                              1,                          1,                              FOPT(PARS_DESCRIPTIONS_DIREZIONE_ALLARME_INVERTER, pars_nc_na),                                   TECH_BITS);
    ps[i++] = PARAMETER(&p->allarme_filtro_off_on,                    0,                              1,                          1,                              FOPT(PARS_DESCRIPTIONS_DIREZIONE_ALLARME_FILTRO, pars_nc_na),                                     TECH_BITS);
    ps[i++] = PARAMETER(&p->emergency_alarm_nc_na,                    0,                              1,                          1,                              FOPT(PARS_DESCRIPTIONS_DIREZIONE_ALLARME_EMERGENZA, pars_nc_na),                                  TECH_BITS);
    ps[i++] = PARAMETER(&p->fan_with_open_porthole_time,              0,                              240,                        3,                              FTIME(PARS_DESCRIPTIONS_TEMPO_VENTILAZIONE_OBLO_APERTO),                                          TECH_BITS);
    ps[i++] = PARAMETER(&p->disabilita_allarmi,                       0,                              1,                          0,                              FOPT(PARS_DESCRIPTIONS_DISABILITA_ALLARMI, pars_nosi),                                            TECH_BITS);
    ps[i++] = PARAMETER(&p->porthole_nc_na,                           0,                              1,                          1,                              FOPT(PARS_DESCRIPTIONS_DIREZIONE_CONTATTO_OBLO, pars_nc_na),                                      TECH_BITS);
    // clang-format on

    if (reset) {
        parameter_reset_to_defaults(parameters, i);
    } else {
        parameter_check_ranges(parameters, i);
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
