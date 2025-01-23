#ifndef MODEL_PARLAV_H_INCLUDED
#define MODEL_PARLAV_H_INCLUDED


#include <stdlib.h>
#include <stdint.h>
#include "model.h"


size_t      parlav_get_tot_parameters(uint8_t al);
void        parlav_operation(model_t *model, size_t parameter, int op, uint8_t al);
const char *parlav_get_description(model_t *model, size_t parameter, uint8_t al);
void        parlav_format_value(model_t *model, char *string, size_t parameter, uint8_t al);
void        parlav_init_drying(mut_model_t *model, program_drying_parameters_t *p);
void        parlav_init_cooling(mut_model_t *model, program_cooling_parameters_t *p);
void        parlav_init_antifold(mut_model_t *model, program_antifold_parameters_t *p);


#endif
