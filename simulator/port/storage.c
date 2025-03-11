#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "cJSON.h"
#include "b64.h"
#include "simulator/cJSON/cJSON.h"


#define DATABASE_FILE ".simulator_db.json"


static char database_read[10000] = {0};


static cJSON *read_database();
static void   write_database(cJSON *json);
static int    load_number(double *value, const char *key);
static int    save_number(double value, const char *key);


void storage_init(void) {}


int storage_load_double(double *value, const char *key) {
    double number = 0;
    if (load_number(&number, key)) {
        return -1;
    } else {
        *value = (double)number;
        return 0;
    }
}


void storage_save_double(double *value, const char *key) {
    save_number(*value, key);
}


int storage_load_uint8(uint8_t *value, const char *key) {
    double number = 0;
    if (load_number(&number, key)) {
        return -1;
    } else {
        *value = (uint16_t)number;
        return 0;
    }
}


void storage_save_uint8(uint8_t *value, const char *key) {
    save_number((double)*value, key);
}


int storage_load_uint16(uint16_t *value, const char *key) {
    double number = 0;
    if (load_number(&number, key)) {
        return -1;
    } else {
        *value = (uint8_t)number;
        return 0;
    }
}


void storage_save_uint16(uint16_t *value, const char *key) {
    save_number((double)*value, key);
}


int storage_load_uint32(uint32_t *value, const char *key) {
    double number = 0;
    if (load_number(&number, key)) {
        return -1;
    } else {
        *value = (uint32_t)number;
        return 0;
    }
}


void storage_save_uint32(uint32_t *value, const char *key) {
    save_number((double)*value, key);
}


int storage_load_uint64(uint64_t *value, const char *key) {
    double number = 0;
    if (load_number(&number, key)) {
        return -1;
    } else {
        *value = (uint8_t)number;
        return 0;
    }
}


void storage_save_uint64(uint64_t *value, const char *key) {
    save_number(*value, key);
}


int storage_load_str(void *value, size_t len, const char *key) {
    (void)len;
    cJSON *json    = read_database();
    cJSON *encoded = cJSON_GetObjectItemCaseSensitive(json, key);
    if (!cJSON_IsString(encoded)) {
        printf("Mi aspettavo una stringa (b64) per %s\n", key);
        cJSON_free(encoded);
        cJSON_free(json);
        return -1;
    } else {
        strcpy(value, encoded->valuestring);
        cJSON_free(encoded);
        cJSON_free(json);
        return 0;
    }
}


void storage_save_str(void *value, const char *key) {
    cJSON *json = read_database();
    cJSON_DeleteItemFromObjectCaseSensitive(json, key);
    if (cJSON_AddStringToObject(json, key, value) == NULL) {
        printf("Non sono riuscito ad aggiungere %s", key);
    } else {
        write_database(json);
    }
    cJSON_free(json);
}


int storage_load_blob(void *value, size_t len, const char *key) {
    cJSON *json    = read_database();
    cJSON *encoded = cJSON_GetObjectItemCaseSensitive(json, key);
    if (!cJSON_IsString(encoded)) {
        printf("Mi aspettavo una stringa (b64) per %s\n", key);
        cJSON_free(encoded);
        cJSON_free(json);
        return -1;
    } else {
        unsigned char *decoded = b64_decode_ex((const char *)encoded->valuestring, strlen(encoded->valuestring), NULL);
        memcpy(value, decoded, len);
        cJSON_free(encoded);
        cJSON_free(json);
        free(decoded);
        return 0;
    }
}


void storage_save_blob(void *value, size_t len, const char *key) {
    char  *encoded = b64_encode((unsigned char *)value, len);
    cJSON *json    = read_database();
    cJSON_DeleteItemFromObjectCaseSensitive(json, key);
    if (cJSON_AddStringToObject(json, key, encoded) == NULL) {
        printf("Non sono riuscito ad aggiungere %s", key);
    } else {
        write_database(json);
    }
    cJSON_free(json);
    free(encoded);
}


static cJSON *read_database() {
    FILE *f = fopen(DATABASE_FILE, "r");
    if (f == NULL) {
        printf("Database file non trovato\n");
        return cJSON_Parse("{}");
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET); /* same as rewind(f); */

    fread(database_read, 1, fsize, f);
    fclose(f);
    return cJSON_Parse(database_read);
}


static void write_database(cJSON *json) {
    FILE *f = fopen(DATABASE_FILE, "w");
    if (f == NULL) {
        printf("Non sono riuscito a scrivere il database\n");
        return;
    }

    char *string = cJSON_Print(json);
    fwrite(string, 1, strlen(string), f);
    free(string);
    fclose(f);
}


static int load_number(double *value, const char *key) {
    cJSON *json   = read_database();
    cJSON *number = cJSON_GetObjectItemCaseSensitive(json, key);
    if (!cJSON_IsNumber(number)) {
        printf("Mi aspettavo un numero per %s\n", key);
        cJSON_free(number);
        cJSON_free(json);
        return -1;
    } else {
        cJSON_free(json);
        *value = number->valuedouble;
        return 0;
    }
}

static int save_number(double value, const char *key) {
    cJSON *json = read_database();
    cJSON_DeleteItemFromObjectCaseSensitive(json, key);
    if (cJSON_AddNumberToObject(json, key, value) == NULL) {
        printf("Non sono riuscito ad aggiungere %s\n", key);
        cJSON_free(json);
        return -1;
    } else {
        write_database(json);
        cJSON_free(json);
        return 0;
    }
}
