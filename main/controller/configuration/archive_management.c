#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <stdio.h>
#include "esp_log.h"
#include "microtar.h"
#include "archive_management.h"
#include "bsp/storage.h"
#include "configuration.h"


#define DATA_VERSION_FILE "version.txt"

#define TAR_PROGRAMS_PATH "programmi"
#define TAR_PARMAC_PATH   "parametri/parmac.bin"


static int  is_archive(struct dirent *dir);
static void copy_file_to_tar(mtar_t *tar, const char *dest_path, const char *source_path);


static const char *TAG = "Archive management";


int archive_management_extract_configuration(const char *zipped_archive) {
    mtar_t        tar;
    mtar_header_t h;

    char archive[128] = {0};
    strcpy(archive, zipped_archive);
    char *dot = strrchr(archive, '.');
    if (dot == NULL) {
        ESP_LOGW(TAG, "invalid zip name: %s", zipped_archive);
        return -1;
    }
    *dot = '\0';

    /* Open archive for reading */
    int res = mtar_open(&tar, zipped_archive, "r");
    if (res != MTAR_ESUCCESS) {
        ESP_LOGW(TAG, "Could not open archive %s: %s (%i)", zipped_archive, mtar_strerror(res), res);
        return -1;
    }

    configuration_delete_all();
    res = 0;

    /* Print all file names and sizes */
    while ((mtar_read_header(&tar, &h)) != MTAR_ENULLRECORD) {
        if ((strcmp(h.name, TAR_PARMAC_PATH) == 0) ||
            (strncmp(h.name, TAR_PROGRAMS_PATH, strlen(TAR_PROGRAMS_PATH)) == 0 &&
             strlen(h.name) > strlen(TAR_PROGRAMS_PATH))) {
            ESP_LOGI(TAG, "Found %s", h.name);

            if (mtar_find(&tar, h.name, &h)) {
                ESP_LOGE(TAG, "Fatal error!");
                mtar_close(&tar);
                return -1;
            }

            if (configuration_copy_from_tar(&tar, h.name, h.size)) {
                ESP_LOGW(TAG, "Unable to copy %s", h.name);
            }
        }
        mtar_next(&tar);
    }

    /* Close archive */
    mtar_close(&tar);
    return res;
}


int archive_management_save_configuration(const char *path, const char *name) {
    mtar_t tar;

    char archive[64] = {0};
    snprintf(archive, sizeof(archive), "%s/%s.WS2020.tar", path, name);

    /* Open archive for reading */
    // int res = mtar_open(&tar, archive, "w");
    int res = mtar_open(&tar, archive, "w");
    if (res != MTAR_ESUCCESS) {
        ESP_LOGW(TAG, "Could not open archive %s: %s (%i)", archive, mtar_strerror(res), res);
        return -1;
    }

    ESP_LOGI(TAG, "Opened %s", archive);

    // Data version
    char version_string[8] = "";
    snprintf(version_string, sizeof(version_string), "%i", CONFIGURATION_COMPATIBILITY_VERSION);
    mtar_write_file_header(&tar, DATA_VERSION_FILE, strlen(version_string));
    mtar_write_data(&tar, version_string, strlen(version_string));

    // Parmac
    copy_file_to_tar(&tar, TAR_PARMAC_PATH, PATH_FILE_PARMAC);

    // Programs
    mtar_write_dir_header(&tar, "programmi");

    struct dirent *entry;
    DIR           *dir = opendir(PROGRAMS_PATH);
    if (dir != NULL) {
        while ((entry = readdir(dir)) != NULL) {
            char dest[128] = {0};
            if (snprintf(dest, sizeof(dest), "programmi/%s", entry->d_name) > (int)sizeof(dest)) {
                continue;
            }
            char source[128] = {0};
            if (snprintf(source, sizeof(source), "%s/%s", PROGRAMS_PATH, entry->d_name) > (int)sizeof(source)) {
                continue;
            }

            copy_file_to_tar(&tar, dest, source);
        }

        (void)closedir(dir);
    } else {
        ESP_LOGW(TAG, "Couldn't open the directory: %s", strerror(errno));
    }

    /* Finalize -- this needs to be the last thing done before closing */
    mtar_finalize(&tar);

    /* Close archive */
    mtar_close(&tar);

    ESP_LOGI(TAG, "writing %s successful!", archive);

    char tar_archive_path[64] = {0};
    snprintf(tar_archive_path, sizeof(tar_archive_path), "%s/%s-dryer-config.tar", path, name);

#if 0
    // Compression
    FILE *source = fopen(archive, "rb");
    // Appending .gz to the file name makes it extractable on host platforms
    FILE *comp = fopen(tar_archive_path, "wb");

    if (comp == NULL || source == NULL) {
        ESP_LOGE(TAG, "Error opening file before compressing");
    }

    res = deflate_file(source, comp);

    fclose(source);
    fclose(comp);
#endif

    remove(archive);

    ESP_LOGI(TAG, "writing %s successful (%i)!", tar_archive_path, res);

    return 0;
}


size_t archive_management_copy_archive_names(char **strings, name_t **archives, size_t len) {
    free(*archives);

    if (len > 0) {
        name_t *archives_array = malloc(sizeof(name_t) * len);
        assert(archives_array != NULL);

        char tmp[64] = {0};

        for (size_t i = 0; i < len; i++) {
            strcpy(tmp, strings[i]);
            char *suffix = strstr(tmp, ARCHIVE_SUFFIX);
            assert(suffix != NULL);
            *suffix = '\0';
            strcpy(archives_array[i], tmp);
        }

        *archives = archives_array;
    }

    return len;
}


int archive_management_list_archives(const char *path, char ***strings) {
    size_t         count = 0;
    DIR           *d;
    struct dirent *dir;
    d = opendir(path);
    if (d == NULL) {
        ESP_LOGW(TAG, "Unable to open folder %s: %s", path, strerror(errno));
        return -1;
    }

    while ((dir = readdir(d)) != NULL) {
        if (is_archive(dir)) {
            // ESP_LOGI(TAG, "Found archive %s", dir->d_name);
            count++;
        }
    }

    if (count > 0) {
        rewinddir(d);

        char **strings_array = malloc(sizeof(char *) * count);
        assert(strings_array != NULL);
        size_t i = 0;
        while ((dir = readdir(d)) != NULL && i < count) {
            if (is_archive(dir)) {
                size_t len       = strlen(dir->d_name) + 1;
                strings_array[i] = malloc(len);
                assert(strings_array[i] != NULL);
                memset(strings_array[i], 0, len - 1);
                strcpy(strings_array[i], dir->d_name);
                i++;
            }
        }

        *strings = strings_array;
    } else {
        *strings = NULL;
    }

    closedir(d);

    return (int)count;
}


static int is_archive(struct dirent *dir) {
    if (dir->d_type == DT_REG) {
        char *found = strstr(dir->d_name, ARCHIVE_SUFFIX);
        if (found != NULL && found + strlen(ARCHIVE_SUFFIX) == dir->d_name + strlen(dir->d_name)) {
            return 1;
        }
    }
    return 0;
}


static void copy_file_to_tar(mtar_t *tar, const char *dest_path, const char *source_path) {
#define BUFFER_SIZE 2048
    ESP_LOGI(TAG, "Tarring %s to %s", source_path, dest_path);

    FILE *f = fopen(source_path, "r");
    if (f == NULL) {
        ESP_LOGW(TAG, "Cannot open file %s: %s", source_path, strerror(errno));
        return;
    }

    char *buffer = malloc(BUFFER_SIZE);
    if (buffer == NULL) {
        ESP_LOGW(TAG, "Unable to allocate memory");
        fclose(f);
        return;
    }


    fseek(f, 0L, SEEK_END);
    size_t size = ftell(f);
    rewind(f);

    mtar_write_file_header(tar, dest_path, size);

    size_t count = 0;
    while (count < size) {
        int read = fread(buffer, 1, BUFFER_SIZE, f);

        if (read <= 0) {
            break;
        } else {
            count += read;
            mtar_write_data(tar, buffer, read);
        }
    }

    fclose(f);
    free(buffer);
#undef BUFFER_SIZE
}
