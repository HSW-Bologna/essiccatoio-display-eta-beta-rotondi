#ifndef CONFIGURATION_H_INCLUDED
#define CONFIGURATION_H_INCLUDED


#include "model/model.h"
#include "model/program.h"
#include "microtar.h"
#include "bsp/fs_storage.h"


#define COMPATIBILITY_VERSION  1
#define BASE_PATH              LITTLEFS_PARTITION_PATH
#define DATA_PATH              BASE_PATH "/data"
#define PROGRAMS_PATH          (DATA_PATH "/programmi")
#define PARAMS_PATH            (DATA_PATH "/parametri")
#define PATH_FILE_INDICE       (DATA_PATH "/programmi/index.txt")
#define PATH_FILE_PARMAC       (DATA_PATH "/parametri/parmac.bin")
#define PATH_FILE_DATA_VERSION (DATA_PATH "/version.txt")


void configuration_init(void);
int  configuration_load_all_data(mut_model_t *pmodel);
int  configuration_save_data_version(void);
void configuration_remove_program(program_t *programs, size_t len, size_t num);
int  configuration_update_program(const program_t *p);
int  configuration_add_program_to_index(char *filename);
int  configuration_read_local_data_version(void);
int  configuration_create_empty_program(model_t *pmodel);
int  configuration_save_parmac(parmac_t *parmac);
void configuration_delete_all(void);
int  configuration_clone_program(mut_model_t *model, uint16_t source, uint16_t destination);
int  configuration_copy_from_tar(mtar_t *tar, const char *name, size_t total);
int  configuration_update_index(program_t *previews, size_t len);
int  configuration_load_programs(model_t *pmodel, program_t *programs);
void configuration_clear_orphan_programs(program_t *programs, uint16_t num);
void configuration_clear_all_programs(void);


#endif
