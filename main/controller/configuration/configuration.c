#include <assert.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <limits.h>
#include "bsp/storage.h"
#include "configuration.h"
#include "esp_log.h"
#include "model/model.h"
#include "model/parmac.h"
#include "model/program.h"
// #include "model/parlav.h"
#include "config/app_config.h"
#include "microtar.h"
#include "services/timestamp.h"


#define BASENAME(x) (strrchr(x, '/') + 1)


#define DIR_CHECK(x)                                                                                                   \
    {                                                                                                                  \
        int res = x;                                                                                                   \
        if (res < 0)                                                                                                   \
            ESP_LOGE(TAG, "Errore nel maneggiare una cartella: %s", strerror(errno));                                  \
    }


static int load_parmac(parmac_t *parmac);


static const char *TAG                               = "Configuration";
static const char *CONFIGURATION_KEY_COMMISSIONED    = "COMMISSIONED";
static const char *CONFIGURATION_KEY_PASSWORD        = "PASSWORD";
static const char *CONFIGURATION_KEY_PRESSURE_OFFSET = "POFFSET";

/*
 * Funzioni di utilita'
 */

static int count_occurrences(const char *str, char c) {
    int i = 0;
    for (i = 0; str[i]; str[i] == c ? i++ : *str++)
        ;
    return i;
}


static char *nth_strrchr(const char *str, char c, int n) {
    const char *s = &str[strlen(str) - 1];

    while ((n -= (*s == c)) && (s != str))
        s--;

    return (char *)s;
}

static int is_dir(const char *path) {
    struct stat path_stat;
    if (stat(path, &path_stat) < 0)
        return 0;
    return S_ISDIR(path_stat.st_mode);
}

static int is_file(const char *path) {
    struct stat path_stat;
    if (stat(path, &path_stat) < 0)
        return 0;
    return S_ISREG(path_stat.st_mode);
}


static int dir_exists(char *name) {
    DIR *dir = opendir(name);
    if (dir) {
        closedir(dir);
        return 1;
    }
    return 0;
}

static void create_dir(char *name) {
    DIR_CHECK(mkdir(name, 0766));
}


void configuration_commissioned(uint8_t commissioned) {
    storage_save_uint8(&commissioned, CONFIGURATION_KEY_COMMISSIONED);
}


int configuration_copy_from_tar(mtar_t *tar, const char *name, size_t total) {
    assert(tar != NULL);
    char to[128] = {0};
    snprintf(to, sizeof(to), "%s/%s", DATA_PATH, name);

    int  fd_to;
    char buf[1024];
    fd_to = open(to, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd_to < 0) {
        ESP_LOGE(TAG, "Non sono riuscito ad aprire %s: %s", to, strerror(errno));
        return -1;
    }


    while (total > 0) {
        size_t nread = total < sizeof(buf) ? total : sizeof(buf);
        if (mtar_read_data(tar, buf, nread) != MTAR_ESUCCESS) {
            ESP_LOGE(TAG, "Errore nella lettura dell'archivio!");
            close(fd_to);
            return -1;
        }
        char   *out_ptr = buf;
        ssize_t nwritten;

        total -= nread;

        do {
            nwritten = write(fd_to, out_ptr, nread);

            if (nwritten >= 0) {
                ESP_LOGE(TAG, "Errore nella scrittura di %s: %s", to, strerror(errno));
                nread -= nwritten;
                out_ptr += nwritten;
            } else {
                close(fd_to);
                return -1;
            }
        } while (nread > 0);
    }

    close(fd_to);

    return 0;
}


/*
 *  Inizializzazione
 */

void configuration_init(mut_model_t *model) {
    (void)is_dir;
    (void)nth_strrchr;
    (void)count_occurrences;

    storage_load_str(model->config.password, MAX_NAME_LENGTH, (char *)CONFIGURATION_KEY_PASSWORD);
    storage_load_uint16(&model->config.pressure_offset, CONFIGURATION_KEY_PRESSURE_OFFSET);

    if (!dir_exists(DATA_PATH)) {
        create_dir(DATA_PATH);
    }
    if (!dir_exists(PROGRAMS_PATH)) {
        create_dir(PROGRAMS_PATH);
    }
    if (!dir_exists(PARAMS_PATH)) {
        create_dir(PARAMS_PATH);
    }
    if (!is_file(PATH_FILE_DATA_VERSION)) {
        configuration_save_data_version();
    }
}


void configuration_save_password(const char *password) {
    storage_save_str(password, CONFIGURATION_KEY_PASSWORD);
}


void configuration_save_pressure_offset(uint16_t pressure_offset) {
    storage_save_uint16(&pressure_offset, CONFIGURATION_KEY_PRESSURE_OFFSET);
}


/*
 *  Programmi
 */

int configuration_clone_program(mut_model_t *model, uint16_t source, uint16_t destination) {
    int res = 0;

    char   path[128] = {0};
    name_t filename;

    snprintf(path, sizeof(path), "%s/%s", PROGRAMS_PATH, model_new_unique_filename(model, filename, timestamp_get()));
    ESP_LOGI(TAG, "Cloning new program %s (from %i to %i)", path, model->config.num_programs, (int)destination);

    uint8_t *buffer = malloc(MAX_PROGRAM_SIZE);
    assert(buffer != NULL);
    size_t size = program_serialize(buffer, &model->config.programs[source]);
    FILE  *fp   = fopen(path, "w");
    if (fwrite(buffer, 1, size, fp) == 0) {
        res = 1;
        ESP_LOGE(TAG, "Non sono riuscito a scrivere il file %s : %s", path, strerror(errno));
    }
    fclose(fp);
    free(buffer);

    if (destination < model->config.num_programs) {
        memcpy(&model->config.programs[destination], &model->config.programs[source], sizeof(program_t));
        strcpy(model->config.programs[destination].filename, filename);
    } else {
        for (size_t i = model->config.num_programs; i > destination; i--) {
            model->config.programs[i] = model->config.programs[i - 1];
        }
        memcpy(&model->config.programs[destination], &model->config.programs[source], sizeof(program_t));
        strcpy(model->config.programs[destination].filename, filename);

        model->config.num_programs++;
    }
    configuration_update_index(model->config.programs, model->config.num_programs);
    return res;
}


int configuration_create_empty_program(model_t *model) {
    uint16_t num       = model->config.num_programs;
    char     path[128] = {0};
    name_t   filename;
    uint8_t  buffer[PROGRAM_SIZE(0)];
    int      res = 0;

    size_t size = program_serialize_empty(buffer, num);

    snprintf(path, sizeof(path), "%s/%s", PROGRAMS_PATH, model_new_unique_filename(model, filename, timestamp_get()));
    ESP_LOGI(TAG, "Creating new program %s", path);
    FILE *fp = fopen(path, "w");
    if (fwrite(buffer, 1, size, fp) == 0) {
        res = 1;
        ESP_LOGE(TAG, "Non sono riuscito a scrivere il file %s : %s", filename, strerror(errno));
    }
    fclose(fp);

    if (configuration_add_program_to_index(filename)) {
        ESP_LOGE(TAG, "Unable to add program to index");
        return -1;
    }
    return res;
}


int configuration_update_program(const program_t *p) {
    char path[128];
    int  res = 0;

    snprintf(path, sizeof(path), "%s/%s", PROGRAMS_PATH, p->filename);
    FILE *fp = fopen(path, "w");
    if (fp == NULL) {
        ESP_LOGE(TAG, "Non sono riuscito ad aprire il file %s in scrittura: %s", path, strerror(errno));
        return -1;
    }

    uint8_t *buffer = malloc(MAX_PROGRAM_SIZE);
    assert(buffer != NULL);
    size_t size = program_serialize(buffer, p);
    if (fwrite(buffer, 1, size, fp) == 0) {
        res = 1;
        ESP_LOGE(TAG, "Non sono riuscito a scrivere il file %s : %s", path, strerror(errno));
    }

    fclose(fp);
    free(buffer);
    return res;
}


int configuration_add_program_to_index(char *filename) {
    int res = 0;

    FILE *findex = fopen(PATH_FILE_INDICE, "a");

    if (!findex) {
        ESP_LOGE(TAG, "Operazione di scrittura dell'indice fallita: %s", strerror(errno));
        res = 1;
    } else {
        char buffer[64] = {0};
        snprintf(buffer, sizeof(buffer), "%s\n", filename);
        if (fwrite(buffer, 1, strlen(buffer), findex) == 0) {
            ESP_LOGE(TAG, "Errore durante la scrittura dell'indice dei programmi: %s", strerror(errno));
            res = 1;
        }
        fclose(findex);
    }

    return res;
}


void configuration_clear_orphan_programs(program_t *programs, uint16_t num) {
    struct dirent *dir;
    char           string[300];

    DIR *d = opendir(PROGRAMS_PATH);

    if (d) {
        while ((dir = readdir(d)) != NULL) {
            int orphan = 1;

            if (strcmp(dir->d_name, "index.txt") == 0) {
                continue;
            }

            for (size_t i = 0; i < num; i++) {
                if (strcmp(programs[i].filename, dir->d_name) == 0) {
                    orphan = 0;
                    break;
                }
            }

            if (orphan) {
                sprintf(string, "%s/%s", PROGRAMS_PATH, dir->d_name);
                remove(string);
            }
        }
        closedir(d);
    }
}


int list_saved_programs(program_t *programs) {
    int count = 0;

    FILE *findex = fopen(PATH_FILE_INDICE, "r");
    if (!findex) {
        ESP_LOGI(TAG, "Indice non trovato: %s", strerror(errno));
        return -1;
    } else {
        char filename[256];

        while (fgets(filename, sizeof(filename), findex) && count < MAX_PROGRAMMI) {
            int len = strlen(filename);
            if (len <= 0) {
                continue;
            }

            if (filename[len - 1] == '\n') {     // Rimuovo il newline
                filename[len - 1] = 0;
            }

            strcpy(programs[count].filename, filename);
            ESP_LOGI(TAG, "File %s in indice", programs[count].filename);
            count++;
        }

        fclose(findex);
    }

    return count;
}


void configuration_delete_all(void) {
    remove(PATH_FILE_PARMAC);
    remove(PATH_FILE_INDICE);
    configuration_clear_orphan_programs(NULL, 0);
}


void configuration_remove_program(program_t *programs, size_t len, size_t num) {
    remove(programs[num].filename);

    for (size_t i = num; i < len - 1; i++) {
        programs[i] = programs[i + 1];
    }

    configuration_update_index(programs, len);
}


int configuration_load_programs(model_t *model, program_t *programs) {
    int16_t num = list_saved_programs(programs);
    char    path[PATH_MAX];
    int     count = 0;

    if (num < 0) {
        remove(PATH_FILE_INDICE);
        num = 0;
    }
    ESP_LOGI(TAG, "%i programs found", num);

    uint8_t *buffer = malloc(MAX_PROGRAM_SIZE);
    for (int16_t i = 0; i < num; i++) {
        sprintf(path, "%s/%s", PROGRAMS_PATH, programs[i].filename);

        if (is_file(path)) {
            FILE *fp = fopen(path, "r");

            if (!fp) {
                ESP_LOGE(TAG, "Non sono riuscito ad aprire il file %s: %s", path, strerror(errno));
                continue;
            }

            size_t read = fread(buffer, 1, MAX_PROGRAM_SIZE, fp);

            if (read == 0) {
                ESP_LOGE(TAG, "Non sono riuscito a leggere il file %s: %s", path, strerror(errno));
            } else {
                program_deserialize(&programs[i], buffer);
                ESP_LOGI(TAG, "Trovato lavaggio %s (%s)", programs[i].nomi[model->config.parmac.language], path);
                count++;
            }

            fclose(fp);
        } else {
            ESP_LOGW(TAG, "Cannot find program %s", path);
        }
    }

    free(buffer);
    return count;
}


/*
 *  Parametri macchina
 */

static int load_parmac(parmac_t *parmac) {
    uint8_t buffer[PARMAC_SIZE] = {0};
    int     res                 = 0;
    FILE   *f                   = fopen(PATH_FILE_PARMAC, "r");

    if (!f) {
        ESP_LOGI(TAG, "Parametri macchina non trovati");
        return -1;
    } else {
        if ((res = fread(buffer, 1, PARMAC_SIZE, f)) == 0) {
            ESP_LOGE(TAG, "Errore nel caricamento dei parametri macchina: %s", strerror(errno));
        }
        fclose(f);

        if (res == 0) {
            return -1;
        } else {
            model_deserialize_parmac(parmac, buffer);
            return 0;
        }
    }
}


int configuration_save_parmac(parmac_t *parmac) {
    uint8_t buffer[PARMAC_SIZE] = {0};
    int     res                 = 0;

    size_t size = model_serialize_parmac(buffer, parmac);

    FILE *f = fopen(PATH_FILE_PARMAC, "w");
    if (!f) {
        ESP_LOGE(TAG, "Non riesco ad aprire %s in scrittura: %s", PATH_FILE_PARMAC, strerror(errno));
        res = 1;
    } else {
        if (fwrite(buffer, 1, size, f) == 0) {
            ESP_LOGE(TAG, "Non riesco a scrivere i parametri macchina: %s", strerror(errno));
            res = 1;
        }
        fclose(f);
    }

    return res;
}


int configuration_save_data_version(void) {
    int res = 0;

    FILE *findex = fopen(PATH_FILE_DATA_VERSION, "w");
    if (!findex) {
        ESP_LOGE(TAG, "Operazione di scrittura della versione fallita: %s", strerror(errno));
        res = -1;
    } else {
        char string[64];
        snprintf(string, sizeof(string), "%i", CONFIGURATION_COMPATIBILITY_VERSION);
        fwrite(string, 1, strlen(string), findex);
        fclose(findex);
    }

    return res;
}


int configuration_load_all_data(mut_model_t *model) {
    int err = load_parmac(&model->config.parmac);

    if (err) {
        //strcpy(model->config.parmac.nome, "");
        configuration_save_parmac(&model->config.parmac);
    }

    model->config.num_programs = configuration_load_programs(model, model->config.programs);
    // Never less than 5 programs
    model_init_default_programs(model);
    configuration_clear_orphan_programs(model->config.programs, model->config.num_programs);

    storage_load_uint8(&model->config.commissioned, CONFIGURATION_KEY_COMMISSIONED);

    return configuration_read_local_data_version();
}


int configuration_read_local_data_version(void) {
    FILE *f          = fopen(PATH_FILE_DATA_VERSION, "r");
    char  buffer[17] = {0};

    if (f) {
        if (fread(buffer, 1, 16, f) <= 0) {
            fclose(f);
            return -1;
        }
        fclose(f);

        int res = atoi(buffer);
        return res;
    } else {
        return -1;
    }
}


int configuration_update_index(program_t *programs, size_t len) {
    FILE *findex = fopen(PATH_FILE_INDICE, "w");
    if (findex == NULL) {
        ESP_LOGE(TAG, "Unable to open index: %s", strerror(errno));
        return -1;
    }

    for (size_t i = 0; i < len; i++) {
        fwrite(programs[i].filename, 1, strlen(programs[i].filename), findex);
        fwrite("\n", 1, 1, findex);
    }

    fclose(findex);
    return 0;
}
