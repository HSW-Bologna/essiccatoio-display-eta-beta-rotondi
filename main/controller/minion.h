#ifndef MINION_H_INCLUDED
#define MINION_H_INCLUDED


#include <stdint.h>
#include "model/model.h"


typedef enum {
    MINION_RESPONSE_TAG_ERROR,
    MINION_RESPONSE_TAG_SYNC,
    MINION_RESPONSE_TAG_HANDSHAKE,
} minion_response_tag_t;


typedef struct {
    minion_response_tag_t tag;
    union {
        struct {
            uint8_t  firmware_version_major;
            uint8_t  firmware_version_minor;
            uint8_t  firmware_version_patch;
            uint8_t  heating;
            uint16_t inputs;
            uint16_t temperature_1_adc;
            uint16_t temperature_1;
            uint16_t temperature_2_adc;
            uint16_t temperature_2;
            uint16_t pressure_adc;
            uint16_t temperature_probe;
            uint16_t humidity_probe;
            uint16_t pressure;
            uint16_t payment;
            uint16_t coins[DIGITAL_COIN_LINES_NUM];

            cycle_state_t cycle_state;
            int16_t       default_temperature;
            uint16_t      remaining_time_seconds;
            uint16_t      alarms;
        } sync;

        struct {
            cycle_state_t cycle_state;
            uint16_t      program_index;
            uint16_t      step_index;
        } handshake;
    } as;
} minion_response_t;


void    minion_init(void);
void    minion_read_state(model_t *model);
void    minion_sync(model_t *model);
uint8_t minion_get_response(minion_response_t *response);
void    minion_retry_communication(void);
void    minion_pause_program(void);
void    minion_program_done(model_t *model);
void    minion_resume_program(model_t *model, uint8_t clear_alarms);
void    minion_handshake(void);
void    minion_clear_coins(void);


#endif
