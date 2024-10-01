#include <assert.h>
#include <math.h>
#include "parstring.h"
#include "parameter.h"
#include <stdio.h>
#include <string.h>
#include "model/model.h"


void *parameter_user_data_new(parameter_user_data_t udata) {
    void *result = malloc(sizeof(udata));
    assert(result);
    memcpy(result, &udata, sizeof(udata));
    return result;
}


void formatta_int(char *string, uint16_t language, const void *arg1, const void *arg2) {
    (void)language;
    (void)arg1;
    const parameter_handle_t    *par   = arg2;
    const parameter_user_data_t *udata = parameter_get_user_data((parameter_handle_t *)par);

    switch (par->type) {
        case PARAMETER_TYPE_UINT16:
            if (udata->fmt != NULL) {
                sprintf(string, udata->fmt, *(uint16_t *)par->pointer);
            } else {
                sprintf(string, "%i", *(uint16_t *)par->pointer);
            }
            break;
        default:
            sprintf(string, "Errore!");
            break;
    }
}


void formatta_opt(char *string, uint16_t language, const void *arg1, const void *arg2) {
    (void)arg1;
    const parameter_handle_t    *par   = arg2;
    const parameter_user_data_t *udata = parameter_get_user_data((parameter_handle_t *)par);
    size_t                       value = parameter_to_index((parameter_handle_t *)par);
    char *(*values)[LANGUAGES_NUM]     = (char *(*)[LANGUAGES_NUM])udata->valori;
    strcpy(string, values[value][language]);
}


void formatta_ms(char *string, uint16_t language, const void *arg1, const void *arg2) {
    (void)arg1;
    (void)language;
    const parameter_handle_t *par = arg2;

    switch (par->type) {
        case PARAMETER_TYPE_UINT16: {
            uint16_t value = *(uint16_t *)par->pointer;
            sprintf(string, "%02im:%02is", value / 60, value % 60);
            break;
        }

        default:
            sprintf(string, "Errore!");
            break;
    }
}


void formatta_decimi_secondo(char *string, uint16_t language, const void *arg1, const void *arg2) {
    (void)language;
    (void)arg1;
    const parameter_handle_t *par = arg2;

    switch (par->type) {
        case PARAMETER_TYPE_UINT16: {
            uint16_t value    = *(uint16_t *)par->pointer;
            float    dotvalue = ((float)value) / 10;
            sprintf(string, "%.02f", dotvalue);
            break;
        }

        default:
            sprintf(string, "Errore!");
            break;
    }
}
