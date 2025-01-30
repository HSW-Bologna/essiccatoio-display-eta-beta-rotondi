#ifndef STORAGE_H_INCLUDED
#define STORAGE_H_INCLUDED

#include <stdint.h>
#include <stdlib.h>

void storage_init(void);

int  storage_load_uint8(uint8_t *value, const char *key);
void storage_save_uint8(uint8_t *value, const char *key);
int  storage_load_uint16(uint16_t *value, const char *key);
void storage_save_uint16(uint16_t *value, const char *key);
int  storage_load_uint32(uint32_t *value, const char *key);
void storage_save_uint32(uint32_t *value, const char *key);
int  storage_load_uint64(uint64_t *value, const char *key);
void storage_save_uint64(uint64_t *value, const char *key);
int  storage_load_blob(void *value, size_t len, const char *key);
void storage_save_blob(void *value, size_t len, const char *key);

#endif
