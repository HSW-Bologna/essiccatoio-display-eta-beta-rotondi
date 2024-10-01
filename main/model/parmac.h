/******************************************************************************/
/*                                                                            */
/*  HSW snc - Casalecchio di Reno (BO) ITALY                                  */
/*  ----------------------------------------                                  */
/*                                                                            */
/*  modulo: parmac.h                                                          */
/*                                                                            */
/*      definizione e gestione parametri macchina                             */
/*                                                                            */
/*  Autore: Maldus (Mattia MALDINI) & Virginia NEGRI & Massimo ZANNA          */
/*                                                                            */
/*  Data  : 19/07/2021      REV  : 00.0                                       */
/*                                                                            */
/*  U.mod.: 29/07/2021      REV  : 01.0                                       */
/*                                                                            */
/******************************************************************************/

#ifndef PARMAC_H_INCLUDED
#define PARMAC_H_INCLUDED

#include "model.h"


typedef enum {
    PARMAC_COMMISSIONING_LINGUA = 0,
    PARMAC_COMMISSIONING_LOGO,
    PARMAC_COMMISSIONING_MODELLO,
    _NUM_PARMAC_COMMISSIONING,
} parmac_commissioning_t;

void        parmac_init(mut_model_t *model, int reset);
void        parmac_setup_commissioning(model_t *p);
size_t      parmac_get_tot_parameters(uint8_t al);
void        parmac_format_value(model_t *pmodel, char *string, size_t parameter, uint8_t al);
const char *parmac_get_description(model_t *pmodel, size_t parameter, uint8_t al);
void        parmac_operation(model_t *pmodel, size_t parameter, int op, uint8_t al);

const char *parmac_commissioning_language_get_description(model_t *pmodel);
void        parmac_commissioning_operation(model_t *pmodel, parmac_commissioning_t parameter, int op);

#endif
