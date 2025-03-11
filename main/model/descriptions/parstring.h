#ifndef PARSTRING_H_INCLUDED
#define PARSTRING_H_INCLUDED


#include <stdint.h>


typedef struct __attribute__((__packed__)) {
    const char **descrizione;
    void (*format)(char *string, uint16_t, const void *, const void *);
    const char   *fmt;
    const char ***valori;
} parameter_user_data_t;


#define FDS(i)                                                                                                         \
    parameter_user_data_new((parameter_user_data_t){pars_descriptions[i], formatta_decimi_secondo, NULL, NULL})
#define FTIME(i)      parameter_user_data_new((parameter_user_data_t){pars_descriptions[i], formatta_ms, NULL, NULL})
#define FFINT(i, fmt) parameter_user_data_new((parameter_user_data_t){pars_descriptions[i], formatta_int, fmt, NULL})
#define FINT(i)       parameter_user_data_new((parameter_user_data_t){pars_descriptions[i], formatta_int, NULL, NULL})
#define FOPT(i, vals)                                                                                                  \
    parameter_user_data_new((parameter_user_data_t){pars_descriptions[i], formatta_opt, NULL, (const char ***)(vals)})
#define FNULL     NULL


void *parameter_user_data_new(parameter_user_data_t udata);
void  formatta_int(char *string, uint16_t language, const void *arg1, const void *arg2);
void  formatta_opt(char *string, uint16_t language, const void *arg1, const void *arg2);
void  formatta_ms(char *string, uint16_t language, const void *arg1, const void *arg2);
void  formatta_decimi_secondo(char *string, uint16_t language, const void *arg1, const void *arg2);


#endif
