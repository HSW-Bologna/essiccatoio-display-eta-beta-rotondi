#ifndef AUTOGEN_FILE_PARS_H_INCLUDED
#define AUTOGEN_FILE_PARS_H_INCLUDED

typedef enum {
    PARS_NOSI_NO = 0,
    PARS_NOSI_SI,
} pars_nosi_t;

extern const char *pars_nosi[2][2];
typedef enum {
    PARS_DESCRIPTIONS_LINGUA = 0,
    PARS_DESCRIPTIONS_LIVELLO_ACCESSO,
    PARS_DESCRIPTIONS_VISUALIZZARE_TEMPERATURA,
    PARS_DESCRIPTIONS_TASTO_MENU,
    PARS_DESCRIPTIONS_TEMPO_TASTO_PAUSA,
    PARS_DESCRIPTIONS_TEMPO_TASTO_STOP,
    PARS_DESCRIPTIONS_TEMPO_STOP_AUTOMATICO,
} pars_descriptions_t;

extern const char *pars_descriptions[7][2];
typedef enum {
    PARS_LINGUE_ITALIANO = 0,
    PARS_LINGUE_INGLESE,
    PARS_LINGUE_SPAGNOLO,
    PARS_LINGUE_FRANCESE,
    PARS_LINGUE_TEDESCO,
} pars_lingue_t;

extern const char *pars_lingue[5][5];

#endif
